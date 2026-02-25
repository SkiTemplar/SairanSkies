// SairanSkies - Clone/Teleport Component Implementation

#include "Character/CloneComponent.h"
#include "Character/SairanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"

UCloneComponent::UCloneComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCloneComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
}

void UCloneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If clone is active, check distance - destroy if out of range
	if (CurrentState == ECloneState::CloneActive && OwnerCharacter)
	{
		if (!IsWithinTeleportRange())
		{
			if (bShowDebug)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
					FString::Printf(TEXT("Clone: Out of range (%.0fm)! Clone destroyed."), GetDistanceToClone() / 100.0f));
			}

			// Play a small VFX at clone location before destroying
			PlayVFXAtLocation(CloneSpawnVFX, CloneLocation);
			DestroyClone();
			return;
		}
	}

	// Debug visualization
	if (bShowDebug && CurrentState == ECloneState::CloneActive && CloneActor)
	{
		float Dist = GetDistanceToClone();
		float MaxDist = OwnerCharacter->bIsWeaponDrawn ? MaxTeleportDistanceCombat : MaxTeleportDistanceExplore;
		DrawDebugSphere(GetWorld(), CloneLocation, 60.0f, 12, FColor::Purple, false, -1.0f, 0, 2.0f);
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple,
			FString::Printf(TEXT("Clone: %.0f/%.0fm"), Dist / 100.0f, MaxDist / 100.0f));
	}
}

// ========== MAIN FUNCTIONS ==========

void UCloneComponent::HandleCloneInput()
{
	if (!OwnerCharacter)
	{
		return;
	}

	switch (CurrentState)
	{
		case ECloneState::Inactive:
			PlaceClone();
			break;

		case ECloneState::CloneActive:
			TeleportToClone();
			break;
	}
}

bool UCloneComponent::PlaceClone()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	// Check if we're on the ground
	if (!IsValidPlacementPosition())
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Clone: Invalid placement - must be grounded with head clearance"));
		}
		return false;
	}

	// Store clone position and rotation
	CloneLocation = OwnerCharacter->GetActorLocation();
	CloneRotation = OwnerCharacter->GetActorRotation();

	// Spawn the visual clone
	SpawnCloneVisual();

	// Push player backward like a short dash
	FVector BackwardDir = -OwnerCharacter->GetActorForwardVector();
	BackwardDir.Z = 0.0f;
	BackwardDir.Normalize();
	// Override horizontal velocity for an instant dash feel, keep vertical
	OwnerCharacter->LaunchCharacter(BackwardDir * PushBackForce, true, false);

	// Play VFX and SFX at clone position
	PlayVFXAtLocation(CloneSpawnVFX, CloneLocation);
	PlaySFXAtLocation(ClonePlaceSound, CloneLocation);

	// Set state
	CurrentState = ECloneState::CloneActive;
	CloneStartTime = GetWorld()->GetTimeSeconds();

	// Start timer for clone expiration
	GetWorld()->GetTimerManager().SetTimer(
		CloneTimerHandle,
		this,
		&UCloneComponent::OnCloneTimerExpired,
		CloneDuration,
		false
	);

	// Broadcast event
	OnClonePlaced.Broadcast(CloneLocation);

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple, TEXT("Clone: Placed!"));
	}

	return true;
}

void UCloneComponent::TeleportToClone()
{
	if (!OwnerCharacter || CurrentState != ECloneState::CloneActive)
	{
		return;
	}

	// Must be on the ground to teleport
	if (OwnerCharacter->GetCharacterMovement()->IsFalling())
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Clone: Must be on the ground to teleport!"));
		}
		return;
	}

	// Check distance
	if (!IsWithinTeleportRange())
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
				FString::Printf(TEXT("Clone: Too far! Distance: %.0f, Max: %.0f"),
					GetDistanceToClone(),
					OwnerCharacter->bIsWeaponDrawn ? MaxTeleportDistanceCombat : MaxTeleportDistanceExplore));
		}
		return;
	}

	FVector OriginalLocation = OwnerCharacter->GetActorLocation();

	// Play teleport VFX at BOTH locations (origin and destination)
	PlayVFXAtLocation(TeleportVFX, OriginalLocation);
	PlayVFXAtLocation(TeleportVFX, CloneLocation);

	// Play teleport sound at destination
	PlaySFXAtLocation(TeleportSound, CloneLocation);

	// Teleport player to clone position
	OwnerCharacter->SetActorLocation(CloneLocation, false, nullptr, ETeleportType::TeleportPhysics);
	OwnerCharacter->SetActorRotation(CloneRotation);

	// Suppress landing sound after teleport
	OwnerCharacter->bSuppressLandingSFX = true;

	// Reset velocity so player doesn't carry momentum
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// Broadcast event
	OnTeleported.Broadcast(OriginalLocation, CloneLocation);

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Clone: Teleported!"));
	}

	// Clean up
	DestroyClone();
}

void UCloneComponent::DestroyClone()
{
	// Clear timer
	GetWorld()->GetTimerManager().ClearTimer(CloneTimerHandle);

	// Destroy visual clone
	if (CloneActor)
	{
		CloneActor->Destroy();
		CloneActor = nullptr;
	}

	// Reset state
	CurrentState = ECloneState::Inactive;
	CloneLocation = FVector::ZeroVector;
	CloneRotation = FRotator::ZeroRotator;
	CloneStartTime = 0.0f;
}

float UCloneComponent::GetCloneTimeRemaining() const
{
	if (CurrentState != ECloneState::CloneActive)
	{
		return 0.0f;
	}

	float Elapsed = GetWorld()->GetTimeSeconds() - CloneStartTime;
	return FMath::Max(0.0f, CloneDuration - Elapsed);
}

float UCloneComponent::GetDistanceToClone() const
{
	if (!OwnerCharacter || CurrentState != ECloneState::CloneActive)
	{
		return 0.0f;
	}
	return FVector::Dist(OwnerCharacter->GetActorLocation(), CloneLocation);
}

bool UCloneComponent::IsWithinTeleportRange() const
{
	if (!OwnerCharacter || CurrentState != ECloneState::CloneActive)
	{
		return false;
	}
	
	float MaxDist = OwnerCharacter->bIsWeaponDrawn ? MaxTeleportDistanceCombat : MaxTeleportDistanceExplore;
	return GetDistanceToClone() <= MaxDist;
}

// ========== PRIVATE FUNCTIONS ==========

bool UCloneComponent::IsValidPlacementPosition() const
{
	if (!OwnerCharacter) return false;

	// Must be on the ground
	if (OwnerCharacter->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	// Check ceiling clearance - trace upward from head
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	FVector EndLocation = StartLocation + FVector(0, 0, MinCeilingClearance);

	FHitResult CeilingHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	bool bCeilingBlocked = GetWorld()->LineTraceSingleByChannel(
		CeilingHit,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	if (bCeilingBlocked)
	{
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Clone: Ceiling too close!"));
		}
		return false;
	}

	return true;
}

void UCloneComponent::SpawnCloneVisual()
{
	// Destroy any existing clone
	if (CloneActor)
	{
		CloneActor->Destroy();
		CloneActor = nullptr;
	}

	if (!OwnerCharacter) return;

	// Create a simple actor to represent the clone
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CloneActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), CloneLocation, CloneRotation, SpawnParams);
	
	if (!CloneActor) return;

	// Create root component at exact position
	USceneComponent* RootComp = NewObject<USceneComponent>(CloneActor, TEXT("CloneRoot"));
	CloneActor->SetRootComponent(RootComp);
	RootComp->RegisterComponent();
	RootComp->SetWorldLocationAndRotation(CloneLocation, CloneRotation);

	// Try to copy the character's mesh as a translucent clone
	USkeletalMeshComponent* CharMesh = OwnerCharacter->GetMesh();
	if (CharMesh && CharMesh->GetSkeletalMeshAsset())
	{
		USkeletalMeshComponent* CloneMesh = NewObject<USkeletalMeshComponent>(CloneActor, TEXT("CloneMesh"));
		CloneMesh->SetupAttachment(RootComp);
		CloneMesh->SetSkeletalMeshAsset(CharMesh->GetSkeletalMeshAsset());
		
		// Copy exact relative transform from the character's mesh
		// This preserves the Z-offset (e.g., -90 so feet touch ground)
		CloneMesh->SetRelativeLocation(CharMesh->GetRelativeLocation());
		CloneMesh->SetRelativeRotation(CharMesh->GetRelativeRotation());
		CloneMesh->SetRelativeScale3D(CharMesh->GetRelativeScale3D());
		
		// Disable shadows to avoid artifacts with translucent meshes
		CloneMesh->SetCastShadow(false);
		CloneMesh->bCastDynamicShadow = false;
		CloneMesh->bAffectDistanceFieldLighting = false;
		
		// Ensure proper occlusion - clone should be hidden behind walls
		CloneMesh->bRenderInMainPass = true;
		CloneMesh->bRenderInDepthPass = true;
		
		CloneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CloneMesh->RegisterComponent();

		// Apply translucent material
		if (CloneGhostMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(CloneGhostMaterial, CloneActor);
			DynMat->SetScalarParameterValue(FName("Opacity"), CloneOpacity);
			for (int32 i = 0; i < CloneMesh->GetNumMaterials(); i++)
			{
				CloneMesh->SetMaterial(i, DynMat);
			}
		}
		else
		{
			// If no ghost material provided, try to make existing materials translucent
			for (int32 i = 0; i < CloneMesh->GetNumMaterials(); i++)
			{
				UMaterialInterface* OrigMat = CharMesh->GetMaterial(i);
				if (OrigMat)
				{
					UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(OrigMat, CloneActor);
					DynMat->SetScalarParameterValue(FName("Opacity"), CloneOpacity);
					CloneMesh->SetMaterial(i, DynMat);
				}
			}
		}
	}
	else
	{
		// Fallback: use a capsule-shaped placeholder
		UStaticMeshComponent* CloneVisual = NewObject<UStaticMeshComponent>(CloneActor, TEXT("CloneVisual"));
		CloneVisual->SetupAttachment(RootComp);
		CloneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CloneVisual->SetCastShadow(false);
		CloneVisual->bCastDynamicShadow = false;
		
		// Load cylinder mesh as placeholder
		UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh)
		{
			CloneVisual->SetStaticMesh(CylinderMesh);
		}
		
		// Scale to approximate character size
		UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
		if (Capsule)
		{
			float Radius = Capsule->GetScaledCapsuleRadius();
			float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			// Cylinder default is 50 radius, 100 height
			CloneVisual->SetRelativeScale3D(FVector(Radius / 50.0f, Radius / 50.0f, HalfHeight / 50.0f));
			// Offset downward so it sits on ground like the capsule
			CloneVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight));
		}
		
		// Make translucent
		if (CloneGhostMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(CloneGhostMaterial, CloneActor);
			DynMat->SetScalarParameterValue(FName("Opacity"), CloneOpacity);
			CloneVisual->SetMaterial(0, DynMat);
		}
		
		CloneVisual->RegisterComponent();
	}

	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple,
			FString::Printf(TEXT("Clone: Visual spawned at %s"), *CloneLocation.ToString()));
	}
}

void UCloneComponent::PlayVFXAtLocation(UNiagaraSystem* System, const FVector& Location)
{
	if (!System) return;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		System,
		Location,
		FRotator::ZeroRotator,
		FVector(1.0f),
		true,
		true
	);
}

void UCloneComponent::PlaySFXAtLocation(USoundBase* Sound, const FVector& Location)
{
	if (!Sound) return;

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
}

void UCloneComponent::OnCloneTimerExpired()
{
	if (bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Clone: Expired!"));
	}

	// Play a small VFX at clone location before destroying
	PlayVFXAtLocation(CloneSpawnVFX, CloneLocation);

	OnCloneExpired.Broadcast();
	DestroyClone();
}
