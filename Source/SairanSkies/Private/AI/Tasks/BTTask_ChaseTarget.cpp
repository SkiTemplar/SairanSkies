// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_ChaseTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
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
	if (!AIController) return EBTNodeResult::Failed;

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy) return EBTNodeResult::Failed;

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	Enemy->SetChaseSpeed();

	// MoveToActor acceptance radius is measured between capsule edges,
	// but GetDistanceToTarget() measures between actor centers.
	// Use a small acceptance so the AI gets close enough.
	float MoveAcceptance = FMath::Max(Enemy->CombatConfig.MaxAttackDistance - 100.0f, 10.0f);

	EPathFollowingRequestResult::Type Result = AIController->MoveToActor(Target, MoveAcceptance);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Verbose, TEXT("Chase: %s persiguiendo a %s (dist: %.0f)"),
		*Enemy->GetName(), *Target->GetName(), Enemy->GetDistanceToTarget());

	return EBTNodeResult::InProgress;
}

void UBTTask_ChaseTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Use center-to-center distance for the range check
	float CurrentDist = Enemy->GetDistanceToTarget();

	if (CurrentDist <= Enemy->CombatConfig.MaxAttackDistance)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Re-request movement if stopped
	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Idle || MoveStatus == EPathFollowingStatus::Waiting)
	{
		float MoveAcceptance = FMath::Max(Enemy->CombatConfig.MaxAttackDistance - 100.0f, 10.0f);
		AIController->MoveToActor(Target, MoveAcceptance);
	}
}

EBTNodeResult::Type UBTTask_ChaseTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController) AIController->StopMovement();
	return EBTNodeResult::Aborted;
}

FString UBTTask_ChaseTarget::GetStaticDescription() const
{
	return TEXT("Chase target until in attack range");
}


