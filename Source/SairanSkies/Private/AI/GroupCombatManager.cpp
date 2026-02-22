// SairanSkies - Group Combat Manager Implementation

#include "AI/GroupCombatManager.h"
#include "Enemies/EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UGroupCombatManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGroupCombatManager::Deinitialize()
{
	ActiveAttackers.Empty();
	CombatEnemies.Empty();
	FlankingEnemies.Empty();
	ProximityScores.Empty();
	SlotCooldowns.Empty();
	Super::Deinitialize();
}

// ═══════════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════════

void UGroupCombatManager::PurgeInvalidEnemies()
{
	auto IsInvalid = [](const AEnemyBase* E) { return !IsValid(E) || E->IsDead(); };
	ActiveAttackers.RemoveAll(IsInvalid);
	CombatEnemies.RemoveAll(IsInvalid);
	FlankingEnemies.RemoveAll(IsInvalid);

	TArray<TWeakObjectPtr<AEnemyBase>> KeysToRemove;
	for (auto& Pair : ProximityScores)
	{
		if (!Pair.Key.IsValid() || Pair.Key->IsDead())
			KeysToRemove.Add(Pair.Key);
	}
	for (auto& Key : KeysToRemove)
	{
		ProximityScores.Remove(Key);
		SlotCooldowns.Remove(Key);
	}
}

FVector UGroupCombatManager::ComputeRingPosition(int32 EnemyIndex, int32 TotalOnRing,
	float MinR, float MaxR, const FVector& TargetLoc, int32 Seed) const
{
	if (TotalOnRing <= 0) TotalOnRing = 1;

	// Golden-angle distribution so positions look organic, not perfectly even
	// Golden angle ≈ 137.508°
	const float GoldenAngle = 137.508f;
	float AngleDeg = GoldenAngle * EnemyIndex;

	// Per-enemy jitter so two enemies at index 0 of different rings don't overlap
	FRandomStream Rng(Seed + EnemyIndex * 7919);
	AngleDeg += Rng.FRandRange(-15.0f, 15.0f);

	float AngleRad = FMath::DegreesToRadians(AngleDeg);
	float Radius = Rng.FRandRange(MinR, MaxR);

	FVector Pos = TargetLoc;
	Pos.X += FMath::Cos(AngleRad) * Radius;
	Pos.Y += FMath::Sin(AngleRad) * Radius;
	return Pos;
}

AEnemyBase* UGroupCombatManager::FindLowestPriorityAttacker() const
{
	AEnemyBase* Lowest = nullptr;
	float LowestScore = MAX_FLT;
	for (AEnemyBase* A : ActiveAttackers)
	{
		if (!IsValid(A)) continue;
		TWeakObjectPtr<AEnemyBase> W(const_cast<AEnemyBase*>(A));
		const float* S = ProximityScores.Find(W);
		float Score = S ? *S : 0.0f;
		if (Score < LowestScore) { LowestScore = Score; Lowest = A; }
	}
	return Lowest;
}

// ═══════════════════════════════════════════════════════════════════════════
// ATTACK SLOT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

bool UGroupCombatManager::RequestAttackSlot(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy)) return false;
	PurgeInvalidEnemies();

	if (ActiveAttackers.Contains(Enemy)) return true;

	TWeakObjectPtr<AEnemyBase> WeakEnemy(Enemy);
	if (float* CD = SlotCooldowns.Find(WeakEnemy))
	{
		if (GetWorld()->GetTimeSeconds() < *CD) return false;
		SlotCooldowns.Remove(WeakEnemy);
	}

	if (ActiveAttackers.Num() < MaxSimultaneousAttackers)
	{
		ActiveAttackers.AddUnique(Enemy);
		FlankingEnemies.Remove(Enemy);
		UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s → ATTACK SLOT (%d/%d)"),
			*Enemy->GetName(), ActiveAttackers.Num(), MaxSimultaneousAttackers);
		return true;
	}

	// Priority swap
	float* MyScore = ProximityScores.Find(WeakEnemy);
	if (MyScore)
	{
		AEnemyBase* Lowest = FindLowestPriorityAttacker();
		if (Lowest)
		{
			TWeakObjectPtr<AEnemyBase> WL(Lowest);
			float* LS = ProximityScores.Find(WL);
			if (LS && *MyScore > *LS + 0.15f)
			{
				ReleaseAttackSlot(Lowest);
				ActiveAttackers.AddUnique(Enemy);
				FlankingEnemies.Remove(Enemy);
				UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s SWAPPED attack from %s (%.2f > %.2f)"),
					*Enemy->GetName(), *Lowest->GetName(), *MyScore, *LS);
				return true;
			}
		}
	}
	return false;
}

void UGroupCombatManager::ReleaseAttackSlot(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	if (ActiveAttackers.Remove(Enemy) > 0)
	{
		TWeakObjectPtr<AEnemyBase> W(Enemy);
		SlotCooldowns.Add(W, GetWorld()->GetTimeSeconds() + SlotCooldownTime);
		UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s released attack slot (%d/%d)"),
			*Enemy->GetName(), ActiveAttackers.Num(), MaxSimultaneousAttackers);
	}
}

bool UGroupCombatManager::CanEnemyAttack(AEnemyBase* Enemy) const
{
	if (!IsValid(Enemy)) return false;
	if (ActiveAttackers.Contains(Enemy)) return true;
	if (ActiveAttackers.Num() >= MaxSimultaneousAttackers) return false;

	TWeakObjectPtr<AEnemyBase> W(const_cast<AEnemyBase*>(Enemy));
	if (const float* CD = SlotCooldowns.Find(W))
	{
		if (GetWorld()->GetTimeSeconds() < *CD) return false;
	}
	return true;
}

int32 UGroupCombatManager::GetActiveAttackerCount() const { return ActiveAttackers.Num(); }
bool UGroupCombatManager::IsActiveAttacker(AEnemyBase* Enemy) const { return ActiveAttackers.Contains(Enemy); }

// ═══════════════════════════════════════════════════════════════════════════
// FLANKING SLOT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

bool UGroupCombatManager::CanEnemyFlank(AEnemyBase* Enemy) const
{
	if (!IsValid(Enemy)) return false;
	if (FlankingEnemies.Contains(Enemy)) return true;
	return FlankingEnemies.Num() < MaxFlankingEnemies;
}

int32 UGroupCombatManager::GetFlankingEnemyCount() const { return FlankingEnemies.Num(); }

bool UGroupCombatManager::RequestFlankingSlot(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy)) return false;
	PurgeInvalidEnemies();
	if (FlankingEnemies.Contains(Enemy)) return true;
	if (ActiveAttackers.Contains(Enemy)) return false;

	if (FlankingEnemies.Num() < MaxFlankingEnemies)
	{
		FlankingEnemies.AddUnique(Enemy);
		return true;
	}

	// Priority swap with furthest flanker
	TWeakObjectPtr<AEnemyBase> WE(Enemy);
	const float* MyScore = ProximityScores.Find(WE);
	if (!MyScore) return false;

	AEnemyBase* Worst = nullptr;
	float WorstScore = MAX_FLT;
	for (AEnemyBase* F : FlankingEnemies)
	{
		if (!IsValid(F)) continue;
		TWeakObjectPtr<AEnemyBase> WF(F);
		const float* FS = ProximityScores.Find(WF);
		float S = FS ? *FS : 0.0f;
		if (S < WorstScore) { WorstScore = S; Worst = F; }
	}
	if (Worst && *MyScore > WorstScore + 0.15f)
	{
		FlankingEnemies.Remove(Worst);
		FlankingEnemies.AddUnique(Enemy);
		return true;
	}
	return false;
}

void UGroupCombatManager::ReleaseFlankingSlot(AEnemyBase* Enemy)
{
	if (Enemy) FlankingEnemies.Remove(Enemy);
}

// ═══════════════════════════════════════════════════════════════════════════
// RING POSITIONS
// ═══════════════════════════════════════════════════════════════════════════

FVector UGroupCombatManager::GetFlankingPosition(AEnemyBase* Enemy, AActor* Target) const
{
	if (!IsValid(Enemy) || !IsValid(Target))
		return Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;

	int32 Idx = FlankingEnemies.IndexOfByKey(Enemy);
	if (Idx == INDEX_NONE) Idx = 0;

	int32 Seed = Enemy->GetUniqueID();
	return ComputeRingPosition(Idx, FlankingEnemies.Num(),
		FlankingMinRadius, FlankingMaxRadius, Target->GetActorLocation(), Seed);
}

FVector UGroupCombatManager::GetOuterRingPosition(AEnemyBase* Enemy, AActor* Target) const
{
	if (!IsValid(Enemy) || !IsValid(Target))
		return Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;

	// Find index among enemies that are NOT attacking and NOT inner-flanking
	int32 OuterIdx = 0;
	int32 TotalOuter = 0;
	for (AEnemyBase* E : CombatEnemies)
	{
		if (!IsValid(E)) continue;
		if (ActiveAttackers.Contains(E) || FlankingEnemies.Contains(E)) continue;
		if (E == Enemy) OuterIdx = TotalOuter;
		TotalOuter++;
	}

	int32 Seed = Enemy->GetUniqueID() + 31337;
	return ComputeRingPosition(OuterIdx, FMath::Max(1, TotalOuter),
		OuterRingMinRadius, OuterRingMaxRadius, Target->GetActorLocation(), Seed);
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIORITY & REGISTRATION
// ═══════════════════════════════════════════════════════════════════════════

void UGroupCombatManager::UpdateProximityPriorities(const FVector& PlayerLocation)
{
	PurgeInvalidEnemies();
	for (AEnemyBase* E : CombatEnemies)
	{
		if (!IsValid(E)) continue;
		float Dist = FVector::Dist(E->GetActorLocation(), PlayerLocation);
		float Score = FMath::Clamp(1.0f - (Dist / GroupProximityRadius), 0.0f, 1.0f);
		TWeakObjectPtr<AEnemyBase> W(E);
		ProximityScores.Add(W, Score);
	}
}

void UGroupCombatManager::RegisterCombatEnemy(AEnemyBase* Enemy)
{
	if (IsValid(Enemy)) CombatEnemies.AddUnique(Enemy);
}

void UGroupCombatManager::UnregisterCombatEnemy(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	CombatEnemies.Remove(Enemy);
	FlankingEnemies.Remove(Enemy);
	ActiveAttackers.Remove(Enemy);
	TWeakObjectPtr<AEnemyBase> W(Enemy);
	ProximityScores.Remove(W);
	SlotCooldowns.Remove(W);
}

