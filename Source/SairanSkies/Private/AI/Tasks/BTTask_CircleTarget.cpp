// SairanSkies - BT Task: Circle Target — Intelligent flanking with feints

#include "AI/Tasks/BTTask_CircleTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "AI/GroupCombatManager.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_CircleTarget::UBTTask_CircleTarget()
{
	NodeName = TEXT("Circle Target (Flank)");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTE — entry point
// ═══════════════════════════════════════════════════════════════════════════

EBTNodeResult::Type UBTTask_CircleTarget::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;
	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) return EBTNodeResult::Failed;
	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	// Reset all state
	TaskTimer = 0.0f;
	RepositionTimer = 0.0f;
	NextRepositionTime = RepositionInterval + FMath::RandRange(-RepositionVariation, RepositionVariation);
	FeintAccumulator = 0.0f;
	FeintState = EFeintState::None;
	bReachedRingTarget = false;

	// Give each enemy a different sway phase so they don't all sway in sync
	SwayPhase = FMath::FRand() * PI * 2.0f;

	// Disable auto-orient — we control rotation manually (face the player)
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = false;

	AIC->StopMovement();
	Enemy->SetMovementSpeed(CircleSpeedMultiplier);

	// Try to grab an inner flanking slot
	bHasFlankSlot = false;
	if (auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>())
		bHasFlankSlot = Mgr->RequestFlankingSlot(Enemy);

	// Pick initial ring target (inner or outer)
	PickNewRingTarget(Enemy, Target);

	return EBTNodeResult::InProgress;
}

// ═══════════════════════════════════════════════════════════════════════════
// TICK — every frame
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_CircleTarget::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	TaskTimer += DeltaSeconds;
	RepositionTimer += DeltaSeconds;

	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();

	// ──────────── 1. CHECK IF WE CAN ATTACK ────────────
	if (Mgr && Mgr->CanEnemyAttack(Enemy))
	{
		CleanupState(Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// ──────────── 2. TRY TO UPGRADE TO INNER RING ────────────
	if (!bHasFlankSlot && Mgr)
	{
		bHasFlankSlot = Mgr->RequestFlankingSlot(Enemy);
		if (bHasFlankSlot)
		{
			PickNewRingTarget(Enemy, Target);
			bReachedRingTarget = false;
		}
	}

	// ──────────── 3. ALWAYS FACE THE PLAYER ────────────
	FacePlayer(Enemy, Target, DeltaSeconds);

	// ──────────── 4. FEINT SYSTEM (inner ring only) ────────────
	if (FeintState != EFeintState::None)
	{
		UpdateFeint(Enemy, Target, DeltaSeconds);
		// During a feint we don't do regular movement
		if (TaskTimer >= MaxTaskDuration) { CleanupState(Enemy); FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded); }
		return;
	}

	if (bHasFlankSlot && bReachedRingTarget)
	{
		// Roll for feint
		FeintAccumulator += DeltaSeconds;
		float FeintRoll = FMath::FRand();
		if (FeintRoll < FeintChancePerSecond * FeintAccumulator)
		{
			FeintAccumulator = 0.0f;
			StartFeint(Enemy, Target);
			return;
		}
	}

	// ──────────── 5. MOVE TO RING TARGET ────────────
	float DistToTarget = FVector::Dist2D(Enemy->GetActorLocation(), CurrentRingTarget);

	if (DistToTarget > 60.0f)
	{
		bReachedRingTarget = false;
		MoveTowardsFacingPlayer(Enemy, Target, CurrentRingTarget, DeltaSeconds);
	}
	else
	{
		bReachedRingTarget = true;
		// At ring position — apply lateral sway to look alive
		ApplySway(Enemy, Target, DeltaSeconds);
	}

	// ──────────── 6. PERIODIC REPOSITION ────────────
	if (RepositionTimer >= NextRepositionTime)
	{
		RepositionTimer = 0.0f;
		NextRepositionTime = RepositionInterval + FMath::RandRange(-RepositionVariation, RepositionVariation);
		PickNewRingTarget(Enemy, Target);
		bReachedRingTarget = false;
	}

	// ──────────── 7. MAX DURATION ────────────
	if (TaskTimer >= MaxTaskDuration)
	{
		CleanupState(Enemy);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_CircleTarget::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		if (AEnemyBase* Enemy = AIC->GetControlledEnemy())
			CleanupState(Enemy);
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_CircleTarget::CleanupState(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = true;

	if (bHasFlankSlot)
	{
		if (auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>())
			Mgr->ReleaseFlankingSlot(Enemy);
		bHasFlankSlot = false;
	}
	FeintState = EFeintState::None;
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPERS
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_CircleTarget::PickNewRingTarget(AEnemyBase* Enemy, AActor* Target)
{
	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();
	if (!Mgr)
	{
		// Fallback — orbit at ~500 units
		FVector Away = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
		float Angle = FMath::RandRange(-70.0f, 70.0f);
		CurrentRingTarget = Target->GetActorLocation() + Away.RotateAngleAxis(Angle, FVector::UpVector) * 500.0f;
		return;
	}

	if (bHasFlankSlot)
		CurrentRingTarget = Mgr->GetFlankingPosition(Enemy, Target);
	else
		CurrentRingTarget = Mgr->GetOuterRingPosition(Enemy, Target);

	// Keep current Z
	CurrentRingTarget.Z = Enemy->GetActorLocation().Z;
}

void UBTTask_CircleTarget::FacePlayer(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	FVector Dir = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return;

	FRotator Look = Dir.Rotation();
	Look.Pitch = 0.0f; Look.Roll = 0.0f;
	FRotator Cur = Enemy->GetActorRotation();
	Cur.Pitch = 0.0f; Cur.Roll = 0.0f;
	Enemy->SetActorRotation(FMath::RInterpTo(Cur, Look, DeltaTime, 8.0f));
}

void UBTTask_CircleTarget::MoveTowardsFacingPlayer(
	AEnemyBase* Enemy, AActor* Target, const FVector& Destination, float DeltaTime)
{
	FVector MoveDir = (Destination - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (MoveDir.IsNearlyZero()) return;

	// The character's bOrientRotationToMovement is OFF, and we set rotation
	// via FacePlayer(), so AddMovementInput in any direction will
	// automatically look like walking backwards / sideways.
	Enemy->AddMovementInput(MoveDir, 1.0f, false);
}

void UBTTask_CircleTarget::ApplySway(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	SwayPhase += DeltaTime * SwaySpeed;

	// Lateral direction = perpendicular to the line toward the player
	FVector ToPlayer = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	FVector Lateral = FVector(-ToPlayer.Y, ToPlayer.X, 0.0f);  // perpendicular on XY

	float SwayOffset = FMath::Sin(SwayPhase) * SwayAmplitude * DeltaTime;
	Enemy->AddMovementInput(Lateral, SwayOffset, false);
}

// ═══════════════════════════════════════════════════════════════════════════
// FEINT SYSTEM — creates tension by faking an attack
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_CircleTarget::StartFeint(AEnemyBase* Enemy, AActor* Target)
{
	FeintState = EFeintState::Lunging;
	FeintTimer = 0.0f;
	FeintOrigin = Enemy->GetActorLocation();

	// Briefly speed up for the lunge
	Enemy->SetMovementSpeed(0.9f);
}

void UBTTask_CircleTarget::UpdateFeint(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	FeintTimer += DeltaTime;

	switch (FeintState)
	{
	case EFeintState::Lunging:
	{
		// Move TOWARDS the player quickly
		FVector ToPlayer = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
		Enemy->AddMovementInput(ToPlayer, 1.0f, false);

		if (FeintTimer >= FeintLungeDuration)
		{
			FeintState = EFeintState::Retreating;
			FeintTimer = 0.0f;
			// Slow down for the retreat
			Enemy->SetMovementSpeed(CircleSpeedMultiplier * 0.8f);
		}
		break;
	}
	case EFeintState::Retreating:
	{
		// Move AWAY from the player (back to ring position)
		FVector AwayFromPlayer = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		Enemy->AddMovementInput(AwayFromPlayer, 1.0f, false);

		if (FeintTimer >= FeintRetreatDuration)
		{
			FeintState = EFeintState::None;
			FeintTimer = 0.0f;
			FeintAccumulator = 0.0f;
			// Restore normal circle speed
			Enemy->SetMovementSpeed(CircleSpeedMultiplier);
		}
		break;
	}
	default:
		FeintState = EFeintState::None;
		break;
	}
}

// ═══════════════════════════════════════════════════════════════════════════

FString UBTTask_CircleTarget::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Circle target — speed x%.0f%%, feint %.0f%%/s, sway ±%.0f, reposition %.1f±%.1fs"),
		CircleSpeedMultiplier * 100.0f, FeintChancePerSecond * 100.0f,
		SwayAmplitude, RepositionInterval, RepositionVariation);
}

