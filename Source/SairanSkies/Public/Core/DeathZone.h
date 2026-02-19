// SairanSkies - Death Zone Actor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeathZone.generated.h"

class UBoxComponent;

/**
 * Place in the level below platforms. When the player overlaps, they respawn
 * at their last safe grounded position.
 */
UCLASS()
class SAIRANSKIES_API ADeathZone : public AActor
{
	GENERATED_BODY()

public:
	ADeathZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DeathZone")
	UBoxComponent* TriggerVolume;
};
