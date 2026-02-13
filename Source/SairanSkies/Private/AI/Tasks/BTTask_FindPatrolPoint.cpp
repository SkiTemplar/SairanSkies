// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_FindPatrolPoint.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Navigation/PatrolPath.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Find Patrol Point");
	PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, PatrolLocationKey));
	PatrolIndexKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, PatrolIndexKey));
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy || !Enemy->PatrolPath)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	APatrolPath* Path = Enemy->PatrolPath;
	if (Path->GetNumPatrolPoints() == 0)
	{
		return EBTNodeResult::Failed;
	}

	int32 CurrentIndex = BlackboardComp->GetValueAsInt(PatrolIndexKey.SelectedKeyName);
	
	int32 NextIndex;
	if (Enemy->PatrolConfig.bRandomPatrol)
	{
		NextIndex = Path->GetRandomPatrolIndex(CurrentIndex);
	}
	else
	{
		bool bReversing = false;
		NextIndex = Path->GetNextPatrolIndex(CurrentIndex, bReversing);
	}

	FVector PatrolLocation = Path->GetPatrolPoint(NextIndex);

	BlackboardComp->SetValueAsVector(PatrolLocationKey.SelectedKeyName, PatrolLocation);
	BlackboardComp->SetValueAsInt(PatrolIndexKey.SelectedKeyName, NextIndex);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_FindPatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find next patrol point and store in %s"), *PatrolLocationKey.SelectedKeyName.ToString());
}
