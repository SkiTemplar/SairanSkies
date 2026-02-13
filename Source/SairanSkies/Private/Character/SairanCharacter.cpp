// SairanSkies - Character Principal Implementation

#include "Character/SairanCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/TargetingComponent.h"
#include "Weapons/WeaponBase.h"
#include "Kismet/KismetMathLibrary.h"

ASairanCharacter::ASairanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule setup
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

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

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Combat Component
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// Targeting Component
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));

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
}

void ASairanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraDistance(DeltaTime);

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
		
		// Parry
		EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Started, this, &ASairanCharacter::Parry);
		
		// Switch Weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ASairanCharacter::SwitchWeapon);
	}
}

void ASairanCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CurrentJumpCount = 0;
}

// ========== INPUT HANDLERS ==========

void ASairanCharacter::Move(const FInputActionValue& Value)
{
	MovementInput = Value.Get<FVector2D>();

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
		if (CurrentJumpCount > 0)
		{
			// Double jump - reset velocity for consistent jump height
			LaunchCharacter(FVector(0, 0, GetCharacterMovement()->JumpZVelocity), false, true);
		}
		else
		{
			Jump();
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
	StartSprint();
}

void ASairanCharacter::SprintEnd(const FInputActionValue& Value)
{
	StopSprint();
}

void ASairanCharacter::Dash(const FInputActionValue& Value)
{
	if (bCanDash && CanPerformAction())
	{
		PerformDash();
	}
}

void ASairanCharacter::LightAttack(const FInputActionValue& Value)
{
	if (CombatComponent && CanPerformAction() && bIsWeaponDrawn)
	{
		CombatComponent->LightAttack();
	}
}

void ASairanCharacter::HeavyAttackStart(const FInputActionValue& Value)
{
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

void ASairanCharacter::Parry(const FInputActionValue& Value)
{
	if (CombatComponent && CanPerformAction() && bIsWeaponDrawn)
	{
		CombatComponent->PerformParry();
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
	}
}

void ASairanCharacter::SheathWeapon()
{
	if (EquippedWeapon && bIsWeaponDrawn)
	{
		bIsWeaponDrawn = false;
		EquippedWeapon->AttachToBack();
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
