// SairanSkies - Group Combat Manager (Two-Circle Model)

#include "AI/GroupCombatManager.h"
#include "Enemies/EnemyBase.h"
#include "Engine/World.h"

void UGroupCombatManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGroupCombatManager::Deinitialize()
{
	CombatEnemies.Empty();
	OuterCircleEnemies.Empty();
	InnerCircleEnemies.Empty();
	InnerCooldowns.Empty();
	Super::Deinitialize();
}

// ═══════════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════════

void UGroupCombatManager::PurgeInvalidEnemies()
{
	auto IsInvalid = [](const AEnemyBase* E) { return !IsValid(E) || E->IsDead(); };
	CombatEnemies.RemoveAll(IsInvalid);
	OuterCircleEnemies.RemoveAll(IsInvalid);
	InnerCircleEnemies.RemoveAll(IsInvalid);

	TArray<TWeakObjectPtr<AEnemyBase>> KeysToRemove;
	for (auto& Pair : InnerCooldowns)
	{
		if (!Pair.Key.IsValid() || Pair.Key->IsDead())
			KeysToRemove.Add(Pair.Key);
	}
	for (auto& Key : KeysToRemove)
		InnerCooldowns.Remove(Key);
}

FVector UGroupCombatManager::ComputeRingPosition(int32 EnemyIndex, int32 TotalOnRing,
	float MinR, float MaxR, const FVector& Center, int32 Seed) const
{
	if (TotalOnRing <= 0) TotalOnRing = 1;

	// Golden-angle distribution for organic spread
	const float GoldenAngle = 137.508f;
	float AngleDeg = GoldenAngle * EnemyIndex;

	FRandomStream Rng(Seed + EnemyIndex * 7919);
	AngleDeg += Rng.FRandRange(-20.0f, 20.0f);

	float AngleRad = FMath::DegreesToRadians(AngleDeg);
	float Radius = Rng.FRandRange(MinR, MaxR);

	FVector Pos = Center;
	Pos.X += FMath::Cos(AngleRad) * Radius;
	Pos.Y += FMath::Sin(AngleRad) * Radius;
	return Pos;
}

AEnemyBase* UGroupCombatManager::PickNextFromOuterCircle(AActor* Target) const
{
	if (OuterCircleEnemies.Num() == 0) return nullptr;

	// Weighted random: closer enemies have higher weight
	float TotalWeight = 0.0f;
	TArray<float> Weights;
	Weights.Reserve(OuterCircleEnemies.Num());

	for (AEnemyBase* E : OuterCircleEnemies)
	{
		if (!IsValid(E)) { Weights.Add(0.0f); continue; }

		// Check cooldown
		TWeakObjectPtr<AEnemyBase> W(const_cast<AEnemyBase*>(E));
		if (const float* CD = InnerCooldowns.Find(W))
		{
			if (GetWorld()->GetTimeSeconds() < *CD)
			{
				Weights.Add(0.0f);
				continue;
			}
		}

		float Dist = Target ? FVector::Dist(E->GetActorLocation(), Target->GetActorLocation()) : 1000.0f;
		float Weight = FMath::Max(1.0f, 2000.0f - Dist); // closer = higher weight
		Weights.Add(Weight);
		TotalWeight += Weight;
	}

	if (TotalWeight <= 0.0f) return nullptr;

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float Accum = 0.0f;
	for (int32 i = 0; i < OuterCircleEnemies.Num(); i++)
	{
		Accum += Weights[i];
		if (Roll <= Accum)
			return OuterCircleEnemies[i];
	}

	return OuterCircleEnemies.Last();
}

// ═══════════════════════════════════════════════════════════════════════════
// REGISTRATION
// ═══════════════════════════════════════════════════════════════════════════

void UGroupCombatManager::RegisterCombatEnemy(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy)) return;
	CombatEnemies.AddUnique(Enemy);
	// New enemies go to outer circle by default
	if (!InnerCircleEnemies.Contains(Enemy))
	{
		OuterCircleEnemies.AddUnique(Enemy);
	}
	UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s registered (outer=%d, inner=%d)"),
		*Enemy->GetName(), OuterCircleEnemies.Num(), InnerCircleEnemies.Num());
}

void UGroupCombatManager::UnregisterCombatEnemy(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	CombatEnemies.Remove(Enemy);
	OuterCircleEnemies.Remove(Enemy);
	InnerCircleEnemies.Remove(Enemy);
	TWeakObjectPtr<AEnemyBase> W(Enemy);
	InnerCooldowns.Remove(W);
	UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s unregistered (outer=%d, inner=%d)"),
		*Enemy->GetName(), OuterCircleEnemies.Num(), InnerCircleEnemies.Num());
}

// ═══════════════════════════════════════════════════════════════════════════
// INNER CIRCLE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

bool UGroupCombatManager::RequestInnerCircleEntry(AEnemyBase* Enemy)
{
	if (!IsValid(Enemy)) return false;
	PurgeInvalidEnemies();

	// Already inside
	if (InnerCircleEnemies.Contains(Enemy)) return true;

	// Check cooldown
	TWeakObjectPtr<AEnemyBase> W(Enemy);
	if (const float* CD = InnerCooldowns.Find(W))
	{
		if (GetWorld()->GetTimeSeconds() < *CD)
		{
			return false;
		}
		InnerCooldowns.Remove(W);
	}

	// Check space
	if (InnerCircleEnemies.Num() >= MaxInnerCircleEnemies)
		return false;

	// Grant entry
	InnerCircleEnemies.AddUnique(Enemy);
	OuterCircleEnemies.Remove(Enemy);

	UE_LOG(LogTemp, Warning, TEXT("GroupCombat: %s → INNER CIRCLE (%d/%d)"),
		*Enemy->GetName(), InnerCircleEnemies.Num(), MaxInnerCircleEnemies);
	return true;
}

AEnemyBase* UGroupCombatManager::OnAttackFinished(AEnemyBase* Enemy, bool bStayInner)
{
	if (!IsValid(Enemy)) return nullptr;
	PurgeInvalidEnemies();

	AEnemyBase* NextAttacker = nullptr;

	if (bStayInner)
	{
		// Stay in inner circle — no one advances
		UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s stays in inner circle after attack"), *Enemy->GetName());
		return nullptr;
	}

	// Retreat to outer circle
	InnerCircleEnemies.Remove(Enemy);
	OuterCircleEnemies.AddUnique(Enemy);

	// Set cooldown
	TWeakObjectPtr<AEnemyBase> W(Enemy);
	InnerCooldowns.Add(W, GetWorld()->GetTimeSeconds() + InnerCircleCooldown);

	UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s → OUTER CIRCLE (cooldown %.1fs)"),
		*Enemy->GetName(), InnerCircleCooldown);

	// Pick next attacker from outer circle
	AActor* Target = Enemy->GetCurrentTarget();
	NextAttacker = PickNextFromOuterCircle(Target);

	if (NextAttacker)
	{
		UE_LOG(LogTemp, Warning, TEXT("GroupCombat: %s should advance to inner circle next"),
			*NextAttacker->GetName());
	}

	return NextAttacker;
}

void UGroupCombatManager::ForceToOuterCircle(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	if (InnerCircleEnemies.Remove(Enemy) > 0)
	{
		OuterCircleEnemies.AddUnique(Enemy);
		TWeakObjectPtr<AEnemyBase> W(Enemy);
		InnerCooldowns.Add(W, GetWorld()->GetTimeSeconds() + InnerCircleCooldown);
		UE_LOG(LogTemp, Log, TEXT("GroupCombat: %s forced to outer circle"), *Enemy->GetName());
	}
}

bool UGroupCombatManager::IsInInnerCircle(AEnemyBase* Enemy) const
{
	return InnerCircleEnemies.Contains(Enemy);
}

bool UGroupCombatManager::IsInOuterCircle(AEnemyBase* Enemy) const
{
	return OuterCircleEnemies.Contains(Enemy);
}

bool UGroupCombatManager::HasInnerCircleSpace() const
{
	return InnerCircleEnemies.Num() < MaxInnerCircleEnemies;
}

// ═══════════════════════════════════════════════════════════════════════════
// POSITIONING
// ═══════════════════════════════════════════════════════════════════════════

FVector UGroupCombatManager::GetOuterCirclePosition(AEnemyBase* Enemy, AActor* Target) const
{
	if (!IsValid(Enemy) || !IsValid(Target))
		return Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;

	int32 Idx = OuterCircleEnemies.IndexOfByKey(Enemy);
	if (Idx == INDEX_NONE) Idx = CombatEnemies.IndexOfByKey(Enemy);
	if (Idx == INDEX_NONE) Idx = 0;

	float Radius = Enemy->CombatConfig.OuterCircleRadius;
	float Var = Enemy->CombatConfig.OuterCircleVariation;

	int32 Seed = Enemy->GetUniqueID();
	FVector Pos = ComputeRingPosition(Idx, FMath::Max(1, OuterCircleEnemies.Num()),
		Radius - Var, Radius + Var, Target->GetActorLocation(), Seed);
	Pos.Z = Enemy->GetActorLocation().Z;
	return Pos;
}

FVector UGroupCombatManager::GetInnerCircleAttackPosition(AEnemyBase* Enemy, AActor* Target) const
{
	if (!IsValid(Enemy) || !IsValid(Target))
		return Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;

	float MinDist = Enemy->CombatConfig.MinAttackPositionDist;
	float MaxDist = Enemy->CombatConfig.MaxAttackPositionDist;

	// Random angle towards the player with some spread
	FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	float AngleSpread = FMath::RandRange(-40.0f, 40.0f);
	FVector Dir = ToTarget.RotateAngleAxis(AngleSpread, FVector::UpVector);

	float Dist = FMath::RandRange(MinDist, MaxDist);

	FVector Pos = Target->GetActorLocation() - Dir * Dist;
	Pos.Z = Enemy->GetActorLocation().Z;
	return Pos;
}

