// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasTarget.generated.h"

UCLASS()
class SAIRANSKIES_API UBTDecorator_HasTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasTarget();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
