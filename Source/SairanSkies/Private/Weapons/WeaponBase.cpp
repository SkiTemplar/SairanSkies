// SairanSkies - Base Weapon Implementation

#include "Weapons/WeaponBase.h"
#include "Character/SairanCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

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

	// Setup hit collision to match weapon size
	HitCollision->SetBoxExtent(FVector(WeaponSize.X / 2.0f, WeaponSize.Y / 2.0f, WeaponSize.Z / 2.0f));
	HitCollision->SetRelativeLocation(FVector(0, 0, WeaponSize.Z / 2.0f));
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

void AWeaponBase::OnHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Weapon overlap events can be used for additional hit effects
	// Main damage is handled by CombatComponent's sphere trace
	
	if (OtherActor && OtherActor != OwnerCharacter && OtherActor->ActorHasTag(FName("Enemy")))
	{
		UE_LOG(LogTemp, Log, TEXT("Weapon overlapped with: %s"), *OtherActor->GetName());
	}
}
