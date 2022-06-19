#pragma once
#include "ExtendedWidget.h"
#include "AnalyticsWorkerTask.h"
#include "AnalyticsCapture.h"
#include "Widgets/Text/STextBlock.h"
#include "AnalyticsWidgetSessions.generated.h"

struct ValueFilter
{
	enum FilterOp
	{
		FILOP_EXIST,
		FILOP_NOT_EXIST,
		FILOP_EQUALS,
		FILOP_GREATER,
		FILOP_LESS,
		FILOP_GREATER_EQUALS,
		FILOP_LESS_EQUALS,
		FILOP_NOT_EQUALS
	};

	ValueFilter(FString Parameter, FString Value, FilterOp Operator) : Parameter(Parameter), Filter(Value), Operator(Operator) {}

	FString Parameter;
	FString Filter;
	FilterOp Operator;

	static bool MatchesFilter(FString Filter, FString Value, FilterOp Operator);
	bool Matches(FAnalyticsCaptureInfo Info);
};


extern DATAWISEEDITOR_API TArray<ValueFilter> ParseFilters(FString);


class SWidgetAnalyticsSessions : public SExtendedWidget
{
	SLATE_USER_ARGS(SWidgetAnalyticsSessions)
	{ }

	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs);
	virtual void Start() override;

	void ReceiveInfoList(TArray<FAnalyticsCaptureInfo> info);

	void SetStatus(int status);

	enum SearchStatus
	{
		SearchPending,
		Searching,
		Finished
	};

	TSharedPtr<SVerticalBox> list;
	TSharedPtr<STextBlock> status_label;

	SearchStatus status = SearchPending;

	TArray<FAnalyticsCaptureInfo> info_list;
	TArray<FAnalyticsCaptureInfo> selection;

	TArray<ValueFilter> Filters;

	void SelectToggle();
	void UpdateStatus();
	void RequestSearch();
	void UpdateSelection();
	void UpdateList();

	void OnRequestNewSearch();
};



UCLASS()
class DATAWISEEDITOR_API UAnalyticsSearchTask : public UAnalyticsWorkerTask {
	GENERATED_BODY()
public:
	bool Execute() override;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSearchStatusUpdate, int);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSearchFinish, TArray<FAnalyticsCaptureInfo>);

	TArray<ValueFilter> Filters;

	FOnSearchStatusUpdate OnSearchStatusUpdated;
	
	FOnSearchFinish OnSearchFinish;

	FThreadSafeBool finished;
};