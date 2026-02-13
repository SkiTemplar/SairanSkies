// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_ApproachForAttack.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"

UBTTask_ApproachForAttack::UBTTask_ApproachForAttack()
{
	NodeName = TEXT("Approach For Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ApproachForAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	if (Enemy->IsInAttackRange())
	{
		return EBTNodeResult::Succeeded;
	}

	Enemy->SetMovementSpeed(ApproachSpeedMultiplier);

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		Target,
		Enemy->CombatConfig.MinAttackDistance,
		true,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_ApproachForAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	if (Enemy->IsInAttackRange())
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Idle)
	{
		AIController->MoveToActor(Target, Enemy->CombatConfig.MinAttackDistance, true, true);
	}
}

EBTNodeResult::Type UBTTask_ApproachForAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AIController->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_ApproachForAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Approach target for attack (%.1fx speed)"), ApproachSpeedMultiplier);
}
