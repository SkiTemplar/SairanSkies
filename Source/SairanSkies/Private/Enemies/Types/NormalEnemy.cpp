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

	// Combat configuration
	CombatConfig.MinAttackDistance = 100.0f;
	CombatConfig.MaxAttackDistance = 150.0f;
	CombatConfig.PositioningDistance = 300.0f;
	CombatConfig.MinPositioningTime = 1.5f;
	CombatConfig.MaxPositioningTime = 3.5f;
	CombatConfig.BaseDamage = 10.0f;
	CombatConfig.AttackCooldown = 1.5f;
	CombatConfig.AllyDetectionRadius = 1500.0f;
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
	PatrolConfig.PatrolSpeedMultiplier = 0.4f;
	PatrolConfig.ChaseSpeedMultiplier = 1.0f;
	PatrolConfig.WaitTimeAtPatrolPoint = 2.0f;
	PatrolConfig.PatrolPointAcceptanceRadius = 100.0f;
	PatrolConfig.bRandomPatrol = false;

	// Behavior
	TauntProbability = 0.3f;
	TauntCooldown = 5.0f;
	LowAlliesAggressionMultiplier = 0.5f;
	HighAlliesAggressionMultiplier = 1.5f;

	TimeSinceLastTaunt = TauntCooldown; // Allow immediate taunt
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

	if (IsDead())
	{
		return;
	}

	// Update taunt cooldown
	TimeSinceLastTaunt += DeltaTime;

	// Adjust combat behavior based on nearby allies
	AdjustCombatDistances();
}

// ==================== COMBAT OVERRIDES ====================

void ANormalEnemy::Attack()
{
	if (!CanAttack() || !GetCurrentTarget())
	{
		return;
	}

	// Apply aggression multiplier to damage
	float AdjustedDamage = CombatConfig.BaseDamage * GetAggressionMultiplier();

	// Set attack state
	SetEnemyState(EEnemyState::Attacking);

	bCanAttack = false;
	AttackCooldownTimer = CombatConfig.AttackCooldown;

	// Apply damage
	UGameplayStatics::ApplyDamage(
		GetCurrentTarget(),
		AdjustedDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
}

void ANormalEnemy::PerformTaunt()
{
	if (IsDead() || TimeSinceLastTaunt < TauntCooldown)
	{
		return;
	}

	SetEnemyState(EEnemyState::Taunting);
	TimeSinceLastTaunt = 0.0f;

	// Optionally alert more allies when taunting
	if (GetCurrentTarget())
	{
		AlertNearbyAllies(GetCurrentTarget());
	}
}

bool ANormalEnemy::ShouldTaunt() const
{
	// Don't taunt if on cooldown
	if (TimeSinceLastTaunt < TauntCooldown)
	{
		return false;
	}

	// Only taunt in combat
	if (!IsInCombat())
	{
		return false;
	}

	// More likely to taunt with more allies
	float AdjustedProbability = TauntProbability;
	if (HasEnoughAlliesForAggression())
	{
		AdjustedProbability *= 1.5f;
	}
	else
	{
		AdjustedProbability *= 0.5f;
	}

	return FMath::RandRange(0.0f, 1.0f) < AdjustedProbability;
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

	case EEnemyState::Positioning:
		SetMovementSpeed(PatrolConfig.ChaseSpeedMultiplier * 0.7f);
		break;

	case EEnemyState::Attacking:
	case EEnemyState::Taunting:
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

void ANormalEnemy::HandleCombatBehavior(float DeltaTime)
{
	Super::HandleCombatBehavior(DeltaTime);

	if (IsInCombat() && GetCurrentTarget())
	{
		bool bShouldBeAggressive = HasEnoughAlliesForAggression();
		
		if (bShouldBeAggressive != bIsAggressive)
		{
			bIsAggressive = bShouldBeAggressive;
			AdjustCombatDistances();
		}
	}
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

void ANormalEnemy::AdjustCombatDistances()
{
	float AggressionMult = GetAggressionMultiplier();

	if (AggressionMult > 1.0f)
	{
		// More aggressive - closer positioning, shorter wait times
		CombatConfig.PositioningDistance = OriginalCombatConfig.PositioningDistance * 0.7f;
		CombatConfig.MinPositioningTime = OriginalCombatConfig.MinPositioningTime * 0.5f;
		CombatConfig.MaxPositioningTime = OriginalCombatConfig.MaxPositioningTime * 0.5f;
	}
	else if (AggressionMult < 1.0f)
	{
		// More cautious - further positioning, longer wait times
		CombatConfig.PositioningDistance = OriginalCombatConfig.PositioningDistance * 1.3f;
		CombatConfig.MinPositioningTime = OriginalCombatConfig.MinPositioningTime * 1.5f;
		CombatConfig.MaxPositioningTime = OriginalCombatConfig.MaxPositioningTime * 1.5f;
	}
	else
	{
		// Normal - restore original values
		CombatConfig.PositioningDistance = OriginalCombatConfig.PositioningDistance;
		CombatConfig.MinPositioningTime = OriginalCombatConfig.MinPositioningTime;
		CombatConfig.MaxPositioningTime = OriginalCombatConfig.MaxPositioningTime;
	}
}
