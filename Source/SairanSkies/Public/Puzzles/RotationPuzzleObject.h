// SairanSkies - Puzzle de Objetos Giratorios

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableActorBase.h"
#include "RotationPuzzleObject.generated.h"

class UStaticMeshComponent;
class ARotationPuzzleManager;

/**
 * Objeto interactuable que gira en pasos de RotationStep grados.
 * Cuando todos los objetos del puzzle están en su ángulo correcto,
 * notifica al ARotationPuzzleManager.
 *
 * Uso:
 *  1. Colocar N ARotationPuzzleObject en el nivel.
 *  2. Asignar el mismo ARotationPuzzleManager a todos.
 *  3. Configurar TargetYaw en cada objeto (la posición "correcta").
 *  4. El diseñador configura qué ocurre en el manager cuando se resuelve.
 */
UCLASS()
class SAIRANSKIES_API ARotationPuzzleObject : public AInteractableActorBase
{
	GENERATED_BODY()

public:
	ARotationPuzzleObject();

protected:
	virtual void BeginPlay() override;

public:
	// ── Puzzle config ──

	/** Manager que gestiona este conjunto de objetos */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	ARotationPuzzleManager* PuzzleManager;

	/** Ángulo Yaw (relativo al spawn) que se considera "correcto" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle",
		meta=(ClampMin="0.0", ClampMax="359.0"))
	float TargetYaw = 0.0f;

	/** Grados que avanza cada interacción (45 = 8 posiciones, 90 = 4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle",
		meta=(ClampMin="1.0", ClampMax="180.0"))
	float RotationStep = 45.0f;

	/** Tolerancia en grados para considerar el objeto "en posición" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle",
		meta=(ClampMin="0.5", ClampMax="10.0"))
	float SnapTolerance = 3.0f;

	/** Sonido al girar */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	USoundBase* RotateSound = nullptr;

	/** Sonido al quedar en posición correcta */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	USoundBase* CorrectPositionSound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bIsInCorrectPosition = false;

	// ── InteractableActorBase override ──
	virtual bool Interact_Implementation(AActor* Interactor) override;

	bool IsInCorrectPosition() const;

private:
	float CurrentYaw = 0.0f; // acumulado desde spawn
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Manager del puzzle de rotación. Recibe notificaciones y aplica
 * la recompensa cuando todos los objetos están en posición.
 */
UCLASS()
class SAIRANSKIES_API ARotationPuzzleManager : public AActor
{
	GENERATED_BODY()

public:
	ARotationPuzzleManager();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	TArray<ARotationPuzzleObject*> Objects;

	/** Actor que recibe la transformación al resolver */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Puzzle")
	AActor* TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FVector TargetTranslation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FRotator TargetRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform")
	FVector TargetScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Transform",
		meta=(ClampMin="0.0"))
	float TransformLerpDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Puzzle")
	USoundBase* SolvedSound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bIsSolved = false;

	/** Llamado por cada ARotationPuzzleObject tras girar */
	void NotifyObjectChanged();

private:
	bool bIsLerping = false;
	float LerpTimer = 0.0f;
	FVector  LerpStartLoc;
	FRotator LerpStartRot;
	FVector  LerpStartScale;

public:
	virtual void Tick(float DeltaTime) override;
};
