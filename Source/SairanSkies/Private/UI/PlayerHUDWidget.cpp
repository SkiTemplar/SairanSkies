// SairanSkies - Player HUD Widget Implementation

#include "UI/PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerHUDWidget::UpdateHealth(float HealthPercent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));

		// Color gradient: green -> yellow -> red
		FLinearColor BarColor;
		if (HealthPercent > 0.5f)
		{
			// Green to Yellow
			float Alpha = (HealthPercent - 0.5f) / 0.5f;
			BarColor = FMath::Lerp(FLinearColor(1.0f, 1.0f, 0.0f), FLinearColor(0.0f, 1.0f, 0.0f), Alpha);
		}
		else
		{
			// Yellow to Red
			float Alpha = HealthPercent / 0.5f;
			BarColor = FMath::Lerp(FLinearColor(1.0f, 0.0f, 0.0f), FLinearColor(1.0f, 1.0f, 0.0f), Alpha);
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	if (HealthText)
	{
		int32 Percent = FMath::RoundToInt(HealthPercent * 100.0f);
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Percent)));
	}
}

void UPlayerHUDWidget::UpdateUltimate(float UltimatePercent)
{
	if (!UltimateBar) return;

	const float Pct = FMath::Clamp(UltimatePercent, 0.0f, 1.0f);
	UltimateBar->SetPercent(Pct);

	// Color: azul oscuro → cian brillante al llenarse; blanco pulsante cuando está lista
	FLinearColor BarColor;
	if (Pct >= 1.0f)
	{
		// Lista: cian brillante
		BarColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		// Llenándose: azul oscuro → cian
		BarColor = FMath::Lerp(
			FLinearColor(0.05f, 0.1f, 0.5f, 1.0f),
			FLinearColor(0.0f,  0.8f, 1.0f, 1.0f),
			Pct);
	}
	UltimateBar->SetFillColorAndOpacity(BarColor);
}

