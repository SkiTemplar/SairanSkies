// SairanSkies - Ultimate Laser Attack Component

#include "Character/UltimateComponent.h"
#include "Character/SairanCharacter.h"
#include "Enemies/EnemyBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
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

	// ── Barra de XP baja progresivamente durante el láser ─────────────────
	CurrentXP = FMath::Max(0.0f, CurrentXP - (MaxXP / LaserDuration) * DeltaTime);
	if (ASairanCharacter* Char = Cast<ASairanCharacter>(GetOwner()))
	{
		Char->UpdateUltimateHUD();
	}

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

	// Crear el componente de rayo continuo si hay VFX asignado
	if (LaserBeamVFX)
	{
		const FVector Origin = Character->GetActorLocation();
		LaserBeamComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			LaserBeamVFX,
			Character->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			false  // No auto-destroy
		);
		if (LaserBeamComponent)
		{
			LaserBeamComponent->SetVectorParameter(LaserBeamStartParam, Origin);
			LaserBeamComponent->SetVectorParameter(LaserBeamEndParam, Origin + Character->GetActorForwardVector() * LaserRange);
			LaserBeamComponent->Activate(true);
		}
	}

	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("Ultimate: ¡LÁSER ACTIVADO! (%.1fs, %.1f–%.1f dmg/%.2fs)"),
		LaserDuration, DamageMin, DamageMax, DamageInterval);
}

// ── Privado ───────────────────────────────────────────────────────────────────

void UUltimateComponent::FireLaserTick()
{
	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character) return;

	// Origen: ojo = centro del personaje (un poco más alto para mejor apuntería)
	const FVector Origin = Character->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// Dirección: cámara (permite apuntar en cualquier dirección)
	FVector Forward = Character->GetActorForwardVector();
	if (Character->FollowCamera)
	{
		Forward = Character->FollowCamera->GetForwardVector();
	}

	const FVector End = Origin + Forward * LaserRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	Params.bTraceComplex = false;
	Params.bReturnPhysicalMaterial = true;

	// Traza con ECC_Pawn para golpear enemigos (que son personajes)
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Pawn, Params);

	UE_LOG(LogTemp, Warning, TEXT("Ultimate FireLaserTick: Trace from %.0f,%.0f,%.0f to %.0f,%.0f,%.0f | Hit=%d | Actor=%s"),
		Origin.X, Origin.Y, Origin.Z, End.X, End.Y, End.Z, bHit, Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"));

	// Actualizar rayo visual
	UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);

#if WITH_EDITOR
	DrawDebugLine(GetWorld(), Origin, bHit ? Hit.ImpactPoint : End,
		FColor::Cyan, false, DamageInterval * 1.5f, 0, 3.0f);
	if (bHit)
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20.0f, 8, FColor::Red, false, DamageInterval * 1.5f);
#endif

	if (bHit && Hit.GetActor())
	{
		const float Damage = FMath::RandRange(DamageMin, DamageMax);

		UE_LOG(LogTemp, Warning, TEXT("Ultimate: HIT! Dealing %.1f damage to %s at impact point"), Damage, *Hit.GetActor()->GetName());

		// Primero intentar llamar directamente a TakeDamageAtLocation en enemigos,
		// que garantiza que el daño se aplica y los efectos VFX salen en el punto correcto
		if (AEnemyBase* Enemy = Cast<AEnemyBase>(Hit.GetActor()))
		{
			Enemy->TakeDamageAtLocation(Damage, Character, Character->GetController(), Hit.ImpactPoint);
		}
		else
		{
			// Para otros actores usar el sistema estándar de daño
			UGameplayStatics::ApplyDamage(Hit.GetActor(), Damage,
				Character->GetController(), Character, nullptr);
		}

		// VFX de impacto en el punto de colisión
		if (LaserImpactVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LaserImpactVFX,
				Hit.ImpactPoint, FRotator::ZeroRotator, FVector(1.0f), true, true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Ultimate: NO HIT - No actor detected in laser path"));
	}
}

void UUltimateComponent::UpdateLaserBeam(const FVector& Origin, const FVector& End)
{
	if (!LaserBeamComponent) return;

	LaserBeamComponent->SetVectorParameter(LaserBeamStartParam, Origin);
	LaserBeamComponent->SetVectorParameter(LaserBeamEndParam, End);
}

void UUltimateComponent::Deactivate()
{
	bLaserActive  = false;
	CurrentXP  = 0.0f;
	SetComponentTickEnabled(false);

	// Destruir el componente del rayo
	if (LaserBeamComponent)
	{
		LaserBeamComponent->Deactivate();
		LaserBeamComponent->DestroyComponent();
		LaserBeamComponent = nullptr;
	}

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
