// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_AttackTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "AI/GroupCombatManager.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// COMBO SELECTION — Probabilistic algorithm based on distance
// 
// Given N combos (montages) and normalized distance T (0=close, 1=far):
//   Combo[0] has highest weight when CLOSE
//   Combo[N-1] has highest weight when FAR
//
// Weight formula: W(i) = max(0.1, 1.0 - abs(idealT - T))
//   where idealT(i) = i / (N-1) for N>1
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_AttackTarget::PickComboByDistance(AEnemyBase* Enemy)
{
	int32 NumMontages = Enemy->AnimationConfig.AttackMontages.Num();
	if (NumMontages <= 0)
	{
		// No montages — 1 debug hit
		TotalHits = 1;
		ChosenComboIndex = 0;
		return;
	}
	if (NumMontages == 1)
	{
		TotalHits = 1;
		ChosenComboIndex = 0;
		return;
	}

	// Normalized distance: 0 = at MinAttackPositionDist, 1 = at MaxAttackPositionDist
	float Dist = Enemy->GetDistanceToTarget();
	float MinD = Enemy->CombatConfig.MinAttackPositionDist;
	float MaxD = Enemy->CombatConfig.MaxAttackPositionDist;
	float T = FMath::Clamp((Dist - MinD) / FMath::Max(MaxD - MinD, 1.0f), 0.0f, 1.0f);

	// Calculate weights for each combo
	TArray<float> Weights;
	Weights.SetNum(NumMontages);
	float TotalWeight = 0.0f;

	for (int32 i = 0; i < NumMontages; i++)
	{
		// idealT for combo i: spreads evenly from 0 (close) to 1 (far)
		float IdealT = (NumMontages > 1) ? (float)i / (float)(NumMontages - 1) : 0.5f;

		// Weight = closeness to ideal position, with a floor so no combo is impossible
		float W = FMath::Max(0.1f, 1.0f - FMath::Abs(IdealT - T));

		// Boost weight slightly for closer combos (more aggressive feel)
		W *= (1.0f + (1.0f - IdealT) * 0.3f);

		Weights[i] = W;
		TotalWeight += W;
	}

	// Weighted random selection
	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float Accum = 0.0f;
	ChosenComboIndex = 0;
	for (int32 i = 0; i < NumMontages; i++)
	{
		Accum += Weights[i];
		if (Roll <= Accum)
		{
			ChosenComboIndex = i;
			break;
		}
	}

	// Close combos (low index) tend to be multi-hit; far combos single hit
	// The number of hits is: more hits for closer combos
	float CloseWeight = 1.0f - T;
	int32 MaxHits = FMath::Max(1, NumMontages);
	float MeanHits = FMath::Lerp(1.0f, (float)MaxHits, CloseWeight);
	TotalHits = FMath::Clamp(FMath::RoundToInt32(MeanHits + FMath::RandRange(-0.5f, 0.5f)), 1, MaxHits);

	UE_LOG(LogTemp, Log, TEXT("Attack: %s combo[%d] (%d hits) | dist=%.0f T=%.2f weights=[%s]"),
		*Enemy->GetName(), ChosenComboIndex, TotalHits, Dist, T,
		*FString::JoinBy(Weights, TEXT(","), [](float W) { return FString::Printf(TEXT("%.2f"), W); }));
}

// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_AttackTarget::ApplyDamage(AEnemyBase* Enemy, AActor* Target)
{
	if (!Enemy || !Target) return;

	float Var = FMath::RandRange(-DamageVariance, DamageVariance);
	float Dmg = FMath::Max(1.0f, Enemy->CombatConfig.BaseDamage * (1.0f + Var));

	UE_LOG(LogTemp, Log, TEXT("Attack: %s → %s  Dmg=%.1f (base %.1f %+.0f%%)"),
		*Enemy->GetName(), *Target->GetName(), Dmg, Enemy->CombatConfig.BaseDamage, Var * 100.0f);

	UGameplayStatics::ApplyDamage(Target, Dmg, Enemy->GetController(), Enemy, UDamageType::StaticClass());
}

void UBTTask_AttackTarget::ApplyDebugRotation(AEnemyBase* Enemy, bool bRestore)
{
	if (bRestore)
	{
		Enemy->SetActorRotation(OriginalRotation);
		return;
	}

	OriginalRotation = Enemy->GetActorRotation();
	float Sign = (CurrentHit % 2 == 0) ? 1.0f : -1.0f;
	FRotator Offset(0.0f, DebugRotationDegrees * Sign, 0.0f);
	Enemy->SetActorRotation(OriginalRotation + Offset);
}

void UBTTask_AttackTarget::Cleanup(UBehaviorTreeComponent& OwnerComp, AEnemyBase* Enemy)
{
	if (!Enemy) return;

	ApplyDebugRotation(Enemy, true);

	// Restore orient to movement
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = true;

	// Decide: stay in inner circle or retreat to outer
	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();
	if (Mgr)
	{
		bool bStay = FMath::FRand() < Enemy->CombatConfig.ChanceToStayInnerAfterAttack;
		AEnemyBase* NextAttacker = Mgr->OnAttackFinished(Enemy, bStay);

		if (bStay)
		{
			UE_LOG(LogTemp, Log, TEXT("Attack: %s se queda en inner circle"), *Enemy->GetName());
			Enemy->SetEnemyState(EEnemyState::InnerCircle);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Attack: %s vuelve al outer circle"), *Enemy->GetName());
			Enemy->SetEnemyState(EEnemyState::OuterCircle);
		}
	}
	else
	{
		Enemy->SetEnemyState(EEnemyState::Chasing);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTE — entry point (enemy already in inner circle)
// ═══════════════════════════════════════════════════════════════════════════

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) return EBTNodeResult::Failed;

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	// Reset state
	CurrentHit = 0;
	TotalHits = 1;
	ChosenComboIndex = 0;
	PhaseTimer = 0.0f;

	// Disable auto-orient — we control rotation manually
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = false;

	Enemy->SetEnemyState(EEnemyState::Attacking);

	// Pick a random attack position within the attack range
	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();
	if (Mgr)
	{
		AttackPosition = Mgr->GetInnerCircleAttackPosition(Enemy, Target);
	}
	else
	{
		// Fallback: position in front of target
		FVector Dir = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		float Dist = FMath::RandRange(Enemy->CombatConfig.MinAttackPositionDist, Enemy->CombatConfig.MaxAttackPositionDist);
		AttackPosition = Target->GetActorLocation() + Dir * Dist;
	}

	// Check if already in attack range
	float DistToTarget = Enemy->GetDistanceToTarget();
	if (DistToTarget <= Enemy->CombatConfig.MaxAttackPositionDist + 30.0f)
	{
		// Already in range — skip approach, start attacking
		Enemy->Attack(); // Start cooldown
		PickComboByDistance(Enemy);

		Phase = EAttackPhase::WindUp;
		PhaseTimer = 0.0f;
		AIC->StopMovement();

		UE_LOG(LogTemp, Warning, TEXT("=== ATTACK START: %s → %s | combo[%d] %d hits | dist=%.0f ==="),
			*Enemy->GetName(), *Target->GetName(), ChosenComboIndex, TotalHits, DistToTarget);
	}
	else
	{
		// Need to close the gap from outer circle → attack range
		Phase = EAttackPhase::Approach;
		PhaseTimer = 0.0f;
		Enemy->SetMovementSpeed(0.5f); // Moderate approach speed
		AIC->MoveToActor(Target, Enemy->CombatConfig.MinAttackPositionDist);

		UE_LOG(LogTemp, Log, TEXT("Attack: %s acercándose a posición de ataque (dist=%.0f)"),
			*Enemy->GetName(), DistToTarget);
	}

	return EBTNodeResult::InProgress;
}

// ═══════════════════════════════════════════════════════════════════════════
// TICK
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		Cleanup(OwnerComp, Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	PhaseTimer += DeltaSeconds;

	// Rotar suavemente hacia el target (excepto durante Strike)
	if (Phase != EAttackPhase::Strike)
	{
		FVector Dir = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
		FRotator Look = Dir.Rotation();
		Look.Pitch = 0.0f; Look.Roll = 0.0f;
		FRotator Cur = Enemy->GetActorRotation();
		Cur.Pitch = 0.0f; Cur.Roll = 0.0f;
		Enemy->SetActorRotation(FMath::RInterpTo(Cur, Look, DeltaSeconds, 8.0f));
	}

	switch (Phase)
	{
	// ═══════════════════════════════════════════════════════════════════
	// APPROACH — close the gap from outer circle to attack range
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Approach:
	{
		float Dist = Enemy->GetDistanceToTarget();

		if (Dist <= Enemy->CombatConfig.MaxAttackPositionDist + 30.0f)
		{
			AIC->StopMovement();
			Enemy->Attack(); // Start cooldown
			PickComboByDistance(Enemy);

			Phase = EAttackPhase::WindUp;
			PhaseTimer = 0.0f;

			UE_LOG(LogTemp, Warning, TEXT("=== ATTACK START: %s → %s | combo[%d] %d hits | dist=%.0f ==="),
				*Enemy->GetName(), *Target->GetName(), ChosenComboIndex, TotalHits, Dist);
			break;
		}

		// Re-request movement if stuck
		EPathFollowingStatus::Type Status = AIC->GetMoveStatus();
		if (Status == EPathFollowingStatus::Idle || Status == EPathFollowingStatus::Waiting)
		{
			AIC->MoveToActor(Target, Enemy->CombatConfig.MinAttackPositionDist);
		}

		// Timeout
		if (PhaseTimer >= MaxApproachTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack: %s approach timeout (dist=%.0f)"), *Enemy->GetName(), Dist);
			Cleanup(OwnerComp, Enemy);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// WINDUP — anticipación, guardar posición
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::WindUp:
	{
		if (PhaseTimer >= WindUpDuration)
		{
			PhaseTimer = 0.0f;
			Phase = EAttackPhase::Strike;
			CurrentHit++;

			StrikeStartLocation = Enemy->GetActorLocation();

			// Debug rotation
			ApplyDebugRotation(Enemy, false);

			// Debug lunge towards target
			if (DebugLungeDistance > 0.0f)
			{
				FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
				Enemy->SetActorLocation(StrikeStartLocation + ToTarget * DebugLungeDistance);
			}

			// Apply damage
			ApplyDamage(Enemy, Target);

			// Play montage if available
			int32 MontageIdx = (ChosenComboIndex + CurrentHit - 1) % FMath::Max(1, Enemy->AnimationConfig.AttackMontages.Num());
			if (Enemy->AnimationConfig.AttackMontages.IsValidIndex(MontageIdx))
			{
				UAnimMontage* M = Enemy->AnimationConfig.AttackMontages[MontageIdx];
				if (M) Enemy->PlayAnimMontage(M);
			}

			UE_LOG(LogTemp, Log, TEXT("Attack: %s GOLPE %d/%d (combo[%d] rot %s%.0f° lunge %.0f)"),
				*Enemy->GetName(), CurrentHit, TotalHits, ChosenComboIndex,
				(CurrentHit % 2 == 0) ? TEXT("+") : TEXT("-"),
				DebugRotationDegrees, DebugLungeDistance);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// STRIKE — golpe activo, volver gradualmente a posición original
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Strike:
	{
		// Interpolate back to original position during strike
		if (DebugLungeDistance > 0.0f)
		{
			float Alpha = FMath::Clamp(PhaseTimer / StrikeDuration, 0.0f, 1.0f);
			FVector Current = Enemy->GetActorLocation();
			FVector BackPos = FMath::Lerp(Current, StrikeStartLocation, Alpha * 0.5f);
			Enemy->SetActorLocation(BackPos);
		}

		if (PhaseTimer >= StrikeDuration)
		{
			PhaseTimer = 0.0f;
			Phase = EAttackPhase::Recovery;

			// Restore position and rotation
			Enemy->SetActorLocation(StrikeStartLocation);
			ApplyDebugRotation(Enemy, true);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// RECOVERY
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Recovery:
	{
		if (PhaseTimer >= RecoveryDuration)
		{
			PhaseTimer = 0.0f;
			Phase = (CurrentHit < TotalHits) ? EAttackPhase::ComboGap : EAttackPhase::Finished;
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// COMBOGAP — pausa entre golpes
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::ComboGap:
	{
		if (PhaseTimer >= ComboGapDuration)
		{
			PhaseTimer = 0.0f;
			Phase = EAttackPhase::WindUp;
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// FINISHED — combo completo
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Finished:
	{
		UE_LOG(LogTemp, Warning, TEXT("=== ATTACK END: %s — %d golpes (combo[%d]) ==="),
			*Enemy->GetName(), CurrentHit, ChosenComboIndex);

		Cleanup(OwnerComp, Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		break;
	}

	default:
		Cleanup(OwnerComp, Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		break;
	}
}

// ═══════════════════════════════════════════════════════════════════════════

FString UBTTask_AttackTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack (combo by distance, dmg ±%.0f%%, rot ±%.0f°, lunge %.0f)"),
		DamageVariance * 100.0f, DebugRotationDegrees, DebugLungeDistance);
}
