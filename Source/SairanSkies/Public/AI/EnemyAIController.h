// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionTypes.h"
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

	// Perception handler
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	void InitializeBlackboardValues();

	// ==================== TEAM SYSTEM ====================
public:
	// Team ID for AI Perception affiliation system
	// 0 = Player, 1 = Enemies, 255 = Neutral
	static constexpr int32 TEAM_ENEMIES = 1;
	static constexpr int32 TEAM_PLAYER = 0;

	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TEAM_ENEMIES); }
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
};