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
class UGrappleComponent;
class UCloneComponent;
class UCheckpointComponent;
class AWeaponBase;
class USceneComponent;
class UPlayerHUDWidget;
class USoundBase;
class UNiagaraSystem;
class UWeaponLerpComponent;
class UInteractionComponent;

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

	/** Override to route damage through CombatComponent parry/block system */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ========== HEALTH ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth = 100.0f;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	// ========== HUD ==========
	/** Widget class for the player HUD (create a WBP inheriting from UPlayerHUDWidget) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UPlayerHUDWidget* HUDWidget;

	/** Update the HUD health bar */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateHUD();

	// ========== COMPONENTS ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UTargetingComponent* TargetingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UGrappleComponent* GrappleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UCloneComponent* CloneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	UCheckpointComponent* CheckpointComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UWeaponLerpComponent* WeaponLerpComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UInteractionComponent* InteractionComponent;


	// ========== WEAPON ATTACH POINTS ==========
	/** Attachment point for weapon when held in hand */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|AttachPoints")
	USceneComponent* WeaponHandAttachPoint;

	/** Attachment point for weapon when sheathed on back */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|AttachPoints")
	USceneComponent* WeaponBackAttachPoint;

	/** Attachment point for weapon when in blocking stance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|AttachPoints")
	USceneComponent* WeaponBlockAttachPoint;

	/** Attachment point for grapple hook in left hand */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grapple|AttachPoints")
	USceneComponent* GrappleHandAttachPoint;

	// ========== WEAPON COMBO LERP POINTS ==========
	/** Idle/rest position for the weapon when not attacking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* WeaponIdlePoint;

	/** Light attack combo positions (5 points - weapon lerps between these) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* LightAttackPoint1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* LightAttackPoint2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* LightAttackPoint3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* LightAttackPoint4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* LightAttackPoint5;

	/** Heavy attack combo positions (2 points - up swing + down swing) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* HeavyAttackPoint1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|ComboPoints")
	USceneComponent* HeavyAttackPoint2;

	// ========== DEATH / RESPAWN ==========
	/** Delay before respawning after death (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float RespawnDelay = 1.5f;

	/** Flag set by CloneComponent to suppress landing SFX after teleport */
	bool bSuppressLandingSFX = false;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* GrappleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* CloneAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

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

	/** Input magnitude threshold to cancel sprint toggle (gamepad only - 0.8 = 80% of max) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float SprintCancelThreshold = 0.8f;

	/** Gravity scale when falling (higher = faster fall, more responsive) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float FallingGravityScale = 2.5f;

	/** Gravity scale during normal gameplay */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float NormalGravityScale = 1.5f;

	// ========== CAMERA SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float DefaultCameraDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float RunningCameraDistance = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraZoomSpeed = 5.0f;

	// ========== MOVEMENT SFX ==========

	/** Sound played when dashing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* DashSound;

	/** Sound played on first jump */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* JumpSound;

	/** Sound played on double jump */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* DoubleJumpSound;

	/** Sound played when landing on ground */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* LandSound;

	/** Sound played for walking footsteps */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* WalkFootstepSound;

	/** Sound played for running footsteps */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	USoundBase* RunFootstepSound;

	/** Interval between walking footstep sounds (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	float WalkFootstepInterval = 0.5f;

	/** Interval between running footstep sounds (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	float RunFootstepInterval = 0.3f;

	/** Minimum fall distance (units) to play landing sound/VFX. Prevents sound on teleport or small drops */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|SFX")
	float MinFallDistanceForLandSound = 150.0f;

	/** Sound played when switching weapon to hand */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|SFX")
	USoundBase* DrawWeaponSound;

	/** Sound played when sheathing weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|SFX")
	USoundBase* SheathWeaponSound;

	// ========== MOVEMENT VFX ==========

	/** VFX spawned at feet when dashing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|VFX")
	class UNiagaraSystem* DashVFX;

	/** VFX spawned at feet on jump */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|VFX")
	class UNiagaraSystem* JumpVFX;

	/** VFX spawned at feet on double jump */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|VFX")
	class UNiagaraSystem* DoubleJumpVFX;

	/** VFX spawned at feet when landing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|VFX")
	class UNiagaraSystem* LandVFX;

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

	/** Sprint toggle activated (for gamepad - stays sprinting until input reduces) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bSprintToggleActive = false;

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
	void ParryStart(const FInputActionValue& Value);
	void ParryRelease(const FInputActionValue& Value);
	void SwitchWeapon(const FInputActionValue& Value);
	void GrappleStart(const FInputActionValue& Value);
	void GrappleRelease(const FInputActionValue& Value);
	void CloneActivate(const FInputActionValue& Value);
	void Interact();

private:
	void UpdateCameraDistance(float DeltaTime);
	void UpdateGravityScale();
	void SpawnWeapon();
	void ResetDash();
	void HandleDeath();

	FVector2D MovementInput;
	float TargetCameraDistance;
	FTimerHandle DashCooldownTimer;
	FTimerHandle RespawnTimerHandle;
	bool bIsDashing = false;

	// Footstep timer
	float FootstepTimer = 0.0f;

	// Fall tracking for landing SFX
	float LastGroundedZ = 0.0f;
};
