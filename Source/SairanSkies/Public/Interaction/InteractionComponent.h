// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

// Forward declarations
class AActor;
class UCameraComponent;

/**
 * Delegado que se dispara cuando cambia el actor en foco
 * Útil para actualizar UI, mostrar/ocultar texto de interacción
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFocusedActorChanged, AActor*, NewFocusedActor, AActor*, PreviousFocusedActor);

/**
 * Delegado que se dispara cuando se realiza una interacción exitosa
 * Útil para reproducir sonidos, animaciones, feedback visual
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionPerformed, AActor*, InteractedActor, bool, bSuccess);

/**
 * UInteractionComponent
 * 
 * Componente que se añade al Character del jugador para gestionar interacciones.
 * Realiza raycasts desde la cámara, detecta objetos interactuables, gestiona el foco
 * y ejecuta las interacciones.
 * 
 * CONFIGURACIÓN:
 * - Añadir este componente al FirstPersonCharacter Blueprint
 * - Configurar propiedades en el editor (distancia, canal de traza, etc.)
 * - Desde el BP del Character, llamar a TryInteract() cuando se pulse el InputAction
 * - Usar GetCurrentInteractionText() para mostrar en UI
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent), Blueprintable)
class SAIRANSKIES_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== CONFIGURACIÓN ====================
	
	/**
	 * Distancia máxima a la que se pueden detectar objetos interactuables
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (ClampMin = "50.0", ClampMax = "5000.0"))
	float InteractionDistance = 300.0f;

	/**
	 * Radio de la esfera usada para detectar interactuables (0 = line trace puro)
	 * Recomendado: 25-50 para objetos pequeños
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float InteractionSphereRadius = 30.0f;

	/**
	 * Canal de traza usado para detectar interactuables
	 * Por defecto: Visibility (bueno para la mayoría de casos)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/**
	 * Si es true, dibuja líneas de debug para visualizar los raycasts
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
	bool bShowDebugTrace = false;

	/**
	 * Si es true, el componente hace trace cada frame automáticamente
	 * Si es false, solo hace trace cuando se llama a TryInteract()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
	bool bContinuousTrace = true;

	/**
	 * Tiempo (segundos) que espera antes de perder foco si el raycast falla
	 * Útil para evitar que objetos pequeños parpadeen de foco constantemente
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FocusLossDebouncTime = 0.2f;

	// ==================== DELEGADOS / EVENTOS ====================

	/**
	 * Se dispara cuando el actor en foco cambia
	 * Conectar desde Blueprint para actualizar UI
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnFocusedActorChanged OnFocusedActorChanged;

	/**
	 * Se dispara cuando se realiza una interacción
	 * Útil para feedback (sonidos, partículas, etc.)
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionPerformed OnInteractionPerformed;

	// ==================== FUNCIONES PÚBLICAS ====================

	/**
	 * Intenta interactuar con el actor actualmente en foco
	 * LLAMAR DESDE BLUEPRINT: Conectar al InputAction "Interact"
	 * @return true si la interacción fue exitosa
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/**
	 * Obtiene el texto de interacción del actor actualmente en foco
	 * USAR EN UI: Para mostrar "Press E to [Action]"
	 * @return El texto de interacción, o vacío si no hay nada en foco
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetCurrentInteractionText() const;

	/**
	 * Obtiene el actor actualmente en foco (al que se está mirando)
	 * @return El actor en foco, o nullptr si no hay ninguno
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentFocusedActor() const { return CurrentFocusedActor; }

	/**
	 * Verifica si hay un actor interactuable en foco
	 * @return true si hay algo con lo que se puede interactuar
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasFocusedActor() const { return CurrentFocusedActor != nullptr; }

	/**
	 * Verifica si el actor en foco puede ser interactuado en este momento
	 * @return true si se puede interactuar
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool CanInteractWithFocusedActor() const;

	/**
	 * Fuerza una actualización del actor en foco (hace un trace inmediato)
	 * Útil si tienes bContinuousTrace = false
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateFocusedActor();

protected:
	/**
	 * Realiza el raycast y encuentra el actor interactuable más cercano
	 * @param OutHitActor - El actor encontrado (nullptr si no hay ninguno)
	 * @return true si se encontró un actor interactuable
	 */
	bool PerformInteractionTrace(AActor*& OutHitActor);

	/**
	 * Actualiza el estado de foco (llama a OnFocusGained/Lost según corresponda)
	 * @param NewFocusedActor - El nuevo actor en foco (puede ser nullptr)
	 */
	void SetFocusedActor(AActor* NewFocusedActor);

	/**
	 * Obtiene el punto de inicio y dirección para el raycast
	 * Por defecto usa la cámara del jugador
	 */
	void GetTraceStartAndDirection(FVector& OutStart, FVector& OutDirection) const;

private:
	/**
	 * Actor interactuable actualmente en foco
	 */
	UPROPERTY()
	AActor* CurrentFocusedActor = nullptr;

	/**
	 * Cache de la cámara del jugador para optimización
	 */
	UPROPERTY()
	UCameraComponent* CachedCamera = nullptr;

	/**
	 * Sistema de debounce para evitar que el foco parpadee
	 */
	bool bWaitingForFocusLoss = false;
	float FocusLossTimer = 0.0f;
};
