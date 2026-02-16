// SairanSkies - Targeting Component Implementation (Arkham-style)

#include "Combat/TargetingComponent.h"
#include "Character/SairanCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update snap movement if in progress
	if (bIsSnapping)
	{
		UpdateSnapMovement(DeltaTime);
	}
}

AActor* UTargetingComponent::FindBestTarget()
{
	TArray<AActor*> ValidTargets = GetAllTargetsInRange();
	
	if (ValidTargets.Num() == 0)
	{
		CurrentTarget = nullptr;
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (AActor* Target : ValidTargets)
	{
		float Score = CalculateTargetScore(Target);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Target;
		}
	}

	CurrentTarget = BestTarget;
	return BestTarget;
}

TArray<AActor*> UTargetingComponent::GetAllTargetsInRange()
{
	TArray<AActor*> ValidTargets;
	
	if (!OwnerCharacter) return ValidTargets;

	// Sphere overlap to find all potential targets
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();

	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		OwnerLocation,
		OwnerLocation,
		TargetingRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && IsValidTarget(HitActor) && HasLineOfSightToTarget(HitActor))
			{
				ValidTargets.AddUnique(HitActor);
			}
		}
	}

	return ValidTargets;
}

bool UTargetingComponent::IsValidTarget(AActor* PotentialTarget) const
{
	if (!PotentialTarget) return false;
	if (PotentialTarget == OwnerCharacter) return false;

	// Check if has Enemy tag
	if (!PotentialTarget->ActorHasTag(EnemyTag))
	{
		return false;
	}

	// Check if target is alive (has health component or similar)
	// For now, we just check if it exists and isn't pending kill
	if (!IsValid(PotentialTarget))
	{
		return false;
	}

	return true;
}

bool UTargetingComponent::HasLineOfSightToTarget(AActor* Target) const
{
	if (!OwnerCharacter || !Target) return false;

	FVector Start = OwnerCharacter->GetActorLocation() + FVector(0, 0, 50); // Eye level
	FVector End = Target->GetActorLocation() + FVector(0, 0, 50);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(Target);

	bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// If nothing blocked the trace, we have line of sight
	return !bBlocked;
}

float UTargetingComponent::CalculateTargetScore(AActor* Target) const
{
	if (!OwnerCharacter || !Target) return -FLT_MAX;

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	
	// Calculate distance score (closer = higher score)
	float Distance = FVector::Dist(OwnerLocation, TargetLocation);
	float NormalizedDistance = FMath::Clamp(Distance / TargetingRadius, 0.0f, 1.0f);
	float DistanceScore = 1.0f - NormalizedDistance; // 0 to 1, higher for closer

	// Calculate direction score based on input direction or facing direction
	FVector InputDirection = OwnerCharacter->GetMovementInputDirection();
	FVector ToTarget = (TargetLocation - OwnerLocation).GetSafeNormal2D();
	
	float DotProduct = FVector::DotProduct(InputDirection.GetSafeNormal2D(), ToTarget);
	
	// Filter out targets that are too far behind
	if (DotProduct < MinDirectionDot)
	{
		return -FLT_MAX;
	}

	// Normalize dot product to 0-1 range
	float DirectionScore = (DotProduct + 1.0f) / 2.0f; // -1 to 1 becomes 0 to 1

	// Combine scores with weighting
	float FinalScore = (DistanceScore * (1.0f - DirectionWeight)) + (DirectionScore * DirectionWeight);

	return FinalScore;
}

void UTargetingComponent::SnapToTarget(AActor* Target)
{
	if (!OwnerCharacter || !Target) return;

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	float Distance = FVector::Dist2D(OwnerLocation, TargetLocation); // Use 2D distance for horizontal snap

	// Don't snap if too far horizontally or already close enough
	if (Distance > MaxSnapDistance || Distance < SnapStopDistance)
	{
		// Just rotate to face target
		FVector ToTarget = (TargetLocation - OwnerLocation).GetSafeNormal2D();
		FRotator TargetRotation = ToTarget.Rotation();
		OwnerCharacter->SetActorRotation(FRotator(0, TargetRotation.Yaw, 0));
		return;
	}

	// Calculate snap end position (stop before reaching target)
	FVector ToTarget2D = (TargetLocation - OwnerLocation);
	ToTarget2D.Z = 0; // Zero out Z for horizontal direction
	ToTarget2D.Normalize();
	
	FVector SnapDestination = TargetLocation - (ToTarget2D * SnapStopDistance);

	// Find the ground at the snap destination to avoid clipping through floor
	// This is important when snapping from the air
	FHitResult GroundHit;
	FVector TraceStart = SnapDestination + FVector(0, 0, 200.0f); // Start above
	FVector TraceEnd = SnapDestination - FVector(0, 0, 500.0f);   // Trace down
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(Target);
	
	bool bHitGround = GetWorld()->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bHitGround)
	{
		// Place the character on the ground (accounting for capsule half-height)
		float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SnapDestination.Z = GroundHit.ImpactPoint.Z + CapsuleHalfHeight;
	}
	else
	{
		// Fallback: use target's Z level if no ground found
		SnapDestination.Z = TargetLocation.Z;
	}

	// Initialize snap
	bIsSnapping = true;
	SnapStartLocation = OwnerLocation;
	SnapEndLocation = SnapDestination;
	SnapElapsedTime = 0.0f;
	SnapTargetRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, TargetLocation);
	SnapTargetRotation.Pitch = 0.0f;
	SnapTargetRotation.Roll = 0.0f;

	// Disable movement during snap
	OwnerCharacter->GetCharacterMovement()->DisableMovement();
}

void UTargetingComponent::UpdateSnapMovement(float DeltaTime)
{
	if (!OwnerCharacter) return;

	SnapElapsedTime += DeltaTime;
	float Alpha = FMath::Clamp(SnapElapsedTime / SnapDuration, 0.0f, 1.0f);

	// Use ease out curve for smooth deceleration
	float EasedAlpha = FMath::Sin(Alpha * PI * 0.5f);

	// Interpolate position
	FVector NewLocation = FMath::Lerp(SnapStartLocation, SnapEndLocation, EasedAlpha);
	OwnerCharacter->SetActorLocation(NewLocation);

	// Interpolate rotation
	FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
	FRotator NewRotation = FMath::Lerp(CurrentRotation, SnapTargetRotation, EasedAlpha);
	OwnerCharacter->SetActorRotation(NewRotation);

	// Check if snap is complete
	if (Alpha >= 1.0f)
	{
		bIsSnapping = false;
		
		// Re-enable movement - set to Walking mode which resets grounded state
		UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
		if (MovementComp)
		{
			MovementComp->SetMovementMode(MOVE_Walking);
			
			// Ensure velocity is zeroed to avoid sliding
			MovementComp->Velocity = FVector::ZeroVector;
		}
		
		// Reset jump count so player can jump again after landing from snap
		OwnerCharacter->CurrentJumpCount = 0;
	}
}
