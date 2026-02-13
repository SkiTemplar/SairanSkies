// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Investigate.generated.h"

UCLASS()
class SAIRANSKIES_API UBTTask_Investigate : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Investigate();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Investigation")
	int32 InvestigationPoints = 3;

	UPROPERTY(EditAnywhere, Category = "Investigation")
	float AcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Investigation")
	float WaitTimeAtPoint = 2.0f;

private:
	int32 CurrentInvestigationPoint;
	float WaitTimer;
	float TotalInvestigationTime;
	bool bWaitingAtPoint;
	FVector CurrentTargetLocation;
};
