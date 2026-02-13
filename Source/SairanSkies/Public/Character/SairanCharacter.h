// SairanSkies - Character Principal

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SairanCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UCombatComponent;
class UTargetingComponent;
class AWeaponBase;

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle,
	Walking,
	Running,
	Jumping,
	Dashing,
	Attacking,
	Parrying,
	Stunned
};

UCLASS()
class SAIRANSKIES_API ASairanCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASairanCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ========== COMPONENTS ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UTargetingComponent* TargetingComponent;

	// ========== INPUT ACTIONS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* LightAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ParryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SwitchWeaponAction;

	// ========== MOVEMENT SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float DashDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float DashDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float DashCooldown = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	int32 MaxJumps = 2;

	// ========== CAMERA SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float DefaultCameraDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float RunningCameraDistance = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraZoomSpeed = 5.0f;

	// ========== WEAPON ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeaponBase* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsWeaponDrawn = true;

	// ========== STATE ==========
	UPROPERTY(BlueprintReadOnly, Category = "State")
	ECharacterState CurrentState = ECharacterState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	int32 CurrentJumpCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bCanDash = true;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector DashDirection;

	// ========== FUNCTIONS ==========
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void PerformDash();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ToggleWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DrawWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SheathWeapon();

	UFUNCTION(BlueprintCallable, Category = "State")
	void SetCharacterState(ECharacterState NewState);

	UFUNCTION(BlueprintPure, Category = "State")
	bool CanPerformAction() const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	FVector GetMovementInputDirection() const;

protected:
	// Input Callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpStart(const FInputActionValue& Value);
	void JumpEnd(const FInputActionValue& Value);
	void SprintStart(const FInputActionValue& Value);
	void SprintEnd(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void LightAttack(const FInputActionValue& Value);
	void HeavyAttackStart(const FInputActionValue& Value);
	void HeavyAttackRelease(const FInputActionValue& Value);
	void Parry(const FInputActionValue& Value);
	void SwitchWeapon(const FInputActionValue& Value);

private:
	void UpdateCameraDistance(float DeltaTime);
	void SpawnWeapon();
	void ResetDash();

	FVector2D MovementInput;
	float TargetCameraDistance;
	FTimerHandle DashCooldownTimer;
	bool bIsDashing = false;
};
