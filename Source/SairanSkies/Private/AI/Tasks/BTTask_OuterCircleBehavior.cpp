// SairanSkies - BT Task: Outer Circle Behavior

#include "AI/Tasks/BTTask_OuterCircleBehavior.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "AI/GroupCombatManager.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_OuterCircleBehavior::UBTTask_OuterCircleBehavior()
{
	NodeName = TEXT("Outer Circle Behavior");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTE
// ═══════════════════════════════════════════════════════════════════════════

EBTNodeResult::Type UBTTask_OuterCircleBehavior::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;
	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) return EBTNodeResult::Failed;
	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	// Reset state
	TaskTimer = 0.0f;
	RepositionTimer = 0.0f;
	NextRepositionTime = RepositionInterval + FMath::RandRange(-RepositionVariation, RepositionVariation);
	TauntAccumulator = 0.0f;
	TauntState = ETauntState::None;
	InnerCheckTimer = 0.0f;
	bReachedTarget = false;
	bWaitingToReposition = false;
	ReactionDelay = 0.0f;
	ReactionTimer = 0.0f;

	SwayPhase = FMath::FRand() * PI * 2.0f;
	LastPlayerPosition = Target->GetActorLocation();

	// Disable auto-orient — we rotate manually to face the player
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = false;

	AIC->StopMovement();
	Enemy->SetMovementSpeed(CircleSpeedMultiplier);
	Enemy->SetEnemyState(EEnemyState::OuterCircle);

	// Pick initial ring position
	PickNewRingTarget(Enemy, Target);

	UE_LOG(LogTemp, Log, TEXT("OuterCircle: %s entrando al círculo exterior"), *Enemy->GetName());

	return EBTNodeResult::InProgress;
}

// ═══════════════════════════════════════════════════════════════════════════
// TICK
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_OuterCircleBehavior::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	AEnemyBase* Enemy = AIC->GetControlledEnemy();
	if (!Enemy) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) { CleanupState(Enemy); FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	TaskTimer += DeltaSeconds;
	RepositionTimer += DeltaSeconds;
	InnerCheckTimer += DeltaSeconds;

	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();

	// ──────────── 1. CHECK IF WE CAN ENTER INNER CIRCLE ────────────
	if (InnerCheckTimer >= InnerCircleCheckInterval)
	{
		InnerCheckTimer = 0.0f;
		if (Mgr && Mgr->RequestInnerCircleEntry(Enemy))
		{
			CleanupState(Enemy);
			UE_LOG(LogTemp, Warning, TEXT("OuterCircle: %s → entrando al INNER CIRCLE"), *Enemy->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// ──────────── 2. REACT TO PLAYER MOVEMENT (with delay) ────────────
	float PlayerMoveDist = FVector::Dist2D(Target->GetActorLocation(), LastPlayerPosition);
	if (PlayerMoveDist > 150.0f)
	{
		if (!bWaitingToReposition)
		{
			// Player moved significantly — set a random delay before reacting
			bWaitingToReposition = true;
			ReactionDelay = FMath::RandRange(
				Enemy->CombatConfig.PlayerMoveReactionDelayMin,
				Enemy->CombatConfig.PlayerMoveReactionDelayMax);
			ReactionTimer = 0.0f;
		}
	}

	if (bWaitingToReposition)
	{
		ReactionTimer += DeltaSeconds;
		if (ReactionTimer >= ReactionDelay)
		{
			// React: update ring target and player position
			PickNewRingTarget(Enemy, Target);
			LastPlayerPosition = Target->GetActorLocation();
			bReachedTarget = false;
			bWaitingToReposition = false;
			UE_LOG(LogTemp, Verbose, TEXT("OuterCircle: %s reacciona al movimiento del jugador"), *Enemy->GetName());
		}
	}

	// ──────────── 3. ALWAYS FACE THE PLAYER ────────────
	FacePlayer(Enemy, Target, DeltaSeconds);

	// ──────────── 4. TAUNT SYSTEM ────────────
	if (TauntState != ETauntState::None)
	{
		UpdateTaunt(Enemy, Target, DeltaSeconds);
		return; // During taunt, skip regular movement
	}

	// Roll for taunt when standing at ring position
	if (bReachedTarget)
	{
		TauntAccumulator += DeltaSeconds;
		float TauntRoll = FMath::FRand();
		if (TauntRoll < TauntChancePerSecond * TauntAccumulator)
		{
			TauntAccumulator = 0.0f;
			StartTaunt(Enemy, Target);
			return;
		}
	}

	// ──────────── 5. MOVE TO RING TARGET ────────────
	float DistToRingTarget = FVector::Dist2D(Enemy->GetActorLocation(), CurrentRingTarget);

	if (DistToRingTarget > 80.0f)
	{
		bReachedTarget = false;
		MoveTowardsFacingPlayer(Enemy, Target, CurrentRingTarget, DeltaSeconds);
	}
	else
	{
		bReachedTarget = true;
		// At ring position — apply lateral sway
		ApplySway(Enemy, Target, DeltaSeconds);
	}

	// ──────────── 6. PERIODIC REPOSITION ────────────
	if (RepositionTimer >= NextRepositionTime)
	{
		RepositionTimer = 0.0f;
		NextRepositionTime = RepositionInterval + FMath::RandRange(-RepositionVariation, RepositionVariation);
		PickNewRingTarget(Enemy, Target);
		bReachedTarget = false;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_OuterCircleBehavior::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		if (AEnemyBase* Enemy = AIC->GetControlledEnemy())
			CleanupState(Enemy);
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_OuterCircleBehavior::CleanupState(AEnemyBase* Enemy)
{
	if (!Enemy) return;
	if (auto* CMC = Enemy->GetCharacterMovement())
		CMC->bOrientRotationToMovement = true;
	TauntState = ETauntState::None;
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPERS
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_OuterCircleBehavior::PickNewRingTarget(AEnemyBase* Enemy, AActor* Target)
{
	auto* Mgr = GetWorld()->GetSubsystem<UGroupCombatManager>();
	if (Mgr)
	{
		CurrentRingTarget = Mgr->GetOuterCirclePosition(Enemy, Target);
	}
	else
	{
		// Fallback
		FVector Away = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
		float Angle = FMath::RandRange(-60.0f, 60.0f);
		CurrentRingTarget = Target->GetActorLocation() +
			Away.RotateAngleAxis(Angle, FVector::UpVector) * Enemy->CombatConfig.OuterCircleRadius;
	}
	CurrentRingTarget.Z = Enemy->GetActorLocation().Z;
}

void UBTTask_OuterCircleBehavior::FacePlayer(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	FVector Dir = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return;

	FRotator Look = Dir.Rotation();
	Look.Pitch = 0.0f; Look.Roll = 0.0f;
	FRotator Cur = Enemy->GetActorRotation();
	Cur.Pitch = 0.0f; Cur.Roll = 0.0f;
	Enemy->SetActorRotation(FMath::RInterpTo(Cur, Look, DeltaTime, 6.0f));
}

void UBTTask_OuterCircleBehavior::MoveTowardsFacingPlayer(
	AEnemyBase* Enemy, AActor* Target, const FVector& Dest, float DeltaTime)
{
	FVector MoveDir = (Dest - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (MoveDir.IsNearlyZero()) return;
	Enemy->AddMovementInput(MoveDir, 1.0f, false);
}

void UBTTask_OuterCircleBehavior::ApplySway(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	SwayPhase += DeltaTime * SwaySpeed;
	FVector ToPlayer = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	FVector Lateral = FVector(-ToPlayer.Y, ToPlayer.X, 0.0f);
	float SwayOffset = FMath::Sin(SwayPhase) * SwayAmplitude * DeltaTime;
	Enemy->AddMovementInput(Lateral, SwayOffset, false);
}

// ═══════════════════════════════════════════════════════════════════════════
// TAUNT SYSTEM — fake rush towards player, then retreat
// ═══════════════════════════════════════════════════════════════════════════

void UBTTask_OuterCircleBehavior::StartTaunt(AEnemyBase* Enemy, AActor* Target)
{
	TauntState = ETauntState::Lunging;
	TauntTimer = 0.0f;
	TauntOrigin = Enemy->GetActorLocation();

	// Speed up briefly for the lunge
	Enemy->SetMovementSpeed(0.6f);

	UE_LOG(LogTemp, Verbose, TEXT("OuterCircle: %s TAUNT → lunge"), *Enemy->GetName());
}

void UBTTask_OuterCircleBehavior::UpdateTaunt(AEnemyBase* Enemy, AActor* Target, float DeltaTime)
{
	TauntTimer += DeltaTime;
	FacePlayer(Enemy, Target, DeltaTime);

	switch (TauntState)
	{
	case ETauntState::Lunging:
	{
		// Rush TOWARDS the player
		FVector ToPlayer = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
		Enemy->AddMovementInput(ToPlayer, 1.0f, false);

		// Check if we've gone far enough or hit the "invisible wall" (outer circle limit)
		float LungedDist = FVector::Dist2D(Enemy->GetActorLocation(), TauntOrigin);
		if (TauntTimer >= TauntLungeDuration || LungedDist >= TauntLungeDistance)
		{
			TauntState = ETauntState::Retreating;
			TauntTimer = 0.0f;
			Enemy->SetMovementSpeed(CircleSpeedMultiplier * 0.7f);
		}
		break;
	}
	case ETauntState::Retreating:
	{
		// Move BACK to ring position
		FVector BackDir = (TauntOrigin - Enemy->GetActorLocation()).GetSafeNormal2D();
		if (!BackDir.IsNearlyZero())
			Enemy->AddMovementInput(BackDir, 1.0f, false);

		if (TauntTimer >= TauntRetreatDuration)
		{
			TauntState = ETauntState::None;
			TauntTimer = 0.0f;
			TauntAccumulator = 0.0f;
			Enemy->SetMovementSpeed(CircleSpeedMultiplier);
		}
		break;
	}
	default:
		TauntState = ETauntState::None;
		break;
	}
}

// ═══════════════════════════════════════════════════════════════════════════

FString UBTTask_OuterCircleBehavior::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Outer circle — speed x%.0f%%, taunt %.0f%%/s, sway ±%.0f"),
		CircleSpeedMultiplier * 100.0f, TauntChancePerSecond * 100.0f, SwayAmplitude);
}

