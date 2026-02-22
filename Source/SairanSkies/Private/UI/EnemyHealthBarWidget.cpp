// SairanSkies - Enemy Health Bar Widget Implementation

#include "UI/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHealthBarWidget::UpdateHealth(float HealthPercent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));

		// Color: green -> yellow -> red
		FLinearColor BarColor;
		if (HealthPercent > 0.5f)
		{
			float Alpha = (HealthPercent - 0.5f) / 0.5f;
			BarColor = FMath::Lerp(FLinearColor(1.0f, 1.0f, 0.0f), FLinearColor(0.0f, 1.0f, 0.0f), Alpha);
		}
		else
		{
			float Alpha = HealthPercent / 0.5f;
			BarColor = FMath::Lerp(FLinearColor(1.0f, 0.0f, 0.0f), FLinearColor(1.0f, 1.0f, 0.0f), Alpha);
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}
}

