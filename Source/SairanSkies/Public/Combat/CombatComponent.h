// SairanSkies - Combat Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ASairanCharacter;
class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockPerformed, float, DamageBlocked, bool, bWasPerfectParry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHitLanded, AActor*, HitActor, FVector, HitLocation, float, Damage);

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

	/** Start holding block/parry stance */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartBlock();

	/** Release block/parry stance */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ReleaseBlock();

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

	/** Called when the player receives an incoming attack. Returns true if fully blocked (parry), false if partial block or no block. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HandleIncomingDamage(float IncomingDamage, AActor* Attacker, float& OutDamageApplied);

	// ========== HIT DETECTION ==========
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EnableHitDetection();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DisableHitDetection();


	/** Called by weapon when its HitCollision detects an overlap */
	void OnWeaponHitDetected(AActor* HitActor, const FVector& HitLocation);

	// ========== STATE ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float LightAttackDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float HeavyAttackDamage = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float ChargedAttackDamage = 80.0f;

	/** +/- random variance applied to each attack's damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float DamageVariance = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Damage")
	float ChargeTimeForMaxDamage = 2.0f;

	// ========== PARRY SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	float ParryWindowDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	float ParryCooldown = 0.5f;

	// ========== PARRY/BLOCK FEEDBACK ==========

	/** VFX when a perfect parry (deflect) occurs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	UNiagaraSystem* ParryDeflectVFX;

	/** VFX when a normal block occurs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	UNiagaraSystem* BlockVFX;

	/** Sound when a perfect parry (deflect) occurs - like Sekiro clang */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	USoundBase* ParryDeflectSound;

	/** Sound when a normal block occurs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry")
	USoundBase* BlockSound;

	// ========== PERFECT PARRY SLOW-MOTION (God of War style) ==========

	/** Duration of the initial hitstop freeze on perfect parry (seconds, real time) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry|SlowMotion")
	float PerfectParryHitstopDuration = 0.08f;

	/** Duration of the slow-motion window after the hitstop ends (seconds, real time) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry|SlowMotion")
	float PerfectParrySlowMoDuration = 0.5f;

	/** Time dilation during the slow-mo window (0.3 = 30% speed = 70% reduction) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Parry|SlowMotion")
	float PerfectParrySlowMoTimeDilation = 0.3f;

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
	/** Radius of the hit detection sphere */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitDetection")
	float HitDetectionRadius = 80.0f;

	/** How far in front of the character the hit detection sphere is placed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitDetection")
	float HitDetectionForwardOffset = 100.0f;

	/** Height offset for hit detection (to avoid hitting the ground) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitDetection")
	float HitDetectionHeightOffset = 50.0f;

	// ========== HIT FEEDBACK ==========
	
	/** Duration of hitstop (game pause) on hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	float HitstopDuration = 0.05f;

	/** Strength of camera shake on hit (0 = none) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	float CameraShakeIntensity = 1.0f;

	/** Camera shake class to play on hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Force applied to enemy on hit (knockback) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	float KnockbackForce = 500.0f;

	/** Knockback force for charged attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	float ChargedKnockbackForce = 1000.0f;

	/** Niagara system for hit particles (assign in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	UNiagaraSystem* HitParticleSystem;

	/** Sound to play on hit (assign in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitFeedback")
	USoundBase* HitSound;

	/** Enable debug visualization for hit detection (should be false in final build) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Debug")
	bool bShowHitDebug = false;

	// ========== EVENTS ==========
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAttackPerformed OnAttackPerformed;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnParrySuccess OnParrySuccess;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnParryWindow OnParryWindow;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnBlockPerformed OnBlockPerformed;

	/** Called when a hit lands on an enemy - use for VFX/SFX */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnHitLanded OnHitLanded;

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
	bool bIsHoldingBlock = false;

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
	void PerformHitDetection();
	float GetDamageForAttackType(EAttackType AttackType) const;
	/** Apply damage variance: base +/- DamageVariance randomly */
	float ApplyDamageVariance(float BaseDamage) const;
	void ApplyDamageToTarget(AActor* Target, float Damage, const FVector& HitLocation);
	
	/** Apply all hit feedback effects */
	void ApplyHitFeedback(AActor* HitActor, const FVector& HitLocation, float Damage);
	void ApplyKnockback(AActor* Target, float Force);
	void TriggerHitstop(EAttackType AttackType);
	void TriggerCameraShake(EAttackType AttackType);
	void ResumeFromHitstop();

	/** Start slow-motion phase after perfect parry hitstop */
	void StartPerfectParrySlowMo();
	/** End slow-motion and fully restore time */
	void EndPerfectParrySlowMo();

	/** Play parry/block VFX and SFX */
	void PlayParryFeedback(bool bPerfectParry);

	FTimerHandle ComboResetTimer;
	FTimerHandle ParryWindowTimer;
	FTimerHandle ParryCooldownTimer;
	FTimerHandle AttackEndTimer;
	FTimerHandle HitstopTimer;
	FTimerHandle PerfectParrySlowMoTimer;
	/** Tracks whether the current hitstop is from a perfect parry (to chain into slow-mo) */
	bool bIsPerfectParryHitstop = false;

	TSet<AActor*> HitActorsThisAttack;
	bool bHitLandedThisAttack = false;
};
