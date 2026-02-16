// SairanSkies - Combat Component Implementation

#include "Combat/CombatComponent.h"
#include "Character/SairanCharacter.h"
#include "Combat/TargetingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapons/WeaponBase.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraShakeBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update charge time when holding heavy attack
	if (bIsChargingAttack)
	{
		CurrentChargeTime += DeltaTime;
		// Cap at max charge time
		CurrentChargeTime = FMath::Min(CurrentChargeTime, ChargeTimeForMaxDamage);
	}

	// Perform hit detection while enabled
	if (bHitDetectionEnabled)
	{
		PerformHitDetection();
	}
}

void UCombatComponent::LightAttack()
{
	if (!OwnerCharacter) return;

	// If already attacking, buffer the input
	if (bIsAttacking)
	{
		bInputBuffered = true;
		BufferedAttackType = EAttackType::Light;
		return;
	}

	ExecuteAttack(EAttackType::Light);
}

void UCombatComponent::StartHeavyAttack()
{
	if (!OwnerCharacter) return;

	// If already attacking, buffer the input
	if (bIsAttacking)
	{
		bInputBuffered = true;
		BufferedAttackType = EAttackType::Heavy;
		return;
	}

	// Start charging
	bIsChargingAttack = true;
	CurrentChargeTime = 0.0f;
}

void UCombatComponent::ReleaseHeavyAttack()
{
	if (!OwnerCharacter || !bIsChargingAttack) return;

	bIsChargingAttack = false;

	// Determine attack type based on charge time
	EAttackType AttackType = (CurrentChargeTime >= ChargeTimeForMaxDamage) ? EAttackType::Charged : EAttackType::Heavy;
	
	ExecuteAttack(AttackType);
	CurrentChargeTime = 0.0f;
}

void UCombatComponent::ExecuteAttack(EAttackType AttackType)
{
	if (!OwnerCharacter) return;

	bIsAttacking = true;
	CurrentAttackType = AttackType;
	HitActorsThisAttack.Empty();

	// Set character state
	OwnerCharacter->SetCharacterState(ECharacterState::Attacking);

	// Find target and snap to it (Arkham style)
	if (OwnerCharacter->TargetingComponent)
	{
		AActor* Target = OwnerCharacter->TargetingComponent->FindBestTarget();
		if (Target)
		{
			OwnerCharacter->TargetingComponent->SnapToTarget(Target);
		}
	}

	// Select and play animation montage
	UAnimMontage* MontageToPlay = nullptr;
	float AttackDuration = 0.5f; // Default duration for placeholder

	switch (AttackType)
	{
	case EAttackType::Light:
		if (LightAttackMontages.Num() > 0)
		{
			int32 MontageIndex = CurrentComboCount % LightAttackMontages.Num();
			MontageToPlay = LightAttackMontages[MontageIndex];
		}
		AttackDuration = 0.4f;
		break;

	case EAttackType::Heavy:
		MontageToPlay = HeavyAttackMontage;
		AttackDuration = 0.6f;
		break;

	case EAttackType::Charged:
		MontageToPlay = ChargedAttackMontage;
		AttackDuration = 0.8f;
		break;

	default:
		break;
	}

	// Play montage if available
	if (MontageToPlay && OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		float PlayRate = 1.0f;
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(MontageToPlay, PlayRate);
		AttackDuration = MontageToPlay->GetPlayLength() / PlayRate;
	}

	// Enable hit detection
	EnableHitDetection();

	// Broadcast event
	OnAttackPerformed.Broadcast(AttackType);

	// Increment combo for light attacks
	if (AttackType == EAttackType::Light)
	{
		IncrementCombo();
	}
	else
	{
		ResetCombo();
	}

	// Set timer to end attack
	GetWorld()->GetTimerManager().SetTimer(AttackEndTimer, this, &UCombatComponent::EndAttack, AttackDuration, false);
}

void UCombatComponent::EndAttack()
{
	bIsAttacking = false;
	CurrentAttackType = EAttackType::None;
	DisableHitDetection();

	if (OwnerCharacter)
	{
		OwnerCharacter->SetCharacterState(ECharacterState::Idle);
	}

	// Process buffered input
	ProcessBufferedInput();
}

void UCombatComponent::ProcessBufferedInput()
{
	if (bInputBuffered)
	{
		bInputBuffered = false;
		EAttackType BufferedType = BufferedAttackType;
		BufferedAttackType = EAttackType::None;

		// Execute buffered attack
		if (BufferedType == EAttackType::Light)
		{
			LightAttack();
		}
		else if (BufferedType == EAttackType::Heavy)
		{
			ExecuteAttack(EAttackType::Heavy);
		}
	}
}

void UCombatComponent::PerformParry()
{
	if (!OwnerCharacter || !bCanParry || bIsAttacking) return;

	bCanParry = false;
	bIsInParryWindow = true;

	// Set character state
	OwnerCharacter->SetCharacterState(ECharacterState::Parrying);

	// Play parry animation if available
	if (ParryMontage && OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(ParryMontage);
	}

	// Broadcast parry window start
	OnParryWindow.Broadcast();

	// End parry window after duration
	GetWorld()->GetTimerManager().SetTimer(ParryWindowTimer, this, &UCombatComponent::EndParryWindow, ParryWindowDuration, false);

	// Start parry cooldown
	GetWorld()->GetTimerManager().SetTimer(ParryCooldownTimer, this, &UCombatComponent::ResetParryCooldown, ParryCooldown, false);
}

void UCombatComponent::EndParryWindow()
{
	bIsInParryWindow = false;
	
	if (OwnerCharacter && OwnerCharacter->CurrentState == ECharacterState::Parrying)
	{
		OwnerCharacter->SetCharacterState(ECharacterState::Idle);
	}
}

void UCombatComponent::ResetParryCooldown()
{
	bCanParry = true;
}

// ========== COMBO SYSTEM ==========

void UCombatComponent::ResetCombo()
{
	CurrentComboCount = 0;
	GetWorld()->GetTimerManager().ClearTimer(ComboResetTimer);
}

void UCombatComponent::IncrementCombo()
{
	CurrentComboCount++;
	
	// Reset combo if max reached
	if (CurrentComboCount >= MaxLightCombo)
	{
		// Delay reset to allow for combo finisher
		GetWorld()->GetTimerManager().SetTimer(ComboResetTimer, this, &UCombatComponent::ResetCombo, ComboResetTime, false);
	}
	else
	{
		// Reset timer for combo continuation
		GetWorld()->GetTimerManager().SetTimer(ComboResetTimer, this, &UCombatComponent::ResetCombo, ComboResetTime, false);
	}
}

// ========== HIT DETECTION ==========

void UCombatComponent::EnableHitDetection()
{
	bHitDetectionEnabled = true;
	HitActorsThisAttack.Empty();
	bHitLandedThisAttack = false;
}

void UCombatComponent::DisableHitDetection()
{
	bHitDetectionEnabled = false;
}

void UCombatComponent::PerformHitDetection()
{
	if (!OwnerCharacter) return;

	// Calculate hit detection sphere position (in front of character)
	FVector TraceStart = OwnerCharacter->GetActorLocation() + 
						 OwnerCharacter->GetActorForwardVector() * HitDetectionForwardOffset;

	// Sphere trace for enemies
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	if (OwnerCharacter->EquippedWeapon)
	{
		ActorsToIgnore.Add(Cast<AActor>(OwnerCharacter->EquippedWeapon));
	}

	// Only draw debug once per attack, not every frame
	EDrawDebugTrace::Type DebugType = bShowHitDebug && !bHitLandedThisAttack ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart,
		TraceStart, // Same point - we just want sphere overlap
		HitDetectionRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		DebugType,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !HitActorsThisAttack.Contains(HitActor))
			{
				// Don't hit self
				if (HitActor == OwnerCharacter) continue;

				// Mark as hit so we don't hit same actor twice per attack
				HitActorsThisAttack.Add(HitActor);

				// Calculate hit location (where the weapon would impact)
				FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? FVector(HitActor->GetActorLocation()) : FVector(Hit.ImpactPoint);

				// Apply damage
				float Damage = GetDamageForAttackType(CurrentAttackType);
				ApplyDamageToTarget(HitActor, Damage, HitLocation);
			}
		}
	}
}

float UCombatComponent::GetDamageForAttackType(EAttackType AttackType) const
{
	switch (AttackType)
	{
	case EAttackType::Light:
		return LightAttackDamage;
	case EAttackType::Heavy:
		return HeavyAttackDamage;
	case EAttackType::Charged:
		return ChargedAttackDamage;
	default:
		return 0.0f;
	}
}

void UCombatComponent::ApplyDamageToTarget(AActor* Target, float Damage, const FVector& HitLocation)
{
	if (!Target || !OwnerCharacter) return;

	// Use Unreal's damage system
	FDamageEvent DamageEvent;
	Target->TakeDamage(Damage, DamageEvent, OwnerCharacter->GetController(), OwnerCharacter);

	// Apply hit feedback effects
	ApplyHitFeedback(Target, HitLocation, Damage);

	// Log for debugging
	UE_LOG(LogTemp, Log, TEXT("Applied %.1f damage to %s"), Damage, *Target->GetName());
}

void UCombatComponent::ApplyHitFeedback(AActor* HitActor, const FVector& HitLocation, float Damage)
{
	if (!OwnerCharacter) return;

	bHitLandedThisAttack = true;

	// 1. Apply knockback to enemy
	float KnockbackToApply = (CurrentAttackType == EAttackType::Charged) ? ChargedKnockbackForce : KnockbackForce;
	ApplyKnockback(HitActor, KnockbackToApply);

	// 2. Trigger hitstop (brief game pause for impact feel)
	TriggerHitstop();

	// 3. Trigger camera shake
	TriggerCameraShake();

	// 4. Spawn hit particles (placeholder - assign NiagaraSystem in Blueprint)
	if (HitParticleSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			HitParticleSystem,
			HitLocation,
			(OwnerCharacter->GetActorLocation() - HitLocation).Rotation()
		);
	}

	// 5. Play hit sound (placeholder - assign sound in Blueprint)
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitLocation);
	}

	// 6. Broadcast event for Blueprint effects
	OnHitLanded.Broadcast(HitActor, HitLocation, Damage);
}

void UCombatComponent::ApplyKnockback(AActor* Target, float Force)
{
	if (!Target || !OwnerCharacter) return;

	// Calculate knockback direction (away from player)
	FVector KnockbackDirection = (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
	KnockbackDirection.Z = 0.1f; // Slight upward component
	KnockbackDirection.Normalize();

	// Apply impulse if target has movement component
	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		if (UCharacterMovementComponent* MovementComp = TargetCharacter->GetCharacterMovement())
		{
			// Brief stagger - push the character back
			TargetCharacter->LaunchCharacter(KnockbackDirection * Force, true, false);
		}
	}
	else
	{
		// For non-character actors with physics
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddImpulse(KnockbackDirection * Force, NAME_None, true);
			}
		}
	}
}

void UCombatComponent::TriggerHitstop()
{
	if (HitstopDuration <= 0.0f) return;

	// Pause the game briefly for impact feel
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.01f);

	// Resume after hitstop duration
	GetWorld()->GetTimerManager().SetTimer(
		HitstopTimer,
		this,
		&UCombatComponent::ResumeFromHitstop,
		HitstopDuration * 0.01f, // Timer uses dilated time
		false
	);
}

void UCombatComponent::ResumeFromHitstop()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

void UCombatComponent::TriggerCameraShake()
{
	if (!OwnerCharacter || CameraShakeIntensity <= 0.0f) return;

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;

	if (HitCameraShake)
	{
		// Use custom camera shake if assigned
		PC->ClientStartCameraShake(HitCameraShake, CameraShakeIntensity);
	}
	else
	{
		// Use a simple procedural shake as fallback
		// This requires a default camera shake BP to be assigned in the component
		// For now, we'll just log that no shake is assigned
		UE_LOG(LogTemp, Warning, TEXT("No HitCameraShake assigned to CombatComponent. Assign one in Blueprint for camera shake effect."));
	}
}

// ========== BLOCK/PARRY HOLD FUNCTIONS ==========

void UCombatComponent::StartBlock()
{
	if (!OwnerCharacter || bIsAttacking) return;

	bIsHoldingBlock = true;

	// Put weapon in blocking stance
	if (OwnerCharacter->EquippedWeapon && OwnerCharacter->bIsWeaponDrawn)
	{
		OwnerCharacter->EquippedWeapon->SetBlockingStance(true);
	}

	// Trigger parry if within window
	if (bCanParry)
	{
		PerformParry();
	}
}

void UCombatComponent::ReleaseBlock()
{
	bIsHoldingBlock = false;

	// Return weapon to normal stance
	if (OwnerCharacter && OwnerCharacter->EquippedWeapon)
	{
		OwnerCharacter->EquippedWeapon->SetBlockingStance(false);
	}

	// Also end parry state if still in it
	if (bIsInParryWindow && OwnerCharacter && OwnerCharacter->CurrentState == ECharacterState::Parrying)
	{
		OwnerCharacter->SetCharacterState(ECharacterState::Idle);
	}
}

