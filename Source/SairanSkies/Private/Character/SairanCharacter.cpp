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
#include "Weapons/WeaponBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	GetCharacterMovement()->GravityScale = NormalGravityScale;

	// Visual mesh (capsule placeholder for character body)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0, 0, 0));

	// ========== WEAPON ATTACH POINTS ==========
	// These are empty scene components that can be repositioned in the editor
	// to define exactly where the weapon should be in each state
	
	// Hand attachment point - weapon held in right hand
	WeaponHandAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponHandAttachPoint"));
	WeaponHandAttachPoint->SetupAttachment(VisualMesh);
	WeaponHandAttachPoint->SetRelativeLocation(FVector(30.0f, 25.0f, 40.0f)); // Right side, mid height
	WeaponHandAttachPoint->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f)); // Blade pointing up
	
	// Back attachment point - weapon sheathed on back (diagonal like God of War)
	WeaponBackAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponBackAttachPoint"));
	WeaponBackAttachPoint->SetupAttachment(VisualMesh);
	WeaponBackAttachPoint->SetRelativeLocation(FVector(-20.0f, 10.0f, 60.0f)); // Behind, slightly offset
	WeaponBackAttachPoint->SetRelativeRotation(FRotator(-35.0f, 45.0f, 0.0f)); // Diagonal on back
	
	// Block attachment point - weapon in defensive stance
	WeaponBlockAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponBlockAttachPoint"));
	WeaponBlockAttachPoint->SetupAttachment(VisualMesh);
	WeaponBlockAttachPoint->SetRelativeLocation(FVector(40.0f, 0.0f, 70.0f)); // In front, higher up
	WeaponBlockAttachPoint->SetRelativeRotation(FRotator(0.0f, 45.0f, -45.0f)); // Angled for blocking

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

	// Setup visual mesh (capsule placeholder)
	SetupVisualMesh();

	// Spawn weapon
	SpawnWeapon();

	// Set initial speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASairanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraDistance(DeltaTime);
	UpdateGravityScale();

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

void ASairanCharacter::SetupVisualMesh()
{
	if (!VisualMesh) return;

	// Load capsule mesh (using cylinder as placeholder since it's closer to a capsule)
	UStaticMesh* CapsuleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CapsuleMesh)
	{
		VisualMesh->SetStaticMesh(CapsuleMesh);
		
		// Scale to match character capsule size (radius 42, half-height 96)
		// Cylinder default is 100x100x100, we want it to be approx 84 diameter, 192 tall
		float Diameter = 84.0f;
		float Height = 192.0f;
		FVector Scale = FVector(Diameter / 100.0f, Diameter / 100.0f, Height / 100.0f);
		VisualMesh->SetRelativeScale3D(Scale);
		
		// Position it centered on the capsule
		VisualMesh->SetRelativeLocation(FVector(0, 0, 0));

		// Create a simple colored material
		UMaterialInstanceDynamic* DynMaterial = VisualMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynMaterial)
		{
			// Light blue color for the character
			DynMaterial->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.3f, 0.5f, 0.8f, 1.0f));
		}
	}
}

