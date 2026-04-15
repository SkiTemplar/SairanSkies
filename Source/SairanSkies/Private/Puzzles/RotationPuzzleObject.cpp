// SairanSkies - Puzzle de Objetos Giratorios

#include "Puzzles/RotationPuzzleObject.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// ─── ARotationPuzzleObject ────────────────────────────────────────────────────

ARotationPuzzleObject::ARotationPuzzleObject()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionText = FText::FromString(TEXT("Girar"));
}

void ARotationPuzzleObject::BeginPlay()
{
	Super::BeginPlay();
	CurrentYaw = 0.0f;
	bIsInCorrectPosition = IsInCorrectPosition();
}

bool ARotationPuzzleObject::Interact_Implementation(AActor* Interactor)
{
	if (!bCanBeInteracted) return false;

	// Avanzar el ángulo
	CurrentYaw = FMath::Fmod(CurrentYaw + RotationStep, 360.0f);
	FRotator NewRot = GetActorRotation();
	NewRot.Yaw += RotationStep;
	SetActorRotation(NewRot);

	if (RotateSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), RotateSound, GetActorLocation());

	// Comprobar si hemos llegado a la posición correcta
	bool bWasCorrect = bIsInCorrectPosition;
	bIsInCorrectPosition = IsInCorrectPosition();

	if (bIsInCorrectPosition && !bWasCorrect && CorrectPositionSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CorrectPositionSound, GetActorLocation());

	UE_LOG(LogTemp, Log, TEXT("RotationPuzzle: %s girado → %.0f° (target %.0f°, correcto: %s)"),
		*GetName(), CurrentYaw, TargetYaw, bIsInCorrectPosition ? TEXT("SÍ") : TEXT("NO"));

	// Notificar al manager
	if (PuzzleManager) PuzzleManager->NotifyObjectChanged();

	return true;
}

bool ARotationPuzzleObject::IsInCorrectPosition() const
{
	float Diff = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));
	return Diff <= SnapTolerance;
}

// ─── ARotationPuzzleManager ───────────────────────────────────────────────────

ARotationPuzzleManager::ARotationPuzzleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARotationPuzzleManager::BeginPlay()
{
	Super::BeginPlay();
	for (ARotationPuzzleObject* Obj : Objects)
	{
		if (Obj) Obj->PuzzleManager = this;
	}
}

void ARotationPuzzleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLerping || !TargetActor) return;

	LerpTimer += DeltaTime;
	float Alpha = FMath::Clamp(LerpTimer / FMath::Max(TransformLerpDuration, 0.01f), 0.0f, 1.0f);
	float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	TargetActor->SetActorLocation(FMath::Lerp(LerpStartLoc,   LerpStartLoc   + TargetTranslation, Eased));
	TargetActor->SetActorRotation(FMath::Lerp(LerpStartRot,   LerpStartRot   + TargetRotation,    Eased));
	TargetActor->SetActorScale3D (FMath::Lerp(LerpStartScale, LerpStartScale * TargetScale,        Eased));

	if (Alpha >= 1.0f) bIsLerping = false;
}

void ARotationPuzzleManager::NotifyObjectChanged()
{
	if (bIsSolved) return;

	for (ARotationPuzzleObject* Obj : Objects)
	{
		if (!Obj || !Obj->IsInCorrectPosition()) return;
	}

	bIsSolved = true;
	UE_LOG(LogTemp, Warning, TEXT("RotationPuzzleManager: ¡RESUELTO! (%d objetos)"), Objects.Num());

	if (SolvedSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SolvedSound, GetActorLocation());

	if (!TargetActor) return;

	if (TransformLerpDuration <= 0.0f)
	{
		TargetActor->SetActorLocation(TargetActor->GetActorLocation() + TargetTranslation);
		TargetActor->SetActorRotation(TargetActor->GetActorRotation() + TargetRotation);
		TargetActor->SetActorScale3D (TargetActor->GetActorScale3D()  * TargetScale);
	}
	else
	{
		LerpStartLoc   = TargetActor->GetActorLocation();
		LerpStartRot   = TargetActor->GetActorRotation();
		LerpStartScale = TargetActor->GetActorScale3D();
		LerpTimer  = 0.0f;
		bIsLerping = true;
	}
}
