// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_IdleBehavior.generated.h"

/**
 * Task that handles natural idle behaviors like random pauses, looking around, etc.
 * Makes the AI feel more natural and less robotic.
 */
UCLASS()
class SAIRANSKIES_API UBTTask_IdleBehavior : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IdleBehavior();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// Probability that this task will actually pause (0-1)
	UPROPERTY(EditAnywhere, Category = "Idle Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PauseChance = 0.3f;

	// Minimum pause duration in seconds
	UPROPERTY(EditAnywhere, Category = "Idle Behavior")
	float MinPauseDuration = 0.5f;

	// Maximum pause duration in seconds
	UPROPERTY(EditAnywhere, Category = "Idle Behavior")
	float MaxPauseDuration = 2.0f;

	// Whether to look around during the pause
	UPROPERTY(EditAnywhere, Category = "Idle Behavior")
	bool bLookAroundDuringPause = true;

	// Whether to use enemy's BehaviorConfig values instead
	UPROPERTY(EditAnywhere, Category = "Idle Behavior")
	bool bUseEnemyConfig = true;

private:
	float CurrentPauseTimer;
	float TargetPauseDuration;
	bool bIsPausing;
};

