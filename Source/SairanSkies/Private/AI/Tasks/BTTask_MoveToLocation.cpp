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
	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToLocation, LocationKey));
}

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	FVector TargetLocation = BlackboardComp->GetValueAsVector(LocationKey.SelectedKeyName);
	if (TargetLocation.IsZero())
	{
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

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		TargetLocation,
		AcceptanceRadius,
		true,
		true,
		false,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
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

	if (Enemy->GetCurrentTarget())
	{
		AIController->StopMovement();
		Enemy->SetEnemyState(EEnemyState::Chasing);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	
	if (MoveStatus == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
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
	return FString::Printf(TEXT("Move to %s (radius: %.0f)"), 
		*LocationKey.SelectedKeyName.ToString(), AcceptanceRadius);
}
