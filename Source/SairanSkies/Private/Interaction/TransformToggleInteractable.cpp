// Fill out your copyright notice in the Description page of Project Settings.
// Last updated: 2026-01-06 - Multi-target lever system with independent transforms

#include "Interaction/TransformToggleInteractable.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ATransformToggleInteractable::ATransformToggleInteractable()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Crear componente raíz
	LeverRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LeverRoot"));
	RootComponent = LeverRoot;

	// Crear mesh de la BASE (estática)
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(LeverRoot);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BaseMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Crear mesh de la PALANCA (que se transforma)
	LeverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeverMesh"));
	LeverMesh->SetupAttachment(LeverRoot);
	LeverMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LeverMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Configuración por defecto
	InteractionText = FText::FromString("Press E to Activate My Friend");
	bCanInteractMultipleTimes = true;

	// Valores por defecto de la palanca
	bLeverApplyRotation = true;
	LeverRotationOffset = FRotator(-45.0f, 0.0f, 0.0f);
	LeverTransitionSpeed = 5.0f;
}

void ATransformToggleInteractable::BeginPlay()
{
	Super::BeginPlay();

	SaveInitialStates();
	InteractionText = TextWhenOriginal;
}

void ATransformToggleInteractable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	bool bStillTransitioning = false;

	// Actualizar transformación de la palanca
	if (bIsLeverTransitioning)
	{
		UpdateLeverTransform(DeltaTime);
		if (!IsLeverTransitionComplete())
		{
			bStillTransitioning = true;
		}
		else
		{
			FinishLeverTransition();
		}
	}

	// Actualizar transformaciones de los objetivos
	UpdateTargetsTransform(DeltaTime);
	if (!AreAllTargetsTransitionComplete())
	{
		bStillTransitioning = true;
	}
	else if (bUseSmoothTransition)
	{
		FinishTargetsTransition();
	}

	// Desactivar tick si no hay nada que actualizar
	if (!bStillTransitioning)
	{
		SetActorTickEnabled(false);
	}

	// Debug visual
	if (bShowDebug)
	{
		for (const FTargetTransformData& TargetData : TargetTransforms)
		{
			if (TargetData.TargetActor && TargetData.bOriginalSaved)
			{
				DrawDebugSphere(GetWorld(), TargetData.OriginalTransform.GetLocation(), 20.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
				DrawDebugSphere(GetWorld(), TargetData.TransformedTransform.GetLocation(), 20.0f, 8, FColor::Red, false, -1.0f, 0, 2.0f);
				DrawDebugLine(GetWorld(), TargetData.OriginalTransform.GetLocation(), TargetData.TransformedTransform.GetLocation(), FColor::Yellow, false, -1.0f, 0, 2.0f);
			}
		}
	}
}

bool ATransformToggleInteractable::Interact_Implementation(AActor* InteractInstigator)
{
	if (!bCanBeInteracted)
	{
		return false;
	}

	// Bloquear si está en transición
	if (bBlockInteractionDuringTransition && IsTransitioning())
	{
		return false;
	}

	// Toggle el estado
	bool bNewState = !bIsTransformed;
	bIsTransformed = bNewState;

	// Reproducir sonido de INICIO de transformación (siempre al inicio)
	if (TransformStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TransformStartSound, GetActorLocation(), SoundVolume);
	}

	// Reproducir sonido de activación/desactivación (adicional)
	if (bNewState && ActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivateSound, GetActorLocation(), SoundVolume);
	}
	else if (!bNewState && DeactivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeactivateSound, GetActorLocation(), SoundVolume);
	}

	// Iniciar movimiento de la palanca
	if (LeverMesh)
	{
		bIsLeverTransitioning = true;
		SetActorTickEnabled(true);
		OnLeverMovementStartedBP(bNewState);
	}

	// Aplicar transformación a todos los objetivos
	ApplyTransformToTargets(bNewState);

	// Actualizar texto
	InteractionText = bIsTransformed ? TextWhenTransformed : TextWhenOriginal;

	OnTransformStartedBP(bNewState);
	OnInteracted(InteractInstigator);
	
	return true;
}

FText ATransformToggleInteractable::GetInteractionText_Implementation() const
{
	return bIsTransformed ? TextWhenTransformed : TextWhenOriginal;
}

void ATransformToggleInteractable::SetTransformed(bool bNewTransformed)
{
	if (bNewTransformed != bIsTransformed)
	{
		bIsTransformed = bNewTransformed;

		if (LeverMesh)
		{
			bIsLeverTransitioning = true;
			SetActorTickEnabled(true);
			OnLeverMovementStartedBP(bNewTransformed);
		}

		ApplyTransformToTargets(bNewTransformed);
		InteractionText = bIsTransformed ? TextWhenTransformed : TextWhenOriginal;
	}
}

bool ATransformToggleInteractable::IsTransitioning() const
{
	if (bIsLeverTransitioning)
	{
		return true;
	}

	for (const FTargetTransformData& TargetData : TargetTransforms)
	{
		if (TargetData.bIsTransitioning)
		{
			return true;
		}
	}

	return false;
}

void ATransformToggleInteractable::SaveInitialStates()
{
	if (bInitialStatesSaved)
	{
		return;
	}

	// Guardar transformación original de la palanca
	if (LeverMesh)
	{
		LeverOriginalTransform = LeverMesh->GetRelativeTransform();

		// Calcular transformación objetivo de la palanca
		FTransform OffsetTransform;
		if (bLeverApplyLocation)
		{
			OffsetTransform.SetLocation(LeverLocationOffset);
		}
		if (bLeverApplyRotation)
		{
			OffsetTransform.SetRotation(LeverRotationOffset.Quaternion());
		}
		if (bLeverApplyScale)
		{
			OffsetTransform.SetScale3D(LeverScaleOffset);
		}

		LeverTargetTransform = LeverOriginalTransform;
		if (bLeverApplyLocation)
		{
			LeverTargetTransform.SetLocation(LeverOriginalTransform.GetLocation() + LeverLocationOffset);
		}
		if (bLeverApplyRotation)
		{
			FRotator NewRotation = LeverOriginalTransform.GetRotation().Rotator() + LeverRotationOffset;
			LeverTargetTransform.SetRotation(NewRotation.Quaternion());
		}
		if (bLeverApplyScale)
		{
			LeverTargetTransform.SetScale3D(LeverOriginalTransform.GetScale3D() * LeverScaleOffset);
		}
	}

	// Guardar estados iniciales de todos los objetivos
	for (FTargetTransformData& TargetData : TargetTransforms)
	{
		if (!TargetData.TargetActor)
		{
			continue;
		}

		TargetData.OriginalTransform = TargetData.TargetActor->GetActorTransform();
		TargetData.bOriginalSaved = true;

		// Calcular transformación objetivo
		if (TargetData.bUseAbsoluteTransform)
		{
			TargetData.TransformedTransform = TargetData.AbsoluteTransform;
		}
		else
		{
			TargetData.TransformedTransform = TargetData.OriginalTransform;

			if (TargetData.bApplyLocation)
			{
				FVector NewLocation = TargetData.OriginalTransform.GetLocation() + TargetData.LocationOffset;
				TargetData.TransformedTransform.SetLocation(NewLocation);
			}
			if (TargetData.bApplyRotation)
			{
				FRotator NewRotation = TargetData.OriginalTransform.GetRotation().Rotator() + TargetData.RotationOffset;
				TargetData.TransformedTransform.SetRotation(NewRotation.Quaternion());
			}
			if (TargetData.bApplyScale)
			{
				FVector NewScale = TargetData.OriginalTransform.GetScale3D() * TargetData.ScaleOffset;
				TargetData.TransformedTransform.SetScale3D(NewScale);
			}
		}

		if (bShowDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("TransformToggle: Saved initial state for %s"), *TargetData.TargetActor->GetName());
		}
	}

	bInitialStatesSaved = true;
}

void ATransformToggleInteractable::ApplyTransformToTargets(bool bToTransformed)
{
	for (FTargetTransformData& TargetData : TargetTransforms)
	{
		if (!TargetData.TargetActor || !TargetData.bOriginalSaved)
		{
			continue;
		}

		FTransform TargetTransform = bToTransformed ? TargetData.TransformedTransform : TargetData.OriginalTransform;

		if (bUseSmoothTransition)
		{
			TargetData.bIsTransitioning = true;
			SetActorTickEnabled(true);
		}
		else
		{
			TargetData.TargetActor->SetActorTransform(TargetTransform);
			TargetData.bIsTransitioning = false;
		}
	}
}

void ATransformToggleInteractable::UpdateLeverTransform(float DeltaTime)
{
	if (!LeverMesh)
	{
		bIsLeverTransitioning = false;
		return;
	}

	FTransform CurrentTransform = LeverMesh->GetRelativeTransform();
	FTransform TargetTransform = bIsTransformed ? LeverTargetTransform : LeverOriginalTransform;

	// Interpolar cada componente según lo configurado
	FVector NewLocation = CurrentTransform.GetLocation();
	FQuat NewRotation = CurrentTransform.GetRotation();
	FVector NewScale = CurrentTransform.GetScale3D();

	if (bLeverApplyLocation)
	{
		NewLocation = FMath::VInterpTo(CurrentTransform.GetLocation(), TargetTransform.GetLocation(), DeltaTime, LeverTransitionSpeed);
	}
	if (bLeverApplyRotation)
	{
		NewRotation = FMath::QInterpTo(CurrentTransform.GetRotation(), TargetTransform.GetRotation(), DeltaTime, LeverTransitionSpeed);
	}
	if (bLeverApplyScale)
	{
		NewScale = FMath::VInterpTo(CurrentTransform.GetScale3D(), TargetTransform.GetScale3D(), DeltaTime, LeverTransitionSpeed);
	}

	FTransform NewTransform;
	NewTransform.SetLocation(NewLocation);
	NewTransform.SetRotation(NewRotation);
	NewTransform.SetScale3D(NewScale);
	LeverMesh->SetRelativeTransform(NewTransform);
}

void ATransformToggleInteractable::UpdateTargetsTransform(float DeltaTime)
{
	for (FTargetTransformData& TargetData : TargetTransforms)
	{
		if (!TargetData.bIsTransitioning || !TargetData.TargetActor)
		{
			continue;
		}

		FTransform CurrentTransform = TargetData.TargetActor->GetActorTransform();
		FTransform TargetTransform = bIsTransformed ? TargetData.TransformedTransform : TargetData.OriginalTransform;

		// Interpolar cada componente según lo configurado
		FVector NewLocation = CurrentTransform.GetLocation();
		FQuat NewRotation = CurrentTransform.GetRotation();
	FVector NewScale = CurrentTransform.GetScale3D();

		if (TargetData.bApplyLocation || TargetData.bUseAbsoluteTransform)
		{
			NewLocation = FMath::VInterpTo(CurrentTransform.GetLocation(), TargetTransform.GetLocation(), DeltaTime, TargetData.TransitionSpeed);
		}
		if (TargetData.bApplyRotation || TargetData.bUseAbsoluteTransform)
		{
			NewRotation = FMath::QInterpTo(CurrentTransform.GetRotation(), TargetTransform.GetRotation(), DeltaTime, TargetData.TransitionSpeed);
		}
		if (TargetData.bApplyScale || TargetData.bUseAbsoluteTransform)
		{
			NewScale = FMath::VInterpTo(CurrentTransform.GetScale3D(), TargetTransform.GetScale3D(), DeltaTime, TargetData.TransitionSpeed);
		}

		FTransform NewTransform;
		NewTransform.SetLocation(NewLocation);
		NewTransform.SetRotation(NewRotation);
		NewTransform.SetScale3D(NewScale);
		TargetData.TargetActor->SetActorTransform(NewTransform);

		// Verificar si llegó al destino
		float DistanceToTarget = FVector::Dist(NewLocation, TargetTransform.GetLocation());
		float RotationDiff = FQuat::Error(NewRotation, TargetTransform.GetRotation());
		float ScaleDiff = FVector::Dist(NewScale, TargetTransform.GetScale3D());

		if (DistanceToTarget <= TransitionTolerance && RotationDiff <= 0.01f && ScaleDiff <= 0.01f)
		{
			TargetData.TargetActor->SetActorTransform(TargetTransform);
			TargetData.bIsTransitioning = false;
		}
	}
}

bool ATransformToggleInteractable::IsLeverTransitionComplete() const
{
	if (!LeverMesh || !bIsLeverTransitioning)
	{
		return true;
	}

	FTransform CurrentTransform = LeverMesh->GetRelativeTransform();
	FTransform TargetTransform = bIsTransformed ? LeverTargetTransform : LeverOriginalTransform;

	float LocationDiff = FVector::Dist(CurrentTransform.GetLocation(), TargetTransform.GetLocation());
	float RotationDiff = FQuat::Error(CurrentTransform.GetRotation(), TargetTransform.GetRotation());
	float ScaleDiff = FVector::Dist(CurrentTransform.GetScale3D(), TargetTransform.GetScale3D());

	return LocationDiff <= TransitionTolerance && RotationDiff <= 0.01f && ScaleDiff <= 0.01f;
}

bool ATransformToggleInteractable::AreAllTargetsTransitionComplete() const
{
	for (const FTargetTransformData& TargetData : TargetTransforms)
	{
		if (TargetData.bIsTransitioning)
		{
			return false;
		}
	}
	return true;
}

void ATransformToggleInteractable::FinishLeverTransition()
{
	if (LeverMesh)
	{
		FTransform TargetTransform = bIsTransformed ? LeverTargetTransform : LeverOriginalTransform;
		LeverMesh->SetRelativeTransform(TargetTransform);
	}

	bIsLeverTransitioning = false;
	OnLeverMovementFinishedBP(bIsTransformed);
}

void ATransformToggleInteractable::FinishTargetsTransition()
{
	bool bWasTransitioning = false;

	for (FTargetTransformData& TargetData : TargetTransforms)
	{
		if (TargetData.bIsTransitioning && TargetData.TargetActor)
		{
			FTransform TargetTransform = bIsTransformed ? TargetData.TransformedTransform : TargetData.OriginalTransform;
			TargetData.TargetActor->SetActorTransform(TargetTransform);
			TargetData.bIsTransitioning = false;
			bWasTransitioning = true;
		}
	}

	if (bWasTransitioning)
	{
		// Reproducir sonido de transformación completa
		if (TransformCompleteSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TransformCompleteSound, GetActorLocation(), SoundVolume);
		}
		
		OnTransformFinishedBP(bIsTransformed);
	}
}
