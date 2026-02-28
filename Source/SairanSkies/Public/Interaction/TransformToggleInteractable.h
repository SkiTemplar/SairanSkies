// Fill out your copyright notice in the Description page of Project Settings.
// Last updated: 2026-01-06 - Multi-target lever system with independent transforms

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableActorBase.h"
#include "TransformToggleInteractable.generated.h"

class USoundBase;

/**
 * Estructura que define la transformación de un objeto objetivo
 */
USTRUCT(BlueprintType)
struct FTargetTransformData
{
	GENERATED_BODY()

	/** Actor objetivo que será transformado */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	AActor* TargetActor = nullptr;

	/** Tipo de transformación a aplicar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bApplyLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bApplyRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bApplyScale = false;

	/** Offset de transformación (relativo al estado original) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FVector ScaleOffset = FVector::OneVector;

	/** Si es true, usa transformación absoluta en lugar de relativa */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bUseAbsoluteTransform = false;

	/** Transformación absoluta (solo si bUseAbsoluteTransform es true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (EditCondition = "bUseAbsoluteTransform"))
	FTransform AbsoluteTransform;

	/** Velocidad de interpolación para este objetivo específico */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float TransitionSpeed = 3.0f;

	// ==================== DATOS INTERNOS (NO EDITAR) ====================

	/** Transformación original guardada */
	FTransform OriginalTransform;

	/** Transformación calculada para el estado transformado */
	FTransform TransformedTransform;

	/** Si ya se guardó la transformación original */
	bool bOriginalSaved = false;

	/** Si está en transición */
	bool bIsTransitioning = false;
};

/**
 * ATransformToggleInteractable
 * 
 * Interactuable tipo palanca que puede afectar MÚLTIPLES objetos con transformaciones independientes.
 * 
 * CARACTERÍSTICAS:
 * - Base estática + Palanca animada (cualquier transformación: Location, Rotation, Scale)
 * - Soporta MÚLTIPLES OBJETIVOS con transformaciones independientes
 * - Cada objetivo puede tener diferente tipo de transformación y velocidad
 * - Toggle: alterna entre estado original y transformado
 * - A prueba de múltiples palancas afectando objetos diferentes
 * 
 * USO:
 * 1. Colocar este actor en la escena
 * 2. Asignar meshes a BaseMesh y LeverMesh
 * 3. Configurar LeverTransformOffset (cómo se mueve la palanca)
 * 4. Añadir objetivos al array TargetTransforms
 * 5. Para cada objetivo, configurar qué transformaciones aplicar
 */
UCLASS(Blueprintable, BlueprintType)
class SAIRANSKIES_API ATransformToggleInteractable : public AInteractableActorBase
{
	GENERATED_BODY()

public:
	ATransformToggleInteractable();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ==================== COMPONENTES DE LA PALANCA ====================

	/** Componente raíz de la escena */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* LeverRoot;

	/** Mesh de la BASE de la palanca (estático, no se mueve) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BaseMesh;

	/** Mesh de la PALANCA que se transforma al interactuar */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LeverMesh;

	// ==================== IMPLEMENTACIÓN DE INTERFAZ ====================

	virtual bool Interact_Implementation(AActor* InteractInstigator) override;
	virtual FText GetInteractionText_Implementation() const override;

	// ==================== CONFIGURACIÓN DE LA PALANCA ====================

	/** Qué tipo de transformación aplicar a la palanca */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	bool bLeverApplyLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	bool bLeverApplyRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	bool bLeverApplyScale = false;

	/** Transformación de la palanca cuando está activada (offset relativo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	FVector LeverLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	FRotator LeverRotationOffset = FRotator(-45.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever")
	FVector LeverScaleOffset = FVector::OneVector;

	/** Velocidad de transformación de la palanca */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Lever", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float LeverTransitionSpeed = 5.0f;

	// ==================== CONFIGURACIÓN DE OBJETIVOS (MÚLTIPLES) ====================

	/** Array de objetivos que serán transformados al interactuar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Targets", meta = (TitleProperty = "TargetActor"))
	TArray<FTargetTransformData> TargetTransforms;

	// ==================== CONFIGURACIÓN DEL WIDGET ====================

	/** Texto cuando está en estado original */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Widget")
	FText TextWhenOriginal = FText::FromString("Pulsa E para activar");

	/** Texto cuando está transformado */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Widget")
	FText TextWhenTransformed = FText::FromString("Pulsa E para desactivar");

	// ==================== CONFIGURACIÓN DE TRANSICIÓN ====================

	/** Si es true, las transformaciones son suaves/animadas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Transition")
	bool bUseSmoothTransition = true;

	/** Tolerancia para considerar que llegó al destino */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Transition")
	float TransitionTolerance = 1.0f;

	/** Si es true, bloquea interacciones mientras hace la transición */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Transition")
	bool bBlockInteractionDuringTransition = false;

	// ==================== DEBUG ====================

	/** Mostrar debug visual */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Debug")
	bool bShowDebug = false;

	// ==================== AUDIO ====================

	/** Sonido al activar la palanca (pasar a estado transformado) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Audio")
	USoundBase* ActivateSound = nullptr;

	/** Sonido al desactivar la palanca (volver a estado original) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Audio")
	USoundBase* DeactivateSound = nullptr;

	/** Sonido cuando INICIA la transformación del objetivo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Audio")
	USoundBase* TransformStartSound = nullptr;

	/** Sonido cuando la transformación del objetivo termina */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Audio")
	USoundBase* TransformCompleteSound = nullptr;

	/** Volumen del sonido */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransformToggle|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SoundVolume = 1.0f;


	// ==================== FUNCIONES PÚBLICAS ====================

	/** Verifica si está en estado transformado */
	UFUNCTION(BlueprintPure, Category = "TransformToggle")
	bool IsTransformed() const { return bIsTransformed; }

	/** Verifica si está en proceso de transición */
	UFUNCTION(BlueprintPure, Category = "TransformToggle")
	bool IsTransitioning() const;

	/** Fuerza el estado transformado sin toggle */
	UFUNCTION(BlueprintCallable, Category = "TransformToggle")
	void SetTransformed(bool bNewTransformed);

	/** Obtiene cuántos objetivos están configurados */
	UFUNCTION(BlueprintPure, Category = "TransformToggle")
	int32 GetTargetCount() const { return TargetTransforms.Num(); }

protected:
	/** Guarda los estados iniciales de todos los objetivos */
	void SaveInitialStates();

	/** Aplica la transformación a todos los objetivos */
	void ApplyTransformToTargets(bool bToTransformed);

	/** Actualiza la transformación de la palanca */
	void UpdateLeverTransform(float DeltaTime);

	/** Actualiza las transformaciones de los objetivos */
	void UpdateTargetsTransform(float DeltaTime);

	/** Verifica si la palanca terminó su transición */
	bool IsLeverTransitionComplete() const;

	/** Verifica si todos los objetivos terminaron sus transiciones */
	bool AreAllTargetsTransitionComplete() const;

	/** Completa la transición de la palanca */
	void FinishLeverTransition();

	/** Completa la transición de todos los objetivos */
	void FinishTargetsTransition();

	// ==================== EVENTOS BLUEPRINT ====================

	/** Evento cuando empieza la transformación */
	UFUNCTION(BlueprintImplementableEvent, Category = "TransformToggle", meta = (DisplayName = "On Transform Started"))
	void OnTransformStartedBP(bool bToTransformed);

	/** Evento cuando termina la transformación */
	UFUNCTION(BlueprintImplementableEvent, Category = "TransformToggle", meta = (DisplayName = "On Transform Finished"))
	void OnTransformFinishedBP(bool bIsNowTransformed);

	/** Evento cuando la palanca empieza a moverse */
	UFUNCTION(BlueprintImplementableEvent, Category = "TransformToggle", meta = (DisplayName = "On Lever Movement Started"))
	void OnLeverMovementStartedBP(bool bGoingToTransformed);

	/** Evento cuando la palanca termina de moverse */
	UFUNCTION(BlueprintImplementableEvent, Category = "TransformToggle", meta = (DisplayName = "On Lever Movement Finished"))
	void OnLeverMovementFinishedBP(bool bLeverIsTransformed);

private:
	/** Estado actual: true = transformado, false = original */
	bool bIsTransformed = false;

	/** Si la palanca está en transición */
	bool bIsLeverTransitioning = false;

	/** Transformación original de la palanca */
	FTransform LeverOriginalTransform;

	/** Transformación objetivo de la palanca */
	FTransform LeverTargetTransform;

	/** Si ya se guardaron los estados iniciales */
	bool bInitialStatesSaved = false;
};
