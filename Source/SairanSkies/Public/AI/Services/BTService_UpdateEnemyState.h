// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateEnemyState.generated.h"

UCLASS()
class SAIRANSKIES_API UBTService_UpdateEnemyState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateEnemyState();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
};
