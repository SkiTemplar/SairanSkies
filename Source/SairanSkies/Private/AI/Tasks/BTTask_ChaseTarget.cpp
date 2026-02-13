// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_ChaseTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"

UBTTask_ChaseTarget::UBTTask_ChaseTarget()
{
	NodeName = TEXT("Chase Target");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	Enemy->SetChaseSpeed();

	float TargetDistance = bUsePositioningDistance ? 
		Enemy->CombatConfig.PositioningDistance : 
		Enemy->CombatConfig.MaxAttackDistance;

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		Target,
		TargetDistance,
		true,
		true,
		false
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_ChaseTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float TargetDistance = bUsePositioningDistance ? 
		Enemy->CombatConfig.PositioningDistance : 
		Enemy->CombatConfig.MaxAttackDistance;

	float CurrentDistance = Enemy->GetDistanceToTarget();
	
	if (CurrentDistance <= TargetDistance + AcceptanceRadius)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Idle || MoveStatus == EPathFollowingStatus::Waiting)
	{
		AIController->MoveToActor(Target, TargetDistance, true, true, false);
	}
}

EBTNodeResult::Type UBTTask_ChaseTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AIController->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_ChaseTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Chase target until within %s distance"), 
		bUsePositioningDistance ? TEXT("positioning") : TEXT("attack"));
}
