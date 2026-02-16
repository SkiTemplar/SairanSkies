// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_MoveToLocation.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"

UBTTask_MoveToLocation::UBTTask_MoveToLocation()
{
	NodeName = TEXT("Move To Location");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: No AIController"));
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: No Enemy"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: No Blackboard"));
		return EBTNodeResult::Failed;
	}

	FVector TargetLocation = BlackboardComp->GetValueAsVector(AEnemyBase::BB_TargetLocation);
	
	// Check if location is valid (not checking IsZero since 0,0,0 could be valid)
	if (!BlackboardComp->IsVectorValueSet(AEnemyBase::BB_TargetLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: TargetLocation not set in Blackboard"));
		return EBTNodeResult::Failed;
	}

	if (bUsePatrolSpeed)
	{
		Enemy->SetPatrolSpeed();
	}
	else
	{
		Enemy->SetChaseSpeed();
	}

	UE_LOG(LogTemp, Log, TEXT("MoveToLocation: %s starting move to %s"), *Enemy->GetName(), *TargetLocation.ToString());

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		TargetLocation,
		AcceptanceRadius,
		true,  // bStopOnOverlap
		true,  // bUsePathfinding
		false, // bProjectDestinationToNavigation
		true,  // bCanStrafe
		TSubclassOf<UNavigationQueryFilter>(),
		true   // bAllowPartialPath
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: MoveToLocation FAILED for %s to %s"), *Enemy->GetName(), *TargetLocation.ToString());
		return EBTNodeResult::Failed;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		UE_LOG(LogTemp, Log, TEXT("MoveToLocation: %s already at goal"), *Enemy->GetName());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_MoveToLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// If we detected a target, abort patrol and switch to chasing
	if (Enemy->GetCurrentTarget())
	{
		AIController->StopMovement();
		Enemy->SetEnemyState(EEnemyState::Chasing);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Check if we've reached destination by distance
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		FVector TargetLocation = BlackboardComp->GetValueAsVector(AEnemyBase::BB_TargetLocation);
		FVector CurrentLocation = Enemy->GetActorLocation();
		float DistanceToTarget = FVector::Dist2D(CurrentLocation, TargetLocation);
		
		if (DistanceToTarget <= AcceptanceRadius)
		{
			UE_LOG(LogTemp, Log, TEXT("MoveToLocation: %s reached destination (distance check: %.1f <= %.1f)"), 
				*Enemy->GetName(), DistanceToTarget, AcceptanceRadius);
			AIController->StopMovement();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	
	switch (MoveStatus)
	{
	case EPathFollowingStatus::Idle:
		// Movement completed or was never started
		UE_LOG(LogTemp, Log, TEXT("MoveToLocation: %s movement status Idle - task complete"), *Enemy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		break;
		
	case EPathFollowingStatus::Waiting:
		// Still waiting for path
		UE_LOG(LogTemp, Verbose, TEXT("MoveToLocation: %s waiting for path..."), *Enemy->GetName());
		break;
		
	case EPathFollowingStatus::Paused:
		// Movement paused, try to resume
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: %s movement paused, resuming..."), *Enemy->GetName());
		AIController->ResumeMove(FAIRequestID::CurrentRequest);
		break;
		
	case EPathFollowingStatus::Moving:
		// Still moving, nothing to do
		break;
		
	default:
		UE_LOG(LogTemp, Warning, TEXT("MoveToLocation: %s unknown move status: %d"), *Enemy->GetName(), (int32)MoveStatus);
		break;
	}
}

EBTNodeResult::Type UBTTask_MoveToLocation::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AIController->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_MoveToLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Move to target location (radius: %.0f)"), AcceptanceRadius);
}
