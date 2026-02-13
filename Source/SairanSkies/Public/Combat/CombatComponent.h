// SairanSkies - Combat Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ASairanCharacter;
class UAnimMontage;

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None,
	Light,
	Heavy,
	Charged
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackPerformed, EAttackType, AttackType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParrySuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParryWindow);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== ATTACK FUNCTIONS ==========
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void LightAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartHeavyAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ReleaseHeavyAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformParry();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ExecuteAttack(EAttackType AttackType);

	// ========== COMBO SYSTEM ==========
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void IncrementCombo();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const { return bIsAttacking; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInParryWindow() const { return bIsInParryWindow; }

	// ========== HIT DETECTION ==========
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EnableHitDetection();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DisableHitDetection();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformHitDetection();

	// ========== DAMAGE VALUES ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float LightAttackDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float HeavyAttackDamage = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float ChargedAttackDamage = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float ChargeTimeForMaxDamage = 2.0f;

	// ========== PARRY SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	float ParryWindowDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	float ParryCooldown = 0.5f;

	// ========== COMBO SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Combo")
	int32 MaxLightCombo = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Combo")
	float ComboResetTime = 1.5f;

	// ========== ATTACK MONTAGES ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TArray<UAnimMontage*> LightAttackMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* HeavyAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* ChargedAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* ParryMontage;

	// ========== HIT DETECTION ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitDetection")
	float HitDetectionRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitDetection")
	float HitDetectionForwardOffset = 100.0f;

	// ========== EVENTS ==========
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAttackPerformed OnAttackPerformed;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnParrySuccess OnParrySuccess;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnParryWindow OnParryWindow;

	// ========== STATE ==========
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 CurrentComboCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EAttackType CurrentAttackType = EAttackType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsChargingAttack = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CurrentChargeTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInParryWindow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCanParry = true;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bHitDetectionEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bInputBuffered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EAttackType BufferedAttackType = EAttackType::None;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

private:
	void EndAttack();
	void EndParryWindow();
	void ResetParryCooldown();
	void ProcessBufferedInput();
	float GetDamageForAttackType(EAttackType AttackType) const;
	void ApplyDamageToTarget(AActor* Target, float Damage);

	FTimerHandle ComboResetTimer;
	FTimerHandle ParryWindowTimer;
	FTimerHandle ParryCooldownTimer;
	FTimerHandle AttackEndTimer;

	TSet<AActor*> HitActorsThisAttack;
};
