// SairanSkies - Weapon Lerp Component Implementation

#include "Weapons/WeaponLerpComponent.h"
#include "Character/SairanCharacter.h"
#include "Weapons/WeaponBase.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UWeaponLerpComponent::UWeaponLerpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponLerpComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	Weapon = OwnerCharacter->EquippedWeapon;

	// Gather scene components from character
	IdlePoint = OwnerCharacter->WeaponIdlePoint;

	LightAttackPoints.Empty();
	if (OwnerCharacter->LightAttackPoint1) LightAttackPoints.Add(OwnerCharacter->LightAttackPoint1);
	if (OwnerCharacter->LightAttackPoint2) LightAttackPoints.Add(OwnerCharacter->LightAttackPoint2);
	if (OwnerCharacter->LightAttackPoint3) LightAttackPoints.Add(OwnerCharacter->LightAttackPoint3);
	if (OwnerCharacter->LightAttackPoint4) LightAttackPoints.Add(OwnerCharacter->LightAttackPoint4);
	if (OwnerCharacter->LightAttackPoint5) LightAttackPoints.Add(OwnerCharacter->LightAttackPoint5);

	HeavyAttackPoints.Empty();
	if (OwnerCharacter->HeavyAttackPoint1) HeavyAttackPoints.Add(OwnerCharacter->HeavyAttackPoint1);
	if (OwnerCharacter->HeavyAttackPoint2) HeavyAttackPoints.Add(OwnerCharacter->HeavyAttackPoint2);

	// Initialize target to idle
	if (IdlePoint)
	{
		TargetLocation = IdlePoint->GetComponentLocation();
		TargetRotation = IdlePoint->GetComponentRotation();
	}
}

void UWeaponLerpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !Weapon) 
	{
		// Try to get weapon reference if it wasn't available at BeginPlay
		if (OwnerCharacter && !Weapon)
		{
			Weapon = OwnerCharacter->EquippedWeapon;
		}
		return;
	}

	// Track inactivity for combo reset
	if (bIsActive && CurrentLerpState != EWeaponLerpState::HeavyCharging)
	{
		TimeSinceLastAttack += DeltaTime;
		if (TimeSinceLastAttack >= ComboInactivityTimeout)
		{
			ReturnToIdle();
		}
	}

	// Update lerp
	if (CurrentLerpState != EWeaponLerpState::Idle)
	{
		UpdateLerp(DeltaTime);

		// Check phase transitions for heavy attack
		if (CurrentLerpState == EWeaponLerpState::HeavyAttacking && HasReachedTarget())
		{
			if (HeavyPhase == 0 && HeavyAttackPoints.Num() > 1)
			{
				// First point reached, go to second
				HeavyPhase = 1;
				SetLerpTarget(HeavyAttackPoints[1], HeavyAttackSnapSpeed);
			}
		}
		else if (CurrentLerpState == EWeaponLerpState::HeavyCharging && HasReachedTarget())
		{
			if (HeavyPhase == 0 && HeavyAttackPoints.Num() > 1)
			{
				// Slow charge reached first point, wait for release
				// Stay here until ReleaseHeavyCharge is called
			}
		}
		else if (CurrentLerpState == EWeaponLerpState::ReturningToIdle && HasReachedTarget())
		{
			CurrentLerpState = EWeaponLerpState::Idle;
			bIsActive = false;
			CurrentLightComboIndex = 0;
		}
	}
}

void UWeaponLerpComponent::StartLightAttackLerp(int32 ComboIndex)
{
	if (LightAttackPoints.Num() == 0) return;

	bIsActive = true;
	TimeSinceLastAttack = 0.0f;
	CurrentLerpState = EWeaponLerpState::LightAttacking;

	// Wrap combo index
	CurrentLightComboIndex = ComboIndex % LightAttackPoints.Num();

	SetLerpTarget(LightAttackPoints[CurrentLightComboIndex], LightAttackLerpSpeed);
}

void UWeaponLerpComponent::StartHeavyAttackLerp()
{
	if (HeavyAttackPoints.Num() == 0) return;

	bIsActive = true;
	TimeSinceLastAttack = 0.0f;
	CurrentLerpState = EWeaponLerpState::HeavyAttacking;
	HeavyPhase = 0;

	SetLerpTarget(HeavyAttackPoints[0], HeavyAttackSnapSpeed);
}

void UWeaponLerpComponent::StartHeavyChargeLerp()
{
	if (HeavyAttackPoints.Num() == 0) return;

	bIsActive = true;
	TimeSinceLastAttack = 0.0f;
	CurrentLerpState = EWeaponLerpState::HeavyCharging;
	HeavyPhase = 0;

	SetLerpTarget(HeavyAttackPoints[0], HeavyChargeLerpSpeed);
}

void UWeaponLerpComponent::ReleaseHeavyCharge()
{
	if (HeavyAttackPoints.Num() < 2) return;

	TimeSinceLastAttack = 0.0f;
	CurrentLerpState = EWeaponLerpState::HeavyAttacking;
	HeavyPhase = 1;

	SetLerpTarget(HeavyAttackPoints[1], HeavyAttackSnapSpeed);
}

void UWeaponLerpComponent::ReturnToIdle()
{
	if (CurrentLerpState == EWeaponLerpState::Idle) return;
	if (!IdlePoint) return;

	CurrentLerpState = EWeaponLerpState::ReturningToIdle;
	CurrentLightComboIndex = 0;
	TimeSinceLastAttack = 0.0f;

	SetLerpTarget(IdlePoint, ReturnToIdleLerpSpeed);
}

void UWeaponLerpComponent::ForceIdle()
{
	CurrentLerpState = EWeaponLerpState::Idle;
	CurrentLightComboIndex = 0;
	TimeSinceLastAttack = 0.0f;
	bIsActive = false;

	if (IdlePoint && Weapon)
	{
		ApplyWeaponTransform(IdlePoint->GetComponentLocation(), IdlePoint->GetComponentRotation());
	}
}

void UWeaponLerpComponent::SetLerpTarget(USceneComponent* TargetPoint, float Speed)
{
	if (!TargetPoint || !Weapon) return;

	StartLocation = Weapon->GetActorLocation();
	StartRotation = Weapon->GetActorRotation();
	TargetLocation = TargetPoint->GetComponentLocation();
	TargetRotation = TargetPoint->GetComponentRotation();
	CurrentLerpSpeed = Speed;
	LerpAlpha = 0.0f;
}

void UWeaponLerpComponent::UpdateLerp(float DeltaTime)
{
	if (!Weapon) return;

	LerpAlpha += DeltaTime * CurrentLerpSpeed;
	LerpAlpha = FMath::Clamp(LerpAlpha, 0.0f, 1.0f);

	// Smooth step for more natural feel
	float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, LerpAlpha);

	FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, SmoothedAlpha);
	FRotator NewRotation = FMath::Lerp(StartRotation, TargetRotation, SmoothedAlpha);

	ApplyWeaponTransform(NewLocation, NewRotation);
}

void UWeaponLerpComponent::ApplyWeaponTransform(const FVector& Location, const FRotator& Rotation)
{
	if (!Weapon) return;

	// Only apply if weapon is drawn (in hand) and not in blocking stance
	if (Weapon->CurrentState != EWeaponState::Drawn && Weapon->CurrentState != EWeaponState::Attacking)
	{
		return;
	}

	if (Weapon->IsInBlockingStance())
	{
		return;
	}

	Weapon->SetActorLocation(Location);
	Weapon->SetActorRotation(Rotation);
}

bool UWeaponLerpComponent::HasReachedTarget() const
{
	return LerpAlpha >= 0.99f;
}

