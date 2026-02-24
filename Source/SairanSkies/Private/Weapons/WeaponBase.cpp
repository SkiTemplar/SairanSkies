// SairanSkies - Base Weapon Implementation

#include "Weapons/WeaponBase.h"
#include "Character/SairanCharacter.h"
#include "Combat/CombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	// Weapon mesh (will be a placeholder cube scaled to look like a greatsword)
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Hit collision box
	HitCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(WeaponMesh);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetGenerateOverlapEvents(true);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// NOTE: No persistent SwingTrailComponent. Trails are spawned dynamically
	// per-attack via ActivateSwingTrail() / SwitchToBloodTrail() so they can
	// coexist and let their particles expire naturally (Lies of P behaviour).
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	SetupPlaceholderMesh();

	// Bind overlap event
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnHitCollisionOverlap);
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponBase::SetupPlaceholderMesh()
{
	// Load default cube mesh
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh)
	{
		WeaponMesh->SetStaticMesh(CubeMesh);
		
		// Scale to look like a greatsword (thin but long)
		// Default cube is 100x100x100, so we scale to get our desired size
		FVector Scale = WeaponSize / 100.0f;
		WeaponMesh->SetRelativeScale3D(Scale);
		
		// Offset so the handle is at the origin
		WeaponMesh->SetRelativeLocation(FVector(0, 0, WeaponSize.Z / 2.0f));

		// Create dynamic material for color
		UMaterialInstanceDynamic* DynMaterial = WeaponMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynMaterial)
		{
			DynMaterial->SetVectorParameterValue(FName("BaseColor"), WeaponColor);
		}
	}

	// Setup hit collision to match weapon blade size (not the whole weapon including handle)
	// Make it slightly smaller than the visual mesh to avoid floor collisions
	// The blade is roughly 2/3 of the total length, positioned at the top
	float BladeLength = WeaponSize.Z * 0.6f; // 60% of total length is the blade
	FVector HitBoxExtent = FVector(WeaponSize.X / 2.0f, WeaponSize.Y / 2.0f, BladeLength / 2.0f);
	HitCollision->SetBoxExtent(HitBoxExtent);
	
	// Position the hit box at the blade area (upper part of the weapon)
	// The blade starts around 40% up from the handle
	float BladeOffsetZ = WeaponSize.Z * 0.7f; // Position at 70% height (middle of the blade)
	HitCollision->SetRelativeLocation(FVector(0, 0, BladeOffsetZ));
}

void AWeaponBase::EquipToCharacter(ASairanCharacter* NewOwner)
{
	if (!NewOwner) return;

	OwnerCharacter = NewOwner;
	SetOwner(NewOwner);
	SetInstigator(NewOwner);

	// Attach to hand by default
	AttachToHand();
}

void AWeaponBase::AttachToHand()
{
	if (!OwnerCharacter) return;

	// Use the attach point component from the character
	USceneComponent* AttachPoint = OwnerCharacter->WeaponHandAttachPoint;
	if (!AttachPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponHandAttachPoint not found on character!"));
		return;
	}

	// Detach first
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Attach to the hand attach point - weapon will inherit position and rotation from the component
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	AttachToComponent(AttachPoint, AttachRules);

	// Reset relative transform - the attach point already has the correct position/rotation
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	CurrentState = EWeaponState::Drawn;
	bInBlockingStance = false;
}

void AWeaponBase::AttachToBack()
{
	if (!OwnerCharacter) return;

	// Use the attach point component from the character
	USceneComponent* AttachPoint = OwnerCharacter->WeaponBackAttachPoint;
	if (!AttachPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponBackAttachPoint not found on character!"));
		return;
	}

	// Detach first
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Attach to the back attach point
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	AttachToComponent(AttachPoint, AttachRules);

	// Reset relative transform - the attach point already has the correct position/rotation
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	CurrentState = EWeaponState::Sheathed;
	bInBlockingStance = false;
}

void AWeaponBase::EnableHitCollision()
{
	HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CurrentState = EWeaponState::Attacking;
}

void AWeaponBase::DisableHitCollision()
{
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CurrentState == EWeaponState::Attacking)
	{
		CurrentState = EWeaponState::Drawn;
	}
}

void AWeaponBase::SetWeaponState(EWeaponState NewState)
{
	CurrentState = NewState;
}

void AWeaponBase::AttachToBlockPosition()
{
	if (!OwnerCharacter) return;

	// Use the attach point component from the character
	USceneComponent* AttachPoint = OwnerCharacter->WeaponBlockAttachPoint;
	if (!AttachPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponBlockAttachPoint not found on character!"));
		return;
	}

	// Detach first
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Attach to the block attach point
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	AttachToComponent(AttachPoint, AttachRules);

	// Reset relative transform - the attach point already has the correct position/rotation
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	// Keep drawn state but mark as blocking
	bInBlockingStance = true;
}

void AWeaponBase::SetBlockingStance(bool bIsBlocking)
{
	if (!OwnerCharacter || CurrentState == EWeaponState::Sheathed) return;

	bInBlockingStance = bIsBlocking;

	if (bIsBlocking)
	{
		// Move to blocking position using the attach point
		AttachToBlockPosition();
	}
	else
	{
		// Return to hand position
		AttachToHand();
	}
}

// ========== SWING TRAIL SYSTEM (Lies of P style) ==========

void AWeaponBase::ActivateSwingTrail()
{
	// Reset blood-trail guard for this new swing
	bBloodTrailSpawnedThisSwing = false;

	// Clean up stale pointers left from previous swings whose particles already finished
	ActiveTrailComponents.RemoveAll([](UNiagaraComponent* C) { return !IsValid(C); });

	if (!NormalSwingTrailFX || !WeaponMesh) return;

	// Spawn a brand-new Niagara component attached to the weapon mesh.
	// bAutoDestroy = true  → UE destroys the component once the system finishes,
	//                        so we don't leak components between attacks.
	UNiagaraComponent* NormalTrail = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NormalSwingTrailFX,
		WeaponMesh,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		/*bAutoDestroy=*/ true
	);

	if (NormalTrail)
	{
		NormalTrail->Activate(true);
		ActiveTrailComponents.Add(NormalTrail);
	}
}

void AWeaponBase::DeactivateSwingTrail()
{
	// For every active trail component we want to STOP NEW EMISSION but let every
	// already-alive particle run out its natural lifetime — exactly what you see in
	// Lies of P where the arc fades rather than snapping off.
	//
	// Strategy:
	//   1. Detach the Niagara component from the weapon mesh so it floats in world
	//      space.  Particles keep moving at their last-calculated velocity and fade
	//      naturally.  Visually the arc "stays in the air" while the sword moves on.
	//   2. Do NOT call Deactivate() — that can kill live particles immediately if
	//      the Niagara asset's Completion Action is set to "Kill".
	//   3. bAutoDestroy=true (set at spawn) ensures UE destroys the component itself
	//      once all its particles have expired, so there is no leak.
	for (UNiagaraComponent* Comp : ActiveTrailComponents)
	{
		if (!IsValid(Comp)) continue;

		// Detach so it floats in world space — particles live out their lifetime.
		Comp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		// bAutoDestroy handles GC once system goes idle. No manual Deactivate needed.
	}

	// Release our tracking references.  Components remain in the world until
	// bAutoDestroy destroys them after all their particles expire.
	ActiveTrailComponents.Empty();
	bBloodTrailSpawnedThisSwing = false;
}

void AWeaponBase::SwitchToBloodTrail()
{
	// Guard: only spawn one blood trail per swing even if we hit multiple enemies.
	if (bBloodTrailSpawnedThisSwing) return;
	bBloodTrailSpawnedThisSwing = true;

	if (!BloodSwingTrailFX || !WeaponMesh) return;

	// ── Key behaviour ──────────────────────────────────────────────────────────
	// We do NOT touch the existing NormalTrail component at all.
	// It keeps emitting exactly where it was, preserving the trail arc already
	// drawn in the air.  We simply SPAWN A NEW component for the blood trail
	// alongside it.  Both components run concurrently.
	// ──────────────────────────────────────────────────────────────────────────

	UNiagaraComponent* BloodTrail = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BloodSwingTrailFX,
		WeaponMesh,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		/*bAutoDestroy=*/ true
	);

	if (BloodTrail)
	{
		BloodTrail->Activate(true);
		ActiveTrailComponents.Add(BloodTrail);
	}
}

void AWeaponBase::OnHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Notify CombatComponent about the hit
	if (!OwnerCharacter || !OtherActor || OtherActor == OwnerCharacter) return;

	// Only process if this is an enemy
	if (!OtherActor->ActorHasTag(FName("Enemy"))) return;

	// Get combat component and notify it
	if (OwnerCharacter->CombatComponent)
	{
		FVector HitLocation = SweepResult.ImpactPoint.IsNearlyZero() ? OtherActor->GetActorLocation() : FVector(SweepResult.ImpactPoint);
		OwnerCharacter->CombatComponent->OnWeaponHitDetected(OtherActor, HitLocation);
	}
}
