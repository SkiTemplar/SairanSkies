// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_Investigate.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"

UBTTask_Investigate::UBTTask_Investigate()
{
	NodeName = TEXT("Investigate Area");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	
	CurrentInvestigationPoint = 0;
	WaitTimer = 0.0f;
	TotalInvestigationTime = 0.0f;
	bWaitingAtPoint = false;
	CurrentTargetLocation = FVector::ZeroVector;
}

EBTNodeResult::Type UBTTask_Investigate::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	Enemy->SetEnemyState(EEnemyState::Investigating);

	CurrentInvestigationPoint = 0;
	WaitTimer = 0.0f;
	TotalInvestigationTime = 0.0f;
	bWaitingAtPoint = false;

	FVector LastKnownLocation = Enemy->GetLastKnownTargetLocation();
	if (LastKnownLocation.IsZero())
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
	if (NavSys)
	{
		FNavLocation NavLocation;
		bool bFound = NavSys->GetRandomReachablePointInRadius(LastKnownLocation, Enemy->PerceptionConfig.InvestigationRadius, NavLocation);
		if (bFound)
		{
			CurrentTargetLocation = NavLocation.Location;
			AIController->MoveToLocation(CurrentTargetLocation, AcceptanceRadius);
		}
		else
		{
			CurrentTargetLocation = LastKnownLocation;
			AIController->MoveToLocation(CurrentTargetLocation, AcceptanceRadius);
		}
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_Investigate::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	TotalInvestigationTime += DeltaSeconds;

	if (TotalInvestigationTime >= Enemy->PerceptionConfig.InvestigationTime)
	{
		Enemy->SetEnemyState(EEnemyState::Patrolling);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (bWaitingAtPoint)
	{
		WaitTimer += DeltaSeconds;

		FRotator CurrentRotation = Enemy->GetActorRotation();
		CurrentRotation.Yaw += 90.0f * DeltaSeconds;
		Enemy->SetActorRotation(CurrentRotation);

		if (WaitTimer >= WaitTimeAtPoint)
		{
			bWaitingAtPoint = false;
			WaitTimer = 0.0f;
			CurrentInvestigationPoint++;

			if (CurrentInvestigationPoint >= InvestigationPoints)
			{
				Enemy->SetEnemyState(EEnemyState::Patrolling);
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}

			FVector LastKnownLocation = Enemy->GetLastKnownTargetLocation();
			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
			if (NavSys)
			{
				FNavLocation NavLocation;
				bool bFound = NavSys->GetRandomReachablePointInRadius(LastKnownLocation, Enemy->PerceptionConfig.InvestigationRadius, NavLocation);
				if (bFound)
				{
					CurrentTargetLocation = NavLocation.Location;
					AIController->MoveToLocation(CurrentTargetLocation, AcceptanceRadius);
				}
			}
		}
	}
	else
	{
		float DistanceToPoint = FVector::Dist(Enemy->GetActorLocation(), CurrentTargetLocation);
		if (DistanceToPoint <= AcceptanceRadius)
		{
			bWaitingAtPoint = true;
			WaitTimer = 0.0f;
			AIController->StopMovement();
		}
	}
}

EBTNodeResult::Type UBTTask_Investigate::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AIController->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_Investigate::GetStaticDescription() const
{
	return FString::Printf(TEXT("Investigate area (%d points, %.1f sec wait)"), InvestigationPoints, WaitTimeAtPoint);
}
