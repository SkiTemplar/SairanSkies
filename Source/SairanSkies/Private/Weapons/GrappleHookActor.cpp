// SairanSkies - Grapple Hook Visual Actor Implementation

#include "Weapons/GrappleHookActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGrappleHookActor::AGrappleHookActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	// Handle mesh (the part you hold)
	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(RootSceneComponent);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Hook mesh (the pointy part)
	HookMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookMesh"));
	HookMesh->SetupAttachment(RootSceneComponent);
	HookMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Load default cube mesh for placeholder
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		HandleMesh->SetStaticMesh(CubeMesh.Object);
		HookMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AGrappleHookActor::BeginPlay()
{
	Super::BeginPlay();
	SetupPlaceholderMesh();
	
	// Start hidden
	HideHook();
}

void AGrappleHookActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGrappleHookActor::SetupPlaceholderMesh()
{
	// Create dynamic material
	if (HandleMesh && HandleMesh->GetStaticMesh())
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(
			HandleMesh->GetMaterial(0), this);
		
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), GrappleColor);
			HandleMesh->SetMaterial(0, DynamicMaterial);
			HookMesh->SetMaterial(0, DynamicMaterial);
		}
	}

	// Scale handle
	FVector HandleScale = HandleSize / 100.0f; // Cube is 100 units
	HandleMesh->SetRelativeScale3D(HandleScale);
	HandleMesh->SetRelativeLocation(FVector(0, 0, 0));

	// Scale and position hook
	FVector HookScale = HookSize / 100.0f;
	HookMesh->SetRelativeScale3D(HookScale);
	// Position hook at the end of handle, pointing forward
	float HookOffsetZ = (HandleSize.Z / 2.0f) + (HookSize.Z / 2.0f);
	HookMesh->SetRelativeLocation(FVector(HookSize.X / 2.0f, 0, HookOffsetZ));
	HookMesh->SetRelativeRotation(FRotator(0, 0, 0));
}

void AGrappleHookActor::ShowHook()
{
	SetActorHiddenInGame(false);
	HandleMesh->SetVisibility(true);
	HookMesh->SetVisibility(true);
}

void AGrappleHookActor::HideHook()
{
	SetActorHiddenInGame(true);
	HandleMesh->SetVisibility(false);
	HookMesh->SetVisibility(false);
}

void AGrappleHookActor::AimAtLocation(const FVector& TargetLocation)
{
	// Make the hook point towards the target
	FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
	FRotator LookAtRotation = Direction.Rotation();
	
	// Adjust rotation so the hook points correctly
	// The hook extends in Z, so we need to rotate appropriately
	LookAtRotation.Pitch -= 90.0f; // Rotate to point hook forward
	
	SetActorRotation(LookAtRotation);
}

void AGrappleHookActor::SetTargetValid(bool bIsValid)
{
	if (DynamicMaterial)
	{
		FLinearColor TargetColor = bIsValid ? ValidColor : InvalidColor;
		DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), TargetColor);
	}
}
