// SairanSkies - Weapon Lerp Component Implementation

#include "Weapons/WeaponLerpComponent.h"
#include "Character/SairanCharacter.h"
#include "Weapons/WeaponBase.h"
#include "Components/SceneComponent.h"

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
}

void UWeaponLerpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter) return;

	// Try to get weapon reference if not available yet
	if (!Weapon)
	{
		Weapon = OwnerCharacter->EquippedWeapon;
		if (!Weapon) return;
	}

	// Don't control weapon if not drawn / in hand
	if (Weapon->CurrentState == EWeaponState::Sheathed)
	{
		if (bIsActive)
		{
			ForceIdle();
		}
		return;
	}

	// Don't override blocking stance
	if (Weapon->IsInBlockingStance())
	{
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

	// Advance the lerp
	if (CurrentLerpState != EWeaponLerpState::Idle)
	{
		UpdateLerp(DeltaTime);

		// Phase transitions
		if (HasReachedTarget())
		{
			if (CurrentLerpState == EWeaponLerpState::HeavyAttacking && HeavyPhase == 0 && HeavyAttackPoints.Num() > 1)
			{
				// Reached point 1 (top), now swing down to point 2
				HeavyPhase = 1;
				SetLerpTarget(HeavyAttackPoints[1], HeavyAttackSnapSpeed);
			}
			else if (CurrentLerpState == EWeaponLerpState::HeavyCharging && HeavyPhase == 0)
			{
				// Charge reached top, hold there — wait for ReleaseHeavyCharge()
			}
			else if (CurrentLerpState == EWeaponLerpState::ReturningToIdle)
			{
				CurrentLerpState = EWeaponLerpState::Idle;
				bIsActive = false;
				CurrentLightComboIndex = 0;
			}
		}

		// Apply the interpolated position every frame
		ApplyLerpedTransform(LerpAlpha);
	}
}

// ===================== PUBLIC API =====================

void UWeaponLerpComponent::StartLightAttackLerp(int32 ComboIndex)
{
	if (LightAttackPoints.Num() == 0) return;

	bIsActive = true;
	TimeSinceLastAttack = 0.0f;
	CurrentLerpState = EWeaponLerpState::LightAttacking;
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
	LerpAlpha = 1.0f;
}

// ===================== INTERNALS =====================

void UWeaponLerpComponent::CaptureCurrentAsStart()
{
	if (!Weapon || !OwnerCharacter) return;

	// Convert current weapon world transform to character-root-relative
	FTransform CharRootTF = OwnerCharacter->GetRootComponent()->GetComponentTransform();
	FTransform WeaponTF = Weapon->GetActorTransform();
	FTransform RelativeTF = WeaponTF.GetRelativeTransform(CharRootTF);

	StartRelLoc = RelativeTF.GetLocation();
	StartRelRot = RelativeTF.Rotator();
}

void UWeaponLerpComponent::SetLerpTarget(USceneComponent* TargetPoint, float Speed)
{
	if (!TargetPoint || !OwnerCharacter) return;

	// Capture where the weapon is right now as the start of the lerp
	CaptureCurrentAsStart();

	// Target: the SceneComponent's relative transform IS relative to the character root
	// (since they're SetupAttachment(RootComponent))
	TargetRelLoc = TargetPoint->GetRelativeLocation();
	TargetRelRot = TargetPoint->GetRelativeRotation();

	CurrentLerpSpeed = Speed;
	LerpAlpha = 0.0f;
}

void UWeaponLerpComponent::UpdateLerp(float DeltaTime)
{
	if (LerpAlpha >= 1.0f) return;

	LerpAlpha += DeltaTime * CurrentLerpSpeed;
	LerpAlpha = FMath::Clamp(LerpAlpha, 0.0f, 1.0f);
}

void UWeaponLerpComponent::ApplyLerpedTransform(float Alpha)
{
	if (!Weapon || !OwnerCharacter) return;

	// Smooth step for nice ease-in/ease-out
	float T = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	// Interpolate in character-root-relative space
	FVector LerpedRelLoc = FMath::Lerp(StartRelLoc, TargetRelLoc, T);
	FQuat LerpedRelQuat = FQuat::Slerp(FQuat(StartRelRot), FQuat(TargetRelRot), T);

	// Convert back to world space using the character root's current transform
	FTransform CharRootTF = OwnerCharacter->GetRootComponent()->GetComponentTransform();
	FTransform LerpedRelTF(LerpedRelQuat, LerpedRelLoc);
	FTransform FinalWorldTF = LerpedRelTF * CharRootTF;

	Weapon->SetActorLocationAndRotation(FinalWorldTF.GetLocation(), FinalWorldTF.GetRotation().Rotator());
}

bool UWeaponLerpComponent::HasReachedTarget() const
{
	return LerpAlpha >= 0.99f;
}
