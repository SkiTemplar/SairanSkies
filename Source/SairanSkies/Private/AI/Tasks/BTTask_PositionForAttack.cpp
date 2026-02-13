// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_PositionForAttack.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"

UBTTask_PositionForAttack::UBTTask_PositionForAttack()
{
	NodeName = TEXT("Position For Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	
	PositioningTimer = 0.0f;
	TargetPositioningTime = 0.0f;
	StrafeDirection = FVector::ZeroVector;
}

EBTNodeResult::Type UBTTask_PositionForAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy || !Enemy->GetCurrentTarget())
	{
		return EBTNodeResult::Failed;
	}

	Enemy->SetEnemyState(EEnemyState::Positioning);

	TargetPositioningTime = FMath::RandRange(
		Enemy->CombatConfig.MinPositioningTime > 0 ? Enemy->CombatConfig.MinPositioningTime : MinPositioningTime,
		Enemy->CombatConfig.MaxPositioningTime > 0 ? Enemy->CombatConfig.MaxPositioningTime : MaxPositioningTime
	);
	PositioningTimer = 0.0f;

	StrafeDirection = FMath::RandBool() ? Enemy->GetActorRightVector() : -Enemy->GetActorRightVector();

	return EBTNodeResult::InProgress;
}

void UBTTask_PositionForAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	AActor* Target = Enemy->GetCurrentTarget();
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	PositioningTimer += DeltaSeconds;

	FVector DirectionToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
	FRotator LookAtRotation = DirectionToTarget.Rotation();
	Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), LookAtRotation, DeltaSeconds, 5.0f));

	if (bStrafeWhilePositioning)
	{
		Enemy->AddMovementInput(StrafeDirection, 0.3f);

		if (FMath::RandRange(0.0f, 1.0f) < 0.01f)
		{
			StrafeDirection = -StrafeDirection;
		}
	}

	if (PositioningTimer >= TargetPositioningTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	float CurrentDistance = Enemy->GetDistanceToTarget();
	if (CurrentDistance > Enemy->CombatConfig.PositioningDistance * 1.5f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
}

EBTNodeResult::Type UBTTask_PositionForAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

FString UBTTask_PositionForAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Position for attack (%.1f - %.1f sec)"), MinPositioningTime, MaxPositioningTime);
}
