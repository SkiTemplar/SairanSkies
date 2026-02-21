// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "InteractableActorBase.generated.h"

// Forward declaration
class UInteractionWidget3DComponent;

/**
 * AInteractableActorBase
 * 
 * Actor base que implementa la interfaz de interacción.
 * Sirve como ejemplo y plantilla para crear objetos interactuables.
 * 
 * USO:
 * 1. En C++: Heredar de esta clase y sobreescribir los métodos _Implementation
 * 2. En Blueprint: Crear un BP hijo y sobreescribir los eventos de la interfaz
 * 
 * CARACTERÍSTICAS:
 * - Tiene un StaticMesh configurable
 * - Cambia de material al ganar/perder foco (si se configuran materiales)
 * - Implementa todos los métodos de IInteractableInterface
 * - Totalmente configurable desde el editor
 * - Opcionalmente puede mostrar un widget 3D flotante en el mundo
 */
UCLASS(Blueprintable)
class SAIRANSKIES_API AInteractableActorBase : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AInteractableActorBase();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== COMPONENTES ====================

	/**
	 * Mesh visual del objeto interactuable
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	/**
	 * Componente de widget 3D para mostrar texto de interacción en el mundo (OPCIONAL)
	 * Si quieres UI 3D en lugar de UI 2D del jugador, activa bUse3DWidget
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractionWidget3DComponent* InteractionWidget3D;

	// ==================== CONFIGURACIÓN DE INTERACCIÓN ====================

	/**
	 * Texto que se muestra cuando el jugador mira este objeto
	 * Ejemplo: "Abrir puerta", "Recoger objeto", "Activar palanca"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionText = FText::FromString("Interactuar");

	/**
	 * Si es false, el objeto no puede ser interactuado (aparece pero no responde)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanBeInteracted = true;

	/**
	 * Distancia máxima personalizada para este objeto (-1 = usar default del componente)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "-1.0", ClampMax = "5000.0"))
	float CustomInteractionDistance = -1.0f;

	/**
	 * Si es true, el objeto puede ser interactuado múltiples veces
	 * Si es false, solo una vez (luego se desactiva)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteractMultipleTimes = true;

	// ==================== WIDGET 3D EN EL MUNDO ====================

	/**
	 * Si es true, usa el widget 3D para mostrar el texto de interacción en el mundo
	 * Si es false, usa la UI 2D tradicional del jugador
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|3D Widget")
	bool bUse3DWidget = false;

	// ==================== HIGHLIGHT / FEEDBACK VISUAL ====================

	/**
	 * Material que se aplica cuando el jugador mira el objeto (opcional)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Visual")
	UMaterialInterface* HighlightMaterial;

	/**
	 * Si es true, se cambia el material automáticamente en OnFocusGained/Lost
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Visual")
	bool bAutoHighlight = true;

	// ==================== IMPLEMENTACIÓN DE IInteractableInterface ====================

	/**
	 * Implementación de Interact - Llamado cuando el jugador interactúa
	 * SOBREESCRIBIR en C++ o BP para comportamiento personalizado
	 */
	virtual bool Interact_Implementation(AActor* InteractInstigator);

	/**
	 * Implementación de CanInteract - Verifica si se puede interactuar
	 * SOBREESCRIBIR si necesitas lógica personalizada (tener llave, estar activado, etc.)
	 */
	virtual bool CanInteract_Implementation(AActor* InteractInstigator) const;

	/**
	 * Implementación de GetInteractionText - Retorna el texto de UI
	 */
	virtual FText GetInteractionText_Implementation() const;

	/**
	 * Implementación de OnFocusGained - El jugador empieza a mirar el objeto
	 * SOBREESCRIBIR para efectos personalizados (outline, glow, etc.)
	 */
	virtual void OnFocusGained_Implementation();

	/**
	 * Implementación de OnFocusLost - El jugador deja de mirar el objeto
	 * SOBREESCRIBIR para efectos personalizados
	 */
	virtual void OnFocusLost_Implementation();

	/**
	 * Implementación de GetInteractionDistance - Distancia personalizada
	 */
	virtual float GetInteractionDistance_Implementation() const;

	// ==================== EVENTOS BLUEPRINTABLES ====================

	/**
	 * Evento que se dispara cuando se interactúa con este objeto
	 * IMPLEMENTAR EN BP para comportamiento específico sin C++
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction", meta = (DisplayName = "On Interacted"))
	void OnInteracted(AActor* InteractInstigator);

	/**
	 * Evento que se dispara cuando el objeto gana foco
	 * IMPLEMENTAR EN BP para efectos visuales personalizados
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction", meta = (DisplayName = "On Focus Gained BP"))
	void OnFocusGainedBP();

	/**
	 * Evento que se dispara cuando el objeto pierde foco
	 * IMPLEMENTAR EN BP para efectos visuales personalizados
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction", meta = (DisplayName = "On Focus Lost BP"))
	void OnFocusLostBP();

protected:
	/**
	 * Contador de interacciones (útil si bCanInteractMultipleTimes = false)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	int32 InteractionCount = 0;

	/**
	 * Material original (guardado para restaurar después del highlight)
	 */
	UPROPERTY()
	UMaterialInterface* OriginalMaterial;
};
