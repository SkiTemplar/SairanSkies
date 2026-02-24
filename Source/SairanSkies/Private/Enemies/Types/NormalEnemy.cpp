// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemies/Types/NormalEnemy.h"
#include "AI/EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ANormalEnemy::ANormalEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Default values for normal enemy
	MaxHealth = 100.0f;

	// Combat configuration — Two Circles
	CombatConfig.OuterCircleRadius = 500.0f;
	CombatConfig.OuterCircleVariation = 80.0f;
	CombatConfig.MinAttackPositionDist = 100.0f;
	CombatConfig.MaxAttackPositionDist = 200.0f;
	CombatConfig.ChanceToStayInnerAfterAttack = 0.25f;
	CombatConfig.PlayerMoveReactionDelayMin = 0.4f;
	CombatConfig.PlayerMoveReactionDelayMax = 1.5f;
	CombatConfig.MinAttackDistance = 100.0f;
	CombatConfig.MaxAttackDistance = 200.0f;
	CombatConfig.BaseDamage = 10.0f;
	CombatConfig.AttackCooldown = 2.0f;
	CombatConfig.AllyDetectionRadius = 1500.0f;
	CombatConfig.MaxSimultaneousAttackers = 2;
	CombatConfig.MinAlliesForAggression = 2;

	// Perception configuration
	PerceptionConfig.SightRadius = 2000.0f;
	PerceptionConfig.PeripheralVisionAngle = 75.0f;
	PerceptionConfig.HearingRadius = 1000.0f;
	PerceptionConfig.ProximityRadius = 250.0f;
	PerceptionConfig.LoseSightTime = 5.0f;
	PerceptionConfig.InvestigationTime = 10.0f;
	PerceptionConfig.InvestigationRadius = 400.0f;

	// Patrol configuration
	PatrolConfig.PatrolSpeedMultiplier = 0.2f;
	PatrolConfig.ChaseSpeedMultiplier = 0.45f;
	PatrolConfig.WaitTimeAtPatrolPoint = 2.0f;
	PatrolConfig.PatrolPointAcceptanceRadius = 100.0f;
	PatrolConfig.bRandomPatrol = false;

	// Behavior
	LowAlliesAggressionMultiplier = 0.5f;
	HighAlliesAggressionMultiplier = 1.5f;

	bIsAggressive = false;

	// Set default AI Controller class
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ANormalEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Store original combat config
	OriginalCombatConfig = CombatConfig;
}

void ANormalEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ==================== COMBAT OVERRIDES ====================

void ANormalEnemy::Attack()
{
	if (!CanAttack() || !GetCurrentTarget())
	{
		return;
	}

	// Let base class handle cooldown and sounds
	Super::Attack();

	// Set attack state
	SetEnemyState(EEnemyState::Attacking);
}


// ==================== STATE MANAGEMENT ====================

void ANormalEnemy::OnStateEnter(EEnemyState NewState)
{
	Super::OnStateEnter(NewState);

	switch (NewState)
	{
	case EEnemyState::Chasing:
		SetChaseSpeed();
		break;

	case EEnemyState::OuterCircle:
		SetOuterCircleSpeed();
		break;

	case EEnemyState::InnerCircle:
		SetChaseSpeed();
		break;

	case EEnemyState::Attacking:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
		break;

	default:
		break;
	}
}

void ANormalEnemy::OnStateExit(EEnemyState OldState)
{
	Super::OnStateExit(OldState);
}

// ==================== BEHAVIOR HELPERS ====================

float ANormalEnemy::GetAggressionMultiplier() const
{
	if (HasEnoughAlliesForAggression())
	{
		return HighAlliesAggressionMultiplier;
	}
	else if (NearbyAlliesCount == 0)
	{
		return LowAlliesAggressionMultiplier;
	}
	
	return 1.0f;
}

