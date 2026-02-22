// SairanSkies - Group Combat Manager
// Manages which enemies can attack simultaneously (Batman Arkham style)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GroupCombatManager.generated.h"

class AEnemyBase;

/**
 * Manages enemy attack slots and proximity-based priority.
 * Batman Arkham-style: only MaxSimultaneousAttackers can attack at once.
 * When the player moves to a new group, those enemies get priority.
 * Enemies without a slot circle/flank instead of rushing in.
 *
 * Flow for each enemy:
 *   1. Detect player → RegisterCombatEnemy
 *   2. Run to the CIRCLE first (never rush blindly into the player)
 *   3. From the circle, the manager assigns attack turns via RequestAttackSlot
 *   4. After attacking, enemy returns to the circle with a cooldown
 */
UCLASS()
class SAIRANSKIES_API UGroupCombatManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========== CONFIGURATION ==========

	/** Max enemies attacking the player at the same time */
	int32 MaxSimultaneousAttackers = 3;

	/** Radius to consider enemies in the "current group" near the player */
	float GroupProximityRadius = 800.0f;

	/** How far flanking/circling enemies stay from the player */
	float FlankingMinRadius = 350.0f;
	float FlankingMaxRadius = 600.0f;

	/** Cooldown before an ex-attacker can request a slot again */
	float SlotCooldownTime = 1.5f;

	/** Max enemies allowed in the inner flanking circle */
	int32 MaxFlankingEnemies = 3;

	/** Outer ring radius for enemies that don't have a flanking slot */
	float OuterRingMinRadius = 600.0f;
	float OuterRingMaxRadius = 850.0f;

	// ========== SLOT MANAGEMENT ==========

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	bool RequestAttackSlot(AEnemyBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void ReleaseAttackSlot(AEnemyBase* Enemy);

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool CanEnemyAttack(AEnemyBase* Enemy) const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetActiveAttackerCount() const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool IsActiveAttacker(AEnemyBase* Enemy) const;

	// ========== FLANKING ==========

	/** Get a flanking position on the inner ring for this enemy */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	FVector GetFlankingPosition(AEnemyBase* Enemy, AActor* Target) const;

	/** Get a position on the outer ring for overflow enemies */
	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	FVector GetOuterRingPosition(AEnemyBase* Enemy, AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	bool CanEnemyFlank(AEnemyBase* Enemy) const;

	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetFlankingEnemyCount() const;

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	bool RequestFlankingSlot(AEnemyBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void ReleaseFlankingSlot(AEnemyBase* Enemy);

	// ========== PRIORITY ==========

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void UpdateProximityPriorities(const FVector& PlayerLocation);

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void RegisterCombatEnemy(AEnemyBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "GroupCombat")
	void UnregisterCombatEnemy(AEnemyBase* Enemy);

	/** How many enemies are currently in combat */
	UFUNCTION(BlueprintPure, Category = "GroupCombat")
	int32 GetTotalCombatEnemies() const { return CombatEnemies.Num(); }

private:
	UPROPERTY()
	TArray<AEnemyBase*> ActiveAttackers;

	UPROPERTY()
	TArray<AEnemyBase*> CombatEnemies;

	UPROPERTY()
	TArray<AEnemyBase*> FlankingEnemies;

	TMap<TWeakObjectPtr<AEnemyBase>, float> ProximityScores;
	TMap<TWeakObjectPtr<AEnemyBase>, float> SlotCooldowns;

	void PurgeInvalidEnemies();
	AEnemyBase* FindLowestPriorityAttacker() const;

	/**
	 * Compute a ring position ensuring enemies are well-spread around the target.
	 * @param EnemyIndex  Which "slot" on the ring this enemy occupies (0..N-1)
	 * @param TotalOnRing How many enemies share this ring
	 * @param MinR / MaxR Radius range
	 * @param TargetLoc   Center of the ring
	 * @param Seed        Per-enemy seed for slight randomness so they don't perfectly align
	 */
	FVector ComputeRingPosition(int32 EnemyIndex, int32 TotalOnRing,
		float MinR, float MaxR, const FVector& TargetLoc, int32 Seed) const;
};
