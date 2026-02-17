// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Navigation/PatrolPath.h"
#include "Animation/EnemyAnimInstance.h"

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
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"

// Blackboard Keys
const FName AEnemyBase::BB_TargetActor = TEXT("TargetActor");
const FName AEnemyBase::BB_TargetLocation = TEXT("TargetLocation");
const FName AEnemyBase::BB_EnemyState = TEXT("EnemyState");
const FName AEnemyBase::BB_CanSeeTarget = TEXT("CanSeeTarget");
const FName AEnemyBase::BB_PatrolIndex = TEXT("PatrolIndex");
const FName AEnemyBase::BB_ShouldTaunt = TEXT("ShouldTaunt");
const FName AEnemyBase::BB_NearbyAllies = TEXT("NearbyAllies");
const FName AEnemyBase::BB_DistanceToTarget = TEXT("DistanceToTarget");
const FName AEnemyBase::BB_SuspicionLevel = TEXT("SuspicionLevel");
const FName AEnemyBase::BB_IsAlerted = TEXT("IsAlerted");
const FName AEnemyBase::BB_IsInPause = TEXT("IsInPause");
const FName AEnemyBase::BB_IsConversing = TEXT("IsConversing");
const FName AEnemyBase::BB_ConversationPartner = TEXT("ConversationPartner");

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

	// Initialize suspicion system
	CurrentSuspicionLevel = 0.0f;
	bIsAlerted = false;
	ReactionTimer = 0.0f;
	bIsProcessingDetection = false;
	SuspiciousActor = nullptr;

	// Initialize idle behavior
	bIsInRandomPause = false;
	RandomPauseTimer = 0.0f;
	RandomPauseDuration = 0.0f;
	bIsLookingAround = false;
	LookAroundTimer = 0.0f;
	CurrentPatrolSpeedModifier = 1.0f;

	// Initialize conversation system
	bIsConversing = false;
	ConversationPartner = nullptr;
	ConversationTimer = 0.0f;
	ConversationDuration = 0.0f;
	GestureTimer = 0.0f;
	ConversationCooldownTimer = 0.0f;
	TimeStandingStill = 0.0f;
	LastPosition = FVector::ZeroVector;
	bIsInitiator = false;
	LastVoiceTime = 0.0f;
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

	// Initialize position tracking for conversation
	LastPosition = GetActorLocation();

	// Note: Perception delegate is now handled by EnemyAIController
	// The controller calls OnPerceptionUpdated when targets are detected

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

	// Update suspicion system (AAA-style gradual awareness)
	UpdateSuspicionSystem(DeltaTime);

	// Update idle behaviors (random pauses, looking around)
	UpdateIdleBehavior(DeltaTime);

	// Update conversation system
	if (bIsConversing)
	{
		UpdateConversation(DeltaTime);
	}

	// Update conversation cooldown
	if (ConversationCooldownTimer > 0.0f)
	{
		ConversationCooldownTimer -= DeltaTime;
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

	UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: %s detected %s (Sensed: %s)"), 
		*GetName(), *Actor->GetName(), Stimulus.WasSuccessfullySensed() ? TEXT("YES") : TEXT("NO"));

	// Check if this is the player
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (Actor != PlayerPawn)
	{
		UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: %s is not the player, ignoring"), *Actor->GetName());
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPerceptionUpdated: %s DETECTED PLAYER!"), *GetName());
		
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
	
	// Only update TargetLocation if we have a target (don't overwrite patrol points!)
	if (CurrentTarget)
	{
		BlackboardComp->SetValueAsVector(BB_TargetLocation, LastKnownTargetLocation);
	}
	
	BlackboardComp->SetValueAsInt(BB_EnemyState, static_cast<int32>(CurrentState));
	BlackboardComp->SetValueAsBool(BB_CanSeeTarget, CanSeeTarget());
	BlackboardComp->SetValueAsBool(BB_ShouldTaunt, ShouldTaunt());
	BlackboardComp->SetValueAsInt(BB_NearbyAllies, NearbyAlliesCount);
	BlackboardComp->SetValueAsFloat(BB_DistanceToTarget, GetDistanceToTarget());
	
	// New AAA-style awareness keys
	BlackboardComp->SetValueAsFloat(BB_SuspicionLevel, CurrentSuspicionLevel);
	BlackboardComp->SetValueAsBool(BB_IsAlerted, bIsAlerted);
	BlackboardComp->SetValueAsBool(BB_IsInPause, bIsInRandomPause);
	
	// Conversation keys
	BlackboardComp->SetValueAsBool(BB_IsConversing, bIsConversing);
	BlackboardComp->SetValueAsObject(BB_ConversationPartner, ConversationPartner);
}

// ==================== SUSPICION SYSTEM ====================

void AEnemyBase::AddSuspicion(float Amount, AActor* Source)
{
	float OldLevel = CurrentSuspicionLevel;
	CurrentSuspicionLevel = FMath::Clamp(CurrentSuspicionLevel + Amount, 0.0f, 1.0f);
	SuspiciousActor = Source;

	// Fire event if significant change
	if (FMath::Abs(CurrentSuspicionLevel - OldLevel) > 0.1f)
	{
		OnSuspicionChanged(CurrentSuspicionLevel, OldLevel);
	}

	// Check thresholds
	if (CurrentSuspicionLevel >= BehaviorConfig.SuspicionThresholdChase && !bIsProcessingDetection)
	{
		// Start reaction timer before fully engaging
		bIsProcessingDetection = true;
		ReactionTimer = FMath::RandRange(BehaviorConfig.ReactionTimeMin, BehaviorConfig.ReactionTimeMax);
	}
	else if (CurrentSuspicionLevel >= BehaviorConfig.SuspicionThresholdInvestigate && !bIsAlerted)
	{
		bIsAlerted = true;
	}
}

void AEnemyBase::SetFullAlert(AActor* Source)
{
	CurrentSuspicionLevel = 1.0f;
	SuspiciousActor = Source;
	bIsAlerted = true;
	bIsProcessingDetection = false;
	
	// Immediate reaction (no delay for direct alert)
	SetTarget(Source, EEnemySenseType::Alert);
}

void AEnemyBase::UpdateSuspicionSystem(float DeltaTime)
{
	// Process detection reaction
	if (bIsProcessingDetection)
	{
		ReactionTimer -= DeltaTime;
		if (ReactionTimer <= 0.0f)
		{
			ProcessDetectionReaction();
		}
		return;
	}

	// Decay suspicion when not seeing anything suspicious
	if (!CanSeeTarget() && CurrentSuspicionLevel > 0.0f)
	{
		float OldLevel = CurrentSuspicionLevel;
		CurrentSuspicionLevel = FMath::Max(0.0f, CurrentSuspicionLevel - BehaviorConfig.SuspicionDecayRate * DeltaTime);
		
		// Reset alert state if suspicion drops low enough
		if (CurrentSuspicionLevel < BehaviorConfig.SuspicionThresholdInvestigate * 0.5f)
		{
			bIsAlerted = false;
		}

		if (CurrentSuspicionLevel <= 0.0f && OldLevel > 0.0f)
		{
			OnSuspicionChanged(0.0f, OldLevel);
		}
	}
	// Build suspicion when seeing the target
	else if (CanSeeTarget() && CurrentTarget)
	{
		AddSuspicion(BehaviorConfig.SuspicionBuildUpRate * DeltaTime, CurrentTarget);
	}
}

void AEnemyBase::ProcessDetectionReaction()
{
	bIsProcessingDetection = false;

	if (SuspiciousActor && CurrentSuspicionLevel >= BehaviorConfig.SuspicionThresholdChase)
	{
		// Full detection - start chasing
		SetTarget(SuspiciousActor, EEnemySenseType::Sight);
		SetEnemyState(EEnemyState::Chasing);
	}
	else if (CurrentSuspicionLevel >= BehaviorConfig.SuspicionThresholdInvestigate)
	{
		// Partial detection - investigate
		if (SuspiciousActor)
		{
			LastKnownTargetLocation = SuspiciousActor->GetActorLocation();
		}
		SetEnemyState(EEnemyState::Investigating);
	}
}

// ==================== IDLE/NATURAL BEHAVIOR ====================

void AEnemyBase::UpdateIdleBehavior(float DeltaTime)
{
	// Only update idle behaviors during patrol/idle states
	if (CurrentState != EEnemyState::Patrolling && CurrentState != EEnemyState::Idle)
	{
		// Reset idle state when not patrolling
		if (bIsInRandomPause)
		{
			bIsInRandomPause = false;
			OnRandomPauseEnded();
		}
		bIsLookingAround = false;
		return;
	}

	// Handle random pause
	if (bIsInRandomPause)
	{
		RandomPauseTimer -= DeltaTime;
		
		// Look around during pause
		UpdateLookAround(DeltaTime);
		
		if (RandomPauseTimer <= 0.0f)
		{
			bIsInRandomPause = false;
			bIsLookingAround = false;
			OnRandomPauseEnded();
		}
	}

	// Handle looking around even when not paused (at patrol points)
	if (bIsLookingAround && !bIsInRandomPause)
	{
		UpdateLookAround(DeltaTime);
	}
}

void AEnemyBase::UpdateLookAround(float DeltaTime)
{
	if (!bIsLookingAround)
	{
		return;
	}

	LookAroundTimer -= DeltaTime;

	// Smoothly rotate towards target rotation
	FRotator CurrentRotation = GetActorRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, LookAroundTargetRotation, DeltaTime, 2.0f);
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));

	// Pick new random direction periodically
	if (LookAroundTimer <= 0.0f)
	{
		if (FMath::RandRange(0.0f, 1.0f) < 0.5f)
		{
			// Continue looking around
			float RandomYaw = FMath::RandRange(-BehaviorConfig.MaxLookAroundAngle, BehaviorConfig.MaxLookAroundAngle);
			LookAroundTargetRotation = GetActorRotation();
			LookAroundTargetRotation.Yaw += RandomYaw;
			LookAroundTimer = FMath::RandRange(1.0f, 2.5f);
		}
		else
		{
			// Stop looking around
			bIsLookingAround = false;
		}
	}
}

void AEnemyBase::StartRandomPause()
{
	if (bIsInRandomPause || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	bIsInRandomPause = true;
	RandomPauseDuration = FMath::RandRange(BehaviorConfig.MinRandomPauseDuration, BehaviorConfig.MaxRandomPauseDuration);
	RandomPauseTimer = RandomPauseDuration;

	// Maybe start looking around
	if (FMath::RandRange(0.0f, 1.0f) < BehaviorConfig.ChanceToLookAround)
	{
		StartLookingAround();
	}

	OnRandomPauseStarted();
}

void AEnemyBase::StartLookingAround()
{
	if (bIsLookingAround)
	{
		return;
	}

	bIsLookingAround = true;
	LookAroundTimer = FMath::RandRange(1.0f, 2.0f);
	
	// Pick initial random direction
	float RandomYaw = FMath::RandRange(-BehaviorConfig.MaxLookAroundAngle, BehaviorConfig.MaxLookAroundAngle);
	LookAroundTargetRotation = GetActorRotation();
	LookAroundTargetRotation.Yaw += RandomYaw;

	OnLookAroundStarted();
}

bool AEnemyBase::ShouldDoRandomPause() const
{
	if (CurrentState != EEnemyState::Patrolling || bIsInRandomPause || bIsAlerted)
	{
		return false;
	}

	return FMath::RandRange(0.0f, 1.0f) < BehaviorConfig.ChanceToStopDuringPatrol;
}

float AEnemyBase::GetRandomizedPatrolSpeed() const
{
	float BaseSpeed = PatrolConfig.PatrolSpeedMultiplier;
	float Variation = BehaviorConfig.PatrolSpeedVariation;
	float RandomModifier = FMath::RandRange(1.0f - Variation, 1.0f + Variation);
	return BaseSpeed * RandomModifier;
}

// ==================== ANIMATION ====================

UEnemyAnimInstance* AEnemyBase::GetEnemyAnimInstance() const
{
	if (GetMesh())
	{
		return Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	}
	return nullptr;
}

void AEnemyBase::PlayAttackMontage(UAnimMontage* Montage, float PlayRate)
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->PlayActionMontage(Montage, PlayRate);
	}
}

void AEnemyBase::PlayTauntMontage(UAnimMontage* Montage, float PlayRate)
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->PlayActionMontage(Montage, PlayRate);
	}
}

void AEnemyBase::PlayHitReactionMontage(UAnimMontage* Montage, float PlayRate)
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->PlayActionMontage(Montage, PlayRate);
	}
}

void AEnemyBase::SetAnimationLookAtTarget(FVector WorldLocation)
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->SetLookAtTarget(WorldLocation);
	}
}

void AEnemyBase::SetAnimationLookAtRotation(float Yaw, float Pitch)
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->SetLookAtRotation(Yaw, Pitch);
	}
}

void AEnemyBase::ClearAnimationLookAt()
{
	UEnemyAnimInstance* AnimInstance = GetEnemyAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->ClearLookAt();
	}
}

// ==================== CONVERSATION SYSTEM ====================

bool AEnemyBase::TryStartConversation(AEnemyBase* OtherEnemy)
{
	if (!OtherEnemy || OtherEnemy == this)
	{
		return false;
	}

	if (!CanStartConversation() || !OtherEnemy->CanStartConversation())
	{
		return false;
	}

	// Start the conversation
	bIsConversing = true;
	bIsInitiator = true;
	ConversationPartner = OtherEnemy;
	ConversationDuration = FMath::RandRange(
		ConversationConfig.MinConversationDuration,
		ConversationConfig.MaxConversationDuration
	);
	ConversationTimer = 0.0f;
	GestureTimer = 0.0f;

	// Make the other enemy join
	OtherEnemy->JoinConversation(this);

	// Set state
	SetEnemyState(EEnemyState::Conversing);

	// Play start animation
	if (AnimationConfig.ConversationStartMontage)
	{
		PlayAttackMontage(AnimationConfig.ConversationStartMontage, 1.0f);
	}

	// Look at partner
	if (ConversationConfig.bLookAtPartner)
	{
		SetAnimationLookAtTarget(OtherEnemy->GetActorLocation() + FVector(0, 0, 50));
	}

	// Broadcast event
	OnConversationStarted.Broadcast(OtherEnemy);
	OnConversationStartedEvent(OtherEnemy);

	UE_LOG(LogTemp, Log, TEXT("Conversation: %s started conversation with %s for %.1f seconds"), 
		*GetName(), *OtherEnemy->GetName(), ConversationDuration);

	return true;
}

void AEnemyBase::JoinConversation(AEnemyBase* Initiator)
{
	if (!Initiator)
	{
		return;
	}

	bIsConversing = true;
	bIsInitiator = false;
	ConversationPartner = Initiator;
	ConversationDuration = Initiator->ConversationDuration;
	ConversationTimer = 0.0f;
	GestureTimer = ConversationConfig.GestureInterval * 0.5f; // Offset gestures

	// Set state
	SetEnemyState(EEnemyState::Conversing);

	// Look at partner
	if (ConversationConfig.bLookAtPartner)
	{
		SetAnimationLookAtTarget(Initiator->GetActorLocation() + FVector(0, 0, 50));
	}

	// Broadcast event
	OnConversationStarted.Broadcast(Initiator);
	OnConversationStartedEvent(Initiator);
}

void AEnemyBase::EndConversation()
{
	if (!bIsConversing)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Conversation: %s ended conversation"), *GetName());

	// Play end animation
	if (AnimationConfig.ConversationEndMontage)
	{
		PlayAttackMontage(AnimationConfig.ConversationEndMontage, 1.0f);
	}

	// Clear look at
	ClearAnimationLookAt();

	// If we're the initiator, also end the partner's conversation
	if (bIsInitiator && ConversationPartner && ConversationPartner->IsConversing())
	{
		ConversationPartner->EndConversation();
	}

	// Reset state
	bIsConversing = false;
	bIsInitiator = false;
	ConversationPartner = nullptr;
	ConversationTimer = 0.0f;
	ConversationCooldownTimer = ConversationConfig.ConversationCooldown;
	TimeStandingStill = 0.0f;

	// Return to patrol
	if (PatrolPath)
	{
		SetEnemyState(EEnemyState::Patrolling);
	}
	else
	{
		SetEnemyState(EEnemyState::Idle);
	}

	// Broadcast event
	OnConversationEnded.Broadcast();
	OnConversationEndedEvent();
}

bool AEnemyBase::CanStartConversation() const
{
	// Can't converse if dead, in combat, or already conversing
	if (CurrentState == EEnemyState::Dead ||
		CurrentState == EEnemyState::Chasing ||
		CurrentState == EEnemyState::Attacking ||
		CurrentState == EEnemyState::Conversing ||
		bIsConversing)
	{
		return false;
	}

	// Can't converse if on cooldown
	if (ConversationCooldownTimer > 0.0f)
	{
		return false;
	}

	// Can't converse if has a target
	if (CurrentTarget != nullptr)
	{
		return false;
	}

	return true;
}

AEnemyBase* AEnemyBase::FindNearbyEnemyForConversation() const
{
	if (!CanStartConversation())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AEnemyBase* OtherEnemy = Cast<AEnemyBase>(Actor);
		if (!OtherEnemy || OtherEnemy == this)
		{
			continue;
		}

		// Check distance
		float Distance = FVector::Dist(GetActorLocation(), OtherEnemy->GetActorLocation());
		if (Distance > ConversationConfig.SamePointRadius)
		{
			continue;
		}

		// Check if the other enemy can converse
		if (!OtherEnemy->CanStartConversation())
		{
			continue;
		}

		// Check if the other enemy is also standing still
		if (OtherEnemy->GetVelocity().SizeSquared() > 100.0f)
		{
			continue;
		}

		return OtherEnemy;
	}

	return nullptr;
}

void AEnemyBase::UpdateConversation(float DeltaTime)
{
	if (!bIsConversing)
	{
		return;
	}

	ConversationTimer += DeltaTime;
	GestureTimer += DeltaTime;

	// Check if conversation should be interrupted by player detection
	if (ConversationConfig.bCanBeInterrupted && CurrentTarget)
	{
		EndConversation();
		return;
	}

	// Check if partner is still valid
	if (!ConversationPartner || !ConversationPartner->IsConversing())
	{
		EndConversation();
		return;
	}

	// Keep looking at partner
	if (ConversationConfig.bLookAtPartner && ConversationPartner)
	{
		SetAnimationLookAtTarget(ConversationPartner->GetActorLocation() + FVector(0, 0, 50));
	}

	// Perform periodic gestures (only initiator controls the timing for sync)
	if (GestureTimer >= ConversationConfig.GestureInterval)
	{
		GestureTimer = 0.0f;
		
		if (FMath::RandRange(0.0f, 1.0f) < ConversationConfig.ChanceToGesture)
		{
			PerformConversationGesture();
		}

		// Play voice line
		PlayConversationVoice();
	}

	// End conversation when time is up (only initiator ends it)
	if (bIsInitiator && ConversationTimer >= ConversationDuration)
	{
		EndConversation();
	}
}

void AEnemyBase::PerformConversationGesture()
{
	// Play random gesture animation
	if (AnimationConfig.ConversationGestureMontages.Num() > 0)
	{
		int32 Index = FMath::RandRange(0, AnimationConfig.ConversationGestureMontages.Num() - 1);
		UAnimMontage* Gesture = AnimationConfig.ConversationGestureMontages[Index];
		if (Gesture)
		{
			PlayAttackMontage(Gesture, 1.0f);
		}
	}

	OnConversationGesture();
}

void AEnemyBase::PlayConversationVoice()
{
	// Check cooldown
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastVoiceTime < SoundConfig.MinTimeBetweenVoices)
	{
		return;
	}

	// Decide what sound to play
	float Roll = FMath::RandRange(0.0f, 1.0f);
	
	if (Roll < ConversationConfig.ChanceToLaugh && SoundConfig.LaughSounds.Num() > 0)
	{
		PlayRandomSound(SoundConfig.LaughSounds, SocketConfig.VoiceSocket);
	}
	else if (SoundConfig.ConversationVoices.Num() > 0)
	{
		PlayRandomSound(SoundConfig.ConversationVoices, SocketConfig.VoiceSocket);
	}

	LastVoiceTime = CurrentTime;
}

// ==================== SOUND/VFX HELPERS ====================

void AEnemyBase::PlayRandomSound(const TArray<USoundBase*>& Sounds, FName SocketName)
{
	if (Sounds.Num() == 0)
	{
		return;
	}

	int32 Index = FMath::RandRange(0, Sounds.Num() - 1);
	USoundBase* Sound = Sounds[Index];
	
	if (Sound)
	{
		if (SocketName != NAME_None && GetMesh())
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				Sound,
				GetMesh()->GetSocketLocation(SocketName),
				SoundConfig.SFXVolume
			);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				Sound,
				GetActorLocation(),
				SoundConfig.SFXVolume
			);
		}
	}
}

void AEnemyBase::PlayVoice(USoundBase* Sound)
{
	if (!Sound)
	{
		return;
	}

	// Check cooldown
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastVoiceTime < SoundConfig.MinTimeBetweenVoices)
	{
		return;
	}

	FVector Location = GetActorLocation();
	if (GetMesh() && SocketConfig.VoiceSocket != NAME_None)
	{
		Location = GetMesh()->GetSocketLocation(SocketConfig.VoiceSocket);
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		Location,
		SoundConfig.VoiceVolume
	);

	LastVoiceTime = CurrentTime;
}

void AEnemyBase::SpawnEffect(UNiagaraSystem* Effect, FName SocketName, FVector Offset)
{
	if (!Effect)
	{
		return;
	}

	FVector Location = GetActorLocation() + Offset;
	FRotator Rotation = GetActorRotation();

	if (SocketName != NAME_None && GetMesh())
	{
		Location = GetMesh()->GetSocketLocation(SocketName) + Offset;
		Rotation = GetMesh()->GetSocketRotation(SocketName);
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		Effect,
		Location,
		Rotation
	);
}

void AEnemyBase::SpawnEffectAtLocation(UNiagaraSystem* Effect, FVector Location, FRotator Rotation)
{
	if (!Effect)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		Effect,
		Location,
		Rotation
	);
}

