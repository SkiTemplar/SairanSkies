// SairanSkies - Character Principal Implementation

#include "Character/SairanCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/TargetingComponent.h"
#include "Combat/GrappleComponent.h"
#include "Character/CloneComponent.h"
#include "Character/CheckpointComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponLerpComponent.h"
#include "UI/PlayerHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

ASairanCharacter::ASairanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule setup
	GetCapsuleComponent()->InitCapsuleSize(42.f, 75.0f);

	// Don't rotate character to camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.4f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->GravityScale = NormalGravityScale;


	// ========== WEAPON ATTACH POINTS ==========
	// These are empty scene components that can be repositioned in the editor
	// to define exactly where the weapon should be in each state
	
	// Hand attachment point - weapon held in right hand
	WeaponHandAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponHandAttachPoint"));
	WeaponHandAttachPoint->SetupAttachment(RootComponent);
	WeaponHandAttachPoint->SetRelativeLocation(FVector(30.0f, 25.0f, 40.0f)); // Right side, mid height
	WeaponHandAttachPoint->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f)); // Blade pointing up
	
	// Back attachment point - weapon sheathed on back (diagonal like God of War)
	WeaponBackAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponBackAttachPoint"));
	WeaponBackAttachPoint->SetupAttachment(RootComponent);
	WeaponBackAttachPoint->SetRelativeLocation(FVector(-20.0f, 10.0f, 60.0f)); // Behind, slightly offset
	WeaponBackAttachPoint->SetRelativeRotation(FRotator(-35.0f, 45.0f, 0.0f)); // Diagonal on back
	
	// Block attachment point - weapon in defensive stance
	WeaponBlockAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponBlockAttachPoint"));
	WeaponBlockAttachPoint->SetupAttachment(RootComponent);
	WeaponBlockAttachPoint->SetRelativeLocation(FVector(40.0f, 0.0f, 70.0f)); // In front, higher up
	WeaponBlockAttachPoint->SetRelativeRotation(FRotator(0.0f, 45.0f, -45.0f)); // Angled for blocking

	// Grapple hand attachment point - grapple hook held in left hand
	GrappleHandAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GrappleHandAttachPoint"));
	GrappleHandAttachPoint->SetupAttachment(RootComponent);
	GrappleHandAttachPoint->SetRelativeLocation(FVector(30.0f, -25.0f, 40.0f)); // Left side, mid height
	GrappleHandAttachPoint->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // Pointing forward

	// ========== WEAPON COMBO LERP POINTS ==========
	// Idle/rest position
	WeaponIdlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponIdlePoint"));
	WeaponIdlePoint->SetupAttachment(RootComponent);
	WeaponIdlePoint->SetRelativeLocation(FVector(30.0f, 25.0f, 40.0f));
	WeaponIdlePoint->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));

	// Light attack combo points (5 positions around the character)
	LightAttackPoint1 = CreateDefaultSubobject<USceneComponent>(TEXT("LightAttackPoint1"));
	LightAttackPoint1->SetupAttachment(RootComponent);
	LightAttackPoint1->SetRelativeLocation(FVector(60.0f, 40.0f, 60.0f));
	LightAttackPoint1->SetRelativeRotation(FRotator(20.0f, 30.0f, -60.0f));

	LightAttackPoint2 = CreateDefaultSubobject<USceneComponent>(TEXT("LightAttackPoint2"));
	LightAttackPoint2->SetupAttachment(RootComponent);
	LightAttackPoint2->SetRelativeLocation(FVector(50.0f, -30.0f, 50.0f));
	LightAttackPoint2->SetRelativeRotation(FRotator(-15.0f, -20.0f, -110.0f));

	LightAttackPoint3 = CreateDefaultSubobject<USceneComponent>(TEXT("LightAttackPoint3"));
	LightAttackPoint3->SetupAttachment(RootComponent);
	LightAttackPoint3->SetRelativeLocation(FVector(55.0f, 35.0f, 30.0f));
	LightAttackPoint3->SetRelativeRotation(FRotator(10.0f, 45.0f, -80.0f));

	LightAttackPoint4 = CreateDefaultSubobject<USceneComponent>(TEXT("LightAttackPoint4"));
	LightAttackPoint4->SetupAttachment(RootComponent);
	LightAttackPoint4->SetRelativeLocation(FVector(45.0f, -25.0f, 70.0f));
	LightAttackPoint4->SetRelativeRotation(FRotator(-25.0f, -35.0f, -100.0f));

	LightAttackPoint5 = CreateDefaultSubobject<USceneComponent>(TEXT("LightAttackPoint5"));
	LightAttackPoint5->SetupAttachment(RootComponent);
	LightAttackPoint5->SetRelativeLocation(FVector(65.0f, 20.0f, 45.0f));
	LightAttackPoint5->SetRelativeRotation(FRotator(15.0f, 10.0f, -70.0f));

	// Heavy attack combo points (2 positions - overhead swing down)
	HeavyAttackPoint1 = CreateDefaultSubobject<USceneComponent>(TEXT("HeavyAttackPoint1"));
	HeavyAttackPoint1->SetupAttachment(RootComponent);
	HeavyAttackPoint1->SetRelativeLocation(FVector(20.0f, 15.0f, 110.0f));
	HeavyAttackPoint1->SetRelativeRotation(FRotator(0.0f, 0.0f, -10.0f));

	HeavyAttackPoint2 = CreateDefaultSubobject<USceneComponent>(TEXT("HeavyAttackPoint2"));
	HeavyAttackPoint2->SetupAttachment(RootComponent);
	HeavyAttackPoint2->SetRelativeLocation(FVector(70.0f, 10.0f, 10.0f));
	HeavyAttackPoint2->SetRelativeRotation(FRotator(-60.0f, 15.0f, -90.0f));

	// Camera boom (third person, over shoulder but a bit farther)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f); // Slight offset to the right shoulder
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 8.0f;
	// Camera collision: collide with world geometry but NOT enemies
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeChannel = ECC_Camera;

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Combat Component
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// Targeting Component
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));

	// Grapple Component
	GrappleComponent = CreateDefaultSubobject<UGrappleComponent>(TEXT("GrappleComponent"));

	// Clone Component
	CloneComponent = CreateDefaultSubobject<UCloneComponent>(TEXT("CloneComponent"));

	// Checkpoint Component (auto-saves last grounded position for respawn)
	CheckpointComponent = CreateDefaultSubobject<UCheckpointComponent>(TEXT("CheckpointComponent"));

	// Weapon Lerp Component (handles combo position lerping)
	WeaponLerpComponent = CreateDefaultSubobject<UWeaponLerpComponent>(TEXT("WeaponLerpComponent"));

	// Initial state
	TargetCameraDistance = DefaultCameraDistance;
}

void ASairanCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}


	// Spawn weapon
	SpawnWeapon();

	// Set initial speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Initialize health
	CurrentHealth = MaxHealth;

	// Initialize fall tracking
	LastGroundedZ = GetActorLocation().Z;

	// Create and display HUD widget
	if (HUDWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC)
		{
			HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport();
				UpdateHUD();
			}
		}
	}
}

// ========== DAMAGE / PARRY / BLOCK ==========
float ASairanCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHealth <= 0.0f) return 0.0f;

	// Dash i-frames: invulnerable during dash
	if (bIsDashing || CurrentState == ECharacterState::Dashing)
	{
		UE_LOG(LogTemp, Log, TEXT("Player: DASH i-frame — damage ignored"));
		return 0.0f;
	}

	float DamageApplied = DamageAmount;

	// Route through CombatComponent for parry/block
	if (CombatComponent)
	{
		bool bParried = CombatComponent->HandleIncomingDamage(DamageAmount, DamageCauser, DamageApplied);
		if (bParried)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player: PARRY! Damage deflected from %s"), *GetNameSafe(DamageCauser));
			return 0.0f;
		}
	}

	// Apply remaining damage
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageApplied, 0.0f, MaxHealth);

	// Update HUD
	UpdateHUD();

	UE_LOG(LogTemp, Warning, TEXT("Player: HP %.0f/%.0f (took %.1f from %s%s)"),
		CurrentHealth, MaxHealth, DamageApplied, *GetNameSafe(DamageCauser),
		(DamageApplied < DamageAmount) ? TEXT(" — BLOCKED") : TEXT(""));

	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("Player: DEAD!"));
		HandleDeath();
	}

	return DamageApplied;
}

void ASairanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraDistance(DeltaTime);
	UpdateGravityScale();

	// Track last grounded Z for landing sound logic
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		LastGroundedZ = GetActorLocation().Z;
	}

	// ========== FOOTSTEP SFX ==========
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling() && GetVelocity().Size2D() > 50.0f)
	{
		float Interval = bIsSprinting ? RunFootstepInterval : WalkFootstepInterval;
		USoundBase* FootstepSound = bIsSprinting ? RunFootstepSound : WalkFootstepSound;

		FootstepTimer += DeltaTime;
		if (FootstepTimer >= Interval && FootstepSound)
		{
			FootstepTimer = 0.0f;
			FVector FeetLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSound, FeetLocation, 0.5f);
		}
	}
	else
	{
		FootstepTimer = 0.0f;
	}

	// Update state based on movement
	if (CurrentState != ECharacterState::Attacking && CurrentState != ECharacterState::Dashing && CurrentState != ECharacterState::Parrying)
	{
		if (GetCharacterMovement()->IsFalling())
		{
			CurrentState = ECharacterState::Jumping;
		}
		else if (GetVelocity().Size2D() > 10.0f)
		{
			CurrentState = bIsSprinting ? ECharacterState::Running : ECharacterState::Walking;
		}
		else
		{
			CurrentState = ECharacterState::Idle;
		}
	}
}

void ASairanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Movement
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASairanCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASairanCharacter::Move);
		
		// Look
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASairanCharacter::Look);
		
		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASairanCharacter::JumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASairanCharacter::JumpEnd);
		
		// Sprint
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ASairanCharacter::SprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASairanCharacter::SprintEnd);
		
		// Dash
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ASairanCharacter::Dash);
		
		// Light Attack
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &ASairanCharacter::LightAttack);
		
		// Heavy Attack (hold support)
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &ASairanCharacter::HeavyAttackStart);
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &ASairanCharacter::HeavyAttackRelease);
		
		// Parry (hold support for blocking stance)
		EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Started, this, &ASairanCharacter::ParryStart);
		EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Completed, this, &ASairanCharacter::ParryRelease);
		
		// Switch Weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ASairanCharacter::SwitchWeapon);
		
		// Grapple (hold L2/F to aim, release to fire)
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Started, this, &ASairanCharacter::GrappleStart);
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Completed, this, &ASairanCharacter::GrappleRelease);

		// Clone/Teleport (Y/Triangle, D-pad Up, R on keyboard)
		EnhancedInputComponent->BindAction(CloneAction, ETriggerEvent::Started, this, &ASairanCharacter::CloneActivate);
	}
}

void ASairanCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CurrentJumpCount = 0;

	// Check if we should suppress landing SFX (after teleport)
	if (bSuppressLandingSFX)
	{
		bSuppressLandingSFX = false;
		return;
	}

	// Only play landing SFX/VFX if we fell from sufficient height
	float FallDistance = LastGroundedZ - GetActorLocation().Z;
	if (FallDistance < MinFallDistanceForLandSound)
	{
		return;
	}

	FVector FeetLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	// Landing SFX
	if (LandSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandSound, FeetLocation);
	}
	// Landing VFX
	if (LandVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LandVFX, FeetLocation,
			FRotator::ZeroRotator, FVector(1.0f), true, true);
	}
}

// ========== INPUT HANDLERS ==========

void ASairanCharacter::Move(const FInputActionValue& Value)
{
	MovementInput = Value.Get<FVector2D>();

	// Check if sprint toggle should be cancelled (gamepad only)
	// If input magnitude drops below threshold, cancel sprint toggle
	if (bSprintToggleActive)
	{
		float InputMagnitude = MovementInput.Size();
		if (InputMagnitude < SprintCancelThreshold)
		{
			// Player reduced stick input - cancel sprint toggle
			bSprintToggleActive = false;
			StopSprint();
		}
	}

	if (Controller != nullptr && !bIsDashing)
	{
		// Get rotation without pitch
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward and right vectors
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input
		AddMovementInput(ForwardDirection, MovementInput.Y);
		AddMovementInput(RightDirection, MovementInput.X);
	}
}

void ASairanCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASairanCharacter::JumpStart(const FInputActionValue& Value)
{
	if (CurrentJumpCount < MaxJumps && CanPerformAction())
	{
		FVector FeetLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		if (CurrentJumpCount > 0)
		{
			// Double jump - reset velocity for consistent jump height
			LaunchCharacter(FVector(0, 0, GetCharacterMovement()->JumpZVelocity), false, true);
			
			// Double jump SFX/VFX
			if (DoubleJumpSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoubleJumpSound, GetActorLocation());
			}
			if (DoubleJumpVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DoubleJumpVFX, FeetLocation,
					FRotator::ZeroRotator, FVector(1.0f), true, true);
			}
		}
		else
		{
			Jump();
			
			// Jump SFX/VFX
			if (JumpSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation());
			}
			if (JumpVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), JumpVFX, FeetLocation,
					FRotator::ZeroRotator, FVector(1.0f), true, true);
			}
		}
		CurrentJumpCount++;
	}
}

void ASairanCharacter::JumpEnd(const FInputActionValue& Value)
{
	StopJumping();
}

void ASairanCharacter::SprintStart(const FInputActionValue& Value)
{
	// Toggle sprint on (for gamepad - stays on until input reduces)
	bSprintToggleActive = true;
	StartSprint();
}

void ASairanCharacter::SprintEnd(const FInputActionValue& Value)
{
	// Only stop sprint if toggle is not active
	// This allows PC players to release sprint key normally
	// But gamepad players keep sprinting until they reduce stick input
	if (!bSprintToggleActive)
	{
		StopSprint();
	}
}

void ASairanCharacter::Dash(const FInputActionValue& Value)
{
	// Cannot dash while in the air
	if (bCanDash && CanPerformAction() && !GetCharacterMovement()->IsFalling())
	{
		PerformDash();
	}
}

void ASairanCharacter::LightAttack(const FInputActionValue& Value)
{
	// Cannot attack while aiming or being pulled by grapple
	if (GrappleComponent && (GrappleComponent->IsAiming() || GrappleComponent->IsGrappling()))
	{
		return;
	}

	// Cannot attack while blocking
	if (CombatComponent && CombatComponent->bIsHoldingBlock)
	{
		return;
	}
	
	if (CombatComponent && CanPerformAction() && bIsWeaponDrawn)
	{
		CombatComponent->LightAttack();
	}
}

void ASairanCharacter::HeavyAttackStart(const FInputActionValue& Value)
{
	// Cannot attack while aiming or being pulled by grapple
	if (GrappleComponent && (GrappleComponent->IsAiming() || GrappleComponent->IsGrappling()))
	{
		return;
	}

	// Cannot attack while blocking
	if (CombatComponent && CombatComponent->bIsHoldingBlock)
	{
		return;
	}
	
	if (CombatComponent && CanPerformAction() && bIsWeaponDrawn)
	{
		CombatComponent->StartHeavyAttack();
	}
}

void ASairanCharacter::HeavyAttackRelease(const FInputActionValue& Value)
{
	if (CombatComponent && bIsWeaponDrawn)
	{
		CombatComponent->ReleaseHeavyAttack();
	}
}

void ASairanCharacter::ParryStart(const FInputActionValue& Value)
{
	if (CombatComponent && CanPerformAction() && bIsWeaponDrawn)
	{
		CombatComponent->StartBlock();
	}
}

void ASairanCharacter::ParryRelease(const FInputActionValue& Value)
{
	if (CombatComponent && bIsWeaponDrawn)
	{
		CombatComponent->ReleaseBlock();
	}
}

void ASairanCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	ToggleWeapon();
}

// ========== MOVEMENT FUNCTIONS ==========

void ASairanCharacter::StartSprint()
{
	if (CanPerformAction())
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		TargetCameraDistance = RunningCameraDistance;
	}
}

void ASairanCharacter::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	TargetCameraDistance = DefaultCameraDistance;
}

void ASairanCharacter::PerformDash()
{
	if (!bCanDash || bIsDashing) return;

	bIsDashing = true;
	bCanDash = false;
	SetCharacterState(ECharacterState::Dashing);

	// Calculate dash direction based on input, or forward if no input
	FVector DashDir;
	if (MovementInput.Size() > 0.1f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		DashDir = (ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X).GetSafeNormal();
	}
	else
	{
		DashDir = GetActorForwardVector();
	}

	DashDirection = DashDir;

	// Calculate dash velocity
	FVector DashVelocity = DashDir * (DashDistance / DashDuration);
	
	// Launch character
	LaunchCharacter(DashVelocity, true, true);

	// Dash SFX
	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, GetActorLocation());
	}
	// Dash VFX at feet
	if (DashVFX)
	{
		FVector FeetLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DashVFX, FeetLocation,
			DashDir.Rotation(), FVector(1.0f), true, true);
	}

	// End dash after duration
	FTimerHandle DashEndTimer;
	GetWorld()->GetTimerManager().SetTimer(DashEndTimer, [this]()
	{
		bIsDashing = false;
		SetCharacterState(ECharacterState::Idle);
	}, DashDuration, false);

	// Start cooldown
	GetWorld()->GetTimerManager().SetTimer(DashCooldownTimer, this, &ASairanCharacter::ResetDash, DashCooldown, false);
}

void ASairanCharacter::ResetDash()
{
	bCanDash = true;
}

// ========== WEAPON FUNCTIONS ==========

void ASairanCharacter::SpawnWeapon()
{
	if (WeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		EquippedWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, SpawnParams);
		if (EquippedWeapon)
		{
			EquippedWeapon->EquipToCharacter(this);
		}
	}
}

void ASairanCharacter::ToggleWeapon()
{
	if (bIsWeaponDrawn)
	{
		SheathWeapon();
	}
	else
	{
		DrawWeapon();
	}
}

void ASairanCharacter::DrawWeapon()
{
	if (EquippedWeapon && !bIsWeaponDrawn)
	{
		bIsWeaponDrawn = true;
		EquippedWeapon->AttachToHand();

		if (DrawWeaponSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DrawWeaponSound, GetActorLocation());
		}
	}
}

void ASairanCharacter::SheathWeapon()
{
	if (EquippedWeapon && bIsWeaponDrawn)
	{
		bIsWeaponDrawn = false;
		EquippedWeapon->AttachToBack();

		if (SheathWeaponSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), SheathWeaponSound, GetActorLocation());
		}
	}
}

// ========== STATE FUNCTIONS ==========

void ASairanCharacter::SetCharacterState(ECharacterState NewState)
{
	CurrentState = NewState;
}

bool ASairanCharacter::CanPerformAction() const
{
	return CurrentState != ECharacterState::Stunned && 
		   CurrentState != ECharacterState::Dashing &&
		   !bIsDashing;
}

FVector ASairanCharacter::GetMovementInputDirection() const
{
	if (MovementInput.Size() < 0.1f)
	{
		return GetActorForwardVector();
	}

	const FRotator Rotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	return (ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X).GetSafeNormal();
}

// ========== CAMERA FUNCTIONS ==========

void ASairanCharacter::UpdateCameraDistance(float DeltaTime)
{
	if (CameraBoom)
	{
		float CurrentDistance = CameraBoom->TargetArmLength;
		float NewDistance = FMath::FInterpTo(CurrentDistance, TargetCameraDistance, DeltaTime, CameraZoomSpeed);
		CameraBoom->TargetArmLength = NewDistance;
	}
}

void ASairanCharacter::UpdateGravityScale()
{
	if (!GetCharacterMovement()) return;

	// Apply higher gravity when falling for snappier double jump feel
	if (GetCharacterMovement()->IsFalling() && GetVelocity().Z < 0)
	{
		// Falling down - apply higher gravity
		GetCharacterMovement()->GravityScale = FallingGravityScale;
	}
	else
	{
		// Rising or grounded - normal gravity
		GetCharacterMovement()->GravityScale = NormalGravityScale;
	}
}

// ========== GRAPPLE INPUT HANDLERS ==========

void ASairanCharacter::GrappleStart(const FInputActionValue& Value)
{
	if (GrappleComponent && CanPerformAction())
	{
		GrappleComponent->StartAiming();
	}
}

void ASairanCharacter::GrappleRelease(const FInputActionValue& Value)
{
	if (GrappleComponent)
	{
		if (GrappleComponent->IsAiming())
		{
			// When releasing the button while aiming, fire the grapple
			GrappleComponent->FireGrapple();
		}
	}
}

// ========== CLONE INPUT HANDLER ==========

void ASairanCharacter::CloneActivate(const FInputActionValue& Value)
{
	if (CloneComponent && CanPerformAction())
	{
		CloneComponent->HandleCloneInput();
	}
}

// ========== HUD ==========

void ASairanCharacter::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(GetHealthPercent());
	}
}

void ASairanCharacter::HandleDeath()
{
	// Disable input momentarily
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}

	// Respawn after delay
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, [this]()
	{
		// Restore full health
		CurrentHealth = MaxHealth;
		UpdateHUD();

		// Respawn at last checkpoint
		if (CheckpointComponent)
		{
			CheckpointComponent->RespawnAtLastCheckpoint();
		}

		// Re-enable input
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			EnableInput(PC);
		}

		UE_LOG(LogTemp, Log, TEXT("Player: Respawned with full health at checkpoint"));
	}, RespawnDelay, false);
}


