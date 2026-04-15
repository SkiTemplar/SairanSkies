// SairanSkies - Ultimate Laser Attack Component

#include "Character/UltimateComponent.h"
#include "Character/SairanCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

// ─── UUltimateComponent ───────────────────────────────────────────────────────

UUltimateComponent::UUltimateComponent()
{
	// Tick desactivado en reposo; se activa sólo durante el láser
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUltimateComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UUltimateComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLaserActive) return;

	LaserTimer  -= DeltaTime;
	DamageTimer -= DeltaTime;

	if (DamageTimer <= 0.0f)
	{
		DamageTimer = DamageInterval;
		FireLaserTick();
	}

	if (LaserTimer <= 0.0f)
	{
		Deactivate();
	}
}

// ── Interfaz pública ──────────────────────────────────────────────────────────

void UUltimateComponent::AddXP(float Amount)
{
	if (bLaserActive) return;  // no acumular durante el láser

	const float Previous = CurrentXP;
	CurrentXP = FMath::Min(CurrentXP + Amount, MaxXP);

	UE_LOG(LogTemp, Log, TEXT("Ultimate: XP %.0f → %.0f / %.0f%s"),
		Previous, CurrentXP, MaxXP, IsReady() ? TEXT(" [LISTA]") : TEXT(""));

	// Actualizar barra HUD
	if (ASairanCharacter* Char = Cast<ASairanCharacter>(GetOwner()))
	{
		Char->UpdateUltimateHUD();
	}
}

void UUltimateComponent::TryActivate()
{
	if (!IsReady()) return;

	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character || !Character->IsAlive()) return;

	bLaserActive    = true;
	LaserTimer   = LaserDuration;
	DamageTimer  = 0.0f;   // primer tick inmediato

	// Paralizar traslación, permitir rotación con la cámara
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		StoredMaxWalkSpeed       = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed   = 0.0f;
		Movement->bOrientRotationToMovement = false;
	}
	// El personaje mira hacia donde apunta el controlador (cámara)
	Character->bUseControllerRotationYaw = true;

	if (ActivateSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ActivateSound, Character->GetActorLocation());

	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("Ultimate: ¡LÁSER ACTIVADO! (%.1fs, %.1f–%.1f dmg/%.2fs)"),
		LaserDuration, DamageMin, DamageMax, DamageInterval);
}

// ── Privado ───────────────────────────────────────────────────────────────────

void UUltimateComponent::FireLaserTick()
{
	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character) return;

	// Origen: ojo = centro del personaje (origen del actor = 75 u sobre el suelo)
	const FVector Origin = Character->GetActorLocation();

	// Dirección: cámara (permite apuntar en cualquier dirección horizontal/vertical)
	FVector Forward = Character->GetActorForwardVector();
	if (Character->FollowCamera)
	{
		Forward = Character->FollowCamera->GetForwardVector();
	}

	const FVector End = Origin + Forward * LaserRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Pawn, Params);

#if WITH_EDITOR
	DrawDebugLine(GetWorld(), Origin, bHit ? Hit.ImpactPoint : End,
		FColor::Cyan, false, DamageInterval * 1.5f, 0, 3.0f);
	if (bHit)
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 12.0f, 6, FColor::Red, false, DamageInterval * 1.5f);
#endif

	if (bHit && Hit.GetActor())
	{
		const float Damage = FMath::RandRange(DamageMin, DamageMax);
		UGameplayStatics::ApplyDamage(Hit.GetActor(), Damage,
			Character->GetController(), Character, nullptr);

		if (LaserImpactVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LaserImpactVFX,
				Hit.ImpactPoint, FRotator::ZeroRotator, FVector(1.0f), true, true);
		}
	}
}

void UUltimateComponent::Deactivate()
{
	bLaserActive  = false;
	CurrentXP  = 0.0f;
	SetComponentTickEnabled(false);

	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character) return;

	// Restaurar movimiento
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed             = StoredMaxWalkSpeed > 0.0f ? StoredMaxWalkSpeed : 400.0f;
		Movement->bOrientRotationToMovement = true;
	}
	Character->bUseControllerRotationYaw = false;

	if (DeactivateSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeactivateSound, Character->GetActorLocation());

	// Sincronizar la HUD a 0 tras el reset de XP
	Character->UpdateUltimateHUD();

	UE_LOG(LogTemp, Warning, TEXT("Ultimate: Láser apagado. XP reiniciada a 0."));
}
