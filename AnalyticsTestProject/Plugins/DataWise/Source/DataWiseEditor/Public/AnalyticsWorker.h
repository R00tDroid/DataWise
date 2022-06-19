#pragma once
#include "AnalyticsWorkerTask.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "AnalyticsWorker.generated.h"

class FAnalyticsWorkerThread : public FRunnable {
public:

	FAnalyticsWorkerThread();
	~FAnalyticsWorkerThread();

	void PauseThread();
	void ContinueThread();

	void StopThread();

	bool Init() override;
	uint32 Run() override;
	void Stop() override;

	bool IsPaused();

	FRunnableThread* thread;

	FCriticalSection mutex; 
	FEvent* semaphore;

	FThreadSafeBool should_stop;
	FThreadSafeBool pause;

	TArray<UAnalyticsWorkerTask*> task_queue;
};

UCLASS()
class DATAWISEEDITOR_API UAnalyticsWorker : public UObject
{
GENERATED_BODY()
public:

	void BeginDestroy() override;

	template<typename T>
	static T* CreateTask()
	{
		T* new_task = NewObject<T>(Get());
		new_task->AddToRoot();
		return new_task;
	}

	static UAnalyticsWorker* Get();
	void AddTask(UAnalyticsWorkerTask* task);

private:
	static UAnalyticsWorker* instance;
	FAnalyticsWorkerThread* thread;
};
