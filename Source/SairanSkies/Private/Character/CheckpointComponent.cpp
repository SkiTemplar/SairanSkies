// SairanSkies - Checkpoint Component Implementation

#include "Character/CheckpointComponent.h"
#include "Character/SairanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"

UCheckpointComponent::UCheckpointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCheckpointComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());

	// Initialize checkpoint to spawn location
	if (OwnerCharacter)
	{
		LastSafeLocation = OwnerCharacter->GetActorLocation();
		LastSafeRotation = OwnerCharacter->GetActorRotation();
		bHasValidCheckpoint = true;
	}
}

void UCheckpointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement())
	{
		return;
	}

	// Only save when grounded
	if (!OwnerCharacter->GetCharacterMovement()->IsFalling())
	{
		SaveTimer += DeltaTime;
		if (SaveTimer >= SaveInterval)
		{
			SaveTimer = 0.0f;

			FVector CurrentLocation = OwnerCharacter->GetActorLocation();
			
			// Only save if we've moved enough from last save
			if (!bHasValidCheckpoint || FVector::Dist(CurrentLocation, LastSafeLocation) >= MinSaveDistance)
			{
				LastSafeLocation = CurrentLocation;
				LastSafeRotation = OwnerCharacter->GetActorRotation();
				bHasValidCheckpoint = true;

				if (bShowDebug)
				{
					DrawDebugSphere(GetWorld(), LastSafeLocation, 30.0f, 8, FColor::Green, false, SaveInterval * 2.0f);
				}
			}
		}
	}
}

void UCheckpointComponent::RespawnAtLastCheckpoint()
{
	if (!OwnerCharacter || !bHasValidCheckpoint)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Checkpoint: Respawning at %s"), *LastSafeLocation.ToString());

	// Play VFX at respawn location
	if (RespawnVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), RespawnVFX, LastSafeLocation,
			FRotator::ZeroRotator, FVector(1.0f), true, true);
	}

	// Play respawn sound
	if (RespawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), RespawnSound, LastSafeLocation);
	}

	// Teleport player to last safe position
	OwnerCharacter->SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	OwnerCharacter->SetActorRotation(LastSafeRotation);

	// Reset velocity
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}

	// Reset jump count
	OwnerCharacter->CurrentJumpCount = 0;

	// Broadcast event
	OnRespawn.Broadcast(LastSafeLocation);
}
