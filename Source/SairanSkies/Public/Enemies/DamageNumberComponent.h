// SairanSkies - Damage Number Component (spawns individual floating damage numbers at hit location)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageNumberComponent.generated.h"

class UTextRenderComponent;
class UFont;
class UMaterialInterface;

/**
 * Stores data for a single floating damage number instance
 */
USTRUCT()
struct FFloatingDamageNumber
{
	GENERATED_BODY()

	UPROPERTY()
	UTextRenderComponent* TextComponent = nullptr;

	float Lifetime = 0.0f;
	float MaxLifetime = 1.0f;
	FVector InitialLocation = FVector::ZeroVector;
};

/**
 * Attach to enemies. Spawns individual damage numbers at hit locations.
 * Numbers float upward and fade out over 1 second.
 * Color goes green -> red based on remaining health percentage.
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

	/** Spawn a floating damage number at a specific world location */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void SpawnDamageNumber(float DamageAmount, float HealthPercent, const FVector& WorldLocation);

	/** Show death marker (X) above the enemy */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void ShowDeathMarker();

	/** Reset - clears all active numbers */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void ResetCombo();

	// ========== SETTINGS ==========

	/** Font to use for damage numbers (leave null for default) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	UFont* DamageFont = nullptr;

	/**
	 * Optional: Material to use for the damage number text.
	 * Use this when your custom Font does not render correctly (invisible text).
	 *
	 * HOW TO SET IT UP:
	 * 1. In the Content Browser, right-click → Material
	 * 2. Set Blend Mode = Translucent, Shading Model = Unlit
	 * 3. Add a "TextureObjectParameter" named "Font" (or leave it for UE to fill)
	 * 4. Add a "VectorParameter" named "Color" and plug it into Emissive Color
	 * 5. Assign this material here AND in the Font asset's "Font Material" slot
	 *
	 * If left null, the font's own material will be used (works fine with default font,
	 * but custom fonts may need this override to show color correctly).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	UMaterialInterface* FontMaterial = nullptr;

	/** How long each damage number lives (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float NumberLifetime = 1.0f;

	/** How fast numbers float upward (units/sec) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float FloatUpSpeed = 80.0f;

	/** Random horizontal scatter when spawning (units) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float HorizontalScatter = 20.0f;

	/** Text size */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float TextSize = 24.0f;

	/** Offset above the enemy's head for death marker (in cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Settings")
	float DeathMarkerHeightOffset = 120.0f;

	/** Color at full health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor FullHealthColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Green

	/** Color at mid health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor MidHealthColor = FLinearColor(1.0f, 0.65f, 0.0f, 1.0f); // Orange

	/** Color at zero health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor ZeroHealthColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red

	/** Health % threshold: FullHealth -> MidHealth */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighToMidThreshold = 0.6f;

	/** Health % threshold: MidHealth -> ZeroHealth */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MidToLowThreshold = 0.25f;

	/** Color for the death X marker */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageNumbers|Colors")
	FLinearColor DeathColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

private:
	FLinearColor GetColorForHealthPercent(float HealthPercent) const;
	void CleanupNumber(int32 Index);

	UPROPERTY()
	TArray<FFloatingDamageNumber> ActiveNumbers;

	UPROPERTY()
	UTextRenderComponent* DeathTextComponent = nullptr;

	bool bIsShowingDeath = false;
	FTimerHandle DeathTimerHandle;
};
