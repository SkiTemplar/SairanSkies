#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionWidget3DComponent.generated.h"

/**
 * UInteractionWidget3DComponent
 * 
 * Componente que muestra un widget 3D flotante en el mundo para mostrar
 * el texto de interacción del objeto al que está adjunto.
 * 
 * USO:
 * 1. Añadir este componente a cualquier actor interactuable
 * 2. Crear un Widget Blueprint (UMG) con un Text Block
 * 3. Asignar el Widget Class en el componente
 * 4. El componente se mostrará/ocultará automáticamente cuando el jugador mire el objeto
 * 
 * CARACTERÍSTICAS:
 * - Se muestra solo cuando el jugador mira el objeto
 * - Orientado siempre hacia la cámara del jugador
 * - Configurable: tamaño, offset, distancia de draw
 * - Se integra automáticamente con IInteractableInterface
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent, DisplayName = "Interaction Widget 3D Component"))
class SAIRANSKIES_API UInteractionWidget3DComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UInteractionWidget3DComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== CONFIGURACIÓN ====================

	/**
	 * Nombre de la variable Text en el Widget Blueprint que mostrará el texto de interacción
	 * Por defecto busca "InteractionText"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	FName TextVariableName = TEXT("InteractionText");

	/**
	 * Offset local respecto al actor (por ejemplo, para ponerlo encima del objeto)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 100.0f);

	/**
	 * Distancia máxima a la que se dibuja el widget (0 = sin límite)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	float MaxDrawDistance = 500.0f;

	/**
	 * Si es true, el widget siempre mira hacia la cámara del jugador
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	bool bAlwaysFaceCamera = true;

	/**
	 * Si es true, el widget se oculta automáticamente cuando el jugador no mira el objeto
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	bool bAutoHideWhenNotFocused = true;

	/**
	 * Si es true, el widget se muestra automáticamente al ganar foco
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	bool bAutoShowOnFocus = true;

	/**
	 * Si es true, el widget se posiciona automáticamente entre la cámara y el objeto
	 * Si es false, usa el WidgetOffset estático
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D")
	bool bPositionBetweenCameraAndObject = true;

	/**
	 * Distancia desde el objeto hacia la cámara donde se coloca el widget (en unidades)
	 * Solo se usa si bPositionBetweenCameraAndObject es true
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D", meta = (EditCondition = "bPositionBetweenCameraAndObject", ClampMin = "10.0", ClampMax = "500.0"))
	float DistanceFromObject = 80.0f;

	/**
	 * Si es true, el widget escala automáticamente con la distancia para mantener tamaño constante en pantalla
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D|Scale")
	bool bScaleWithDistance = true;

	/**
	 * Distancia de referencia para el tamaño base (en unidades)
	 * El widget tendrá el tamaño configurado en DrawSize a esta distancia
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D|Scale", meta = (EditCondition = "bScaleWithDistance", ClampMin = "10.0"))
	float ReferenceDistance = 200.0f;

	/**
	 * Multiplicador de escala mínimo (para evitar que sea demasiado pequeño de cerca)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D|Scale", meta = (EditCondition = "bScaleWithDistance", ClampMin = "0.1", ClampMax = "1.0"))
	float MinScaleMultiplier = 0.3f;

	/**
	 * Multiplicador de escala máximo (para evitar que sea demasiado grande de lejos)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Widget 3D|Scale", meta = (EditCondition = "bScaleWithDistance", ClampMin = "1.0", ClampMax = "5.0"))
	float MaxScaleMultiplier = 2.0f;

	// ==================== FUNCIONES PÚBLICAS ====================

	/**
	 * Muestra el widget 3D
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void ShowWidget();

	/**
	 * Oculta el widget 3D
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void HideWidget();

	/**
	 * Actualiza el texto del widget (llama al método del widget UMG)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void UpdateInteractionText(const FText& NewText);

	/**	 * Actualiza el texto desde el actor owner (si implementa IInteractableInterface)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void UpdateTextFromOwner();

	/**
	 * Llamado cuando el actor owner gana foco
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void OnOwnerFocusGained();

	/**
	 * Llamado cuando el actor owner pierde foco
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction Widget 3D")
	void OnOwnerFocusLost();

protected:
	/**
	 * Rota el widget para que mire a la cámara del jugador
	 */
	void UpdateRotationToFaceCamera();

	/**
	 * Actualiza la posición del widget para que esté entre la cámara y el objeto
	 */
	void UpdatePositionBetweenCameraAndObject();

	/**
	 * Actualiza la escala del widget basándose en la distancia para mantener tamaño constante en pantalla
	 */
	void UpdateScaleWithDistance();

	/**
	 * Verifica si el jugador está dentro del rango para ver el widget
	 */
	bool IsPlayerInRange() const;

	/**
	 * Cache del widget user object
	 */
	UPROPERTY()
	UUserWidget* CachedWidget;

	/**
	 * Estado de visibilidad
	 */
	bool bIsCurrentlyVisible = false;
};
// Fill out your copyright notice in the Description page of Project Settings.
