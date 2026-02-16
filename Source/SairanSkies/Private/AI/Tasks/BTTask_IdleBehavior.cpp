// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_IdleBehavior.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTTask_IdleBehavior::UBTTask_IdleBehavior()
{
	NodeName = TEXT("Idle Behavior (Random Pause)");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	
	CurrentPauseTimer = 0.0f;
	TargetPauseDuration = 0.0f;
	bIsPausing = false;
}

EBTNodeResult::Type UBTTask_IdleBehavior::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Succeeded; // Don't fail, just skip
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		return EBTNodeResult::Succeeded;
	}

	// Don't pause if alerted
	if (Enemy->IsAlerted())
	{
		return EBTNodeResult::Succeeded;
	}

	// Get config values
	float ChanceToUse = bUseEnemyConfig ? Enemy->BehaviorConfig.ChanceToStopDuringPatrol : PauseChance;
	
	// Roll for pause
	if (FMath::RandRange(0.0f, 1.0f) > ChanceToUse)
	{
		// No pause this time
		return EBTNodeResult::Succeeded;
	}

	// Start pausing
	bIsPausing = true;
	
	if (bUseEnemyConfig)
	{
		TargetPauseDuration = FMath::RandRange(
			Enemy->BehaviorConfig.MinRandomPauseDuration,
			Enemy->BehaviorConfig.MaxRandomPauseDuration
		);
	}
	else
	{
		TargetPauseDuration = FMath::RandRange(MinPauseDuration, MaxPauseDuration);
	}
	
	CurrentPauseTimer = 0.0f;

	// Stop movement
	AIController->StopMovement();

	// Start looking around
	if (bLookAroundDuringPause)
	{
		Enemy->StartLookingAround();
	}

	// Notify enemy
	Enemy->StartRandomPause();

	UE_LOG(LogTemp, Verbose, TEXT("IdleBehavior: %s starting random pause for %.1f seconds"), *Enemy->GetName(), TargetPauseDuration);

	return EBTNodeResult::InProgress;
}

void UBTTask_IdleBehavior::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!bIsPausing)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;

	// Abort pause if alerted or has target
	if (Enemy && (Enemy->IsAlerted() || Enemy->GetCurrentTarget()))
	{
		bIsPausing = false;
		UE_LOG(LogTemp, Verbose, TEXT("IdleBehavior: %s pause interrupted (alerted)"), *Enemy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	CurrentPauseTimer += DeltaSeconds;

	if (CurrentPauseTimer >= TargetPauseDuration)
	{
		bIsPausing = false;
		UE_LOG(LogTemp, Verbose, TEXT("IdleBehavior: %s pause complete"), Enemy ? *Enemy->GetName() : TEXT("Unknown"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_IdleBehavior::GetStaticDescription() const
{
	if (bUseEnemyConfig)
	{
		return TEXT("Random pause using enemy's BehaviorConfig");
	}
	return FString::Printf(TEXT("%.0f%% chance to pause for %.1f-%.1f sec"), PauseChance * 100.0f, MinPauseDuration, MaxPauseDuration);
}

