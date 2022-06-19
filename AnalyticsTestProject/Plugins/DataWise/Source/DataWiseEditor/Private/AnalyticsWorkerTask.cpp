#include "AnalyticsWorkerTask.h"
#include "DataWiseEditor.h"
#include "Async/Async.h"
#include "Framework/Notifications/NotificationManager.h"

void UAnalyticsWorkerTask::ShowNotification(FString text, SNotificationItem::ECompletionState state, bool auto_expire, float expire_duration)
{
	FNotificationInfo info(FText::FromString(text));
	updating_notification = true;
	info.ExpireDuration = expire_duration;
	info.bFireAndForget = auto_expire;
	info.bUseLargeFont = false;

	AsyncTask(ENamedThreads::GameThread, [this, info, state]() 
	{
		notification = FSlateNotificationManager::Get().AddNotification(info);

		if (notification.IsValid())
		{
			notification->SetCompletionState(state);
		}

		updating_notification = false;
	});

	while(updating_notification)
	{
		FPlatformProcess::Sleep(.1f);
	}
}

void UAnalyticsWorkerTask::SetNotificationText(FString text)
{
	updating_notification = true;

	AsyncTask(ENamedThreads::GameThread, [this, text]() 
	{
		if (notification.IsValid())
		{
			notification->SetText(FText::FromString(text));
		}
		updating_notification = false;
	});

	while (updating_notification)
	{
		FPlatformProcess::Sleep(.1f);
	}
}

void UAnalyticsWorkerTask::SetNotificationState(SNotificationItem::ECompletionState state)
{
	updating_notification = true;

	AsyncTask(ENamedThreads::GameThread, [this, state]()
	{
		if (notification.IsValid())
		{
			notification->SetCompletionState(state);
		}
		updating_notification = false;
	});

	while (updating_notification)
	{
		FPlatformProcess::Sleep(.1f);
	}
}

void UAnalyticsWorkerTask::SetNotificationLink(FString text, FSimpleDelegate task)
{
	updating_notification = true;

	AsyncTask(ENamedThreads::GameThread, [this, text, task]()
	{
		if (notification.IsValid())
		{
			const TAttribute<FText> text_label = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([text]()
			{
				return FText::FromString(text);
			}));

			notification->SetHyperlink(task, text_label);
		}
		updating_notification = false;
	});

	while (updating_notification)
	{
		FPlatformProcess::Sleep(.1f);
	}
}

void UAnalyticsWorkerTask::DismissNotification(float duration)
{
	updating_notification = true;

	AsyncTask(ENamedThreads::GameThread, [this, duration]()
	{
		if (notification.IsValid())
		{
			notification->SetExpireDuration(duration);
			notification->ExpireAndFadeout();
		}
		updating_notification = false;
	});

	while (updating_notification)
	{
		FPlatformProcess::Sleep(.1f);
	}
}
