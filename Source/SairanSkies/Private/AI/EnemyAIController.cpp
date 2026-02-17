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
		if (Blackboard)
		{
			Blackboard->InitializeBlackboard(*Tree->BlackboardAsset);
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
		UE_LOG(LogTemp, Warning, TEXT("SetupPerceptionSystem: No controlled enemy!"));
		return;
	}

	UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent();
	if (!PerceptionComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetupPerceptionSystem: No perception component!"));
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
		
		// Detect ALL targets - we filter by team attitude, not affiliation flags
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

		PerceptionComp->ConfigureSense(*SightConfig);
		
		UE_LOG(LogTemp, Log, TEXT("SetupPerceptionSystem: Sight configured - Radius: %.1f, Angle: %.1f"), 
			SightConfig->SightRadius, SightConfig->PeripheralVisionAngleDegrees);
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
		
		UE_LOG(LogTemp, Log, TEXT("SetupPerceptionSystem: Hearing configured - Range: %.1f"), 
			HearingConfig->HearingRange);
	}

	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
	
	// Connect perception delegate
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);
	
	// Force perception update
	PerceptionComp->RequestStimuliListenerUpdate();
	
	UE_LOG(LogTemp, Log, TEXT("SetupPerceptionSystem: %s perception system ready"), *Enemy->GetName());
}

void AEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	AEnemyBase* Enemy = GetControlledEnemy();
	if (!Enemy || !Actor)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("AIController OnPerception: %s detected %s (Success: %s)"), 
		*Enemy->GetName(), *Actor->GetName(), 
		Stimulus.WasSuccessfullySensed() ? TEXT("YES") : TEXT("NO"));
	
	// Forward to Enemy for processing
	Enemy->OnPerceptionUpdated(Actor, Stimulus);
}

void AEnemyAIController::InitializeBlackboardValues()
{
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsObject(AEnemyBase::BB_TargetActor, nullptr);
	Blackboard->SetValueAsVector(AEnemyBase::BB_TargetLocation, FVector::ZeroVector);
	Blackboard->SetValueAsInt(AEnemyBase::BB_EnemyState, static_cast<int32>(EEnemyState::Idle));
	Blackboard->SetValueAsBool(AEnemyBase::BB_CanSeeTarget, false);
	Blackboard->SetValueAsInt(AEnemyBase::BB_PatrolIndex, 0);
	Blackboard->SetValueAsFloat(AEnemyBase::BB_DistanceToTarget, 0.0f);
	Blackboard->SetValueAsBool(AEnemyBase::BB_CanAttack, true);
	Blackboard->SetValueAsBool(AEnemyBase::BB_IsInPause, false);
	Blackboard->SetValueAsBool(AEnemyBase::BB_IsConversing, false);
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// Check if the other actor has a team interface
	const IGenericTeamAgentInterface* TeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	if (TeamAgent)
	{
		FGenericTeamId OtherTeamId = TeamAgent->GetGenericTeamId();
		
		// Same team = Friendly
		if (OtherTeamId == GetGenericTeamId())
		{
			return ETeamAttitude::Friendly;
		}
		
		// Player team = Hostile
		if (OtherTeamId.GetId() == TEAM_PLAYER)
		{
			return ETeamAttitude::Hostile;
		}
	}
	
	// If no team interface, check if it's the player pawn
	const APawn* OtherPawn = Cast<const APawn>(&Other);
	if (OtherPawn)
	{
		// Check if controlled by player controller
		if (OtherPawn->IsPlayerControlled())
		{
			return ETeamAttitude::Hostile;
		}
		
		// Check if it's another enemy
		if (Cast<const AEnemyBase>(OtherPawn))
		{
			return ETeamAttitude::Friendly;
		}
	}
	
	// Default: treat as neutral (will be detected if bDetectNeutrals is true)
	return ETeamAttitude::Neutral;
}

