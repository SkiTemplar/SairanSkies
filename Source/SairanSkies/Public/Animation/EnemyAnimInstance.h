// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Enemies/EnemyTypes.h"
#include "EnemyAnimInstance.generated.h"

class AEnemyBase;

/**
 * Animation Instance base class for enemies.
 * Handles smooth transitions, look-at behavior, and state-driven animations.
 */
UCLASS()
class SAIRANSKIES_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UEnemyAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ==================== MOVEMENT ====================
	
	// Current movement speed (for blend spaces)
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed;

	// Movement direction relative to actor forward (-180 to 180)
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float MovementDirection;

	// Is the enemy currently moving?
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving;

	// Is the enemy in the air?
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;

	// ==================== STATE ====================

	// Current enemy state
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EEnemyState CurrentState;

	// Previous state (for transition logic)
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EEnemyState PreviousState;

	// Is enemy dead?
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;

	// Is enemy in combat (chasing, attacking, etc)?
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInCombat;

	// Is enemy alerted (suspicious)?
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAlerted;

	// Suspicion level (0-1) for blend between relaxed and alert poses
	UPROPERTY(BlueprintReadOnly, Category = "State")
	float SuspicionLevel;

	// ==================== LOOK AT / AIM OFFSET ====================
	
	// Yaw rotation for head/torso look at (use in Aim Offset)
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float LookAtYaw;

	// Pitch rotation for head/torso look at
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float LookAtPitch;

	// Target look at yaw (for smooth interpolation)
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float TargetLookAtYaw;

	// Target look at pitch
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float TargetLookAtPitch;

	// How much to blend the look at (0 = no look at, 1 = full look at)
	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float LookAtAlpha;

	// ==================== TURN IN PLACE ====================

	// Angle difference between mesh and desired rotation
	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float RootYawOffset;

	// Is currently playing turn in place animation?
	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	bool bIsTurningInPlace;

	// Should turn left? (false = turn right)
	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	bool bTurnLeft;

	// Threshold angle to trigger turn in place
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TurnInPlace")
	float TurnInPlaceThreshold = 70.0f;

	// ==================== ACTIONS ====================

	// Is currently attacking?
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	bool bIsAttacking;

	// Is currently taunting?
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	bool bIsTaunting;

	// Is taking damage?
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	bool bIsTakingHit;

	// Is in random idle pause?
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	bool bIsInIdlePause;

	// ==================== CONFIGURATION ====================

	// Speed at which look at rotations interpolate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	float LookAtInterpSpeed = 5.0f;

	// Maximum yaw for look at (head won't turn beyond this)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	float MaxLookAtYaw = 90.0f;

	// Maximum pitch for look at
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	float MaxLookAtPitch = 45.0f;

	// Speed threshold to consider "moving"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	float MovingSpeedThreshold = 10.0f;

	// ==================== FUNCTIONS ====================

	// Set look at target in world space
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void SetLookAtTarget(FVector WorldLocation);

	// Set look at rotation directly
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void SetLookAtRotation(float Yaw, float Pitch);

	// Clear look at (return to forward)
	UFUNCTION(BlueprintCallable, Category = "LookAt")
	void ClearLookAt();

	// Called when a montage should be played (attack, taunt, etc)
	UFUNCTION(BlueprintCallable, Category = "Actions")
	void PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

	// Notify that turn in place animation finished
	UFUNCTION(BlueprintCallable, Category = "TurnInPlace")
	void OnTurnInPlaceFinished();

protected:
	UPROPERTY()
	AEnemyBase* OwnerEnemy;

	// Cache previous state for transition detection
	EEnemyState LastFrameState;

	// Update movement values
	void UpdateMovementValues(float DeltaSeconds);

	// Update state values
	void UpdateStateValues();

	// Update look at interpolation
	void UpdateLookAt(float DeltaSeconds);

	// Update turn in place logic
	void UpdateTurnInPlace(float DeltaSeconds);

	// Calculate movement direction
	float CalculateMovementDirection() const;
};

