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
#include "Enemies/EnemyBase.h"

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

	// Perform hit detection while attacking
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

void UCombatComponent::OnWeaponHitDetected(AActor* HitActor, const FVector& HitLocation)
{
	// Only process hits when hit detection is enabled
	if (!bHitDetectionEnabled || !OwnerCharacter || !HitActor) return;

	// Don't hit self
	if (HitActor == OwnerCharacter) return;

	// Don't hit same actor twice in one attack
	if (HitActorsThisAttack.Contains(HitActor)) return;

	// Mark as hit
	HitActorsThisAttack.Add(HitActor);

	// Apply damage
	float Damage = GetDamageForAttackType(CurrentAttackType);
	ApplyDamageToTarget(HitActor, Damage, HitLocation);
}

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

	// Calculate hit detection position - in front of character, elevated to avoid ground
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector ForwardOffset = OwnerCharacter->GetActorForwardVector() * HitDetectionForwardOffset;
	FVector HeightOffset = FVector(0, 0, HitDetectionHeightOffset);
	
	FVector TraceStart = CharacterLocation + ForwardOffset + HeightOffset;

	// Sphere trace for enemies
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	if (OwnerCharacter->EquippedWeapon)
	{
		ActorsToIgnore.Add(Cast<AActor>(OwnerCharacter->EquippedWeapon));
	}

	// Debug visualization
	EDrawDebugTrace::Type DebugType = bShowHitDebug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart,
		TraceStart, // Same point - we just want sphere overlap at this position
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

				// Check if actor has Enemy tag
				if (!HitActor->ActorHasTag(FName("Enemy"))) continue;

				// Mark as hit so we don't hit same actor twice per attack
				HitActorsThisAttack.Add(HitActor);

				// Calculate hit location
				FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? HitActor->GetActorLocation() : FVector(Hit.ImpactPoint);

				// Apply damage
				float Damage = GetDamageForAttackType(CurrentAttackType);
				ApplyDamageToTarget(HitActor, Damage, HitLocation);
			}
		}
	}
}

float UCombatComponent::GetDamageForAttackType(EAttackType AttackType) const
{
	float BaseDamage = 0.0f;
	switch (AttackType)
	{
	case EAttackType::Light:
		BaseDamage = LightAttackDamage;
		break;
	case EAttackType::Heavy:
		BaseDamage = HeavyAttackDamage;
		break;
	case EAttackType::Charged:
		BaseDamage = ChargedAttackDamage;
		break;
	default:
		return 0.0f;
	}
	return ApplyDamageVariance(BaseDamage);
}

float UCombatComponent::ApplyDamageVariance(float BaseDamage) const
{
	if (DamageVariance <= 0.0f)
	{
		return BaseDamage;
	}
	// Random integer in range [Base - Variance, Base + Variance]
	int32 MinDmg = FMath::RoundToInt(BaseDamage - DamageVariance);
	int32 MaxDmg = FMath::RoundToInt(BaseDamage + DamageVariance);
	return static_cast<float>(FMath::RandRange(MinDmg, MaxDmg));
}

void UCombatComponent::ApplyDamageToTarget(AActor* Target, float Damage, const FVector& HitLocation)
{
	if (!Target || !OwnerCharacter) return;

	// Check if target is an EnemyBase - use custom function that handles particles attached to enemy
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(Target))
	{
		// Use the enemy's TakeDamageAtLocation which spawns particles attached to the enemy
		Enemy->TakeDamageAtLocation(Damage, OwnerCharacter, OwnerCharacter->GetController(), HitLocation);
	}
	else
	{
		// Use Unreal's standard damage system for other actors
		FDamageEvent DamageEvent;
		Target->TakeDamage(Damage, DamageEvent, OwnerCharacter->GetController(), OwnerCharacter);
	}

	// Apply hit feedback effects (camera shake, hitstop, knockback - but NOT particles, enemy handles those)
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

	// 2. Trigger hitstop (brief game pause for impact feel) - intensity based on attack type
	TriggerHitstop(CurrentAttackType);

	// 3. Trigger camera shake - intensity based on attack type
	TriggerCameraShake(CurrentAttackType);

	// 4. Hit particles are now spawned by the enemy (attached to them so they follow knockback)
	// See EnemyBase::TakeDamageAtLocation()

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

void UCombatComponent::TriggerHitstop(EAttackType AttackType)
{
	if (HitstopDuration <= 0.0f) return;

	// Scale duration based on attack type
	float Duration = HitstopDuration;
	switch (AttackType)
	{
		case EAttackType::Heavy:
			Duration = HitstopDuration * 2.0f;
			break;
		case EAttackType::Charged:
			Duration = HitstopDuration * 3.0f;
			break;
		default:
			Duration = HitstopDuration;
			break;
	}

	UE_LOG(LogTemp, Log, TEXT("HITSTOP TRIGGERED - Type: %d, Duration(real): %f"), (int)AttackType, Duration);

	// Pause the game briefly for impact feel
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.01f);

	// IMPORTANT: We need to use a real-time delay because the timer manager
	// is affected by time dilation. With dilation at 0.01, a 0.05s timer 
	// would take 5 real seconds. Instead we use FTimerDelegate with the
	// timer rate adjusted to compensate for the dilation.
	float CurrentDilation = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
	float AdjustedDuration = Duration * CurrentDilation; // Scale down so real-world time is correct

	GetWorld()->GetTimerManager().SetTimer(
		HitstopTimer,
		this,
		&UCombatComponent::ResumeFromHitstop,
		AdjustedDuration,
		false
	);
}

void UCombatComponent::ResumeFromHitstop()
{
	if (bIsPerfectParryHitstop)
	{
		bIsPerfectParryHitstop = false;
		StartPerfectParrySlowMo();
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	UE_LOG(LogTemp, Log, TEXT("HITSTOP ENDED - Game speed resumed"));
}

void UCombatComponent::StartPerfectParrySlowMo()
{
	float Dilation = FMath::Clamp(PerfectParrySlowMoTimeDilation, 0.01f, 1.0f);
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), Dilation);

	UE_LOG(LogTemp, Warning, TEXT("PERFECT PARRY SLOW-MO START - TimeDilation: %f, RealDuration: %f"), Dilation, PerfectParrySlowMoDuration);

	// Timer manager runs in game-time, so we must compensate:
	// We want PerfectParrySlowMoDuration in REAL seconds.
	// Game-time runs at (Dilation) speed, so game-timer of X fires after X/Dilation real seconds.
	// To get the timer to fire after PerfectParrySlowMoDuration real seconds:
	//   GameTimerValue = PerfectParrySlowMoDuration * Dilation
	float AdjustedDuration = PerfectParrySlowMoDuration * Dilation;

	GetWorld()->GetTimerManager().SetTimer(
		PerfectParrySlowMoTimer,
		this,
		&UCombatComponent::EndPerfectParrySlowMo,
		AdjustedDuration,
		false
	);
}

void UCombatComponent::EndPerfectParrySlowMo()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	UE_LOG(LogTemp, Warning, TEXT("PERFECT PARRY SLOW-MO ENDED - Game speed fully restored"));
}

void UCombatComponent::TriggerCameraShake(EAttackType AttackType)
{
	// Scale shake intensity based on attack type
	float Intensity = CameraShakeIntensity;
	switch (AttackType)
	{
		case EAttackType::Heavy:
			Intensity = CameraShakeIntensity * 1.5f; // 1.5x for heavy attacks
			break;
		case EAttackType::Charged:
			Intensity = CameraShakeIntensity * 2.5f; // 2.5x for charged attacks
			break;
		default:
			Intensity = CameraShakeIntensity;
			break;
	}

	if (!OwnerCharacter || Intensity <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraShake failed - OwnerCharacter: %s, Intensity: %f"), 
			OwnerCharacter ? TEXT("Valid") : TEXT("NULL"), Intensity);
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraShake failed - No PlayerController found"));
		return;
	}

	if (HitCameraShake)
	{
		// Use custom camera shake if assigned
		PC->ClientStartCameraShake(HitCameraShake, Intensity);
		UE_LOG(LogTemp, Log, TEXT("📹 CAMERA SHAKE TRIGGERED - Type: %d, Intensity: %f"), (int)AttackType, Intensity);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ NO HitCameraShake assigned! Create a CameraShake Blueprint and assign it to CombatComponent."));
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

// ========== INCOMING DAMAGE / PARRY SYSTEM ==========

bool UCombatComponent::HandleIncomingDamage(float IncomingDamage, AActor* Attacker, float& OutDamageApplied)
{
	if (!OwnerCharacter)
	{
		OutDamageApplied = IncomingDamage;
		return false;
	}

	// Perfect parry (Sekiro deflect) - within parry window
	if (bIsInParryWindow)
	{
		OutDamageApplied = 0.0f;
		PlayParryFeedback(true);
		OnParrySuccess.Broadcast();
		
		UE_LOG(LogTemp, Log, TEXT("PERFECT PARRY! Damage fully deflected."));
		return true;
	}

	// Normal block - holding block but outside parry window
	if (bIsHoldingBlock)
	{
		// Partial damage - 30% gets through the block
		OutDamageApplied = IncomingDamage * 0.3f;
		PlayParryFeedback(false);
		OnBlockPerformed.Broadcast(IncomingDamage - OutDamageApplied, false);
		
		UE_LOG(LogTemp, Log, TEXT("BLOCK! Reduced damage from %.1f to %.1f"), IncomingDamage, OutDamageApplied);
		return false;
	}

	// No block at all
	OutDamageApplied = IncomingDamage;
	return false;
}

void UCombatComponent::PlayParryFeedback(bool bPerfectParry)
{
	if (!OwnerCharacter) return;

	FVector FeedbackLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 80.0f;
	FeedbackLocation.Z += 50.0f; // Slightly above center

	if (bPerfectParry)
	{
		// Perfect parry - Sekiro-style clang
		if (ParryDeflectVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), ParryDeflectVFX, FeedbackLocation,
				FRotator::ZeroRotator, FVector(1.0f), true, true);
		}
		if (ParryDeflectSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ParryDeflectSound, FeedbackLocation);
		}

		// Trigger a strong camera shake for perfect parry
		TriggerCameraShake(EAttackType::Heavy);

		// God of War style: hitstop freeze -> slow-motion window
		bIsPerfectParryHitstop = true;
		if (PerfectParryHitstopDuration > 0.0f)
		{
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.01f);

			// Compensate timer for time dilation (same fix as TriggerHitstop)
			float AdjustedDuration = PerfectParryHitstopDuration * 0.01f;

			GetWorld()->GetTimerManager().SetTimer(
				HitstopTimer,
				this,
				&UCombatComponent::ResumeFromHitstop,
				AdjustedDuration,
				false
			);
			
			UE_LOG(LogTemp, Warning, TEXT("PERFECT PARRY HITSTOP - Freeze: %fs (adjusted: %fs), then SlowMo: %fs at %f dilation"),
				PerfectParryHitstopDuration, AdjustedDuration, PerfectParrySlowMoDuration, PerfectParrySlowMoTimeDilation);
		}
		else
		{
			// Skip hitstop, go straight to slow-mo
			StartPerfectParrySlowMo();
		}
	}
	else
	{
		// Normal block
		if (BlockVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), BlockVFX, FeedbackLocation,
				FRotator::ZeroRotator, FVector(1.0f), true, true);
		}
		if (BlockSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), BlockSound, FeedbackLocation);
		}
	}
}

