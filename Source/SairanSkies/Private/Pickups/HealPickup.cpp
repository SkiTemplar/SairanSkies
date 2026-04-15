// SairanSkies - Heal Pickup Implementation

#include "Pickups/HealPickup.h"
#include "Character/SairanCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AHealPickup::AHealPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(60.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = CollisionSphere;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Default: pequeña esfera verde — el diseñador asigna la mesh real en el Blueprint
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(SphereMesh.Object);
	}
	MeshComp->SetRelativeScale3D(FVector(0.4f));

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Rotation"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void AHealPickup::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AHealPickup::OnOverlapBegin);

	// Auto-destruir tras LifeTime segundos
	SetLifeSpan(LifeTime);

	// Escalar la mesh según el tipo para dar feedback visual de valor
	switch (HealType)
	{
	case EHealType::Small:  MeshComp->SetRelativeScale3D(FVector(0.35f)); break;
	case EHealType::Medium: MeshComp->SetRelativeScale3D(FVector(0.55f)); break;
	case EHealType::Full:   MeshComp->SetRelativeScale3D(FVector(0.75f)); break;
	}
}

void AHealPickup::OnOverlapBegin(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	ASairanCharacter* Player = Cast<ASairanCharacter>(OtherActor);
	if (!Player || !Player->IsAlive()) return;

	// No curar si ya está al máximo
	if (Player->CurrentHealth >= Player->MaxHealth) return;

	float Amount = GetHealAmount(Player->MaxHealth);
	Player->CurrentHealth = FMath::Clamp(Player->CurrentHealth + Amount, 0.0f, Player->MaxHealth);
	Player->UpdateHUD();

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("HealPickup: %s recogió curación %s (+%.0f HP → %.0f/%.0f)"),
		*Player->GetName(),
		HealType == EHealType::Small  ? TEXT("Small")  :
		HealType == EHealType::Medium ? TEXT("Medium") : TEXT("Full"),
		Amount, Player->CurrentHealth, Player->MaxHealth);

	Destroy();
}

float AHealPickup::GetHealAmount(float MaxHealth) const
{
	switch (HealType)
	{
	case EHealType::Small:  return SmallHealAmount;
	case EHealType::Medium: return MediumHealAmount;
	case EHealType::Full:   return MaxHealth; // 100 %
	}
	return SmallHealAmount;
}
