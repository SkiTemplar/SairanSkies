// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "Navigation/PatrolPath.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/DamageNumberComponent.h"
#include "Components/CapsuleComponent.h"

// Blackboard Keys
const FName AEnemyBase::BB_TargetActor = TEXT("TargetActor");
const FName AEnemyBase::BB_TargetLocation = TEXT("TargetLocation");
const FName AEnemyBase::BB_EnemyState = TEXT("EnemyState");
const FName AEnemyBase::BB_CanSeeTarget = TEXT("CanSeeTarget");
const FName AEnemyBase::BB_PatrolIndex = TEXT("PatrolIndex");
const FName AEnemyBase::BB_DistanceToTarget = TEXT("DistanceToTarget");
const FName AEnemyBase::BB_CanAttack = TEXT("CanAttack");
const FName AEnemyBase::BB_IsInPause = TEXT("IsInPause");
const FName AEnemyBase::BB_IsConversing = TEXT("IsConversing");

// Static attacker tracking
TArray<AEnemyBase*> AEnemyBase::ActiveAttackers;

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// UPROPERTY defaults
	BehaviorTree = nullptr;
	PatrolPath = nullptr;

	CurrentState = EEnemyState::Idle;
	CurrentTarget = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	CurrentHealth = MaxHealth;
	bCanAttack = true;
	TimeSinceLastSawTarget = 0.0f;
	AttackCooldownTimer = 0.0f;
	BaseMaxWalkSpeed = 600.0f;

	// Damage numbers component
	DamageNumberComponent = CreateDefaultSubobject<UDamageNumberComponent>(TEXT("DamageNumberComponent"));

	// Enemies should NOT push the player's camera - ignore Camera trace channel
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	
	// Strafe
	bIsStrafing = false;
	StrafeDirection = 1.0f;
	StrafeTimer = 0.0f;
	bIsActiveAttacker = false;
	
	// Natural behavior
	bIsInRandomPause = false;
	RandomPauseTimer = 0.0f;
	RandomPauseDuration = 0.0f;
	bIsLookingAround = false;
	LookAroundTimer = 0.0f;
	OriginalRotation = FRotator::ZeroRotator;
	TargetLookRotation = FRotator::ZeroRotator;
	
	// Conversation
	ConversationPartner = nullptr;
	ConversationTimer = 0.0f;
	ConversationDuration = 0.0f;
	GestureTimer = 0.0f;
	ConversationCooldownTimer = 0.0f;
	TimeWaitingAtPoint = 0.0f;
	bIsConversationInitiator = false;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (GetCharacterMovement())
	{
		BaseMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

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

	// Update cooldowns
	if (!bCanAttack)
	{
		AttackCooldownTimer -= DeltaTime;
		if (AttackCooldownTimer <= 0.0f)
		{
			bCanAttack = true;
		}
	}

	if (ConversationCooldownTimer > 0.0f)
	{
		ConversationCooldownTimer -= DeltaTime;
	}

	// Update strafe
	if (bIsStrafing)
	{
		UpdateStrafe(DeltaTime);
	}

	// Update natural behaviors
	if (bIsInRandomPause)
	{
		UpdateRandomPause(DeltaTime);
	}

	if (bIsLookingAround)
	{
		UpdateLookAround(DeltaTime);
	}

	// Update conversation
	if (CurrentState == EEnemyState::Conversing)
	{
		UpdateConversation(DeltaTime);
	}

	// Track time waiting at point for conversation trigger
	if (CurrentState == EEnemyState::Idle || bIsInRandomPause)
	{
		TimeWaitingAtPoint += DeltaTime;
		
		// Try to start conversation if waiting long enough
		if (TimeWaitingAtPoint >= ConversationConfig.TimeBeforeConversation && CanStartConversation())
		{
			AEnemyBase* Partner = FindNearbyEnemyForConversation();
			if (Partner)
			{
				TryStartConversation(Partner);
			}
		}
	}
	else if (CurrentState != EEnemyState::Conversing)
	{
		TimeWaitingAtPoint = 0.0f;
	}

	// Handle target tracking
	if (CurrentTarget && IsInCombat())
	{
		if (CanSeeTarget())
		{
			TimeSinceLastSawTarget = 0.0f;
			LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		}
		else
		{
			TimeSinceLastSawTarget += DeltaTime;
			if (TimeSinceLastSawTarget >= PerceptionConfig.LoseSightTime)
			{
				LoseTarget();
			}
		}
	}
}

// ==================== STATE MANAGEMENT ====================

void AEnemyBase::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	
	OnStateExit(OldState);
	CurrentState = NewState;
	OnStateEnter(NewState);

	OnEnemyStateChanged.Broadcast(OldState, NewState);

	// Update movement speed based on state
	switch (NewState)
	{
	case EEnemyState::Patrolling:
	case EEnemyState::Investigating:
		SetPatrolSpeedWithVariation();
		break;
	case EEnemyState::Chasing:
	case EEnemyState::Attacking:
		SetChaseSpeed();
		break;
	case EEnemyState::Conversing:
	case EEnemyState::Idle:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		}
		break;
	default:
		break;
	}

	// Cleanup when leaving combat states
	if (OldState == EEnemyState::Attacking || OldState == EEnemyState::Chasing || OldState == EEnemyState::Positioning)
	{
		if (NewState != EEnemyState::Attacking && NewState != EEnemyState::Chasing && NewState != EEnemyState::Positioning)
		{
			UnregisterAsAttacker();
			StopStrafe();
		}
	}

	// Cleanup when leaving natural behavior states
	if (OldState == EEnemyState::Conversing)
	{
		EndConversation();
	}
}

bool AEnemyBase::IsInCombat() const
{
	return CurrentState == EEnemyState::Chasing ||
		   CurrentState == EEnemyState::Positioning ||
		   CurrentState == EEnemyState::Attacking;
}

bool AEnemyBase::CanSeeTarget() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	FHitResult HitResult;
	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = CurrentTarget->GetActorLocation() + FVector(0, 0, 50);
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
	
	if (!bHit)
	{
		return true;
	}
	
	return HitResult.GetActor() == CurrentTarget;
}

bool AEnemyBase::IsAlerted() const
{
	return CurrentTarget != nullptr || IsInCombat();
}

float AEnemyBase::GetSuspicionLevel() const
{
	if (!CurrentTarget)
	{
		return 0.0f;
	}

	float Distance = GetDistanceToTarget();
	float Radius = PerceptionConfig.SightRadius;
	
	// Suspicion increases with proximity
	float Suspicion = 1.0f - (Distance / Radius);
	return FMath::Clamp(Suspicion, 0.0f, 1.0f);
}

void AEnemyBase::OnStateEnter(EEnemyState NewState)
{
	// Override in subclasses
}

void AEnemyBase::OnStateExit(EEnemyState OldState)
{
	// Override in subclasses
}

// ==================== PERCEPTION ====================

void AEnemyBase::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (CurrentState == EEnemyState::Dead || !Actor)
	{
		return;
	}

	// Interrupt conversation if player detected
	if (CurrentState == EEnemyState::Conversing && Stimulus.WasSuccessfullySensed())
	{
		EndConversation();
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (Actor != PlayerPawn)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Log, TEXT("%s DETECTED PLAYER!"), *GetName());
		
		EEnemySenseType SenseType = EEnemySenseType::Sight;
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			SenseType = EEnemySenseType::Hearing;
		}

		SetTarget(Actor, SenseType);
	}
}

void AEnemyBase::LoseTarget()
{
	if (CurrentTarget)
	{
		UnregisterAsAttacker();
		StopStrafe();
		CurrentTarget = nullptr;
		OnPlayerLost.Broadcast();

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

	// End any natural behaviors
	EndRandomPause();
	StopLookAround();
	if (CurrentState == EEnemyState::Conversing)
	{
		EndConversation();
	}

	bool bNewTarget = (CurrentTarget != NewTarget);
	CurrentTarget = NewTarget;
	LastKnownTargetLocation = NewTarget->GetActorLocation();
	TimeSinceLastSawTarget = 0.0f;

	if (bNewTarget)
	{
		AlertNearbyAllies(NewTarget);
		OnPlayerDetected.Broadcast(NewTarget, SenseType);
		PlayRandomSound(SoundConfig.AlertSounds);
		SetEnemyState(EEnemyState::Chasing);
	}
}

// ==================== COMBAT ====================

void AEnemyBase::Attack()
{
	if (!CanAttackNow() || !CurrentTarget)
	{
		return;
	}

	bCanAttack = false;
	AttackCooldownTimer = CombatConfig.AttackCooldown;

	SetEnemyState(EEnemyState::Attacking);
	PlayRandomSound(SoundConfig.AttackSounds);

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

	PlayHitReaction();
	PlayRandomSound(SoundConfig.PainSounds);
	SpawnHitEffect(GetActorLocation());

	if (!CurrentTarget && DamageSource)
	{
		SetTarget(DamageSource, EEnemySenseType::Damage);
	}

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

	UnregisterAsAttacker();
	SetEnemyState(EEnemyState::Dead);
	CurrentHealth = 0.0f;

	PlayRandomSound(SoundConfig.DeathSounds);
	OnEnemyDeath.Broadcast(InstigatorController);

	// Show death X marker on damage numbers
	if (DamageNumberComponent)
	{
		DamageNumberComponent->ShowDeathMarker();
	}

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

bool AEnemyBase::CanAttack() const
{
	return bCanAttack && CurrentState != EEnemyState::Dead;
}

void AEnemyBase::PerformTaunt()
{
	// Base implementation - subclasses can override
	// This is called when the enemy wants to taunt
	SetEnemyState(EEnemyState::Taunting);
}

bool AEnemyBase::ShouldTaunt() const
{
	// Base implementation - subclasses can override
	// By default, enemies don't taunt
	return false;
}

bool AEnemyBase::HasEnoughAlliesForAggression() const
{
	return NearbyAlliesCount >= CombatConfig.MinAlliesForAggression;
}

void AEnemyBase::HandleCombatBehavior(float DeltaTime)
{
	// Base implementation - subclasses can override
	// This is called during combat to handle special behaviors
}

// ==================== ALLY COORDINATION ====================

void AEnemyBase::AlertNearbyAllies(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AEnemyBase* Ally = Cast<AEnemyBase>(Actor);
		if (Ally && Ally != this && !Ally->IsDead())
		{
			float Distance = FVector::Dist(GetActorLocation(), Ally->GetActorLocation());
			if (Distance <= CombatConfig.AllyDetectionRadius)
			{
				Ally->ReceiveAlertFromAlly(Target, this);
			}
		}
	}
}

void AEnemyBase::ReceiveAlertFromAlly(AActor* Target, AEnemyBase* AlertingAlly)
{
	if (!Target || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	if (!IsInCombat())
	{
		SetTarget(Target, EEnemySenseType::Alert);
	}
}

int32 AEnemyBase::GetAttackersCount() const
{
	return ActiveAttackers.Num();
}

bool AEnemyBase::CanJoinAttack() const
{
	return GetAttackersCount() < CombatConfig.MaxSimultaneousAttackers || bIsActiveAttacker;
}

void AEnemyBase::RegisterAsAttacker()
{
	if (!bIsActiveAttacker)
	{
		bIsActiveAttacker = true;
		ActiveAttackers.AddUnique(this);
		UE_LOG(LogTemp, Log, TEXT("%s registered as attacker (%d total)"), *GetName(), ActiveAttackers.Num());
	}
}

void AEnemyBase::UnregisterAsAttacker()
{
	if (bIsActiveAttacker)
	{
		bIsActiveAttacker = false;
		ActiveAttackers.Remove(this);
		UE_LOG(LogTemp, Log, TEXT("%s unregistered as attacker (%d total)"), *GetName(), ActiveAttackers.Num());
	}
}

// ==================== MOVEMENT ====================

void AEnemyBase::SetPatrolSpeed()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMaxWalkSpeed * PatrolConfig.PatrolSpeedMultiplier;
	}
}

void AEnemyBase::SetPatrolSpeedWithVariation()
{
	if (GetCharacterMovement())
	{
		float Variation = FMath::RandRange(-BehaviorConfig.PatrolSpeedVariation, BehaviorConfig.PatrolSpeedVariation);
		float FinalMultiplier = PatrolConfig.PatrolSpeedMultiplier * (1.0f + Variation);
		GetCharacterMovement()->MaxWalkSpeed = BaseMaxWalkSpeed * FinalMultiplier;
	}
}

void AEnemyBase::SetChaseSpeed()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMaxWalkSpeed * PatrolConfig.ChaseSpeedMultiplier;
	}
}

void AEnemyBase::SetMovementSpeed(float SpeedMultiplier)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMaxWalkSpeed * SpeedMultiplier;
	}
}

void AEnemyBase::StartStrafe(bool bStrafeRight)
{
	bIsStrafing = true;
	StrafeDirection = bStrafeRight ? 1.0f : -1.0f;
	StrafeTimer = BehaviorConfig.StrafeDuration;
}

void AEnemyBase::StopStrafe()
{
	bIsStrafing = false;
	StrafeTimer = 0.0f;
}

void AEnemyBase::UpdateStrafe(float DeltaTime)
{
	if (!bIsStrafing || !CurrentTarget)
	{
		return;
	}

	StrafeTimer -= DeltaTime;
	if (StrafeTimer <= 0.0f)
	{
		// Change strafe direction randomly
		StrafeDirection = FMath::RandBool() ? 1.0f : -1.0f;
		StrafeTimer = BehaviorConfig.StrafeDuration;
	}

	FVector ToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector StrafeDir = FVector::CrossProduct(ToTarget, FVector::UpVector) * StrafeDirection;
	
	AddMovementInput(StrafeDir, BehaviorConfig.StrafeSpeed * DeltaTime / 100.0f);

	FRotator LookAtRotation = ToTarget.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));
}

// ==================== NATURAL BEHAVIOR ====================

void AEnemyBase::StartRandomPause()
{
	if (bIsInRandomPause || IsInCombat())
	{
		return;
	}

	bIsInRandomPause = true;
	RandomPauseDuration = FMath::RandRange(BehaviorConfig.MinPauseDuration, BehaviorConfig.MaxPauseDuration);
	RandomPauseTimer = 0.0f;

	// Stop movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	}

	OnRandomPauseStarted();

	// Maybe look around during pause
	if (FMath::FRand() < BehaviorConfig.ChanceToLookAround)
	{
		StartLookAround();
	}
}

void AEnemyBase::EndRandomPause()
{
	if (!bIsInRandomPause)
	{
		return;
	}

	bIsInRandomPause = false;
	RandomPauseTimer = 0.0f;
	StopLookAround();

	SetPatrolSpeedWithVariation();
	OnRandomPauseEnded();
}

void AEnemyBase::UpdateRandomPause(float DeltaTime)
{
	RandomPauseTimer += DeltaTime;
	if (RandomPauseTimer >= RandomPauseDuration)
	{
		EndRandomPause();
	}
}

void AEnemyBase::StartLookAround()
{
	if (bIsLookingAround)
	{
		return;
	}

	bIsLookingAround = true;
	LookAroundTimer = 0.0f;
	OriginalRotation = GetActorRotation();
	
	float RandomYaw = FMath::RandRange(-BehaviorConfig.MaxLookAroundAngle, BehaviorConfig.MaxLookAroundAngle);
	TargetLookRotation = OriginalRotation;
	TargetLookRotation.Yaw += RandomYaw;

	OnLookAroundStarted();

	// Play look around montage if available
	if (AnimationConfig.LookAroundMontage)
	{
		PlayAnimMontage(AnimationConfig.LookAroundMontage);
	}
}

void AEnemyBase::StopLookAround()
{
	if (!bIsLookingAround)
	{
		return;
	}

	bIsLookingAround = false;
	LookAroundTimer = 0.0f;
}

void AEnemyBase::UpdateLookAround(float DeltaTime)
{
	LookAroundTimer += DeltaTime;

	float LookDuration = BehaviorConfig.MaxLookAroundAngle / BehaviorConfig.LookAroundSpeed;
	
	if (LookAroundTimer < LookDuration)
	{
		// Rotate towards target
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetLookRotation, DeltaTime, 2.0f);
		SetActorRotation(NewRotation);
	}
	else if (LookAroundTimer < LookDuration * 2.0f)
	{
		// Rotate back
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), OriginalRotation, DeltaTime, 2.0f);
		SetActorRotation(NewRotation);
	}
	else
	{
		// Maybe look in another direction
		if (FMath::FRand() < 0.5f && bIsInRandomPause)
		{
			float RandomYaw = FMath::RandRange(-BehaviorConfig.MaxLookAroundAngle, BehaviorConfig.MaxLookAroundAngle);
			TargetLookRotation = OriginalRotation;
			TargetLookRotation.Yaw += RandomYaw;
			LookAroundTimer = 0.0f;
		}
		else
		{
			StopLookAround();
		}
	}
}

bool AEnemyBase::ShouldRandomPause() const
{
	if (IsInCombat() || bIsInRandomPause || CurrentState == EEnemyState::Conversing)
	{
		return false;
	}
	return FMath::FRand() < BehaviorConfig.ChanceToPauseDuringPatrol;
}

// ==================== CONVERSATION SYSTEM ====================

bool AEnemyBase::CanStartConversation() const
{
	return !IsInCombat() && 
		   !IsConversing() && 
		   ConversationCooldownTimer <= 0.0f &&
		   CurrentState != EEnemyState::Dead;
}

AEnemyBase* AEnemyBase::FindNearbyEnemyForConversation() const
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AEnemyBase* OtherEnemy = Cast<AEnemyBase>(Actor);
		if (OtherEnemy && OtherEnemy != this && OtherEnemy->CanStartConversation())
		{
			float Distance = FVector::Dist(GetActorLocation(), OtherEnemy->GetActorLocation());
			if (Distance <= ConversationConfig.ConversationRadius)
			{
				// Check if they're also waiting
				if (OtherEnemy->TimeWaitingAtPoint >= ConversationConfig.TimeBeforeConversation)
				{
					return OtherEnemy;
				}
			}
		}
	}

	return nullptr;
}

bool AEnemyBase::TryStartConversation(AEnemyBase* OtherEnemy)
{
	if (!OtherEnemy || !CanStartConversation() || !OtherEnemy->CanStartConversation())
	{
		return false;
	}

	// Start conversation
	bIsConversationInitiator = true;
	ConversationPartner = OtherEnemy;
	ConversationDuration = FMath::RandRange(ConversationConfig.MinConversationDuration, ConversationConfig.MaxConversationDuration);
	ConversationTimer = 0.0f;
	GestureTimer = 0.0f;

	SetEnemyState(EEnemyState::Conversing);
	OtherEnemy->JoinConversation(this);

	OnConversationStarted.Broadcast(OtherEnemy);

	UE_LOG(LogTemp, Log, TEXT("%s started conversation with %s"), *GetName(), *OtherEnemy->GetName());

	return true;
}

void AEnemyBase::JoinConversation(AEnemyBase* Initiator)
{
	if (!Initiator)
	{
		return;
	}

	bIsConversationInitiator = false;
	ConversationPartner = Initiator;
	ConversationDuration = Initiator->ConversationDuration;
	ConversationTimer = 0.0f;
	GestureTimer = 0.0f;

	SetEnemyState(EEnemyState::Conversing);
	OnConversationStarted.Broadcast(Initiator);
}

void AEnemyBase::EndConversation()
{
	if (ConversationPartner && bIsConversationInitiator)
	{
		// End partner's conversation too
		if (ConversationPartner->IsConversing())
		{
			ConversationPartner->ConversationPartner = nullptr;
			ConversationPartner->ConversationCooldownTimer = ConversationConfig.ConversationCooldown;
			ConversationPartner->SetEnemyState(EEnemyState::Patrolling);
			ConversationPartner->OnConversationEnded.Broadcast();
		}
	}

	ConversationPartner = nullptr;
	ConversationCooldownTimer = ConversationConfig.ConversationCooldown;
	TimeWaitingAtPoint = 0.0f;

	OnConversationEnded.Broadcast();

	if (CurrentState == EEnemyState::Conversing)
	{
		SetEnemyState(EEnemyState::Patrolling);
	}
}

void AEnemyBase::UpdateConversation(float DeltaTime)
{
	ConversationTimer += DeltaTime;
	GestureTimer += DeltaTime;

	// Face conversation partner
	if (ConversationPartner)
	{
		FVector ToPartner = (ConversationPartner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FRotator LookAtRotation = ToPartner.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAtRotation, DeltaTime, 3.0f));
	}

	// Perform gestures periodically
	if (GestureTimer >= ConversationConfig.GestureInterval)
	{
		GestureTimer = 0.0f;
		if (FMath::FRand() < ConversationConfig.ChanceToGesture)
		{
			PerformConversationGesture();
		}
	}

	// End conversation when time is up
	if (ConversationTimer >= ConversationDuration)
	{
		if (bIsConversationInitiator)
		{
			EndConversation();
		}
	}
}

void AEnemyBase::PerformConversationGesture()
{
	PlayConversationGesture();
	
	// Random chance to play conversation sound or laugh
	if (FMath::FRand() < 0.5f)
	{
		PlayRandomSound(SoundConfig.ConversationSounds);
	}
	else if (FMath::FRand() < 0.3f)
	{
		PlayRandomSound(SoundConfig.LaughSounds);
	}

	OnConversationGesture();
}

// ==================== ANIMATION ====================

UAnimMontage* AEnemyBase::GetRandomAttackMontage()
{
	if (AnimationConfig.AttackMontages.Num() == 0)
	{
		return nullptr;
	}
	int32 Index = FMath::RandRange(0, AnimationConfig.AttackMontages.Num() - 1);
	return AnimationConfig.AttackMontages[Index];
}

void AEnemyBase::PlayHitReaction()
{
	if (AnimationConfig.HitReactionMontages.Num() == 0)
	{
		return;
	}
	int32 Index = FMath::RandRange(0, AnimationConfig.HitReactionMontages.Num() - 1);
	UAnimMontage* Montage = AnimationConfig.HitReactionMontages[Index];
	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

void AEnemyBase::PlayConversationGesture()
{
	if (AnimationConfig.ConversationGestures.Num() == 0)
	{
		return;
	}
	int32 Index = FMath::RandRange(0, AnimationConfig.ConversationGestures.Num() - 1);
	UAnimMontage* Montage = AnimationConfig.ConversationGestures[Index];
	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

// ==================== SOUND/VFX ====================

void AEnemyBase::PlayRandomSound(const TArray<USoundBase*>& Sounds)
{
	if (Sounds.Num() == 0)
	{
		return;
	}
	int32 Index = FMath::RandRange(0, Sounds.Num() - 1);
	USoundBase* Sound = Sounds[Index];
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), SoundConfig.Volume);
	}
}

void AEnemyBase::SpawnHitEffect(FVector Location)
{
	if (VFXConfig.HitEffect)
	{
		// Spawn attached to the enemy so particles follow them when knocked back
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			VFXConfig.HitEffect,
			GetMesh(),  // Attach to mesh
			NAME_None,  // No specific socket
			GetMesh()->GetComponentTransform().InverseTransformPosition(Location),  // Convert to local space
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true  // Auto destroy
		);
	}
}

void AEnemyBase::TakeDamageAtLocation(float DamageAmount, AActor* DamageSource, AController* InstigatorController, const FVector& HitWorldLocation)
{
	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	CurrentHealth -= DamageAmount;

	PlayHitReaction();
	PlayRandomSound(SoundConfig.PainSounds);
	
	// Spawn hit effect attached to enemy at the hit location
	SpawnHitEffect(HitWorldLocation);
	
	// Start the hit flash (Blasphemous-style visual feedback)
	StartHitFlash();

	// Update floating damage numbers
	if (DamageNumberComponent)
	{
		DamageNumberComponent->AddDamage(DamageAmount, GetHealthPercent());
	}

	if (!CurrentTarget && DamageSource)
	{
		SetTarget(DamageSource, EEnemySenseType::Damage);
	}

	if (CurrentHealth <= 0.0f)
	{
		Die(InstigatorController);
	}
}

// ==================== HIT FLASH SYSTEM ====================

void AEnemyBase::StartHitFlash()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// Cache original materials if not already done
	if (!bMaterialsCached)
	{
		OriginalMaterials.Empty();
		for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
		{
			OriginalMaterials.Add(MeshComp->GetMaterial(i));
		}
		bMaterialsCached = true;
	}

	// Create a simple emissive material for the flash
	// We'll override all materials with a solid color
	if (!FlashMaterialInstance)
	{
		// Create a simple material that shows a solid color
		UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BaseMaterial)
		{
			FlashMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (FlashMaterialInstance)
			{
				// Set emissive color for the flash
				FlashMaterialInstance->SetVectorParameterValue(FName("Color"), HitFlashColor);
			}
		}
	}

	// Apply flash material to all slots
	if (FlashMaterialInstance)
	{
		for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
		{
			MeshComp->SetMaterial(i, FlashMaterialInstance);
		}
	}

	// Clear any existing timer and set new one to stop the flash
	GetWorld()->GetTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(HitFlashTimerHandle, this, &AEnemyBase::StopHitFlash, HitFlashDuration, false);
}

void AEnemyBase::StopHitFlash()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !bMaterialsCached)
	{
		return;
	}

	// Restore original materials
	for (int32 i = 0; i < OriginalMaterials.Num() && i < MeshComp->GetNumMaterials(); i++)
	{
		if (OriginalMaterials[i])
		{
			MeshComp->SetMaterial(i, OriginalMaterials[i]);
		}
	}
}

