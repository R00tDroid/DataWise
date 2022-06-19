#include "ExtendedWidget.h"
#include "DataWiseEditor.h"

void SExtendedWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (!started) 
	{
		started = true;
		Start();
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}
