// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatrolPath.generated.h"

UCLASS()
class SAIRANSKIES_API APatrolPath : public AActor
{
	GENERATED_BODY()
	
public:	
	APatrolPath();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bLoopPatrol = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bPingPongPatrol = false;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FVector GetPatrolPoint(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Patrol")
	int32 GetNumPatrolPoints() const { return PatrolPoints.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	int32 GetNextPatrolIndex(int32 CurrentIndex, bool& bReversing) const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	int32 GetRandomPatrolIndex(int32 CurrentIndex) const;

protected:
	virtual void BeginPlay() override;
};

