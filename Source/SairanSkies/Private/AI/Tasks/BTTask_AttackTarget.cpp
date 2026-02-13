// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_AttackTarget.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	AttackTimer = 0.0f;
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	if (!Enemy->CanAttack())
	{
		return EBTNodeResult::Failed;
	}

	if (!Enemy->IsInAttackRange())
	{
		return EBTNodeResult::Failed;
	}

	FVector DirectionToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
	Enemy->SetActorRotation(DirectionToTarget.Rotation());

	Enemy->Attack();

	AttackTimer = 0.0f;

	return EBTNodeResult::InProgress;
}

void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AttackTimer += DeltaSeconds;

	if (AttackTimer >= AttackDuration)
	{
		AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
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

FString UBTTask_AttackTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack target (%.1f sec duration)"), AttackDuration);
}
