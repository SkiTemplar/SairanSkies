// SairanSkies - Puzzle de Placas de Presión

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlatePuzzle.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Una placa individual. Se activa cuando el jugador/enemigo la pisa.
 * Informa al APressurePlatePuzzle (su owner) de su estado.
 */
UCLASS()
class SAIRANSKIES_API APressurePlate : public AActor
{
	GENERATED_BODY()
public:
	APressurePlate();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PlateMesh;

	/** Referencia al puzzle manager que gestiona esta placa */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	class APressurePlatePuzzle* OwnerPuzzle;

	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bIsActivated = false;

	/** Material cuando está activada (asignar en BP) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	UMaterialInterface* ActiveMaterial = nullptr;

	/** Material por defecto */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	UMaterialInterface* InactiveMaterial = nullptr;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetActivated(bool bActive);
	int32 OverlapCount = 0;
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gestor del puzzle. Recibe notificaciones de cada APressurePlate.
 * Cuando TODAS están activas aplica la transformación al TargetActor.
 */
UCLASS()
class SAIRANSKIES_API APressurePlatePuzzle : public AActor
{
	GENERATED_BODY()
public:
	APressurePlatePuzzle();

protected:
	virtual void BeginPlay() override;

public:
	/** Todas las placas que deben estar pisadas para activar el puzzle */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	TArray<APressurePlate*> Plates;

	/** Actor que recibirá la transformación al resolver el puzzle */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	AActor* TargetActor;

	/** Traslación aplicada al TargetActor al resolver */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FVector TargetTranslation = FVector::ZeroVector;

	/** Rotación aplicada al TargetActor al resolver */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FRotator TargetRotation = FRotator::ZeroRotator;

	/** Escala aplicada al TargetActor al resolver (FVector::OneVector = sin cambio) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FVector TargetScale = FVector::OneVector;

	/** Duración de la interpolación al aplicar la transformación (0 = instantáneo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform",
		meta=(ClampMin="0.0"))
	float TransformLerpDuration = 1.5f;

	/** Sonido al resolver el puzzle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	USoundBase* SolvedSound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bIsSolved = false;

	/** Llamado por cada APressurePlate cuando cambia su estado */
	void NotifyPlateChanged();

private:
	void ApplyTransform();

	// Lerp en Tick
	bool bIsLerping = false;
	float LerpTimer = 0.0f;
	FVector   LerpStartLoc;
	FRotator  LerpStartRot;
	FVector   LerpStartScale;

public:
	virtual void Tick(float DeltaTime) override;
};
