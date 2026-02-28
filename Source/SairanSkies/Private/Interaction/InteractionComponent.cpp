// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedCamera = Owner->FindComponentByClass<UCameraComponent>();
	}
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bContinuousTrace)
	{
		UpdateFocusedActor();
	}
}

bool UInteractionComponent::TryInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("TryInteract: Llamado"));

	if (!bContinuousTrace)
	{
		UpdateFocusedActor();
	}

	if (!CurrentFocusedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryInteract: FALLO - CurrentFocusedActor es NULL"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TryInteract: CurrentFocusedActor = %s"), *GetNameSafe(CurrentFocusedActor));

	if (!CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryInteract: FALLO - No implementa interfaz"));
		return false;
	}

	IInteractableInterface* Interactable = Cast<IInteractableInterface>(CurrentFocusedActor);
	if (!Interactable)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryInteract: FALLO - Cast a IInteractableInterface falló"));
		return false;
	}

	if (!IInteractableInterface::Execute_CanInteract(CurrentFocusedActor, GetOwner()))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryInteract: FALLO - CanInteract retornó false"));
		OnInteractionPerformed.Broadcast(CurrentFocusedActor, false);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TryInteract: ÉXITO - Ejecutando Interact()"));
	bool bSuccess = IInteractableInterface::Execute_Interact(CurrentFocusedActor, GetOwner());
	OnInteractionPerformed.Broadcast(CurrentFocusedActor, bSuccess);

	UE_LOG(LogTemp, Warning, TEXT("TryInteract: Interact retornó %s"), bSuccess ? TEXT("TRUE") : TEXT("FALSE"));
	return bSuccess;
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
	if (!CurrentFocusedActor)
	{
		return FText::GetEmpty();
	}

	if (!CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return FText::GetEmpty();
	}

	return IInteractableInterface::Execute_GetInteractionText(CurrentFocusedActor);
}

bool UInteractionComponent::CanInteractWithFocusedActor() const
{
	if (!CurrentFocusedActor)
	{
		return false;
	}

	if (!CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return false;
	}

	return IInteractableInterface::Execute_CanInteract(CurrentFocusedActor, GetOwner());
}

void UInteractionComponent::UpdateFocusedActor()
{
	AActor* HitActor = nullptr;
	PerformInteractionTrace(HitActor);

	if (HitActor != CurrentFocusedActor)
	{
		SetFocusedActor(HitActor);
	}
}

bool UInteractionComponent::PerformInteractionTrace(AActor*& OutHitActor)
{
	OutHitActor = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector TraceStart;
	FVector TraceDirection;
	GetTraceStartAndDirection(TraceStart, TraceDirection);

	FVector TraceEnd = TraceStart + (TraceDirection * InteractionDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	FHitResult HitResult;

	bool bHit;
	if (InteractionSphereRadius > 0.0f)
	{
		// Sweep sphere trace - mejor para tercera persona con margen de error
		bHit = World->SweepSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(InteractionSphereRadius),
			QueryParams
		);
	}
	else
	{
		// Line trace puro - sin margen de error
		bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			TraceChannel,
			QueryParams
		);
	}

	if (bShowDebugTrace)
	{
		FColor DebugColor = bHit ? FColor::Green : FColor::Red;
		
		if (InteractionSphereRadius > 0.0f)
		{
			// Dibujar la esfera de detección
			DrawDebugSphere(World, bHit ? HitResult.Location : TraceEnd, InteractionSphereRadius, 12, DebugColor, false, 0.0f);
		}
		
		// Dibujar la línea de traza
		DrawDebugLine(World, TraceStart, bHit ? HitResult.Location : TraceEnd, DebugColor, false, 0.0f, 0, 2.0f);
		
		if (bHit)
		{
			DrawDebugPoint(World, HitResult.ImpactPoint, 10.0f, FColor::Yellow, false, 0.0f);
		}
	}

	if (!bHit || !HitResult.GetActor())
	{
		return false;
	}

	AActor* HitActor = HitResult.GetActor();

	if (!HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return false;
	}

	float CustomDistance = IInteractableInterface::Execute_GetInteractionDistance(HitActor);
	float ActualDistance = FVector::Dist(TraceStart, HitResult.Location);
	float MaxAllowedDistance = (CustomDistance > 0.0f) ? CustomDistance : InteractionDistance;

	if (ActualDistance > MaxAllowedDistance)
	{
		return false;
	}
	
	OutHitActor = HitActor;
	return true;
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocusedActor)
{
	// Validar que el nuevo actor puede ser interactuado
	if (NewFocusedActor)
	{
		if (!NewFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			NewFocusedActor = nullptr;
		}
		else if (!IInteractableInterface::Execute_CanInteract(NewFocusedActor, GetOwner()))
		{
			NewFocusedActor = nullptr;
		}
	}

	// Si no cambió, no hacer nada
	if (NewFocusedActor == CurrentFocusedActor)
	{
		return;
	}

	AActor* PreviousFocusedActor = CurrentFocusedActor;

	// Perder foco del actor anterior
	if (PreviousFocusedActor && PreviousFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("LOST FOCUS: %s"), *GetNameSafe(PreviousFocusedActor));
		IInteractableInterface::Execute_OnFocusLost(PreviousFocusedActor);
	}

	CurrentFocusedActor = NewFocusedActor;

	// Ganar foco en el nuevo actor
	if (CurrentFocusedActor && CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("GAINED FOCUS: %s"), *GetNameSafe(CurrentFocusedActor));
		IInteractableInterface::Execute_OnFocusGained(CurrentFocusedActor);
	}

	OnFocusedActorChanged.Broadcast(CurrentFocusedActor, PreviousFocusedActor);
}

void UInteractionComponent::GetTraceStartAndDirection(FVector& OutStart, FVector& OutDirection) const
{
	if (CachedCamera)
	{
		OutStart = CachedCamera->GetComponentLocation();
		OutDirection = CachedCamera->GetForwardVector();
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		UCameraComponent* FoundCamera = Owner->FindComponentByClass<UCameraComponent>();
		if (FoundCamera)
		{
			OutStart = FoundCamera->GetComponentLocation();
			OutDirection = FoundCamera->GetForwardVector();
			return;
		}

		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (AController* Controller = OwnerPawn->GetController())
			{
				OutStart = Owner->GetActorLocation();
				OutDirection = Controller->GetControlRotation().Vector();
				return;
			}
		}

		OutStart = Owner->GetActorLocation();
		OutDirection = Owner->GetActorForwardVector();
		return;
	}

	OutStart = FVector::ZeroVector;
	OutDirection = FVector::ForwardVector;
}
