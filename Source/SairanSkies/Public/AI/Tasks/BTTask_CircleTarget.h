// SairanSkies - BT Task: Circle Target (intelligent flanking with feints)

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CircleTarget.generated.h"

class AEnemyBase;

/**
 * Intelligent flanking behavior — Batman Arkham style.
 *
 * Enemies NEVER rush blindly through allies. When they enter combat
 * they go to the circle first. From there:
 *
 *  • Inner-ring enemies (max 3): actively circle the player, doing feints
 *    (small lunges forward and retreats) to create tension.
 *  • Outer-ring enemies (overflow): hold position further out, swaying
 *    and occasionally stepping in, always facing the player.
 *
 * All enemies face the player at all times (walking sideways/backwards).
 * The task checks every tick if an attack slot opened up and exits immediately.
 */
UCLASS()
class SAIRANSKIES_API UBTTask_CircleTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CircleTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

protected:
	// ── Movement ──
	/** Speed multiplier while circling (relative to base walk speed) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CircleSpeedMultiplier = 0.4f;

	/** How often to pick a new position on the ring */
	UPROPERTY(EditAnywhere, Category = "Flanking|Movement", meta = (ClampMin = "1.0"))
	float RepositionInterval = 2.5f;

	/** Random ± variation on reposition interval */
	UPROPERTY(EditAnywhere, Category = "Flanking|Movement", meta = (ClampMin = "0.0"))
	float RepositionVariation = 1.0f;

	// ── Feints (inner ring only) ──
	/** Chance per second to perform a feint (lunge forward then retreat) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Feints", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FeintChancePerSecond = 0.25f;

	/** How far the feint lunge goes towards the player (units) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Feints", meta = (ClampMin = "10.0"))
	float FeintLungeDistance = 120.0f;

	/** Duration of the forward lunge portion (seconds) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Feints", meta = (ClampMin = "0.05"))
	float FeintLungeDuration = 0.25f;

	/** Duration of the retreat portion (seconds) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Feints", meta = (ClampMin = "0.05"))
	float FeintRetreatDuration = 0.35f;

	// ── Weight-shift sway (both rings) ──
	/** Lateral sway amplitude while standing/waiting (units) */
	UPROPERTY(EditAnywhere, Category = "Flanking|Sway", meta = (ClampMin = "0.0"))
	float SwayAmplitude = 35.0f;

	/** Speed of the lateral sway oscillation */
	UPROPERTY(EditAnywhere, Category = "Flanking|Sway", meta = (ClampMin = "0.1"))
	float SwaySpeed = 1.8f;

	// ── Timing ──
	/** Max time before this task returns so the BT can re-evaluate */
	UPROPERTY(EditAnywhere, Category = "Flanking|Timing", meta = (ClampMin = "1.0"))
	float MaxTaskDuration = 6.0f;

private:
	// State
	float TaskTimer = 0.0f;
	float RepositionTimer = 0.0f;
	float NextRepositionTime = 3.0f;
	float FeintAccumulator = 0.0f;  // rolls feint chance
	float SwayPhase = 0.0f;         // oscillation phase

	// Feint state machine
	enum class EFeintState : uint8 { None, Lunging, Retreating };
	EFeintState FeintState = EFeintState::None;
	float FeintTimer = 0.0f;
	FVector FeintOrigin = FVector::ZeroVector;

	// Flanking
	FVector CurrentRingTarget = FVector::ZeroVector;
	bool bHasFlankSlot = false;
	bool bReachedRingTarget = false;

	// Helpers
	void PickNewRingTarget(AEnemyBase* Enemy, AActor* Target);
	void MoveTowardsFacingPlayer(AEnemyBase* Enemy, AActor* Target, const FVector& Destination, float DeltaTime);
	void FacePlayer(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void ApplySway(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void StartFeint(AEnemyBase* Enemy, AActor* Target);
	void UpdateFeint(AEnemyBase* Enemy, AActor* Target, float DeltaTime);
	void CleanupState(AEnemyBase* Enemy);
};
