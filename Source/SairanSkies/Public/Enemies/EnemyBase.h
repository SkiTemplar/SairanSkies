// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyBase.generated.h"

// Forward declarations
class UBehaviorTree;
class APatrolPath;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UDamageNumberComponent;

UCLASS(Abstract)
class SAIRANSKIES_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ==================== CONFIGURATION ====================
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	FEnemyCombatConfig CombatConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Perception")
	FEnemyPerceptionConfig PerceptionConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
	FEnemyPatrolConfig PatrolConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Behavior")
	FEnemyBehaviorConfig BehaviorConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Conversation")
	FEnemyConversationConfig ConversationConfig;

	// ==================== ASSETS ====================
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	FEnemyAnimationConfig AnimationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sound")
	FEnemySoundConfig SoundConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|VFX")
	FEnemyVFXConfig VFXConfig;

	/** Floating damage numbers above the enemy head */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|UI")
	UDamageNumberComponent* DamageNumberComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
	APatrolPath* PatrolPath;

	// ==================== STATE ====================
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	AActor* CurrentTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	FVector LastKnownTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	bool bCanAttack = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	float TimeSinceLastSawTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	int32 NearbyAlliesCount = 0;

	// ==================== DELEGATES ====================
public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyStateChanged OnEnemyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnPlayerDetected OnPlayerDetected;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnPlayerLost OnPlayerLost;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDeath OnEnemyDeath;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnConversationStarted OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnConversationEnded OnConversationEnded;

	// ==================== STATE MANAGEMENT ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|State")
	virtual void SetEnemyState(EEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	EEnemyState GetEnemyState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool IsInCombat() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool CanSeeTarget() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool IsAlerted() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	float GetSuspicionLevel() const;

	// ==================== PERCEPTION ====================
public:
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Perception")
	void LoseTarget();

	UFUNCTION(BlueprintPure, Category = "Enemy|Perception")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Perception")
	FVector GetLastKnownTargetLocation() const { return LastKnownTargetLocation; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Perception")
	void SetTarget(AActor* NewTarget, EEnemySenseType SenseType);

	// ==================== COMBAT ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void Attack();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void TakeDamageFromSource(float DamageAmount, AActor* DamageSource, AController* InstigatorController);

	/** Take damage at a specific world location - spawns particles attached to enemy */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void TakeDamageAtLocation(float DamageAmount, AActor* DamageSource, AController* InstigatorController, const FVector& HitWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void Die(AController* InstigatorController);

	// ==================== HIT FLASH SYSTEM ====================
	/** Color to flash when hit (like Blasphemous/2D games) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|VFX")
	FLinearColor HitFlashColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	/** Duration of the hit flash in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|VFX")
	float HitFlashDuration = 0.1f;

	/** Start the hit flash effect */
	UFUNCTION(BlueprintCallable, Category = "Enemy|VFX")
	void StartHitFlash();

protected:
	/** Stop the hit flash effect and restore original materials */
	void StopHitFlash();

	/** Timer handle for hit flash */
	FTimerHandle HitFlashTimerHandle;

	/** Original materials to restore after flash */
	UPROPERTY()
	TArray<UMaterialInterface*> OriginalMaterials;

	/** Dynamic material instance for flash effect */
	UPROPERTY()
	UMaterialInstanceDynamic* FlashMaterialInstance;

	/** Whether we have cached the original materials */
	bool bMaterialsCached = false;

public:

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool CanAttackNow() const { return bCanAttack && CurrentState != EEnemyState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetHealthPercent() const { return MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool IsDead() const { return CurrentState == EEnemyState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetDistanceToTarget() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool IsInAttackRange() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	virtual bool CanAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void PerformTaunt();

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	virtual bool ShouldTaunt() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	virtual bool HasEnoughAlliesForAggression() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void HandleCombatBehavior(float DeltaTime);

	// ==================== ALLY COORDINATION ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void AlertNearbyAllies(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void ReceiveAlertFromAlly(AActor* Target, AEnemyBase* AlertingAlly);

	UFUNCTION(BlueprintPure, Category = "Enemy|Coordination")
	int32 GetAttackersCount() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Coordination")
	bool CanJoinAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void RegisterAsAttacker();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void UnregisterAsAttacker();

	// ==================== MOVEMENT ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetPatrolSpeed();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetChaseSpeed();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetMovementSpeed(float SpeedMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void StartStrafe(bool bStrafeRight);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void StopStrafe();

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	bool IsStrafing() const { return bIsStrafing; }

	// Velocidad con variación natural
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetPatrolSpeedWithVariation();

protected:
	UPROPERTY()
	float BaseMaxWalkSpeed;

	bool bIsStrafing = false;
	float StrafeDirection = 1.0f;
	float StrafeTimer = 0.0f;

	void UpdateStrafe(float DeltaTime);

	// ==================== NATURAL BEHAVIOR (AAA-style) ====================
public:
	// Iniciar pausa aleatoria durante patrulla
	UFUNCTION(BlueprintCallable, Category = "Enemy|Behavior")
	void StartRandomPause();

	// Terminar pausa
	UFUNCTION(BlueprintCallable, Category = "Enemy|Behavior")
	void EndRandomPause();

	// Empezar a mirar alrededor
	UFUNCTION(BlueprintCallable, Category = "Enemy|Behavior")
	void StartLookAround();

	// Parar de mirar alrededor
	UFUNCTION(BlueprintCallable, Category = "Enemy|Behavior")
	void StopLookAround();

	UFUNCTION(BlueprintPure, Category = "Enemy|Behavior")
	bool IsInRandomPause() const { return bIsInRandomPause; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Behavior")
	bool IsLookingAround() const { return bIsLookingAround; }

	// Decide aleatoriamente si debería pausar
	UFUNCTION(BlueprintPure, Category = "Enemy|Behavior")
	bool ShouldRandomPause() const;

protected:
	bool bIsInRandomPause = false;
	float RandomPauseTimer = 0.0f;
	float RandomPauseDuration = 0.0f;

	bool bIsLookingAround = false;
	float LookAroundTimer = 0.0f;
	FRotator OriginalRotation;
	FRotator TargetLookRotation;

	void UpdateRandomPause(float DeltaTime);
	void UpdateLookAround(float DeltaTime);

	// ==================== CONVERSATION SYSTEM ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	bool TryStartConversation(AEnemyBase* OtherEnemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	void JoinConversation(AEnemyBase* Initiator);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	void EndConversation();

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	bool IsConversing() const { return CurrentState == EEnemyState::Conversing; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	AEnemyBase* GetConversationPartner() const { return ConversationPartner; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	bool CanStartConversation() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	AEnemyBase* FindNearbyEnemyForConversation() const;

protected:
	UPROPERTY()
	AEnemyBase* ConversationPartner = nullptr;

	float ConversationTimer = 0.0f;
	float ConversationDuration = 0.0f;
	float GestureTimer = 0.0f;
	float ConversationCooldownTimer = 0.0f;
	float TimeWaitingAtPoint = 0.0f;
	bool bIsConversationInitiator = false;

	void UpdateConversation(float DeltaTime);
	void PerformConversationGesture();

	// ==================== ANIMATION ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	UAnimMontage* GetRandomAttackMontage();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void PlayHitReaction();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void PlayConversationGesture();

	// ==================== SOUND/VFX ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Audio")
	void PlayRandomSound(const TArray<USoundBase*>& Sounds);

	UFUNCTION(BlueprintCallable, Category = "Enemy|VFX")
	void SpawnHitEffect(FVector Location);

protected:
	float AttackCooldownTimer;

	// ==================== BLACKBOARD KEYS ====================
public:
	static const FName BB_TargetActor;
	static const FName BB_TargetLocation;
	static const FName BB_EnemyState;
	static const FName BB_CanSeeTarget;
	static const FName BB_PatrolIndex;
	static const FName BB_DistanceToTarget;
	static const FName BB_CanAttack;
	static const FName BB_IsInPause;
	static const FName BB_IsConversing;

	// ==================== STATIC ATTACKER TRACKING ====================
protected:
	static TArray<AEnemyBase*> ActiveAttackers;
	bool bIsActiveAttacker = false;

	// ==================== VIRTUAL METHODS FOR SUBCLASSES ====================
protected:
	virtual void OnStateEnter(EEnemyState NewState);
	virtual void OnStateExit(EEnemyState OldState);

	// ==================== BLUEPRINT EVENTS ====================
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnRandomPauseStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnRandomPauseEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnLookAroundStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnConversationGesture();
};
