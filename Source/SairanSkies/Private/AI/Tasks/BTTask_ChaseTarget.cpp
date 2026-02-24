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
	Enemy->SetEnemyState(EEnemyState::Chasing);

	// Chase until we reach the OUTER CIRCLE radius
	float AcceptanceRadius = Enemy->CombatConfig.OuterCircleRadius - 50.0f;

	EPathFollowingRequestResult::Type Result = AIController->MoveToActor(Target, AcceptanceRadius);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	// If already in range, succeed immediately
	float Dist = Enemy->GetDistanceToTarget();
	if (Dist <= Enemy->CombatConfig.OuterCircleRadius)
	{
		AIController->StopMovement();
		UE_LOG(LogTemp, Log, TEXT("Chase: %s ya en outer circle (dist=%.0f)"), *Enemy->GetName(), Dist);
		return EBTNodeResult::Succeeded;
	}

	UE_LOG(LogTemp, Log, TEXT("Chase: %s persiguiendo a %s (dist=%.0f, objetivo=%.0f)"),
		*Enemy->GetName(), *Target->GetName(), Dist, Enemy->CombatConfig.OuterCircleRadius);

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

	float CurrentDist = Enemy->GetDistanceToTarget();

	// Reached the outer circle
	if (CurrentDist <= Enemy->CombatConfig.OuterCircleRadius)
	{
		AIController->StopMovement();
		UE_LOG(LogTemp, Log, TEXT("Chase: %s llegó al outer circle (dist=%.0f)"), *Enemy->GetName(), CurrentDist);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Re-request movement if stopped
	EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Idle || MoveStatus == EPathFollowingStatus::Waiting)
	{
		float AcceptanceRadius = Enemy->CombatConfig.OuterCircleRadius - 50.0f;
		AIController->MoveToActor(Target, AcceptanceRadius);
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
	return TEXT("Chase target until reaching outer circle (~5m)");
}
