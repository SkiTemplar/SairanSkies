// Fill out your copyright notice in the Description page of Project Settings.

#include "Navigation/PatrolPath.h"

APatrolPath::APatrolPath()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void APatrolPath::BeginPlay()
{
	Super::BeginPlay();
}

FVector APatrolPath::GetPatrolPoint(int32 Index) const
{
	if (PatrolPoints.Num() == 0)
	{
		return GetActorLocation();
	}

	Index = FMath::Clamp(Index, 0, PatrolPoints.Num() - 1);
	return GetActorTransform().TransformPosition(PatrolPoints[Index]);
}

int32 APatrolPath::GetNextPatrolIndex(int32 CurrentIndex, bool& bReversing) const
{
	if (PatrolPoints.Num() <= 1)
	{
		return 0;
	}

	int32 NextIndex = CurrentIndex;

	if (bPingPongPatrol)
	{
		if (bReversing)
		{
			NextIndex = CurrentIndex - 1;
			if (NextIndex < 0)
			{
				NextIndex = 1;
				bReversing = false;
			}
		}
		else
		{
			NextIndex = CurrentIndex + 1;
			if (NextIndex >= PatrolPoints.Num())
			{
				NextIndex = PatrolPoints.Num() - 2;
				bReversing = true;
			}
		}
	}
	else if (bLoopPatrol)
	{
		NextIndex = (CurrentIndex + 1) % PatrolPoints.Num();
	}
	else
	{
		NextIndex = FMath::Min(CurrentIndex + 1, PatrolPoints.Num() - 1);
	}

	return NextIndex;
}

int32 APatrolPath::GetRandomPatrolIndex(int32 CurrentIndex) const
{
	if (PatrolPoints.Num() <= 1)
	{
		return 0;
	}

	int32 RandomIndex;
	do
	{
		RandomIndex = FMath::RandRange(0, PatrolPoints.Num() - 1);
	}
	while (RandomIndex == CurrentIndex && PatrolPoints.Num() > 1);

	return RandomIndex;
}
