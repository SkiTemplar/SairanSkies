// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_AttackTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

// ---------------------------------------------------------------------------
void UBTTask_AttackTarget::PickComboByDistance(AEnemyBase* Enemy)
{
	int32 NumMontages = Enemy->AnimationConfig.AttackMontages.Num();
	if (NumMontages <= 1)
	{
		TotalHits = FMath::Max(1, NumMontages);
		if (NumMontages == 0) TotalHits = 1; // sin montajes → 1 golpe debug
		return;
	}

	float Dist = Enemy->GetDistanceToTarget();
	float Min = Enemy->CombatConfig.MinAttackDistance;
	float Max = Enemy->CombatConfig.MaxAttackDistance;

	float T = FMath::Clamp((Dist - Min) / FMath::Max(Max - Min, 1.0f), 0.0f, 1.0f);
	float CloseWeight = 1.0f - T;

	float MeanHits = FMath::Lerp(1.0f, (float)NumMontages, CloseWeight);
	int32 Hits = FMath::RoundToInt32(MeanHits + FMath::RandRange(-0.5f, 0.5f));
	TotalHits = FMath::Clamp(Hits, 1, NumMontages);

	UE_LOG(LogTemp, Log, TEXT("Attack: %s combo=%d (dist=%.0f, closeness=%.0f%%, montajes=%d)"),
		*Enemy->GetName(), TotalHits, Dist, CloseWeight * 100.0f, NumMontages);
}

// ---------------------------------------------------------------------------
void UBTTask_AttackTarget::ApplyDamage(AEnemyBase* Enemy, AActor* Target)
{
	if (!Enemy || !Target) return;

	float Var = FMath::RandRange(-DamageVariance, DamageVariance);
	float Dmg = FMath::Max(1.0f, Enemy->CombatConfig.BaseDamage * (1.0f + Var));

	UE_LOG(LogTemp, Log, TEXT("Attack: %s → %s  Dmg=%.1f (base %.1f %+.0f%%)"),
		*Enemy->GetName(), *Target->GetName(), Dmg, Enemy->CombatConfig.BaseDamage, Var * 100.0f);

	UGameplayStatics::ApplyDamage(Target, Dmg, Enemy->GetController(), Enemy, UDamageType::StaticClass());
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
void UBTTask_AttackTarget::Cleanup(AEnemyBase* Enemy)
{
	if (Enemy)
	{
		ApplyDebugRotation(Enemy, true);
		Enemy->UnregisterAsAttacker();
	}
}

// ---------------------------------------------------------------------------
EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) return EBTNodeResult::Failed;

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	CurrentHit = 0;
	TotalHits = 1;
	// MoveToActor measures between capsule edges, not centers.
	// Use a small acceptance so enemy gets truly close.
	ChosenAttackDist = FMath::Max(Enemy->CombatConfig.MaxAttackDistance - 100.0f, 10.0f);

	// Si cooldown activo → esperar internamente (no fallar)
	if (!Enemy->CanAttack())
	{
		Phase = EAttackPhase::WaitCooldown;
		PhaseTimer = 0.0f;
		AIC->StopMovement();
		UE_LOG(LogTemp, Log, TEXT("Attack: %s — esperando cooldown..."), *Enemy->GetName());
		return EBTNodeResult::InProgress;
	}

	// Cooldown listo → ir directo a approach
	Phase = EAttackPhase::Approach;
	PhaseTimer = 0.0f;
	Enemy->SetChaseSpeed();
	AIC->MoveToActor(Target, ChosenAttackDist);

	UE_LOG(LogTemp, Log, TEXT("Attack: %s — APPROACH (dist=%.0f, stop=%.0f)"),
		*Enemy->GetName(), Enemy->GetDistanceToTarget(), ChosenAttackDist);

	return EBTNodeResult::InProgress;
}

// ---------------------------------------------------------------------------
void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		Cleanup(Enemy);
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
		Enemy->SetActorRotation(FMath::RInterpTo(Cur, Look, DeltaSeconds, 6.0f));
	}

	switch (Phase)
	{
	// ═══════════════════════════════════════════════════════════════════
	// WAIT COOLDOWN — espera quieto mirando al jugador hasta que pueda atacar
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::WaitCooldown:
	{
		if (Enemy->CanAttack())
		{
			Phase = EAttackPhase::Approach;
			PhaseTimer = 0.0f;
			Enemy->SetChaseSpeed();
			AIC->MoveToActor(Target, ChosenAttackDist);
			UE_LOG(LogTemp, Log, TEXT("Attack: %s — cooldown listo, APPROACH"), *Enemy->GetName());
			break;
		}

		// Si el jugador se acerca/aleja, seguir mirándole pero no moverse
		// Timeout largo — el cooldown normalmente es 1.5-2s
		if (PhaseTimer >= 10.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack: %s — timeout cooldown"), *Enemy->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// APPROACH — acercarse al rango
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Approach:
	{
		float Dist = Enemy->GetDistanceToTarget();

		if (Dist <= Enemy->CombatConfig.MaxAttackDistance)
		{
			AIC->StopMovement();
			Phase = EAttackPhase::WaitTurn;
			PhaseTimer = 0.0f;
			UE_LOG(LogTemp, Log, TEXT("Attack: %s — en rango (%.0f), esperando turno..."), *Enemy->GetName(), Dist);
			break;
		}

		EPathFollowingStatus::Type Status = AIC->GetMoveStatus();
		if (Status == EPathFollowingStatus::Idle || Status == EPathFollowingStatus::Waiting)
		{
			AIC->MoveToActor(Target, ChosenAttackDist);
		}

		if (PhaseTimer >= 10.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack: %s — timeout approach"), *Enemy->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// WAIT TURN — esperar hueco de atacante
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::WaitTurn:
	{
		if (Enemy->CanJoinAttack() && Enemy->CanAttackNow())
		{
			Enemy->RegisterAsAttacker();
			Enemy->Attack();
			Enemy->SetEnemyState(EEnemyState::Attacking);
			AIC->StopMovement();

			PickComboByDistance(Enemy);

			Phase = EAttackPhase::WindUp;
			PhaseTimer = 0.0f;

			UE_LOG(LogTemp, Warning, TEXT("=== ATTACK START: %s → %s | %d golpes | dist=%.0f ==="),
				*Enemy->GetName(), *Target->GetName(), TotalHits, Enemy->GetDistanceToTarget());
			break;
		}

		if (Enemy->GetDistanceToTarget() > Enemy->CombatConfig.MaxAttackDistance * 1.3f)
		{
			Phase = EAttackPhase::Approach;
			PhaseTimer = 0.0f;
			AIC->MoveToActor(Target, ChosenAttackDist);
			break;
		}

		if (PhaseTimer >= MaxWaitTurnTime)
		{
			UE_LOG(LogTemp, Log, TEXT("Attack: %s — timeout esperando turno"), *Enemy->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// WINDUP — anticipación, guardar posición para lunge
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::WindUp:
	{
		if (PhaseTimer >= WindUpDuration)
		{
			PhaseTimer = 0.0f;
			Phase = EAttackPhase::Strike;
			CurrentHit++;

			// Guardar posición antes del lunge
			StrikeStartLocation = Enemy->GetActorLocation();

			// Debug rotación
			ApplyDebugRotation(Enemy, false);

			// Debug lunge: mover hacia el target
			if (DebugLungeDistance > 0.0f)
			{
				FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
				ToTarget.Z = 0.0f;
				Enemy->SetActorLocation(StrikeStartLocation + ToTarget * DebugLungeDistance);
			}

			// Daño
			ApplyDamage(Enemy, Target);

			// Montaje si existe
			int32 Idx = (CurrentHit - 1) % FMath::Max(1, Enemy->AnimationConfig.AttackMontages.Num());
			if (Enemy->AnimationConfig.AttackMontages.IsValidIndex(Idx))
			{
				UAnimMontage* M = Enemy->AnimationConfig.AttackMontages[Idx];
				if (M) Enemy->PlayAnimMontage(M);
			}

			UE_LOG(LogTemp, Log, TEXT("Attack: %s GOLPE %d/%d (rot %s%.0f°, lunge %.0f)"),
				*Enemy->GetName(), CurrentHit, TotalHits,
				(CurrentHit % 2 == 0) ? TEXT("+") : TEXT("-"),
				DebugRotationDegrees, DebugLungeDistance);
		}
		break;
	}

	// ═══════════════════════════════════════════════════════════════════
	// STRIKE — golpe activo, volver a posición original gradualmente
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Strike:
	{
		// Interpolar de vuelta a posición original durante el Strike
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

			// Restaurar posición y rotación
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
	// COMBOGAP
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
	// FINISHED
	// ═══════════════════════════════════════════════════════════════════
	case EAttackPhase::Finished:
	{
		Cleanup(Enemy);
		Enemy->SetEnemyState(EEnemyState::Chasing);

		UE_LOG(LogTemp, Warning, TEXT("=== ATTACK END: %s — %d golpes ==="), *Enemy->GetName(), CurrentHit);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		break;
	}

	default:
		Cleanup(Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		break;
	}
}

// ---------------------------------------------------------------------------
FString UBTTask_AttackTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack (combo by distance, dmg ±%.0f%%, rot ±%.0f°, lunge %.0f)"),
		DamageVariance * 100.0f, DebugRotationDegrees, DebugLungeDistance);
}
