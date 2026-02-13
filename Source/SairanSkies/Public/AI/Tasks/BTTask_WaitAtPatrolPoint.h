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
	UPROPERTY(EditAnywhere, Category = "Wait")
	bool bUseEnemyWaitTime = true;

	UPROPERTY(EditAnywhere, Category = "Wait", meta = (EditCondition = "!bUseEnemyWaitTime"))
	float CustomWaitTime = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Wait")
	bool bLookAround = true;

	UPROPERTY(EditAnywhere, Category = "Wait", meta = (EditCondition = "bLookAround"))
	float LookAroundSpeed = 45.0f;

private:
	float WaitTimer;
	float TargetWaitTime;
};
