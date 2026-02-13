// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AEnemyBase;

UCLASS()
class SAIRANSKIES_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComponent;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StartBehaviorTree(UBehaviorTree* BehaviorTree);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopBehaviorTree();

	UFUNCTION(BlueprintPure, Category = "AI")
	AEnemyBase* GetControlledEnemy() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetupPerceptionSystem();

protected:
	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	void InitializeBlackboardValues();
};