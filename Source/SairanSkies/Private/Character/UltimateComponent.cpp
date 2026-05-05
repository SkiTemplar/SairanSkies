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

	FVector Origin;
	FVector End;
	FHitResult Hit;
	const bool bHit = TraceLaser(Origin, End, Hit);
	UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);

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

	if (LaserBeamVFX)
	{
		FVector Origin;
		FVector End;
		FHitResult Hit;
		const bool bHit = TraceLaser(Origin, End, Hit);

		// Spawnear en el origen del rayo con escala inicial de 1.0
		LaserBeamComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			LaserBeamVFX,
			Origin,
			GetLaserBeamRotation(Origin, bHit ? Hit.ImpactPoint : End),
			FVector(1.0f, 1.0f, 1.0f),  // ← Escala inicial uniforme, pero se va a cambiar en UpdateLaserBeam
			false,
			false
		);

		if (LaserBeamComponent)
		{
			LaserBeamComponent->Activate(true);
			// Esto calcula la escala correcta en Z basada en la distancia del rayo
			UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);
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

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	Params.bTraceComplex = false;

	// ── Traza doble para máxima fiabilidad ──────────────────────────────────
	// ECC_Pawn como CANAL solo detecta lo que BLOQUEA dicho canal.
	// La cápsula de los personajes tiene Pawn→Overlap, NO Block → nunca detecta.
	// Solución: LineTraceSingleByObjectType con ECC_Pawn detecta todos los Pawns
	// independientemente de su perfil de colisión.

	// 1) Encontrar Pawns (enemigos) por tipo de objeto
	FCollisionObjectQueryParams PawnObjQuery;
	PawnObjQuery.AddObjectTypesToQuery(ECC_Pawn);
	FHitResult HitPawn;
	const bool bHitPawn = GetWorld()->LineTraceSingleByObjectType(HitPawn, Origin, End, PawnObjQuery, Params);

	// 2) Encontrar geometría visible (paredes, props) por canal
	FHitResult HitVis;
	const bool bHitVis = GetWorld()->LineTraceSingleByChannel(HitVis, Origin, End, ECC_Visibility, Params);

	// 3) Usar el impacto más cercano al origen
	FHitResult Hit;
	bool bHit = false;
	if (bHitPawn && bHitVis)
	{
		Hit  = (HitPawn.Distance <= HitVis.Distance) ? HitPawn : HitVis;
		bHit = true;
	}
	else if (bHitPawn) { Hit = HitPawn; bHit = true; }
	else if (bHitVis)  { Hit = HitVis;  bHit = true; }

	UE_LOG(LogTemp, VeryVerbose, TEXT("Ultimate Laser: Pawn=%d Vis=%d -> bHit=%d Actor=%s"),
		bHitPawn, bHitVis, bHit, Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"));

#if WITH_EDITOR
	if (bShowLaserDebug)
	{
		DrawDebugLine(GetWorld(), Origin, bHit ? Hit.ImpactPoint : End,
			FColor::Cyan, false, DamageInterval * 1.5f, 0, 3.0f);
		if (bHit)
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20.0f, 8, FColor::Red, false, DamageInterval * 1.5f);
	}
#endif

	if (bHit && Hit.GetActor())
	{
		const float Damage = FMath::RandRange(DamageMin, DamageMax);

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
}

bool UUltimateComponent::TraceLaser(FVector& OutOrigin, FVector& OutEnd, FHitResult& OutHit) const
{
	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character || !GetWorld()) return false;

	OutOrigin = Character->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FVector Forward = Character->GetActorForwardVector();
	if (Character->FollowCamera)
	{
		Forward = Character->FollowCamera->GetForwardVector();
	}

	OutEnd = OutOrigin + Forward * LaserRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	Params.bTraceComplex = false;

	FCollisionObjectQueryParams PawnObjQuery;
	PawnObjQuery.AddObjectTypesToQuery(ECC_Pawn);

	FHitResult HitPawn;
	const bool bHitPawn = GetWorld()->LineTraceSingleByObjectType(HitPawn, OutOrigin, OutEnd, PawnObjQuery, Params);

	FHitResult HitVis;
	const bool bHitVis = GetWorld()->LineTraceSingleByChannel(HitVis, OutOrigin, OutEnd, ECC_Visibility, Params);

	bool bHit = false;
	if (bHitPawn && bHitVis)
	{
		OutHit = (HitPawn.Distance <= HitVis.Distance) ? HitPawn : HitVis;
		bHit = true;
	}
	else if (bHitPawn)
	{
		OutHit = HitPawn;
		bHit = true;
	}
	else if (bHitVis)
	{
		OutHit = HitVis;
		bHit = true;
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("Ultimate Laser: Pawn=%d Vis=%d -> bHit=%d Actor=%s"),
		bHitPawn, bHitVis, bHit, OutHit.GetActor() ? *OutHit.GetActor()->GetName() : TEXT("None"));

	return bHit;
}

void UUltimateComponent::UpdateLaserBeam(const FVector& Origin, const FVector& End)
{
	if (!LaserBeamComponent) return;

	ASairanCharacter* Character = Cast<ASairanCharacter>(GetOwner());
	if (!Character) return;

	// ── Dirección hacia donde mira el personaje EN ESTE FRAME ──────────────
	FVector Forward = Character->GetActorForwardVector();
	if (Character->FollowCamera)
	{
		Forward = Character->FollowCamera->GetForwardVector();
	}

	// ── Posición: 200cm adelante en la dirección que mira AHORA ────────────
	const FVector LaserPosition = Character->GetActorLocation() + Forward * 200.0f;

	// ── Actualizar posición y rotación EN TIEMPO REAL ─────────────────────
	LaserBeamComponent->SetWorldLocation(LaserPosition);
	LaserBeamComponent->SetWorldRotation(Forward.Rotation());
}

FRotator UUltimateComponent::GetLaserBeamRotation(const FVector& Origin, const FVector& End) const
{
	const FVector Direction = (End - Origin).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return LaserBeamRotationOffset;
	}

	return Direction.Rotation() + LaserBeamRotationOffset;
}

void UUltimateComponent::Deactivate()
{
	bLaserActive  = false;
	CurrentXP  = 0.0f;
	SetComponentTickEnabled(false);

	if (LaserBeamComponent)
	{
		// Desactivar completamente y destruir el componente
		LaserBeamComponent->Deactivate();
		// Pequeño delay antes de destruir para que se vea el apagado suave
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick([this]()
			{
				if (LaserBeamComponent)
				{
					LaserBeamComponent->DestroyComponent(true);
					LaserBeamComponent = nullptr;
				}
			});
		}
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
