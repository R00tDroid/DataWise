#include "AnalyticsWorker.h"
#include "DataWiseEditor.h"

UAnalyticsWorker* UAnalyticsWorker::instance = nullptr;

FAnalyticsWorkerThread::FAnalyticsWorkerThread() {
	should_stop = false; 
	pause = false;

	semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	thread = FRunnableThread::Create(this, TEXT("FAnalyticsWorkerThread"), 0, TPri_BelowNormal);
}

FAnalyticsWorkerThread::~FAnalyticsWorkerThread() {
	if (semaphore)
	{
		FGenericPlatformProcess::ReturnSynchEventToPool(semaphore); semaphore = nullptr;
	}

	if (thread)
	{
		delete thread; 
		thread = nullptr;
	}
}

bool FAnalyticsWorkerThread::Init() 
{
	UE_LOG(AnalyticsLogEditor, Log, TEXT("Analytics worker starting"));
	return true;
}

uint32 FAnalyticsWorkerThread::Run() 
{ 
	FPlatformProcess::Sleep(0.03);

	while (!should_stop)
	{
		if (pause)
		{
			semaphore->Wait();
		}
		else {

			// Get task
			mutex.Lock();
			UAnalyticsWorkerTask* task = nullptr;
			if(task_queue.Num()!=0)
			{
				task = task_queue[0];
			}
			mutex.Unlock();

			// Perform task execution
			if (task != nullptr) 
			{
				UE_LOG(AnalyticsLogEditor, Log, TEXT("Executing task: %s"), *task->GetClass()->GetName());
				task->Execute();
				UE_LOG(AnalyticsLogEditor, Log, TEXT("Finished task: %s"), *task->GetClass()->GetName());

				// Destroy task
				mutex.Lock();
				task_queue.Remove(task);
				task->RemoveFromRoot();
				task->ConditionalBeginDestroy();
				task = nullptr;
				GEngine->ForceGarbageCollection(true);
				mutex.Unlock();
			}

			FPlatformProcess::Sleep(0.1);
		}
	}

	UE_LOG(AnalyticsLogEditor, Log, TEXT("Analytics worker stopped"));

	return 0;
}

void FAnalyticsWorkerThread::PauseThread()
{
	pause = true;
}

void FAnalyticsWorkerThread::ContinueThread() 
{
	pause = false;

	if (semaphore)
	{
		semaphore->Trigger();
	}
}

void FAnalyticsWorkerThread::Stop() 
{
	should_stop = true;

	if (semaphore)
	{
		semaphore->Trigger();
	}
}

void FAnalyticsWorkerThread::StopThread()
{
	Stop();
	if (thread)
	{
		thread->WaitForCompletion();
	}
}

bool FAnalyticsWorkerThread::IsPaused() {

	return pause;
}

UAnalyticsWorker* UAnalyticsWorker::Get()
{
	if(instance == nullptr)
	{
		instance = NewObject<UAnalyticsWorker>();
		instance->AddToRoot();

		instance->thread = new FAnalyticsWorkerThread();
	}
	return instance;
}

void UAnalyticsWorker::AddTask(UAnalyticsWorkerTask* task)
{
	thread->mutex.Lock();
	thread->task_queue.Add(task);
	thread->mutex.Unlock();
}

void UAnalyticsWorker::BeginDestroy()
{
	Super::BeginDestroy();

	if (thread != nullptr) 
	{
		thread->StopThread();
		delete thread;
		thread = nullptr;
	}

	instance = nullptr;
}
