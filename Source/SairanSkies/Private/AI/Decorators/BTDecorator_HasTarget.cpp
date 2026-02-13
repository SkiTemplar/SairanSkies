// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Decorators/BTDecorator_HasTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasTarget::UBTDecorator_HasTarget()
{
	NodeName = TEXT("Has Target");
}

bool UBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
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

	return (Enemy->GetCurrentTarget() != nullptr);
}

FString UBTDecorator_HasTarget::GetStaticDescription() const
{
	return TEXT("Has target");
}
