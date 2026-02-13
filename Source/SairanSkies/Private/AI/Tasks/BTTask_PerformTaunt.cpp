// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_PerformTaunt.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTTask_PerformTaunt::UBTTask_PerformTaunt()
{
	NodeName = TEXT("Perform Taunt");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	TauntTimer = 0.0f;
}

EBTNodeResult::Type UBTTask_PerformTaunt::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	if (!Enemy->ShouldTaunt())
	{
		return EBTNodeResult::Failed;
	}

	AIController->StopMovement();
	Enemy->PerformTaunt();
	TauntTimer = 0.0f;

	return EBTNodeResult::InProgress;
}

void UBTTask_PerformTaunt::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	TauntTimer += DeltaSeconds;

	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AEnemyBase* Enemy = AIController->GetControlledEnemy();
		if (Enemy)
		{
			AActor* Target = Enemy->GetCurrentTarget();
			if (Target)
			{
				FVector DirectionToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
				Enemy->SetActorRotation(DirectionToTarget.Rotation());
			}
		}
	}

	if (TauntTimer >= TauntDuration)
	{
		if (AIController)
		{
			AEnemyBase* Enemy = AIController->GetControlledEnemy();
			if (Enemy && Enemy->GetCurrentTarget())
			{
				Enemy->SetEnemyState(EEnemyState::Chasing);
			}
		}

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_PerformTaunt::GetStaticDescription() const
{
	return FString::Printf(TEXT("Perform taunt (%.1f sec)"), TauntDuration);
}
