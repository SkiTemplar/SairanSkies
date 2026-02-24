// SairanSkies - BT Task: Outer Circle Behavior
// Enemies wait in the outer circle, do taunting/feints, and wait for their turn

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_OuterCircleBehavior.generated.h"

class AEnemyBase;

/**
 * Outer Circle behavior — enemies stay at ~5m, do taunts, feints,
 * and lateral movement. They look menacing while waiting for an
 * opening in the inner circle.
 *
 * Exits with Succeeded when the enemy gets an inner circle slot.
 * Also repositions naturally when the player moves away.
 */
UCLASS()
class SAIRANSKIES_API UBTTask_OuterCircleBehavior : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_OuterCircleBehavior();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

protected:
	// ── Movement ──
	/** Speed multiplier while in outer circle */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Movement", meta = (ClampMin = "0.1", ClampMax = "0.6"))
	float CircleSpeedMultiplier = 0.25f;

	/** How often to pick a new position on the ring */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Movement", meta = (ClampMin = "2.0"))
	float RepositionInterval = 4.0f;

	/** Random variation on reposition interval */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Movement", meta = (ClampMin = "0.0"))
	float RepositionVariation = 1.5f;

	// ── Taunting / Feints ──
	/** Chance per second to do a taunt (fake rush towards player) */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Taunt", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float TauntChancePerSecond = 0.12f;

	/** Distance of the taunt lunge towards player */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Taunt", meta = (ClampMin = "30.0"))
	float TauntLungeDistance = 150.0f;

	/** Duration of the forward taunt lunge */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Taunt", meta = (ClampMin = "0.1"))
	float TauntLungeDuration = 0.3f;

	/** Duration of the retreat after taunt */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Taunt", meta = (ClampMin = "0.1"))
	float TauntRetreatDuration = 0.5f;

	// ── Sway ──
	/** Lateral sway amplitude */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Sway", meta = (ClampMin = "0.0"))
	float SwayAmplitude = 40.0f;

	/** Sway oscillation speed */
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Sway", meta = (ClampMin = "0.1"))
	float SwaySpeed = 1.5f;

	// ── Check interval to try entering inner circle ──
	UPROPERTY(EditAnywhere, Category = "OuterCircle|Timing", meta = (ClampMin = "0.2"))
	float InnerCircleCheckInterval = 0.5f;

private:
	// Timers
	float TaskTimer = 0.0f;
	float RepositionTimer = 0.0f;
	float NextRepositionTime = 4.0f;
	float TauntAccumulator = 0.0f;
	float SwayPhase = 0.0f;
	float InnerCheckTimer = 0.0f;
	float ReactionDelay = 0.0f;
	float ReactionTimer = 0.0f;
	bool bWaitingToReposition = false;

	// Last known player position for reaction delay
	FVector LastPlayerPosition = FVector::ZeroVector;

	// Taunt state machine
	enum class ETauntState : uint8 { None, Lunging, Retreating };
	ETauntState TauntState = ETauntState::None;
	float TauntTimer = 0.0f;
	FVector TauntOrigin = FVector::ZeroVector;

	// Movement target
	FVector CurrentRingTarget = FVector::ZeroVector;
	bool bReachedTarget = false;

	// Helpers
	void PickNewRingTarget(AEnemyBase* Enemy, AActor* Target);
	void FacePlayer(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void MoveTowardsFacingPlayer(AEnemyBase* Enemy, AActor* Target, const FVector& Dest, float DeltaTime);
	void ApplySway(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void StartTaunt(AEnemyBase* Enemy, AActor* Target);
	void UpdateTaunt(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void CleanupState(AEnemyBase* Enemy);
};

