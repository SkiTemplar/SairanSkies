// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Enemies/EnemyTypes.h"
#include "BTDecorator_CheckEnemyState.generated.h"

UCLASS()
class SAIRANSKIES_API UBTDecorator_CheckEnemyState : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckEnemyState();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Condition")
	EEnemyState StateToCheck;
};
