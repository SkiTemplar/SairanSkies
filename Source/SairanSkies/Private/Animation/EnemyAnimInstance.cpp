// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/EnemyAnimInstance.h"
#include "Enemies/EnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UEnemyAnimInstance::UEnemyAnimInstance()
{
	Speed = 0.0f;
	MovementDirection = 0.0f;
	bIsMoving = false;
	bIsInAir = false;

	CurrentState = EEnemyState::Idle;
	PreviousState = EEnemyState::Idle;
	LastFrameState = EEnemyState::Idle;
	bIsDead = false;
	bIsInCombat = false;
	bIsAlerted = false;
	SuspicionLevel = 0.0f;

	LookAtYaw = 0.0f;
	LookAtPitch = 0.0f;
	TargetLookAtYaw = 0.0f;
	TargetLookAtPitch = 0.0f;
	LookAtAlpha = 0.0f;

	RootYawOffset = 0.0f;
	bIsTurningInPlace = false;
	bTurnLeft = false;

	bIsAttacking = false;
	bIsTaunting = false;
	bIsTakingHit = false;
	bIsInIdlePause = false;

	OwnerEnemy = nullptr;
}

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache reference to owner enemy
	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		OwnerEnemy = Cast<AEnemyBase>(PawnOwner);
	}
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerEnemy)
	{
		APawn* PawnOwner = TryGetPawnOwner();
		if (PawnOwner)
		{
			OwnerEnemy = Cast<AEnemyBase>(PawnOwner);
		}
		
		if (!OwnerEnemy)
		{
			return;
		}
	}

	UpdateMovementValues(DeltaSeconds);
	UpdateStateValues();
	UpdateLookAt(DeltaSeconds);
	UpdateTurnInPlace(DeltaSeconds);
}

void UEnemyAnimInstance::UpdateMovementValues(float DeltaSeconds)
{
	if (!OwnerEnemy)
	{
		return;
	}

	// Get velocity
	FVector Velocity = OwnerEnemy->GetVelocity();
	Speed = Velocity.Size2D();
	bIsMoving = Speed > MovingSpeedThreshold;

	// Calculate movement direction
	MovementDirection = CalculateMovementDirection();

	// Check if in air
	UCharacterMovementComponent* MovementComp = OwnerEnemy->GetCharacterMovement();
	if (MovementComp)
	{
		bIsInAir = MovementComp->IsFalling();
	}
}

void UEnemyAnimInstance::UpdateStateValues()
{
	if (!OwnerEnemy)
	{
		return;
	}

	// Store previous state for transition detection
	PreviousState = LastFrameState;
	LastFrameState = CurrentState;

	// Get current state
	CurrentState = OwnerEnemy->GetEnemyState();
	bIsDead = OwnerEnemy->IsDead();
	bIsInCombat = OwnerEnemy->IsInCombat();
	bIsAlerted = OwnerEnemy->IsAlerted();
	SuspicionLevel = OwnerEnemy->GetSuspicionLevel();
	bIsInIdlePause = OwnerEnemy->IsInRandomPause();

	// Determine if attacking/taunting based on state
	bIsAttacking = (CurrentState == EEnemyState::Attacking);
	bIsTaunting = (CurrentState == EEnemyState::Taunting);
}

void UEnemyAnimInstance::UpdateLookAt(float DeltaSeconds)
{
	// Smoothly interpolate look at values
	LookAtYaw = FMath::FInterpTo(LookAtYaw, TargetLookAtYaw, DeltaSeconds, LookAtInterpSpeed);
	LookAtPitch = FMath::FInterpTo(LookAtPitch, TargetLookAtPitch, DeltaSeconds, LookAtInterpSpeed);

	// Fade out look at when moving fast or in certain states
	float TargetAlpha = 1.0f;
	
	if (bIsMoving && Speed > 200.0f)
	{
		// Reduce look at when moving fast
		TargetAlpha = FMath::GetMappedRangeValueClamped(FVector2D(200.0f, 400.0f), FVector2D(1.0f, 0.3f), Speed);
	}
	
	if (bIsAttacking || bIsDead)
	{
		TargetAlpha = 0.0f;
	}

	LookAtAlpha = FMath::FInterpTo(LookAtAlpha, TargetAlpha, DeltaSeconds, 5.0f);
}

void UEnemyAnimInstance::UpdateTurnInPlace(float DeltaSeconds)
{
	if (!OwnerEnemy || bIsMoving || bIsDead)
	{
		RootYawOffset = 0.0f;
		bIsTurningInPlace = false;
		return;
	}

	// Calculate desired rotation vs current mesh rotation
	// This would typically be set by the AI when it wants to face a direction
	// For now, we just track if we need to turn

	// Only trigger turn in place when stationary and angle exceeds threshold
	if (!bIsMoving && FMath::Abs(RootYawOffset) > TurnInPlaceThreshold && !bIsTurningInPlace)
	{
		bIsTurningInPlace = true;
		bTurnLeft = RootYawOffset < 0.0f;
	}
}

float UEnemyAnimInstance::CalculateMovementDirection() const
{
	if (!OwnerEnemy || !bIsMoving)
	{
		return 0.0f;
	}

	FVector Velocity = OwnerEnemy->GetVelocity();
	FRotator ActorRotation = OwnerEnemy->GetActorRotation();

	// Calculate angle between velocity and forward vector
	FVector Forward = ActorRotation.Vector();
	FVector VelocityNormalized = Velocity.GetSafeNormal2D();

	float DotProduct = FVector::DotProduct(Forward, VelocityNormalized);
	float CrossProduct = FVector::CrossProduct(Forward, VelocityNormalized).Z;

	float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
	
	if (CrossProduct < 0.0f)
	{
		Angle = -Angle;
	}

	return Angle;
}

void UEnemyAnimInstance::SetLookAtTarget(FVector WorldLocation)
{
	if (!OwnerEnemy)
	{
		return;
	}

	// Calculate rotation from enemy to target
	FVector EnemyLocation = OwnerEnemy->GetActorLocation();
	FVector EnemyEyeLocation = EnemyLocation + FVector(0.0f, 0.0f, 80.0f); // Approximate eye height
	
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(EnemyEyeLocation, WorldLocation);
	FRotator ActorRotation = OwnerEnemy->GetActorRotation();

	// Calculate relative rotation
	float DeltaYaw = FMath::FindDeltaAngleDegrees(ActorRotation.Yaw, LookAtRotation.Yaw);
	float DeltaPitch = LookAtRotation.Pitch;

	// Clamp to maximum values
	TargetLookAtYaw = FMath::Clamp(DeltaYaw, -MaxLookAtYaw, MaxLookAtYaw);
	TargetLookAtPitch = FMath::Clamp(DeltaPitch, -MaxLookAtPitch, MaxLookAtPitch);
}

void UEnemyAnimInstance::SetLookAtRotation(float Yaw, float Pitch)
{
	TargetLookAtYaw = FMath::Clamp(Yaw, -MaxLookAtYaw, MaxLookAtYaw);
	TargetLookAtPitch = FMath::Clamp(Pitch, -MaxLookAtPitch, MaxLookAtPitch);
}

void UEnemyAnimInstance::ClearLookAt()
{
	TargetLookAtYaw = 0.0f;
	TargetLookAtPitch = 0.0f;
}

void UEnemyAnimInstance::PlayActionMontage(UAnimMontage* Montage, float PlayRate)
{
	if (Montage)
	{
		Montage_Play(Montage, PlayRate);
	}
}

void UEnemyAnimInstance::OnTurnInPlaceFinished()
{
	bIsTurningInPlace = false;
	RootYawOffset = 0.0f;
}

