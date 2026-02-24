// SairanSkies - Group Combat Manager (Two-Circle Model)
// Outer circle: enemies wait, taunt, fake advances
// Inner circle: only 1-2 enemies attack at a time

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GroupCombatManager.generated.h"

class AEnemyBase;

/**
 * Manages enemy combat positioning using two concentric circles.
 *
 * OUTER CIRCLE (~5m): Enemies wait here, doing taunts and feints.
 * INNER CIRCLE (attack range): Only MaxInnerCircleEnemies can be here.
 *
 * Flow:
 *   1. Enemy detects player → RegisterCombatEnemy → goes to outer circle
 *   2. From outer circle, checks RequestInnerCircleEntry
 *   3. If granted → enters inner circle, positions randomly, attacks
 *   4. After attack → probability to stay or retreat to outer circle
 *   5. If retreats → NotifyInnerCircleFreed → next outer enemy advances
 */
UCLASS()
class SAIRANSKIES_API UGroupCombatManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========== CONFIGURATION ==========

	/** Max enemies allowed in the inner circle (attacking) at the same time */
	int32 MaxInnerCircleEnemies = 2;

	/** Cooldown before an enemy can re-enter the inner circle after retreating */
	float InnerCircleCooldown = 2.0f;

	// ========== REGISTRATION ==========

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void RegisterCombatEnemy(AEnemyBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void UnregisterCombatEnemy(AEnemyBase* Enemy);

	// ========== INNER CIRCLE ==========

	/** Request to enter the inner circle. Returns true if there's space. */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	bool RequestInnerCircleEntry(AEnemyBase* Enemy);

	/** Called when an enemy finishes attacking.
	 *  bStayInner: if true, stays in inner; if false, retreats to outer.
	 *  Returns the next enemy that should advance from outer circle (can be nullptr).
	 */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	AEnemyBase* OnAttackFinished(AEnemyBase* Enemy, bool bStayInner);

	/** Force an enemy out of the inner circle */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void ForceToOuterCircle(AEnemyBase* Enemy);

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool IsInInnerCircle(AEnemyBase* Enemy) const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool IsInOuterCircle(AEnemyBase* Enemy) const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetInnerCircleCount() const { return InnerCircleEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetOuterCircleCount() const { return OuterCircleEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetTotalCombatEnemies() const { return CombatEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool HasInnerCircleSpace() const;

	// ========== POSITIONING ==========

	/** Get a position on the outer circle for this enemy */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	FVector GetOuterCirclePosition(AEnemyBase* Enemy, AActor* Target) const;

	/** Pick a random attack position within the inner circle */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	FVector GetInnerCircleAttackPosition(AEnemyBase* Enemy, AActor* Target) const;

	// ========== LEGACY COMPATIBILITY ==========

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	bool RequestAttackSlot(AEnemyBase* Enemy) { return RequestInnerCircleEntry(Enemy); }

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void ReleaseAttackSlot(AEnemyBase* Enemy) { ForceToOuterCircle(Enemy); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool CanEnemyAttack(AEnemyBase* Enemy) const { return HasInnerCircleSpace() || IsInInnerCircle(Enemy); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetActiveAttackerCount() const { return InnerCircleEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool IsActiveAttacker(AEnemyBase* Enemy) const { return IsInInnerCircle(Enemy); }

	FVector GetFlankingPosition(AEnemyBase* Enemy, AActor* Target) const { return GetOuterCirclePosition(Enemy, Target); }
	FVector GetOuterRingPosition(AEnemyBase* Enemy, AActor* Target) const { return GetOuterCirclePosition(Enemy, Target); }
	bool CanEnemyFlank(AEnemyBase* Enemy) const { return true; }
	int32 GetFlankingEnemyCount() const { return OuterCircleEnemies.Num(); }
	bool RequestFlankingSlot(AEnemyBase* Enemy) { return true; }
	void ReleaseFlankingSlot(AEnemyBase* Enemy) {}
	void UpdateProximityPriorities(const FVector& PlayerLocation) {}

private:
	UPROPERTY()
	TArray<AEnemyBase*> CombatEnemies;

	UPROPERTY()
	TArray<AEnemyBase*> OuterCircleEnemies;

	UPROPERTY()
	TArray<AEnemyBase*> InnerCircleEnemies;

	TMap<TWeakObjectPtr<AEnemyBase>, float> InnerCooldowns;

	void PurgeInvalidEnemies();

	AEnemyBase* PickNextFromOuterCircle(AActor* Target) const;

	FVector ComputeRingPosition(int32 EnemyIndex, int32 TotalOnRing,
		float MinR, float MaxR, const FVector& Center, int32 Seed) const;
};
