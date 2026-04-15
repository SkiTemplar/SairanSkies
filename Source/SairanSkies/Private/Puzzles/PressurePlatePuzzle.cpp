// SairanSkies - Puzzle de Placas de Presión

#include "Puzzles/PressurePlatePuzzle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// ─── APressurePlate ───────────────────────────────────────────────────────────

APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->InitBoxExtent(FVector(60.f, 60.f, 8.f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAll"));
	RootComponent = TriggerBox;

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(RootComponent);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded())
	{
		PlateMesh->SetStaticMesh(Cube.Object);
		PlateMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.15f));
	}
}

void APressurePlate::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::OnBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::OnEndOverlap);
}

void APressurePlate::OnBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!OtherActor || OtherActor == this) return;
	OverlapCount++;
	if (OverlapCount == 1) SetActivated(true);
}

void APressurePlate::OnEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!OtherActor || OtherActor == this) return;
	OverlapCount = FMath::Max(0, OverlapCount - 1);
	if (OverlapCount == 0) SetActivated(false);
}

void APressurePlate::SetActivated(bool bActive)
{
	if (bIsActivated == bActive) return;
	bIsActivated = bActive;

	// Feedback visual
	UMaterialInterface* Mat = bActive ? ActiveMaterial : InactiveMaterial;
	if (Mat) PlateMesh->SetMaterial(0, Mat);

	// Notificar al puzzle manager
	if (OwnerPuzzle) OwnerPuzzle->NotifyPlateChanged();
}

// ─── APressurePlatePuzzle ─────────────────────────────────────────────────────

APressurePlatePuzzle::APressurePlatePuzzle()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APressurePlatePuzzle::BeginPlay()
{
	Super::BeginPlay();

	// Auto-registrar las placas que ya tienen referencia a este puzzle
	for (APressurePlate* Plate : Plates)
	{
		if (Plate) Plate->OwnerPuzzle = this;
	}
}

void APressurePlatePuzzle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLerping || !TargetActor) return;

	LerpTimer += DeltaTime;
	float Alpha = FMath::Clamp(LerpTimer / FMath::Max(TransformLerpDuration, 0.01f), 0.0f, 1.0f);
	float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	FVector   NewLoc   = FMath::Lerp(LerpStartLoc,   LerpStartLoc   + TargetTranslation, EasedAlpha);
	FRotator  NewRot   = FMath::Lerp(LerpStartRot,   LerpStartRot   + TargetRotation,    EasedAlpha);
	FVector   NewScale = FMath::Lerp(LerpStartScale, LerpStartScale * TargetScale,        EasedAlpha);

	TargetActor->SetActorLocation(NewLoc);
	TargetActor->SetActorRotation(NewRot);
	TargetActor->SetActorScale3D(NewScale);

	if (Alpha >= 1.0f) bIsLerping = false;
}

void APressurePlatePuzzle::NotifyPlateChanged()
{
	if (bIsSolved) return;

	// ¿Todas activas?
	for (APressurePlate* Plate : Plates)
	{
		if (!Plate || !Plate->bIsActivated) return;
	}

	// ¡Puzzle resuelto!
	bIsSolved = true;
	UE_LOG(LogTemp, Warning, TEXT("PressurePlatePuzzle: ¡RESUELTO! (%d placas)"), Plates.Num());

	if (SolvedSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SolvedSound, GetActorLocation());

	ApplyTransform();
}

void APressurePlatePuzzle::ApplyTransform()
{
	if (!TargetActor) return;

	if (TransformLerpDuration <= 0.0f)
	{
		// Instantáneo
		TargetActor->SetActorLocation(TargetActor->GetActorLocation() + TargetTranslation);
		TargetActor->SetActorRotation(TargetActor->GetActorRotation() + TargetRotation);
		TargetActor->SetActorScale3D(TargetActor->GetActorScale3D() * TargetScale);
	}
	else
	{
		// Guardar punto de partida para el lerp en Tick
		LerpStartLoc   = TargetActor->GetActorLocation();
		LerpStartRot   = TargetActor->GetActorRotation();
		LerpStartScale = TargetActor->GetActorScale3D();
		LerpTimer  = 0.0f;
		bIsLerping = true;
	}
}
