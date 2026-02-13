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

protected:
	void UpdateBlackboard();
	float AttackCooldownTimer;

	// ==================== VIRTUAL METHODS FOR SUBCLASSES ====================
protected:
	virtual void OnStateEnter(EEnemyState NewState);
	virtual void OnStateExit(EEnemyState OldState);
	virtual void HandleCombatBehavior(float DeltaTime);
};
