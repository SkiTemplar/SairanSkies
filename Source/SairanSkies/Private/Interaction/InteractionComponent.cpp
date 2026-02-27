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
	if (!bContinuousTrace)
	{
		UpdateFocusedActor();
	}

	if (!CurrentFocusedActor)
	{
		return false;
	}

	if (!CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return false;
	}

	IInteractableInterface* Interactable = Cast<IInteractableInterface>(CurrentFocusedActor);
	if (!Interactable)
	{
		return false;
	}

	if (!IInteractableInterface::Execute_CanInteract(CurrentFocusedActor, GetOwner()))
	{
		OnInteractionPerformed.Broadcast(CurrentFocusedActor, false);
		return false;
	}

	bool bSuccess = IInteractableInterface::Execute_Interact(CurrentFocusedActor, GetOwner());
	OnInteractionPerformed.Broadcast(CurrentFocusedActor, bSuccess);

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
			DrawDebugSphere(World, bHit ? HitResult.Location : TraceEnd, InteractionSphereRadius, 12, DebugColor, false, 0.0f);
		}
		
		DrawDebugLine(World, TraceStart, bHit ? HitResult.Location : TraceEnd, DebugColor, false, 0.0f, 0, 2.0f);
		
		if (bHit)
		{
			DrawDebugPoint(World, HitResult.ImpactPoint, 10.0f, FColor::Yellow, false, 0.0f);
		}
	}

	if (!bHit || !HitResult.GetActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("Trace: No hit or no actor"));
		return false;
	}

	AActor* HitActor = HitResult.GetActor();
	UE_LOG(LogTemp, Warning, TEXT("Trace: Hit actor %s"), *GetNameSafe(HitActor));

	if (!HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Trace: %s does NOT implement InteractableInterface"), *GetNameSafe(HitActor));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Trace: %s implements InteractableInterface"), *GetNameSafe(HitActor));

	float CustomDistance = IInteractableInterface::Execute_GetInteractionDistance(HitActor);
	if (CustomDistance > 0.0f)
	{
		float Distance = FVector::Dist(TraceStart, HitResult.Location);
		UE_LOG(LogTemp, Warning, TEXT("Trace: %s custom distance check: distance=%.1f, max=%.1f"), *GetNameSafe(HitActor), Distance, CustomDistance);
		if (Distance > CustomDistance)
		{
			UE_LOG(LogTemp, Warning, TEXT("Trace: %s REJECTED - distance too far"), *GetNameSafe(HitActor));
			return false;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Trace: SUCCESS - %s can be interacted"), *GetNameSafe(HitActor));
	OutHitActor = HitActor;
	return true;
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocusedActor)
{
	// Si el nuevo actor no puede ser interactuado, tratarlo como null
	if (NewFocusedActor && NewFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		if (!IInteractableInterface::Execute_CanInteract(NewFocusedActor, GetOwner()))
		{
			// El actor no puede ser interactuado (ej: ya fue usado), no darle foco
			NewFocusedActor = nullptr;
		}
	}

	// Si el nuevo actor es null pero aún tenemos un actor en foco, aplicar debounce
	if (NewFocusedActor == nullptr && CurrentFocusedActor != nullptr)
	{
		// El raycast falló. Esperamos FocusLossDebouncTime segundos antes de perder foco
		if (!bWaitingForFocusLoss)
		{
			bWaitingForFocusLoss = true;
			FocusLossTimer = 0.0f;
			return; // No perdemos foco todavía
		}

		// Contar tiempo
		FocusLossTimer += GetWorld()->DeltaTimeSeconds;
		if (FocusLossTimer < FocusLossDebouncTime)
		{
			return; // Aún en período de debounce, mantener foco
		}
	}
	else
	{
		// Raycast detectó algo o el foco cambió, resetear debounce
		bWaitingForFocusLoss = false;
		FocusLossTimer = 0.0f;
	}

	if (NewFocusedActor == CurrentFocusedActor)
	{
		return;
	}

	AActor* PreviousFocusedActor = CurrentFocusedActor;

	// Perder foco del actor anterior
	if (PreviousFocusedActor && PreviousFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		IInteractableInterface::Execute_OnFocusLost(PreviousFocusedActor);
	}

	CurrentFocusedActor = NewFocusedActor;

	// Ganar foco en el nuevo actor (solo si no es null)
	if (CurrentFocusedActor && CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
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
		OutStart = Owner->GetActorLocation();
		OutDirection = Owner->GetActorForwardVector();

		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (AController* Controller = OwnerPawn->GetController())
			{
				OutDirection = Controller->GetControlRotation().Vector();
			}
		}
	}
}
