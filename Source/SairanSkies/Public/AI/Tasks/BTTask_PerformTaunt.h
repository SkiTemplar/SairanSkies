// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PerformTaunt.generated.h"

UCLASS()
class SAIRANSKIES_API UBTTask_PerformTaunt : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformTaunt();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Taunt")
	float TauntDuration = 1.5f;

private:
	float TauntTimer;
};
