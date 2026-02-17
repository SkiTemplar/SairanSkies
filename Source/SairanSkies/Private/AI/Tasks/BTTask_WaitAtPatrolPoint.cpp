// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/BTTask_WaitAtPatrolPoint.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Animation/EnemyAnimInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

UBTTask_WaitAtPatrolPoint::UBTTask_WaitAtPatrolPoint()
{
	NodeName = TEXT("Wait At Patrol Point");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	
	WaitTimer = 0.0f;
	TargetWaitTime = 0.0f;
	CurrentLookIndex = 0;
	LookTimer = 0.0f;
	bIsRotating = false;
	bCheckedForConversation = false;
	bInConversation = false;
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

	// Calculate wait time with variation
	if (bUseEnemyWaitTime)
	{
		TargetWaitTime = FMath::RandRange(
			Enemy->PatrolConfig.WaitTimeAtPatrolPoint,
			Enemy->PatrolConfig.MaxWaitTimeAtPatrolPoint
		);
	}
	else
	{
		TargetWaitTime = FMath::RandRange(CustomWaitTimeMin, CustomWaitTimeMax);
	}

	WaitTimer = 0.0f;
	AIController->StopMovement();

	// Initialize conversation check state
	bCheckedForConversation = false;
	bInConversation = false;

	// Initialize look around system - using animation system instead of rotating body
	OriginalRotation = Enemy->GetActorRotation();
	CurrentLookIndex = 0;
	LookTimer = 0.0f;
	bIsRotating = false;

	// Set initial random look direction via animation system
	if (bLookAround && LookAroundCount > 0)
	{
		float RandomYaw = FMath::RandRange(-MaxLookAngle, MaxLookAngle);
		TargetLookRotation = FRotator(0.0f, RandomYaw, 0.0f);
		bIsRotating = true;
		
		// Use animation system if available
		UEnemyAnimInstance* AnimInstance = Cast<UEnemyAnimInstance>(Enemy->GetMesh()->GetAnimInstance());
		if (AnimInstance)
		{
			AnimInstance->SetLookAtRotation(RandomYaw, 0.0f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("WaitAtPatrolPoint: %s starting wait for %.1f seconds"), *Enemy->GetName(), TargetWaitTime);

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

	// Get animation instance
	UEnemyAnimInstance* AnimInstance = Cast<UEnemyAnimInstance>(Enemy->GetMesh()->GetAnimInstance());

	// Interrupt if target detected
	if (Enemy->GetCurrentTarget() || Enemy->IsAlerted())
	{
		// Clear look at
		if (AnimInstance)
		{
			AnimInstance->ClearLookAt();
		}
		
		if (Enemy->GetCurrentTarget())
		{
			Enemy->SetEnemyState(EEnemyState::Chasing);
		}
		else if (Enemy->IsAlerted())
		{
			Enemy->SetEnemyState(EEnemyState::Investigating);
		}
		
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	WaitTimer += DeltaSeconds;

	// ========== CONVERSATION SYSTEM ==========
	// Check if we should look for a conversation partner
	if (bCheckForConversation && !bCheckedForConversation && !bInConversation)
	{
		if (WaitTimer >= TimeBeforeConversationCheck)
		{
			bCheckedForConversation = true;
			
			// Look for nearby enemy to converse with
			AEnemyBase* NearbyEnemy = Enemy->FindNearbyEnemyForConversation();
			if (NearbyEnemy)
			{
				// Try to start conversation
				if (Enemy->TryStartConversation(NearbyEnemy))
				{
					bInConversation = true;
					// Extend wait time to conversation duration
					TargetWaitTime = Enemy->ConversationConfig.MaxConversationDuration + WaitTimer;
					
					UE_LOG(LogTemp, Log, TEXT("WaitAtPatrolPoint: %s starting conversation, extending wait to %.1f seconds"), 
						*Enemy->GetName(), TargetWaitTime);
				}
			}
		}
	}

	// If in conversation, check if it ended
	if (bInConversation && !Enemy->IsConversing())
	{
		bInConversation = false;
		// End the task since conversation is over
		if (AnimInstance)
		{
			AnimInstance->ClearLookAt();
		}
		UE_LOG(LogTemp, Log, TEXT("WaitAtPatrolPoint: %s conversation ended, continuing patrol"), *Enemy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// If in conversation, don't do normal look around behavior
	if (bInConversation)
	{
		// Just wait for conversation to end
		return;
	}

	// Handle looking around behavior via animation system (head/torso only)
	if (bLookAround && CurrentLookIndex < LookAroundCount)
	{
		LookTimer += DeltaSeconds;
		
		if (LookTimer >= TimePerLookDirection)
		{
			CurrentLookIndex++;
			LookTimer = 0.0f;
			
			if (CurrentLookIndex < LookAroundCount)
			{
				// Pick new look direction (alternating sides for more natural look)
				float BaseAngle = (CurrentLookIndex % 2 == 0) ? MaxLookAngle : -MaxLookAngle;
				float RandomVariation = FMath::RandRange(-20.0f, 20.0f);
				float NewYaw = BaseAngle * FMath::RandRange(0.5f, 1.0f) + RandomVariation;
				
				// Update animation system
				if (AnimInstance)
				{
					AnimInstance->SetLookAtRotation(NewYaw, FMath::RandRange(-10.0f, 10.0f));
				}
			}
			else
			{
				// Return to looking forward
				if (AnimInstance)
				{
					AnimInstance->ClearLookAt();
				}
			}
		}
	}

	// Check if wait is complete
	if (WaitTimer >= TargetWaitTime)
	{
		// Clear any look at
		if (AnimInstance)
		{
			AnimInstance->ClearLookAt();
		}
		
		UE_LOG(LogTemp, Log, TEXT("WaitAtPatrolPoint: %s finished waiting, continuing patrol"), *Enemy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_WaitAtPatrolPoint::GetStaticDescription() const
{
	FString Description;
	
	if (bUseEnemyWaitTime)
	{
		Description = TEXT("Wait using enemy config time");
	}
	else
	{
		Description = FString::Printf(TEXT("Wait %.1f-%.1f sec"), CustomWaitTimeMin, CustomWaitTimeMax);
	}
	
	if (bLookAround)
	{
		Description += FString::Printf(TEXT(", look around %d times (via animation)"), LookAroundCount);
	}
	
	return Description;
}
