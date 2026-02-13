// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/EnemyAIController.h"
#include "Enemies/EnemyBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"

AEnemyAIController::AEnemyAIController()
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent")));
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemyBase* Enemy = Cast<AEnemyBase>(InPawn);
	if (Enemy)
	{
		SetupPerceptionSystem();

		if (Enemy->BehaviorTree)
		{
			StartBehaviorTree(Enemy->BehaviorTree);
		}
	}
}

void AEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	StopBehaviorTree();
}

void AEnemyAIController::StartBehaviorTree(UBehaviorTree* Tree)
{
	if (!Tree)
	{
		return;
	}

	if (Tree->BlackboardAsset)
	{
		UBlackboardComponent* BlackboardComp = Blackboard.Get();
		if (UseBlackboard(Tree->BlackboardAsset, BlackboardComp))
		{
			InitializeBlackboardValues();
		}
	}

	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StartTree(*Tree);
	}
}

void AEnemyAIController::StopBehaviorTree()
{
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree();
	}
}

AEnemyBase* AEnemyAIController::GetControlledEnemy() const
{
	return Cast<AEnemyBase>(GetPawn());
}

void AEnemyAIController::SetupPerceptionSystem()
{
	AEnemyBase* Enemy = GetControlledEnemy();
	if (!Enemy)
	{
		return;
	}

	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp)
	{
		return;
	}

	SightConfig = NewObject<UAISenseConfig_Sight>(this, UAISenseConfig_Sight::StaticClass(), TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = Enemy->PerceptionConfig.SightRadius;
		SightConfig->LoseSightRadius = Enemy->PerceptionConfig.SightRadius + 500.0f;
		SightConfig->PeripheralVisionAngleDegrees = Enemy->PerceptionConfig.PeripheralVisionAngle;
		SightConfig->SetMaxAge(Enemy->PerceptionConfig.LoseSightTime);
		SightConfig->AutoSuccessRangeFromLastSeenLocation = Enemy->PerceptionConfig.ProximityRadius;
		
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

		PerceptionComp->ConfigureSense(*SightConfig);
	}

	HearingConfig = NewObject<UAISenseConfig_Hearing>(this, UAISenseConfig_Hearing::StaticClass(), TEXT("HearingConfig"));
	if (HearingConfig)
	{
		HearingConfig->HearingRange = Enemy->PerceptionConfig.HearingRadius;
		HearingConfig->SetMaxAge(Enemy->PerceptionConfig.LoseSightTime);
		
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

		PerceptionComp->ConfigureSense(*HearingConfig);
	}

	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}

void AEnemyAIController::InitializeBlackboardValues()
{
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsObject(AEnemyBase::BB_TargetActor, nullptr);
	Blackboard->SetValueAsVector(AEnemyBase::BB_TargetLocation, FVector::ZeroVector);
	Blackboard->SetValueAsEnum(AEnemyBase::BB_EnemyState, static_cast<uint8>(EEnemyState::Idle));
	Blackboard->SetValueAsBool(AEnemyBase::BB_CanSeeTarget, false);
	Blackboard->SetValueAsInt(AEnemyBase::BB_PatrolIndex, 0);
	Blackboard->SetValueAsBool(AEnemyBase::BB_ShouldTaunt, false);
	Blackboard->SetValueAsInt(AEnemyBase::BB_NearbyAllies, 0);
	Blackboard->SetValueAsFloat(AEnemyBase::BB_DistanceToTarget, 0.0f);
}
