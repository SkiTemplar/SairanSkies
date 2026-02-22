// SairanSkies - Enemy Health Bar Widget (floating above enemy)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

/**
 * Floating health bar for enemies.
 * Create a Widget Blueprint inheriting from this and add a ProgressBar named "HealthBar".
 */
UCLASS()
class SAIRANSKIES_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Update the health bar fill and color */
	UFUNCTION(BlueprintCallable, Category = "EnemyUI")
	void UpdateHealth(float HealthPercent);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;
};

