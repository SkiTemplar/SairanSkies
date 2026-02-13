// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Navigation/PatrolPath.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Blackboard Keys
const FName AEnemyBase::BB_TargetActor = TEXT("TargetActor");
const FName AEnemyBase::BB_TargetLocation = TEXT("TargetLocation");
const FName AEnemyBase::BB_EnemyState = TEXT("EnemyState");
const FName AEnemyBase::BB_CanSeeTarget = TEXT("CanSeeTarget");
const FName AEnemyBase::BB_PatrolIndex = TEXT("PatrolIndex");
const FName AEnemyBase::BB_ShouldTaunt = TEXT("ShouldTaunt");
const FName AEnemyBase::BB_NearbyAllies = TEXT("NearbyAllies");
const FName AEnemyBase::BB_DistanceToTarget = TEXT("DistanceToTarget");

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create AI Perception Component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	// Initialize state
	CurrentState = EEnemyState::Idle;
	CurrentTarget = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	CurrentHealth = MaxHealth;
	bCanAttack = true;
	TimeSinceLastSawTarget = 0.0f;
	NearbyAlliesCount = 0;
	AttackCooldownTimer = 0.0f;
	BaseMaxWalkSpeed = 600.0f;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health
	CurrentHealth = MaxHealth;

	// Store base movement speed
	if (GetCharacterMovement())
	{
		BaseMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	// Setup perception delegate
	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyBase::OnPerceptionUpdated);
	}

	// Start in patrol state if we have a patrol path
	if (PatrolPath)
	{
		SetEnemyState(EEnemyState::Patrolling);
	}
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	// Update attack cooldown
	if (!bCanAttack)
	{
		AttackCooldownTimer -= DeltaTime;
		if (AttackCooldownTimer <= 0.0f)
		{
			bCanAttack = true;
		}
	}

	// Update perception
	UpdateTargetPerception();

	// Update nearby allies count periodically
	UpdateNearbyAlliesCount();

	// Handle combat behavior
	HandleCombatBehavior(DeltaTime);

	// Update blackboard
	UpdateBlackboard();
}

// ==================== STATE MANAGEMENT ====================

void AEnemyBase::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	
	// Exit old state
	OnStateExit(OldState);

	// Change state
	CurrentState = NewState;

	// Enter new state
	OnStateEnter(NewState);

	// Broadcast state change
	OnEnemyStateChanged.Broadcast(OldState, NewState);

	// Update movement speed based on state
	switch (NewState)
	{
	case EEnemyState::Patrolling:
	case EEnemyState::Investigating:
		SetPatrolSpeed();
		break;
	case EEnemyState::Chasing:
	case EEnemyState::Attacking:
		SetChaseSpeed();
		break;
	default:
		break;
	}
}

bool AEnemyBase::IsInCombat() const
{
	return CurrentState == EEnemyState::Chasing ||
		   CurrentState == EEnemyState::Positioning ||
		   CurrentState == EEnemyState::Attacking ||
		   CurrentState == EEnemyState::Taunting;
}

bool AEnemyBase::CanSeeTarget() const
{
	if (!CurrentTarget || !AIPerceptionComponent)
	{
		return false;
	}

	FActorPerceptionBlueprintInfo Info;
	AIPerceptionComponent->GetActorsPerception(CurrentTarget, Info);

	for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
	{
		if (Stimulus.WasSuccessfullySensed() && Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			return true;
		}
	}

	return false;
}

void AEnemyBase::OnStateEnter(EEnemyState NewState)
{
	// Override in subclasses for specific behavior
}

void AEnemyBase::OnStateExit(EEnemyState OldState)
{
	// Override in subclasses for specific behavior
}

void AEnemyBase::HandleCombatBehavior(float DeltaTime)
{
	// Override in subclasses for specific combat behavior
	if (IsInCombat() && CurrentTarget)
	{
		// Update time since last saw target
		if (CanSeeTarget())
		{
			TimeSinceLastSawTarget = 0.0f;
			LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		}
		else
		{
			TimeSinceLastSawTarget += DeltaTime;

			// Lose target after timeout
			if (TimeSinceLastSawTarget >= PerceptionConfig.LoseSightTime)
			{
				LoseTarget();
			}
		}
	}
}

// ==================== PERCEPTION ====================

void AEnemyBase::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (CurrentState == EEnemyState::Dead || !Actor)
	{
		return;
	}

	// Check if this is the player
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (Actor != PlayerPawn)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Determine sense type
		EEnemySenseType SenseType = EEnemySenseType::None;
		
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			SenseType = EEnemySenseType::Sight;
		}
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			SenseType = EEnemySenseType::Hearing;
		}

		SetTarget(Actor, SenseType);
	}
}

void AEnemyBase::UpdateTargetPerception()
{
	if (!CurrentTarget)
	{
		return;
	}

	// Check proximity
	float DistanceToTarget = GetDistanceToTarget();
	if (DistanceToTarget <= PerceptionConfig.ProximityRadius)
	{
		TimeSinceLastSawTarget = 0.0f;
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
	}
}

void AEnemyBase::LoseTarget()
{
	if (CurrentTarget)
	{
		CurrentTarget = nullptr;
		OnPlayerLost.Broadcast();

		// Transition to investigation state
		if (LastKnownTargetLocation != FVector::ZeroVector)
		{
			SetEnemyState(EEnemyState::Investigating);
		}
		else
		{
			SetEnemyState(EEnemyState::Patrolling);
		}
	}
}

void AEnemyBase::SetTarget(AActor* NewTarget, EEnemySenseType SenseType)
{
	if (!NewTarget || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	bool bNewTarget = (CurrentTarget != NewTarget);
	CurrentTarget = NewTarget;
	LastKnownTargetLocation = NewTarget->GetActorLocation();
	TimeSinceLastSawTarget = 0.0f;

	if (bNewTarget)
	{
		// Alert nearby allies
		AlertNearbyAllies(NewTarget);

		// Broadcast detection
		OnPlayerDetected.Broadcast(NewTarget, SenseType);

		// Transition to chasing state
		SetEnemyState(EEnemyState::Chasing);
	}
}

// ==================== COMBAT ====================

void AEnemyBase::Attack()
{
	if (!CanAttack() || !CurrentTarget)
	{
		return;
	}

	bCanAttack = false;
	AttackCooldownTimer = CombatConfig.AttackCooldown;

	SetEnemyState(EEnemyState::Attacking);

	// Apply damage to target (override in subclasses for specific damage behavior)
	UGameplayStatics::ApplyDamage(
		CurrentTarget,
		CombatConfig.BaseDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
}

void AEnemyBase::TakeDamageFromSource(float DamageAmount, AActor* DamageSource, AController* InstigatorController)
{
	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	CurrentHealth -= DamageAmount;

	// If we don't have a target and we got damaged, set the damage source as target
	if (!CurrentTarget && DamageSource)
	{
		SetTarget(DamageSource, EEnemySenseType::Damage);
	}

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		Die(InstigatorController);
	}
}

void AEnemyBase::Die(AController* InstigatorController)
{
	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	SetEnemyState(EEnemyState::Dead);
	CurrentHealth = 0.0f;

	// Broadcast death
	OnEnemyDeath.Broadcast(InstigatorController);

	// Disable collision and movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}
	SetActorEnableCollision(false);
}

float AEnemyBase::GetDistanceToTarget() const
{
	if (!CurrentTarget)
	{
		return MAX_FLT;
	}

	return FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
}

bool AEnemyBase::IsInAttackRange() const
{
	float Distance = GetDistanceToTarget();
	return Distance >= CombatConfig.MinAttackDistance && Distance <= CombatConfig.MaxAttackDistance;
}

bool AEnemyBase::ShouldApproachTarget() const
{
	return GetDistanceToTarget() > CombatConfig.MaxAttackDistance;
}

// ==================== ALLY COORDINATION ====================

void AEnemyBase::AlertNearbyAllies(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	TArray<AEnemyBase*> Allies = GetNearbyAllies();
	for (AEnemyBase* Ally : Allies)
	{
		if (Ally && Ally != this && !Ally->IsDead())
		{
			Ally->ReceiveAlertFromAlly(Target, this);
		}
	}
}

void AEnemyBase::ReceiveAlertFromAlly(AActor* Target, AEnemyBase* AlertingAlly)
{
	if (!Target || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	// Only react if not already in combat
	if (!IsInCombat())
	{
		SetTarget(Target, EEnemySenseType::Alert);
	}
}

void AEnemyBase::UpdateNearbyAlliesCount()
{
	NearbyAlliesCount = GetNearbyAllies().Num();
}

bool AEnemyBase::HasEnoughAlliesForAggression() const
{
	return NearbyAlliesCount >= CombatConfig.MinAlliesForAggression;
}

TArray<AEnemyBase*> AEnemyBase::GetNearbyAllies() const
{
	TArray<AEnemyBase*> Allies;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(Actor);
		if (Enemy && Enemy != this && !Enemy->IsDead())
		{
			float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation());
			if (Distance <= CombatConfig.AllyDetectionRadius)
			{
				Allies.Add(Enemy);
			}
		}
	}

	return Allies;
}

// ==================== TAUNTING ====================

void AEnemyBase::PerformTaunt()
{
	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	SetEnemyState(EEnemyState::Taunting);
	
	// Override in subclasses for specific taunt behavior
}

bool AEnemyBase::ShouldTaunt() const
{
	// Taunt when we have allies and are confident
	return HasEnoughAlliesForAggression() && IsInCombat() && FMath::RandRange(0.0f, 1.0f) < 0.3f;
}

// ==================== MOVEMENT ====================

void AEnemyBase::SetMovementSpeed(float SpeedMultiplier)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMaxWalkSpeed * FMath::Clamp(SpeedMultiplier, 0.0f, 2.0f);
	}
}

void AEnemyBase::SetPatrolSpeed()
{
	SetMovementSpeed(PatrolConfig.PatrolSpeedMultiplier);
}

void AEnemyBase::SetChaseSpeed()
{
	SetMovementSpeed(PatrolConfig.ChaseSpeedMultiplier);
}

// ==================== BLACKBOARD ====================

void AEnemyBase::UpdateBlackboard()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	BlackboardComp->SetValueAsObject(BB_TargetActor, CurrentTarget);
	BlackboardComp->SetValueAsVector(BB_TargetLocation, LastKnownTargetLocation);
	BlackboardComp->SetValueAsEnum(BB_EnemyState, static_cast<uint8>(CurrentState));
	BlackboardComp->SetValueAsBool(BB_CanSeeTarget, CanSeeTarget());
	BlackboardComp->SetValueAsBool(BB_ShouldTaunt, ShouldTaunt());
	BlackboardComp->SetValueAsInt(BB_NearbyAllies, NearbyAlliesCount);
	BlackboardComp->SetValueAsFloat(BB_DistanceToTarget, GetDistanceToTarget());
}
