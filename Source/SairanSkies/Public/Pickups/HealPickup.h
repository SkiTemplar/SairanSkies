// SairanSkies - Heal Pickup

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;

UENUM(BlueprintType)
enum class EHealType : uint8
{
	Small   UMETA(DisplayName = "Small  (+20 HP)"),
	Medium  UMETA(DisplayName = "Medium (+50 HP)"),
	Full    UMETA(DisplayName = "Full   (100 HP)")
};

/**
 * Pickup de curación que los enemigos dejan al morir.
 * Se auto-destruye si nadie lo recoge en LifeTime segundos.
 */
UCLASS()
class SAIRANSKIES_API AHealPickup : public AActor
{
	GENERATED_BODY()

public:
	AHealPickup();

protected:
	virtual void BeginPlay() override;

public:
	// ── Config ──

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EHealType HealType = EHealType::Small;

	/** HP recuperados por Small heal */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup",
		meta=(ClampMin="1.0"))
	float SmallHealAmount = 20.0f;

	/** HP recuperados por Medium heal */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup",
		meta=(ClampMin="1.0"))
	float MediumHealAmount = 50.0f;

	/** Tiempo en segundos antes de que el pickup desaparezca solo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup",
		meta=(ClampMin="1.0"))
	float LifeTime = 15.0f;

	/** Sonido al recoger */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup")
	USoundBase* PickupSound = nullptr;

	// ── Components ──

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	URotatingMovementComponent* RotatingMovement;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** Calcula la cantidad de HP a recuperar según HealType */
	float GetHealAmount(float MaxHealth) const;
};
