// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Decorators/BTDecorator_CheckEnemyState.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CheckEnemyState::UBTDecorator_CheckEnemyState()
{
	NodeName = TEXT("Check Enemy State");
	StateToCheck = EEnemyState::Idle;
}

bool UBTDecorator_CheckEnemyState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return false;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		return false;
	}

	return (Enemy->GetEnemyState() == StateToCheck);
}

FString UBTDecorator_CheckEnemyState::GetStaticDescription() const
{
	FString StateName;
	switch (StateToCheck)
	{
	case EEnemyState::Idle: StateName = TEXT("Idle"); break;
	case EEnemyState::Patrolling: StateName = TEXT("Patrolling"); break;
	case EEnemyState::Investigating: StateName = TEXT("Investigating"); break;
	case EEnemyState::Chasing: StateName = TEXT("Chasing"); break;
	case EEnemyState::Attacking: StateName = TEXT("Attacking"); break;
	case EEnemyState::Conversing: StateName = TEXT("Conversing"); break;
	case EEnemyState::Dead: StateName = TEXT("Dead"); break;
	default: StateName = TEXT("Unknown"); break;
	}

	return FString::Printf(TEXT("State is %s"), *StateName);
}
