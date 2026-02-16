// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_WaitAtPatrolPoint.generated.h"

UCLASS()
class SAIRANSKIES_API UBTTask_WaitAtPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_WaitAtPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// Use enemy's PatrolConfig wait time range
	UPROPERTY(EditAnywhere, Category = "Wait")
	bool bUseEnemyWaitTime = true;

	UPROPERTY(EditAnywhere, Category = "Wait", meta = (EditCondition = "!bUseEnemyWaitTime"))
	float CustomWaitTimeMin = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Wait", meta = (EditCondition = "!bUseEnemyWaitTime"))
	float CustomWaitTimeMax = 4.0f;

	// Look around while waiting (more natural)
	UPROPERTY(EditAnywhere, Category = "Look Around")
	bool bLookAround = true;

	// Number of directions to look at during wait
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (EditCondition = "bLookAround"))
	int32 LookAroundCount = 2;

	// Time spent looking in each direction
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (EditCondition = "bLookAround"))
	float TimePerLookDirection = 1.5f;

	// Maximum angle to turn when looking around
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (EditCondition = "bLookAround"))
	float MaxLookAngle = 90.0f;

	// Rotation speed when looking around (degrees per second)
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (EditCondition = "bLookAround"))
	float LookAroundRotationSpeed = 60.0f;

private:
	float WaitTimer;
	float TargetWaitTime;
	
	// Look around state
	FRotator OriginalRotation;
	FRotator TargetLookRotation;
	int32 CurrentLookIndex;
	float LookTimer;
	bool bIsRotating;
};
