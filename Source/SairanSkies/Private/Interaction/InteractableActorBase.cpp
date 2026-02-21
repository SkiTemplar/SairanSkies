// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractableActorBase.h"
#include "Interaction/InteractionWidget3DComponent.h"
#include "Components/StaticMeshComponent.h"

AInteractableActorBase::AInteractableActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	// Crear el componente de widget 3D (opcional, se activa con bUse3DWidget)
	InteractionWidget3D = CreateDefaultSubobject<UInteractionWidget3DComponent>(TEXT("InteractionWidget3D"));
	InteractionWidget3D->SetupAttachment(RootComponent);
}

void AInteractableActorBase::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent && MeshComponent->GetNumMaterials() > 0)
	{
		OriginalMaterial = MeshComponent->GetMaterial(0);
	}

	// Configurar el widget 3D según la preferencia del usuario
	if (InteractionWidget3D)
	{
		if (!bUse3DWidget)
		{
			// Si no queremos usar widget 3D, ocultarlo por completo
			InteractionWidget3D->SetVisibility(false);
			InteractionWidget3D->SetComponentTickEnabled(false);
		}
	}
}

bool AInteractableActorBase::Interact_Implementation(AActor* InteractInstigator)
{
	if (!bCanBeInteracted)
	{
		return false;
	}

	InteractionCount++;

	if (GEngine)
	{
		FString Message = FString::Printf(
			TEXT("Interactuando con %s (Interacción #%d)"),
			*GetName(),
			InteractionCount
		);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Message);
	}

	// Llamar al evento de interacción
	OnInteracted(InteractInstigator);

	// Si no se puede interactuar más veces, desactivar y forzar pérdida de foco
	if (!bCanInteractMultipleTimes)
	{
		bCanBeInteracted = false;
		
		// Forzar la pérdida de foco inmediatamente (ocultar widget, quitar highlight)
		OnFocusLost_Implementation();
	}

	return true;
}

bool AInteractableActorBase::CanInteract_Implementation(AActor* InteractInstigator) const
{
	return bCanBeInteracted;
}

FText AInteractableActorBase::GetInteractionText_Implementation() const
{
	return InteractionText;
}

void AInteractableActorBase::OnFocusGained_Implementation()
{
	// Solo hacer highlight si el objeto puede ser interactuado
	if (!bCanBeInteracted)
	{
		return;
	}

	if (bAutoHighlight && HighlightMaterial && MeshComponent)
	{
		MeshComponent->SetMaterial(0, HighlightMaterial);
	}

	// Notificar al widget 3D si está activo
	if (InteractionWidget3D && bUse3DWidget)
	{
		InteractionWidget3D->OnOwnerFocusGained();
	}

	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("Foco ganado: %s"), *GetName());
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, Message);
	}

	OnFocusGainedBP();
}

void AInteractableActorBase::OnFocusLost_Implementation()
{
	// Restaurar material si estaba con highlight
	if (bAutoHighlight && OriginalMaterial && MeshComponent)
	{
		MeshComponent->SetMaterial(0, OriginalMaterial);
	}

	// Notificar al widget 3D si está activo
	if (InteractionWidget3D && bUse3DWidget)
	{
		InteractionWidget3D->OnOwnerFocusLost();
	}

	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("Foco perdido: %s"), *GetName());
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Orange, Message);
	}

	OnFocusLostBP();
}

float AInteractableActorBase::GetInteractionDistance_Implementation() const
{
	return CustomInteractionDistance;
}
