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
	float LowAlliesAggressionMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal Enemy|Behavior")
	float HighAlliesAggressionMultiplier = 1.5f;

	virtual void Attack() override;

protected:
	virtual void OnStateEnter(EEnemyState NewState) override;
	virtual void OnStateExit(EEnemyState OldState) override;

	UFUNCTION(BlueprintCallable, Category = "Normal Enemy")
	float GetAggressionMultiplier() const;

private:
	FEnemyCombatConfig OriginalCombatConfig;
	bool bIsAggressive;
};
