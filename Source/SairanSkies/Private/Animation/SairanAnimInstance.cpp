// SairanSkies - Custom Animation Instance Implementation

#include "Animation/SairanAnimInstance.h"
#include "Character/SairanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USairanAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ASairanCharacter>(TryGetPawnOwner());
}

void USairanAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter) return;

	// Get velocity
	FVector Velocity = OwnerCharacter->GetVelocity();
	Speed = Velocity.Size2D();

	// Calculate direction relative to character facing
	if (Speed > 0.0f)
	{
		FRotator ActorRotation = OwnerCharacter->GetActorRotation();
		Direction = UKismetMathLibrary::NormalizedDeltaRotator(
			Velocity.ToOrientationRotator(),
			ActorRotation
		).Yaw;
	}

	// Air state
	bIsInAir = OwnerCharacter->GetCharacterMovement()->IsFalling();
	VerticalVelocity = Velocity.Z;

	// Sprint state
	bIsSprinting = OwnerCharacter->bIsSprinting;

	// Combat state
	if (OwnerCharacter->CombatComponent)
	{
		bIsAttacking = OwnerCharacter->CombatComponent->IsAttacking();
	}

	// Weapon state
	bIsWeaponDrawn = OwnerCharacter->bIsWeaponDrawn;
}
