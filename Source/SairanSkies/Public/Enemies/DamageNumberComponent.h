// SairanSkies - Damage Number Component (shows stacking combo damage above enemy)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageNumberComponent.generated.h"

class UWidgetComponent;
class UTextRenderComponent;

/**
 * Attach to enemies. Shows stacking damage numbers above their head.
 * Numbers go green -> red based on remaining health percentage.
 * Shows X on death.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UDamageNumberComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDamageNumberComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MAIN FUNCTIONS ==========

	/** Add damage to the combo counter and display it */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void AddDamage(float DamageAmount, float HealthPercent);

	/** Show death marker (X) */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void ShowDeathMarker();

	/** Reset the combo counter */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void ResetCombo();

	// ========== SETTINGS ==========

	/** How long the damage number stays visible after last hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float DisplayDuration = 3.0f;

	/** Offset above the enemy's head (in cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float HeightOffset = 120.0f;

	/** Horizontal offset to the left */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float LeftOffset = 40.0f;

	/** Text size */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float TextSize = 24.0f;

	/** Color at full health (stage 1 - healthy) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor FullHealthColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Green

	/** Color at mid health (stage 2 - warning) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor MidHealthColor = FLinearColor(1.0f, 0.65f, 0.0f, 1.0f); // Orange

	/** Color at zero health (stage 3 - critical) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor ZeroHealthColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red

	/** Health % threshold to transition from FullHealth to MidHealth color (0-1). 
	 *  e.g. 0.6 means green->orange happens between 100%-60% health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighToMidThreshold = 0.6f;

	/** Health % threshold to transition from MidHealth to ZeroHealth color (0-1).
	 *  e.g. 0.25 means orange->red happens between 60%-25% health. Below this is full red. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MidToLowThreshold = 0.25f;

	/** Color for the death X marker */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	FLinearColor DeathColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

private:
	void UpdateTextDisplay();
	void HideText();
	void OnDisplayTimerExpired();
	/** Calculate color based on health % using the 3-stage threshold system */
	FLinearColor GetColorForHealthPercent(float HealthPercent) const;

	UPROPERTY()
	UTextRenderComponent* TextComponent;

	float AccumulatedDamage = 0.0f;
	float LastHealthPercent = 1.0f;
	bool bIsShowingDeath = false;
	bool bIsVisible = false;

	FTimerHandle DisplayTimerHandle;
};
