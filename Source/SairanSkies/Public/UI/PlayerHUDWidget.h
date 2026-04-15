// SairanSkies - Player HUD Widget (health bar)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Main player HUD showing health bar.
 * Create a Widget Blueprint that inherits from this class and add a ProgressBar named "HealthBar".
 */
UCLASS()
class SAIRANSKIES_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Update the health bar display */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float HealthPercent);

	/**
	 * Update the Ultimate XP bar.
	 * @param UltimatePercent  0.0 = vacío, 1.0 = listo para activar.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateUltimate(float UltimatePercent);

protected:
	/** Bind to a ProgressBar named "HealthBar" in the Widget Blueprint */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	/** Optional text display for HP numbers */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	/**
	 * Barra de XP del ultimate.
	 * Nombrar el widget "UltimateBar" en el Blueprint.
	 * Llena de azul oscuro → cian brillante cuando está lista.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* UltimateBar;
};

