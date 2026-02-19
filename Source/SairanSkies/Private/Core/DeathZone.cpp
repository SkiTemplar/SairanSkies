// SairanSkies - Death Zone Actor Implementation

#include "Core/DeathZone.h"
#include "Components/BoxComponent.h"
#include "Character/SairanCharacter.h"
#include "Character/CheckpointComponent.h"

ADeathZone::ADeathZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetBoxExtent(FVector(5000.0f, 5000.0f, 50.0f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = TriggerVolume;
}

void ADeathZone::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ADeathZone::OnOverlapBegin);
}

void ADeathZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ASairanCharacter* Player = Cast<ASairanCharacter>(OtherActor);
	if (!Player) return;

	if (Player->CheckpointComponent)
	{
		Player->CheckpointComponent->RespawnAtLastCheckpoint();
	}
}
