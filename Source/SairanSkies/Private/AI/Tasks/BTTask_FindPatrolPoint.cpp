// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_FindPatrolPoint.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Navigation/PatrolPath.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Find Patrol Point");
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPatrolPoint: No AIController"));
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPatrolPoint: No Enemy"));
		return EBTNodeResult::Failed;
	}
	
	if (!Enemy->PatrolPath)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPatrolPoint: Enemy %s has no PatrolPath assigned!"), *Enemy->GetName());
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	APatrolPath* Path = Enemy->PatrolPath;
	int32 NumPoints = Path->GetNumPatrolPoints();
	if (NumPoints == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPatrolPoint: PatrolPath %s has 0 points! Add patrol points in the editor."), *Path->GetName());
		return EBTNodeResult::Failed;
	}

	// Get current patrol index from blackboard
	int32 CurrentIndex = BlackboardComp->GetValueAsInt(AEnemyBase::BB_PatrolIndex);
	
	// Ensure index is valid
	if (CurrentIndex < 0 || CurrentIndex >= NumPoints)
	{
		CurrentIndex = 0;
	}

	// Use current index as target
	FVector PatrolLocation = Path->GetPatrolPoint(CurrentIndex);

	// Calculate NEXT index for the next iteration (will be used after MoveToLocation + WaitAtPatrolPoint)
	int32 NextIndex;
	if (Enemy->PatrolConfig.bRandomPatrol)
	{
		NextIndex = Path->GetRandomPatrolIndex(CurrentIndex);
	}
	else
	{
		// Simple looping: 0 -> 1 -> 2 -> 0 -> 1 -> 2...
		NextIndex = (CurrentIndex + 1) % NumPoints;
	}

	// Update blackboard: store location to move to, and the NEXT index for after we arrive
	BlackboardComp->SetValueAsVector(AEnemyBase::BB_TargetLocation, PatrolLocation);
	BlackboardComp->SetValueAsInt(AEnemyBase::BB_PatrolIndex, NextIndex);

	UE_LOG(LogTemp, Log, TEXT("FindPatrolPoint: %s going to point %d/%d at %s (next will be %d)"), 
		*Enemy->GetName(), CurrentIndex, NumPoints, *PatrolLocation.ToString(), NextIndex);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_FindPatrolPoint::GetStaticDescription() const
{
	return TEXT("Find next patrol point and store in TargetLocation");
}
