// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PositionForAttack.generated.h"

UCLASS()
class SAIRANSKIES_API UBTTask_PositionForAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PositionForAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Positioning")
	float MinPositioningTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Positioning")
	float MaxPositioningTime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Positioning")
	bool bStrafeWhilePositioning = true;

private:
	float PositioningTimer;
	float TargetPositioningTime;
	FVector StrafeDirection;
};
