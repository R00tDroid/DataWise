#pragma once
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/ThreadSafeBool.h"
#include "AnalyticsWorkerTask.generated.h"


UCLASS()
class UAnalyticsWorkerTask : public UObject
{
GENERATED_BODY()
public:
	virtual bool Execute() { return true; };

protected:
	void ShowNotification(FString text, SNotificationItem::ECompletionState state = SNotificationItem::CS_None, bool auto_expire = true, float expire_duration = 2.0f);
	void SetNotificationText(FString text);
	void SetNotificationState(SNotificationItem::ECompletionState state);
	void SetNotificationLink(FString text, FSimpleDelegate task);
	void DismissNotification(float duration = 2.0f);

private:
	FThreadSafeBool updating_notification = false;
	TSharedPtr<SNotificationItem> notification;
};
