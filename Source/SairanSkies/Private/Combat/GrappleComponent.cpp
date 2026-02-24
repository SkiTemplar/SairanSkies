// SairanSkies - Grapple Hook Component Implementation Complete

#include "Combat/GrappleComponent.h"
#include "Character/SairanCharacter.h"
#include "Weapons/GrappleHookActor.h"
#include "UI/GrappleCrosshairWidget.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"

UGrappleComponent::UGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
	
	if (OwnerCharacter && OwnerCharacter->CameraBoom)
	{
		// Store original camera values
		OriginalCameraDistance = OwnerCharacter->CameraBoom->TargetArmLength;
		OriginalCameraOffset = OwnerCharacter->CameraBoom->SocketOffset;
		TargetCameraDistance = OriginalCameraDistance;
		TargetCameraOffset = OriginalCameraOffset;
		
		// Store original camera rotation settings
		bOriginalUsePawnControlRotation = OwnerCharacter->CameraBoom->bUsePawnControlRotation;
		bOriginalEnableCameraLag = OwnerCharacter->CameraBoom->bEnableCameraLag;
	}

	// Store original rotation setting and gravity
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		bOriginalOrientRotationToMovement = OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement;
		OriginalGravityScale = OwnerCharacter->GetCharacterMovement()->GravityScale;
	}

	// Spawn the grapple hook visual actor
	SpawnGrappleHookActor();

	// Create the crosshair widget
	CreateCrosshairWidget();
}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (CurrentState)
	{
		case EGrappleState::Aiming:
			UpdateAiming(DeltaTime);
			UpdateCharacterRotation(DeltaTime);
			UpdateGrappleHookVisual();
			UpdateCrosshair(DeltaTime);
			break;
		case EGrappleState::Pulling:
			UpdatePulling(DeltaTime);
			break;
		case EGrappleState::Releasing:
			// Apply velocity dampening after grapple release
			if (bIsDampeningVelocity)
			{
				UpdateVelocityDampening(DeltaTime);
			}
			else
			{
				// Dampening finished - grapple is now available to use again
				// Reset to Idle immediately (don't wait for landing)
				SetState(EGrappleState::Idle);
			}
			// Also reset if we've landed
			if (OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsFalling())
			{
				ResetGrapple();
			}
			break;
		default:
			break;
	}

	// Update camera smoothly
	if (bCameraTransitioning)
	{
		UpdateCamera(DeltaTime);
	}
}

// ========== MAIN FUNCTIONS ==========

void UGrappleComponent::StartAiming()
{
	if (!OwnerCharacter || CurrentState != EGrappleState::Idle)
	{
		return;
	}

	SetState(EGrappleState::Aiming);
	
	// Set target camera for aiming mode
	TargetCameraDistance = AimingCameraDistance;
	TargetCameraOffset = AimingCameraOffset;
	bCameraTransitioning = true;

	// Make character follow camera rotation (like aiming)
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	OwnerCharacter->bUseControllerRotationYaw = true;

	// Show the grapple hook in hand
	if (GrappleHookActor)
	{
		GrappleHookActor->ShowHook();
	}

	// Show the crosshair
	ShowCrosshair();

	// Play aiming SFX
	if (AimingSound && OwnerCharacter)
	{
		AimingAudioComponent = UGameplayStatics::SpawnSoundAttached(
			AimingSound, OwnerCharacter->GetRootComponent(),
			NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, 1.0f, 1.0f, 0.0f);
	}

	OnGrappleAimStart.Broadcast();

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Grapple: Aiming Started"));
	}
}

void UGrappleComponent::StopAiming()
{
	if (CurrentState != EGrappleState::Aiming)
	{
		return;
	}

	// Return camera to normal
	if (OwnerCharacter && OwnerCharacter->CameraBoom)
	{
		TargetCameraDistance = OwnerCharacter->DefaultCameraDistance;
		TargetCameraOffset = OriginalCameraOffset;
	}

	// Restore character rotation behavior
	if (OwnerCharacter)
	{
		if (OwnerCharacter->GetCharacterMovement())
		{
			OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
		}
		OwnerCharacter->bUseControllerRotationYaw = false;
	}

	// Hide the grapple hook
	if (GrappleHookActor)
	{
		GrappleHookActor->HideHook();
	}

	// Hide the crosshair
	HideCrosshair();
	bCrosshairInitialized = false;

	// Stop aiming SFX
	if (AimingAudioComponent)
	{
		AimingAudioComponent->Stop();
		AimingAudioComponent = nullptr;
	}

	SetState(EGrappleState::Idle);
	bHasValidTarget = false;
	AimTargetLocation = FVector::ZeroVector;
	CurrentSoftLockTarget = nullptr;

	OnGrappleAimEnd.Broadcast();

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Grapple: Aiming Stopped"));
	}
}

void UGrappleComponent::FireGrapple()
{
	if (CurrentState != EGrappleState::Aiming || !bHasValidTarget || !OwnerCharacter)
	{
		// No valid target, just stop aiming
		StopAiming();
		return;
	}

	// Store grapple data
	GrappleStartPoint = OwnerCharacter->GetActorLocation();
	GrappleTargetPoint = AimTargetLocation;
	InitialDistanceToTarget = FVector::Dist(GrappleStartPoint, GrappleTargetPoint);
	
	// Calculate midpoint (where we'll release) - it's the horizontal position of the target
	GrappleMidpoint = GrappleTargetPoint;

	SetState(EGrappleState::Pulling);

	// Hide crosshair when firing
	HideCrosshair();

	// Stop aiming SFX, play fire and pull SFX
	if (AimingAudioComponent)
	{
		AimingAudioComponent->Stop();
		AimingAudioComponent = nullptr;
	}
	if (FireSound && OwnerCharacter)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, OwnerCharacter->GetActorLocation());
	}
	if (PullingSound && OwnerCharacter)
	{
		PullingAudioComponent = UGameplayStatics::SpawnSoundAttached(
			PullingSound, OwnerCharacter->GetRootComponent(),
			NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, 1.0f, 1.0f, 0.0f);
	}

	// Restore character rotation for the pull
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
		// Disable gravity during pull for clean, direct movement
		OwnerCharacter->GetCharacterMovement()->GravityScale = 0.0f;
		
		// Reset velocity to zero so all grapples start from a clean state
		// This prevents momentum from affecting the grapple trajectory
		OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}
	OwnerCharacter->bUseControllerRotationYaw = false;

	// Calculate direction with angle offset (like Batman - hook retracting)
	FVector GrappleDirection = CalculateGrappleDirection();
	
	// Use LaunchCharacter for physics-based movement (like Batman Arkham)
	// This gives a natural arc and respects collision
	FVector LaunchVelocity = GrappleDirection * GrapplePullSpeed;
	OwnerCharacter->LaunchCharacter(LaunchVelocity, true, true);

	// Lock camera during pull (sense of being dragged)
	LockCamera();

	// Move camera farther back during grapple (like running)
	TargetCameraDistance = OwnerCharacter->RunningCameraDistance;
	bCameraTransitioning = true;

	// Start particle trail from character
	StartGrappleTrailParticles();

	OnGrappleFired.Broadcast(GrappleTargetPoint);

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
			FString::Printf(TEXT("Grapple: Fired to %s"), *GrappleTargetPoint.ToString()));
	}
}

void UGrappleComponent::CancelGrapple()
{
	if (CurrentState == EGrappleState::Idle)
	{
		return;
	}

	// Restore movement and gravity
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		OwnerCharacter->GetCharacterMovement()->GravityScale = OriginalGravityScale;
	}

	ResetGrapple();

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Grapple: Cancelled"));
	}
}

// ========== UPDATE FUNCTIONS ==========

void UGrappleComponent::UpdateAiming(float DeltaTime)
{
	// PRIORITY 1: Aim assist - find tagged grappleable targets in screen area
	AActor* BestTarget = FindBestGrappleTarget();
	
	if (BestTarget)
	{
		// We have a soft-lock target - snap aim to it
		CurrentSoftLockTarget = BestTarget;
		bHasValidTarget = true;
		
		// Smooth interpolation towards the target for "sticky" feel
		FVector DesiredLocation = BestTarget->GetActorLocation();
		AimTargetLocation = FMath::VInterpTo(AimTargetLocation, DesiredLocation, DeltaTime, AimAssistStickySpeed);

		if (bShowDebug)
		{
			DrawDebugSphere(GetWorld(), AimTargetLocation, 40.0f, 12, FColor::Green, false, -1.0f, 0, 3.0f);
			DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), AimTargetLocation, FColor::Green, false, -1.0f, 0, 2.0f);
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, 
				FString::Printf(TEXT("Grapple AimAssist: Locked onto %s"), *BestTarget->GetName()));
		}
		return;
	}

	// PRIORITY 2: Fallback - direct line trace from camera center
	// This catches surfaces/actors with the Grapple tag that aren't picked up by aim assist,
	// or any surface if bRequireGrappleTag is false
	CurrentSoftLockTarget = nullptr;

	FHitResult HitResult = PerformAimTrace();
	
	if (HitResult.bBlockingHit)
	{
		bHasValidTarget = true;
		AimTargetLocation = HitResult.ImpactPoint;

		if (bShowDebug)
		{
			DrawDebugSphere(GetWorld(), AimTargetLocation, 30.0f, 12, FColor::Green, false, -1.0f, 0, 2.0f);
			DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), AimTargetLocation, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}
	else
	{
		// No valid target at all
		bHasValidTarget = false;
		
		if (OwnerCharacter && OwnerCharacter->FollowCamera)
		{
			FVector CameraLocation = OwnerCharacter->FollowCamera->GetComponentLocation();
			FVector CameraForward = OwnerCharacter->FollowCamera->GetForwardVector();
			AimTargetLocation = CameraLocation + CameraForward * MaxGrappleRange;

			if (bShowDebug)
			{
				DrawDebugSphere(GetWorld(), AimTargetLocation, 30.0f, 12, FColor::Red, false, -1.0f, 0, 2.0f);
				DrawDebugLine(GetWorld(), CameraLocation, AimTargetLocation, FColor::Red, false, -1.0f, 0, 1.0f);
			}
		}
	}
}

void UGrappleComponent::UpdatePulling(float DeltaTime)
{
	if (!OwnerCharacter)
	{
		ResetGrapple();
		return;
	}

	// Check if we've passed the midpoint
	if (HasPassedMidpoint())
	{
		// Release - let physics take over naturally
		SetState(EGrappleState::Releasing);
		
		// Restore gravity so physics takes over naturally
		if (OwnerCharacter->GetCharacterMovement())
		{
			OwnerCharacter->GetCharacterMovement()->GravityScale = OriginalGravityScale;
		}
		
		// Unlock camera (player can control again)
		UnlockCamera();
		
		// Stop particle trail
		StopGrappleTrailParticles();

		// Stop pull SFX, play release SFX
		if (PullingAudioComponent)
		{
			PullingAudioComponent->Stop();
			PullingAudioComponent = nullptr;
		}
		if (ReleaseSound && OwnerCharacter)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ReleaseSound, OwnerCharacter->GetActorLocation());
		}

		// Return camera to normal
		TargetCameraDistance = OwnerCharacter->DefaultCameraDistance;
		TargetCameraOffset = OriginalCameraOffset;

		// Give only 1 extra jump after grapple (set to MaxJumps - 1)
		OwnerCharacter->CurrentJumpCount = 1;

		// Start velocity dampening (reduce 80% over 1 second)
		bIsDampeningVelocity = true;
		DampeningTimeRemaining = DampeningDuration;

		OnGrappleComplete.Broadcast();

		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, TEXT("Grapple: Passed midpoint, releasing"));
		}
		return;
	}

	// Apply continuous force towards target (like hook retracting - Batman style)
	FVector GrappleDirection = CalculateGrappleDirection();
	
	// Calculate how much force to apply based on distance
	// More force when far, less when close for smoother arrival
	float CurrentDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), GrappleTargetPoint);
	float DistanceRatio = FMath::Clamp(CurrentDistance / InitialDistanceToTarget, 0.3f, 1.0f);
	
	// Apply force (physics-based, respects collision)
	FVector PullForce = GrappleDirection * GrapplePullSpeed * DistanceRatio * DeltaTime * 50.0f;
	OwnerCharacter->LaunchCharacter(PullForce, false, false);

	// Rotate character to face movement direction
	FRotator TargetRotation = GrappleDirection.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;
	OwnerCharacter->SetActorRotation(FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRotation, DeltaTime, 10.0f));

	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), GrappleTargetPoint, FColor::Cyan, false, -1.0f, 0, 3.0f);
		DrawDebugSphere(GetWorld(), GrappleTargetPoint, 50.0f, 16, FColor::Cyan, false, -1.0f, 0, 2.0f);
	}
}

void UGrappleComponent::UpdateCamera(float DeltaTime)
{
	if (!OwnerCharacter || !OwnerCharacter->CameraBoom)
	{
		return;
	}

	USpringArmComponent* CameraBoom = OwnerCharacter->CameraBoom;
	
	// Interpolate camera distance
	float CurrentDistance = CameraBoom->TargetArmLength;
	float NewDistance = FMath::FInterpTo(CurrentDistance, TargetCameraDistance, DeltaTime, CameraZoomSpeed);
	CameraBoom->TargetArmLength = NewDistance;

	// Interpolate camera offset
	FVector CurrentOffset = CameraBoom->SocketOffset;
	FVector NewOffset = FMath::VInterpTo(CurrentOffset, TargetCameraOffset, DeltaTime, CameraZoomSpeed);
	CameraBoom->SocketOffset = NewOffset;

	// Check if we've reached target values
	if (FMath::IsNearlyEqual(NewDistance, TargetCameraDistance, 1.0f) &&
		CurrentOffset.Equals(TargetCameraOffset, 1.0f))
	{
		bCameraTransitioning = false;
	}
}

FHitResult UGrappleComponent::PerformAimTrace()
{
	FHitResult HitResult;
	
	if (!OwnerCharacter || !OwnerCharacter->FollowCamera)
	{
		return HitResult;
	}

	// Trace from camera center forward
	FVector CameraLocation = OwnerCharacter->FollowCamera->GetComponentLocation();
	FVector CameraForward = OwnerCharacter->FollowCamera->GetForwardVector();
	FVector TraceEnd = CameraLocation + CameraForward * MaxGrappleRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		TraceEnd,
		GrappleTraceChannel,
		QueryParams
	);

	// Tag filter: only accept hits on actors with the Grapple tag (if required)
	if (bRequireGrappleTag && HitResult.bBlockingHit && HitResult.GetActor())
	{
		if (!HitResult.GetActor()->ActorHasTag(GrappleTag))
		{
			HitResult.bBlockingHit = false;
		}
	}

	return HitResult;
}

AActor* UGrappleComponent::FindBestGrappleTarget()
{
	if (!OwnerCharacter || !OwnerCharacter->FollowCamera)
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return nullptr;
	}

	// Get all actors with the Grapple tag
	TArray<AActor*> GrappleActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), GrappleTag, GrappleActors);

	if (GrappleActors.Num() == 0)
	{
		return nullptr;
	}

	// Get viewport size for screen-space calculations
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);

	AActor* BestTarget = nullptr;
	float BestScreenDistance = AimAssistScreenRadius; // Only consider targets within this screen radius

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	for (AActor* Actor : GrappleActors)
	{
		if (!Actor) continue;

		// Check world-space distance
		float WorldDistance = FVector::Dist(CharacterLocation, Actor->GetActorLocation());
		if (WorldDistance > MaxGrappleRange)
		{
			continue; // Too far
		}

		// Check line of sight
		FHitResult LOSHit;
		FCollisionQueryParams LOSParams;
		LOSParams.AddIgnoredActor(OwnerCharacter);
		LOSParams.AddIgnoredActor(Actor);

		FVector CameraLoc = OwnerCharacter->FollowCamera->GetComponentLocation();
		bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			LOSHit,
			CameraLoc,
			Actor->GetActorLocation(),
			ECC_Visibility,
			LOSParams
		);

		if (bBlocked && LOSHit.GetActor() != Actor)
		{
			continue; // Line of sight blocked
		}

		// Project actor to screen space
		FVector2D ScreenPosition;
		bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPosition, true);

		if (!bOnScreen)
		{
			continue; // Not on screen
		}

		// Calculate screen-space distance from center
		float ScreenDistance = FVector2D::Distance(ScreenCenter, ScreenPosition);

		if (ScreenDistance < BestScreenDistance)
		{
			BestScreenDistance = ScreenDistance;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

FVector UGrappleComponent::CalculateGrappleDirection() const
{
	if (!OwnerCharacter)
	{
		return FVector::ForwardVector;
	}

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector DirectionToTarget = (GrappleTargetPoint - CurrentLocation).GetSafeNormal();

	// Apply angle offset (aim slightly below the target point)
	// This prevents collision with the surface we're grappling to
	FRotator DirectionRotator = DirectionToTarget.Rotation();
	DirectionRotator.Pitch -= GrappleAngleOffset; // Lower the aim by 15 degrees
	
	return DirectionRotator.Vector();
}

bool UGrappleComponent::HasPassedMidpoint() const
{
	if (!OwnerCharacter)
	{
		return true;
	}

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	
	// Calculate horizontal distance to the target's vertical line
	// We release when we're close to being directly under/over the grapple point
	FVector2D CurrentXY(CurrentLocation.X, CurrentLocation.Y);
	FVector2D TargetXY(GrappleTargetPoint.X, GrappleTargetPoint.Y);
	
	float HorizontalDistance = FVector2D::Distance(CurrentXY, TargetXY);
	
	return HorizontalDistance <= MidpointReleaseDistance;
}

void UGrappleComponent::SetState(EGrappleState NewState)
{
	CurrentState = NewState;
}

void UGrappleComponent::ResetGrapple()
{
	SetState(EGrappleState::Idle);
	bHasValidTarget = false;
	GrappleTargetPoint = FVector::ZeroVector;
	AimTargetLocation = FVector::ZeroVector;
	GrappleStartPoint = FVector::ZeroVector;
	GrappleMidpoint = FVector::ZeroVector;
	InitialDistanceToTarget = 0.0f;
	bIsDampeningVelocity = false;
	DampeningTimeRemaining = 0.0f;
	CurrentSoftLockTarget = nullptr;

	// Restore character rotation behavior and gravity
	if (OwnerCharacter)
	{
		if (OwnerCharacter->GetCharacterMovement())
		{
			OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
			// Ensure gravity is always restored
			OwnerCharacter->GetCharacterMovement()->GravityScale = OriginalGravityScale;
		}
		OwnerCharacter->bUseControllerRotationYaw = false;
	}

	// Hide the grapple hook
	if (GrappleHookActor)
	{
		GrappleHookActor->HideHook();
	}

	// Hide the crosshair
	HideCrosshair();

	// Unlock camera (in case it was locked)
	UnlockCamera();

	// Stop particles (in case they were active)
	StopGrappleTrailParticles();

	// Stop any lingering grapple audio
	if (AimingAudioComponent)
	{
		AimingAudioComponent->Stop();
		AimingAudioComponent = nullptr;
	}
	if (PullingAudioComponent)
	{
		PullingAudioComponent->Stop();
		PullingAudioComponent = nullptr;
	}

	// Ensure camera returns to normal
	if (OwnerCharacter && OwnerCharacter->CameraBoom)
	{
		TargetCameraDistance = OwnerCharacter->DefaultCameraDistance;
		TargetCameraOffset = OriginalCameraOffset;
		bCameraTransitioning = true;
	}
}

void UGrappleComponent::SpawnGrappleHookActor()
{
	if (!OwnerCharacter || !GrappleHookClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter->GetInstigator();

	GrappleHookActor = GetWorld()->SpawnActor<AGrappleHookActor>(GrappleHookClass, SpawnParams);
	
	if (GrappleHookActor && OwnerCharacter->GrappleHandAttachPoint)
	{
		// Attach the hook to the left hand attachment point
		GrappleHookActor->AttachToComponent(
			OwnerCharacter->GrappleHandAttachPoint,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
		
		// Start hidden
		GrappleHookActor->HideHook();
	}
}

void UGrappleComponent::UpdateGrappleHookVisual()
{
	if (!GrappleHookActor || !OwnerCharacter)
	{
		return;
	}

	// Update hook to aim at target location
	GrappleHookActor->AimAtLocation(AimTargetLocation);
	
	// Update color based on target validity
	GrappleHookActor->SetTargetValid(bHasValidTarget);
}

void UGrappleComponent::UpdateCharacterRotation(float DeltaTime)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// Character already follows controller rotation via bUseControllerRotationYaw
	// This function can be used for additional rotation smoothing if needed
}

void UGrappleComponent::CreateCrosshairWidget()
{
	if (!CrosshairWidgetClass || !OwnerCharacter)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return;
	}

	CrosshairWidget = CreateWidget<UGrappleCrosshairWidget>(PC, CrosshairWidgetClass);
	if (CrosshairWidget)
	{
		CrosshairWidget->AddToViewport(10); // High Z-order so it's on top
		CrosshairWidget->HideCrosshair();
	}
}

void UGrappleComponent::ShowCrosshair()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->ShowCrosshair();
	}
}

void UGrappleComponent::HideCrosshair()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->HideCrosshair();
	}
}

void UGrappleComponent::UpdateCrosshair(float DeltaTime)
{
	if (!CrosshairWidget || !OwnerCharacter)
	{
		return;
	}

	CrosshairWidget->SetTargetValid(bHasValidTarget);

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return;
	}

	// Calculate screen center
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);

	// Initialize crosshair position at center on first frame
	if (!bCrosshairInitialized)
	{
		CurrentCrosshairPos = ScreenCenter;
		bCrosshairInitialized = true;
	}

	// Determine target screen position
	FVector2D DesiredScreenPos = ScreenCenter; // Default: center

	if (bHasValidTarget && CurrentSoftLockTarget)
	{
		FVector2D ProjectedPos;
		bool bOnScreen = PC->ProjectWorldLocationToScreen(AimTargetLocation, ProjectedPos, true);
		if (bOnScreen)
		{
			DesiredScreenPos = ProjectedPos;
		}
	}

	// Smoothly interpolate towards desired position
	CurrentCrosshairPos = FMath::Vector2DInterpTo(CurrentCrosshairPos, DesiredScreenPos, DeltaTime, CrosshairLerpSpeed);

	// Apply to widget
	CrosshairWidget->SetScreenPosition(CurrentCrosshairPos);
}

void UGrappleComponent::StartGrappleTrailParticles()
{
	if (!GrappleTrailParticles || !OwnerCharacter)
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Grapple: No particle system or owner!"));
		}
		return;
	}

	// Stop any existing particles first
	StopGrappleTrailParticles();

	// Spawn Niagara particle system attached to character
	// bAutoDestroy = false so it keeps emitting until we manually stop it
	GrappleTrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		GrappleTrailParticles,
		OwnerCharacter->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false // Don't auto destroy - we control when it stops
	);

	// Ensure the system is activated and looping
	if (GrappleTrailComponent)
	{
		GrappleTrailComponent->Activate(true); // Reset and activate
		
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Magenta, TEXT("Grapple: Particle trail started (continuous)"));
		}
	}
	else if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Grapple: Failed to spawn particles!"));
	}
}

void UGrappleComponent::StopGrappleTrailParticles()
{
	if (GrappleTrailComponent)
	{
		// Deactivate to stop emitting new particles
		GrappleTrailComponent->Deactivate();
		// Destroy the component
		GrappleTrailComponent->DestroyComponent();
		GrappleTrailComponent = nullptr;

		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Magenta, TEXT("Grapple: Particle trail stopped"));
		}
	}
}

void UGrappleComponent::LockCamera()
{
	if (!OwnerCharacter || !OwnerCharacter->CameraBoom)
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Grapple: No camera boom!"));
		}
		return;
	}

	// Disable camera rotation and lag for locked feel
	OwnerCharacter->CameraBoom->bUsePawnControlRotation = false;
	OwnerCharacter->CameraBoom->bEnableCameraLag = false;
	OwnerCharacter->CameraBoom->bEnableCameraRotationLag = false;

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Grapple: Camera locked"));
	}
}

void UGrappleComponent::UnlockCamera()
{
	if (!OwnerCharacter || !OwnerCharacter->CameraBoom)
	{
		return;
	}

	// Restore original camera settings
	OwnerCharacter->CameraBoom->bUsePawnControlRotation = bOriginalUsePawnControlRotation;
	OwnerCharacter->CameraBoom->bEnableCameraLag = bOriginalEnableCameraLag;
	OwnerCharacter->CameraBoom->bEnableCameraRotationLag = true;

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Grapple: Camera unlocked"));
	}
}

void UGrappleComponent::UpdateVelocityDampening(float DeltaTime)
{
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement())
	{
		bIsDampeningVelocity = false;
		return;
	}

	DampeningTimeRemaining -= DeltaTime;

	if (DampeningTimeRemaining <= 0.0f)
	{
		// Dampening complete
		bIsDampeningVelocity = false;
		DampeningTimeRemaining = 0.0f;

		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Grapple: Velocity dampening complete"));
		}
		return;
	}

	// Calculate how much to reduce velocity this frame
	// We want to reduce DampeningFactor (80%) of the velocity over DampeningDuration (1 second)
	// So each frame we reduce: (DampeningFactor / DampeningDuration) * DeltaTime
	float ReductionThisFrame = (DampeningFactor / DampeningDuration) * DeltaTime;

	// Get current velocity
	FVector CurrentVelocity = OwnerCharacter->GetCharacterMovement()->Velocity;
	
	// Only dampen horizontal velocity (let gravity handle vertical)
	FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
	FVector VerticalVelocity = FVector(0.0f, 0.0f, CurrentVelocity.Z);

	// Apply reduction to horizontal velocity
	HorizontalVelocity *= (1.0f - ReductionThisFrame);

	// Combine and set new velocity
	OwnerCharacter->GetCharacterMovement()->Velocity = HorizontalVelocity + VerticalVelocity;

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, 
			FString::Printf(TEXT("Grapple: Dampening velocity (%.1f%% remaining)"), 
				(DampeningTimeRemaining / DampeningDuration) * 100.0f));
	}
}


