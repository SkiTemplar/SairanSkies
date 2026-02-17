// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyBase.generated.h"

// Forward declarations
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;
class UBlackboardComponent;
class APatrolPath;
class UEnemyAnimInstance;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

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

	// ==================== COMPONENTS ====================
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComponent;

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

	// ==================== SERIALIZABLE ASSETS ====================
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	FEnemyAnimationConfig AnimationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sound")
	FEnemySoundConfig SoundConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|VFX")
	FEnemyVFXConfig VFXConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Mesh")
	FEnemyMeshConfig MeshConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sockets")
	FEnemySocketConfig SocketConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Conversation")
	FEnemyConversationConfig ConversationConfig;

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
	int32 NearbyAlliesCount;

	// ==================== SUSPICION SYSTEM (AAA-style awareness) ====================
protected:
	// Nivel de sospecha actual (0 = tranquilo, 1 = alerta máxima)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Awareness")
	float CurrentSuspicionLevel;

	// Si está en estado de alerta (vio algo sospechoso)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Awareness")
	bool bIsAlerted;

	// Timer de reacción (simula tiempo de procesamiento)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Awareness")
	float ReactionTimer;

	// Si está procesando una detección
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Awareness")
	bool bIsProcessingDetection;

	// Actor que causó la sospecha
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Awareness")
	AActor* SuspiciousActor;

	// ==================== IDLE BEHAVIOR STATE ====================
protected:
	// Si está en una pausa aleatoria
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|IdleBehavior")
	bool bIsInRandomPause;

	// Timer para la pausa actual
	float RandomPauseTimer;
	float RandomPauseDuration;

	// Rotación objetivo para mirar alrededor
	FRotator LookAroundTargetRotation;
	bool bIsLookingAround;
	float LookAroundTimer;

	// Velocidad de patrulla modificada actual
	float CurrentPatrolSpeedModifier;

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

	// ==================== PERCEPTION ====================
public:
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Perception")
	void UpdateTargetPerception();

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

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void Die(AController* InstigatorController);

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool CanAttack() const { return bCanAttack && CurrentState != EEnemyState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetHealthPercent() const { return MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool IsDead() const { return CurrentState == EEnemyState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetDistanceToTarget() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool IsInAttackRange() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool ShouldApproachTarget() const;

	// ==================== ALLY COORDINATION ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void AlertNearbyAllies(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void ReceiveAlertFromAlly(AActor* Target, AEnemyBase* AlertingAlly);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	void UpdateNearbyAlliesCount();

	UFUNCTION(BlueprintPure, Category = "Enemy|Coordination")
	int32 GetNearbyAlliesCount() const { return NearbyAlliesCount; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Coordination")
	bool HasEnoughAlliesForAggression() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Coordination")
	TArray<AEnemyBase*> GetNearbyAllies() const;

	// ==================== TAUNTING ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Behavior")
	virtual void PerformTaunt();

	UFUNCTION(BlueprintPure, Category = "Enemy|Behavior")
	virtual bool ShouldTaunt() const;

	// ==================== SUSPICION SYSTEM ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Awareness")
	void AddSuspicion(float Amount, AActor* Source);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Awareness")
	void SetFullAlert(AActor* Source);

	UFUNCTION(BlueprintPure, Category = "Enemy|Awareness")
	float GetSuspicionLevel() const { return CurrentSuspicionLevel; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Awareness")
	bool IsAlerted() const { return bIsAlerted; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Awareness")
	bool IsProcessingDetection() const { return bIsProcessingDetection; }

protected:
	void UpdateSuspicionSystem(float DeltaTime);
	void ProcessDetectionReaction();

	// ==================== IDLE/NATURAL BEHAVIOR ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|IdleBehavior")
	void StartRandomPause();

	UFUNCTION(BlueprintCallable, Category = "Enemy|IdleBehavior")
	void StartLookingAround();

	UFUNCTION(BlueprintPure, Category = "Enemy|IdleBehavior")
	bool IsInRandomPause() const { return bIsInRandomPause; }

	UFUNCTION(BlueprintPure, Category = "Enemy|IdleBehavior")
	bool IsLookingAround() const { return bIsLookingAround; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|IdleBehavior")
	bool ShouldDoRandomPause() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|IdleBehavior")
	float GetRandomizedPatrolSpeed() const;

protected:
	void UpdateIdleBehavior(float DeltaTime);
	void UpdateLookAround(float DeltaTime);

	// ==================== ANIMATION ====================
public:
	// Get the enemy's animation instance
	UFUNCTION(BlueprintPure, Category = "Enemy|Animation")
	UEnemyAnimInstance* GetEnemyAnimInstance() const;

	// Play an attack montage
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void PlayAttackMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

	// Play a taunt montage
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void PlayTauntMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

	// Play a hit reaction montage
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void PlayHitReactionMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

	// Set where the enemy should look (via animation, not body rotation)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void SetAnimationLookAtTarget(FVector WorldLocation);

	// Set look at rotation directly
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void SetAnimationLookAtRotation(float Yaw, float Pitch);

	// Clear look at animation
	UFUNCTION(BlueprintCallable, Category = "Enemy|Animation")
	void ClearAnimationLookAt();

protected:
	// Animation montages (set in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TArray<UAnimMontage*> TauntMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TArray<UAnimMontage*> HitReactionMontages;

	// ==================== MOVEMENT ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetMovementSpeed(float SpeedMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetPatrolSpeed();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetChaseSpeed();

protected:
	UPROPERTY()
	float BaseMaxWalkSpeed;

	// ==================== BLACKBOARD KEYS ====================
public:
	static const FName BB_TargetActor;
	static const FName BB_TargetLocation;
	static const FName BB_EnemyState;
	static const FName BB_CanSeeTarget;
	static const FName BB_PatrolIndex;
	static const FName BB_ShouldTaunt;
	static const FName BB_NearbyAllies;
	static const FName BB_DistanceToTarget;
	static const FName BB_SuspicionLevel;
	static const FName BB_IsAlerted;
	static const FName BB_IsInPause;
	static const FName BB_IsConversing;
	static const FName BB_ConversationPartner;

protected:
	void UpdateBlackboard();
	float AttackCooldownTimer;

	// ==================== CONVERSATION SYSTEM ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	bool TryStartConversation(AEnemyBase* OtherEnemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	void JoinConversation(AEnemyBase* Initiator);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	void EndConversation();

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	bool IsConversing() const { return bIsConversing; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	AEnemyBase* GetConversationPartner() const { return ConversationPartner; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Conversation")
	bool CanStartConversation() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Conversation")
	AEnemyBase* FindNearbyEnemyForConversation() const;

protected:
	void UpdateConversation(float DeltaTime);
	void PerformConversationGesture();
	void PlayConversationVoice();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	bool bIsConversing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	AEnemyBase* ConversationPartner = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	float ConversationTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	float ConversationDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	float GestureTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	float ConversationCooldownTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Conversation")
	float TimeStandingStill = 0.0f;

	FVector LastPosition;
	bool bIsInitiator = false;

	// ==================== SOUND/VFX HELPERS ====================
public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Audio")
	void PlayRandomSound(const TArray<USoundBase*>& Sounds, FName SocketName = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Audio")
	void PlayVoice(USoundBase* Sound);

	UFUNCTION(BlueprintCallable, Category = "Enemy|VFX")
	void SpawnEffect(UNiagaraSystem* Effect, FName SocketName = NAME_None, FVector Offset = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Enemy|VFX")
	void SpawnEffectAtLocation(UNiagaraSystem* Effect, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

protected:
	float LastVoiceTime = 0.0f;

	// ==================== VIRTUAL METHODS FOR SUBCLASSES ====================
protected:
	virtual void OnStateEnter(EEnemyState NewState);
	virtual void OnStateExit(EEnemyState OldState);
	virtual void HandleCombatBehavior(float DeltaTime);

	// ==================== DELEGATES ====================
public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnConversationStarted OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnConversationEnded OnConversationEnded;

	// ==================== BLUEPRINT EVENTS ====================
public:
	// Llamado cuando el enemigo empieza una pausa aleatoria (para animaciones)
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnRandomPauseStarted();

	// Llamado cuando el enemigo termina una pausa aleatoria
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnRandomPauseEnded();

	// Llamado cuando el enemigo mira alrededor
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnLookAroundStarted();

	// Llamado cuando el nivel de sospecha cambia significativamente
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnSuspicionChanged(float NewLevel, float OldLevel);

	// Llamado cuando el enemigo muestra confusión (durante investigación)
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnShowConfusion();

	// Llamado cuando empieza una conversación con otro enemigo
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnConversationStartedEvent(AEnemyBase* Partner);

	// Llamado cuando termina una conversación
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnConversationEndedEvent();

	// Llamado cuando hace un gesto durante la conversación
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnConversationGesture();
};
