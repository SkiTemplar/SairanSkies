// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractionWidget3DComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

UInteractionWidget3DComponent::UInteractionWidget3DComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Configuración base del WidgetComponent
	SetWidgetSpace(EWidgetSpace::World);
	SetDrawSize(FVector2D(200.0f, 50.0f)); // Tamaño por defecto
	SetVisibility(false);
	
	// Configuración de colisión (no bloquear nada)
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UInteractionWidget3DComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Aplicar offset
	SetRelativeLocation(WidgetOffset);
	
	// Cache del widget
	CachedWidget = GetWidget();
	
	// Ocultar al inicio si está configurado
	if (bAutoHideWhenNotFocused)
	{
		HideWidget();
	}
	
	// Actualizar texto inicial desde el owner
	UpdateTextFromOwner();
}

void UInteractionWidget3DComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsCurrentlyVisible)
	{
		// Actualizar posición si está en modo "entre cámara y objeto"
		if (bPositionBetweenCameraAndObject)
		{
			UpdatePositionBetweenCameraAndObject();
		}
		
		// Rotar hacia la cámara
		if (bAlwaysFaceCamera)
		{
			UpdateRotationToFaceCamera();
		}
		
		// Actualizar escala con la distancia
		if (bScaleWithDistance)
		{
			UpdateScaleWithDistance();
		}
	}
	
	// Verificar distancia
	if (MaxDrawDistance > 0.0f && bIsCurrentlyVisible)
	{
		if (!IsPlayerInRange())
		{
			HideWidget();
		}
	}
}

void UInteractionWidget3DComponent::ShowWidget()
{
	if (!bIsCurrentlyVisible)
	{
		SetVisibility(true);
		bIsCurrentlyVisible = true;
		
		// Actualizar texto al mostrar
		UpdateTextFromOwner();
	}
}

void UInteractionWidget3DComponent::HideWidget()
{
	if (bIsCurrentlyVisible)
	{
		SetVisibility(false);
		bIsCurrentlyVisible = false;
	}
}

void UInteractionWidget3DComponent::UpdateInteractionText(const FText& NewText)
{
	if (!CachedWidget)
	{
		CachedWidget = GetWidget();
	}
	
	if (CachedWidget)
	{
		// Buscar el TextBlock por nombre
		UTextBlock* TextBlock = Cast<UTextBlock>(CachedWidget->GetWidgetFromName(TextVariableName));
		if (TextBlock)
		{
			TextBlock->SetText(NewText);
		}
		else
		{
			// Si no se encuentra por nombre, intentar buscar el primer TextBlock
			// Esto es un fallback para mayor compatibilidad
			UE_LOG(LogTemp, Warning, TEXT("InteractionWidget3DComponent: No se encontró TextBlock con nombre '%s' en el widget. Asegúrate de nombrar tu TextBlock correctamente."), *TextVariableName.ToString());
		}
	}
}

void UInteractionWidget3DComponent::UpdateTextFromOwner()
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<UInteractableInterface>())
	{
		FText InteractionText = IInteractableInterface::Execute_GetInteractionText(Owner);
		UpdateInteractionText(InteractionText);
	}
}

void UInteractionWidget3DComponent::OnOwnerFocusGained()
{
	if (bAutoShowOnFocus)
	{
		ShowWidget();
	}
}

void UInteractionWidget3DComponent::OnOwnerFocusLost()
{
	if (bAutoHideWhenNotFocused)
	{
		HideWidget();
	}
}

void UInteractionWidget3DComponent::UpdateRotationToFaceCamera()
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (CameraManager)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		FVector WidgetLocation = GetComponentLocation();
		
		// Calcular rotación hacia la cámara
		FRotator LookAtRotation = (CameraLocation - WidgetLocation).Rotation();
		
		// Aplicar rotación
		SetWorldRotation(LookAtRotation);
	}
}

void UInteractionWidget3DComponent::UpdatePositionBetweenCameraAndObject()
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	AActor* Owner = GetOwner();
	
	if (CameraManager && Owner)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		FVector ObjectLocation = Owner->GetActorLocation();
		
		// Calcular dirección desde el objeto hacia la cámara
		FVector Direction = (CameraLocation - ObjectLocation).GetSafeNormal();
		
		// Posicionar el widget a cierta distancia del objeto en dirección a la cámara
		FVector WidgetPosition = ObjectLocation + (Direction * DistanceFromObject);
		
		// Aplicar la posición
		SetWorldLocation(WidgetPosition);
	}
}

void UInteractionWidget3DComponent::UpdateScaleWithDistance()
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!CameraManager)
	{
		return;
	}
	
	FVector CameraLocation = CameraManager->GetCameraLocation();
	FVector WidgetLocation = GetComponentLocation();
	
	// Calcular distancia actual a la cámara
	float CurrentDistance = FVector::Dist(CameraLocation, WidgetLocation);
	
	// Calcular el factor de escala basado en la distancia
	// A distancia de referencia = escala 1.0
	// Más cerca = más pequeño, más lejos = más grande
	float ScaleFactor = CurrentDistance / ReferenceDistance;
	
	// Aplicar límites de escala
	ScaleFactor = FMath::Clamp(ScaleFactor, MinScaleMultiplier, MaxScaleMultiplier);
	
	// Aplicar la escala al componente completo (no solo DrawSize)
	// Esto escala todo: el widget, el texto, etc.
	FVector NewScale = FVector(ScaleFactor, ScaleFactor, ScaleFactor);
	SetWorldScale3D(NewScale);
}

bool UInteractionWidget3DComponent::IsPlayerInRange() const
{
	if (MaxDrawDistance <= 0.0f)
	{
		return true; // Sin límite de distancia
	}
	
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (CameraManager)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		FVector WidgetLocation = GetComponentLocation();
		
		float DistanceSquared = FVector::DistSquared(CameraLocation, WidgetLocation);
		float MaxDistanceSquared = MaxDrawDistance * MaxDrawDistance;
		
		return DistanceSquared <= MaxDistanceSquared;
	}
	
	return false;
}
