// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "NormalEnemy.generated.h"

UCLASS()
class SAIRANSKIES_API ANormalEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	ANormalEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal Enemy|Behavior")
	float TauntProbability = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal Enemy|Behavior")
	float TauntCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal Enemy|Behavior")
	float LowAlliesAggressionMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal Enemy|Behavior")
	float HighAlliesAggressionMultiplier = 1.5f;

	virtual void Attack() override;
	virtual void PerformTaunt() override;
	virtual bool ShouldTaunt() const override;

protected:
	virtual void OnStateEnter(EEnemyState NewState) override;
	virtual void OnStateExit(EEnemyState OldState) override;
	virtual void HandleCombatBehavior(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Normal Enemy")
	float GetAggressionMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "Normal Enemy")
	void AdjustCombatDistances();

private:
	float TimeSinceLastTaunt;
	FEnemyCombatConfig OriginalCombatConfig;
	bool bIsAggressive;
};
