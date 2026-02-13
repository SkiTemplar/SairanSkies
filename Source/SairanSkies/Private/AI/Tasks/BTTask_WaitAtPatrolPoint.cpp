// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_WaitAtPatrolPoint.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTTask_WaitAtPatrolPoint::UBTTask_WaitAtPatrolPoint()
{
	NodeName = TEXT("Wait At Patrol Point");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	
	WaitTimer = 0.0f;
	TargetWaitTime = 0.0f;
}

EBTNodeResult::Type UBTTask_WaitAtPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	if (bUseEnemyWaitTime)
	{
		TargetWaitTime = Enemy->PatrolConfig.WaitTimeAtPatrolPoint;
	}
	else
	{
		TargetWaitTime = CustomWaitTime;
	}

	TargetWaitTime *= FMath::RandRange(0.8f, 1.2f);

	WaitTimer = 0.0f;
	AIController->StopMovement();

	return EBTNodeResult::InProgress;
}

void UBTTask_WaitAtPatrolPoint::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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
		Enemy->SetEnemyState(EEnemyState::Chasing);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	WaitTimer += DeltaSeconds;

	if (bLookAround)
	{
		FRotator CurrentRotation = Enemy->GetActorRotation();
		float OscillationAngle = FMath::Sin(WaitTimer * 2.0f) * LookAroundSpeed;
		CurrentRotation.Yaw += OscillationAngle * DeltaSeconds;
		Enemy->SetActorRotation(CurrentRotation);
	}

	if (WaitTimer >= TargetWaitTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_WaitAtPatrolPoint::GetStaticDescription() const
{
	if (bUseEnemyWaitTime)
	{
		return TEXT("Wait at patrol point (using enemy config)");
	}
	return FString::Printf(TEXT("Wait at patrol point (%.1f sec)"), CustomWaitTime);
}
