// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChaseTarget.generated.h"

UCLASS()
class SAIRANSKIES_API UBTTask_ChaseTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ChaseTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Chase")
	float AcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Chase")
	bool bUsePositioningDistance = true;
};
