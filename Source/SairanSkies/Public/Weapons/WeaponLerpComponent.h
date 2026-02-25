// SairanSkies - Weapon Lerp Component (handles combo position lerping)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatComponent.h"
#include "WeaponLerpComponent.generated.h"

class ASairanCharacter;
class AWeaponBase;
class USceneComponent;

UENUM(BlueprintType)
enum class EWeaponLerpState : uint8
{
	Idle,              // Weapon at rest position
	LightAttacking,    // Lerping between light attack points
	HeavyAttacking,    // Quick snap to heavy points
	HeavyCharging,     // Slow lerp during charge
	ReturningToIdle    // Smoothly returning to idle after inactivity
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UWeaponLerpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponLerpComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MAIN FUNCTIONS ==========

	/** Start a light attack lerp to the next combo point */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void StartLightAttackLerp(int32 ComboIndex);

	/** Start heavy attack lerp (quick: snap up then down) */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void StartHeavyAttackLerp();

	/** Start heavy charge lerp (slow: wind up gradually) */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void StartHeavyChargeLerp();

	/** Release the charged heavy attack (snap to end position) */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void ReleaseHeavyCharge();

	/** Reset combo and return weapon to idle after inactivity */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void ReturnToIdle();

	/** Force immediate return to idle (no lerp) */
	UFUNCTION(BlueprintCallable, Category = "WeaponLerp")
	void ForceIdle();

	// ========== SETTINGS ==========

	/** Speed of light attack lerps (higher = faster swing) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponLerp|Settings")
	float LightAttackLerpSpeed = 15.0f;

	/** Speed of heavy attack quick snap */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponLerp|Settings")
	float HeavyAttackSnapSpeed = 20.0f;

	/** Speed of heavy charge wind-up */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponLerp|Settings")
	float HeavyChargeLerpSpeed = 1.5f;

	/** Speed of returning to idle position */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponLerp|Settings")
	float ReturnToIdleLerpSpeed = 5.0f;

	/** Time after last attack before weapon returns to idle (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponLerp|Settings")
	float ComboInactivityTimeout = 2.0f;

	// ========== STATE ==========
	UPROPERTY(BlueprintReadOnly, Category = "WeaponLerp")
	EWeaponLerpState CurrentLerpState = EWeaponLerpState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "WeaponLerp")
	int32 CurrentLightComboIndex = 0;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

	UPROPERTY()
	AWeaponBase* Weapon;

private:
	// Scene components used as lerp targets (set from character's attach points)
	UPROPERTY()
	USceneComponent* IdlePoint = nullptr;

	UPROPERTY()
	TArray<USceneComponent*> LightAttackPoints;

	UPROPERTY()
	TArray<USceneComponent*> HeavyAttackPoints;

	// Current lerp target
	FVector TargetLocation;
	FRotator TargetRotation;
	FVector StartLocation;
	FRotator StartRotation;
	float LerpAlpha = 0.0f;
	float CurrentLerpSpeed = 10.0f;

	// Heavy attack phase tracking
	int32 HeavyPhase = 0; // 0 = going to point1, 1 = going to point2

	// Inactivity timer
	float TimeSinceLastAttack = 0.0f;
	bool bIsActive = false;

	void SetLerpTarget(USceneComponent* TargetPoint, float Speed);
	void UpdateLerp(float DeltaTime);
	void ApplyWeaponTransform(const FVector& Location, const FRotator& Rotation);
	bool HasReachedTarget() const;
};


