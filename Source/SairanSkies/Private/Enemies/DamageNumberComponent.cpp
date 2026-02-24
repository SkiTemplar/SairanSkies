// SairanSkies - Damage Number Component Implementation (individual floating numbers)

#include "Enemies/DamageNumberComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

UDamageNumberComponent::UDamageNumberComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDamageNumberComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDamageNumberComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	FVector CameraLocation = CameraManager ? CameraManager->GetCameraLocation() : FVector::ZeroVector;

	// Update all active floating numbers
	for (int32 i = ActiveNumbers.Num() - 1; i >= 0; --i)
	{
		FFloatingDamageNumber& Num = ActiveNumbers[i];
		if (!Num.TextComponent)
		{
			ActiveNumbers.RemoveAt(i);
			continue;
		}

		Num.Lifetime += DeltaTime;

		if (Num.Lifetime >= Num.MaxLifetime)
		{
			CleanupNumber(i);
			continue;
		}

		// Float upward
		FVector CurrentLoc = Num.TextComponent->GetComponentLocation();
		CurrentLoc.Z += FloatUpSpeed * DeltaTime;
		Num.TextComponent->SetWorldLocation(CurrentLoc);

		// Face camera
		if (CameraManager)
		{
			FVector Direction = CameraLocation - CurrentLoc;
			Direction.Z = 0.0f;
			if (!Direction.IsNearlyZero())
			{
				Num.TextComponent->SetWorldRotation(Direction.Rotation());
			}
		}

		// Fade out (reduce opacity over lifetime)
		float Alpha = 1.0f - (Num.Lifetime / Num.MaxLifetime);
		// Scale the text size to simulate fade (TextRenderComponent doesn't support alpha easily)
		float ScaledSize = TextSize * (0.5f + 0.5f * Alpha);
		Num.TextComponent->SetWorldSize(ScaledSize);
	}

	// Death marker - face camera
	if (bIsShowingDeath && DeathTextComponent && CameraManager)
	{
		FVector TextLoc = DeathTextComponent->GetComponentLocation();
		FVector Direction = CameraLocation - TextLoc;
		Direction.Z = 0.0f;
		if (!Direction.IsNearlyZero())
		{
			DeathTextComponent->SetWorldRotation(Direction.Rotation());
		}
	}
}

void UDamageNumberComponent::SpawnDamageNumber(float DamageAmount, float HealthPercent, const FVector& WorldLocation)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Create a new TextRenderComponent at the hit location
	UTextRenderComponent* NewText = NewObject<UTextRenderComponent>(Owner);
	if (!NewText) return;

	// Random scatter to avoid overlapping
	FVector SpawnLoc = WorldLocation;
	SpawnLoc.X += FMath::RandRange(-HorizontalScatter, HorizontalScatter);
	SpawnLoc.Y += FMath::RandRange(-HorizontalScatter, HorizontalScatter);
	SpawnLoc.Z += FMath::RandRange(0.0f, HorizontalScatter * 0.5f);

	NewText->SetWorldLocation(SpawnLoc);
	NewText->SetHorizontalAlignment(EHTA_Center);
	NewText->SetVerticalAlignment(EVRTA_TextCenter);
	NewText->SetWorldSize(TextSize);
	NewText->SetAbsolute(true, true, true); // World space, not relative

	// Apply custom font BEFORE RegisterComponent so the internal material is built with the correct font
	if (DamageFont)
	{
		NewText->SetFont(DamageFont);
	}

	// If a manual FontMaterial override is set, apply it before registering so the render proxy
	// is created with the correct material from the start (avoids the invisible-text problem with
	// custom fonts whose internal material doesn't expose a Color parameter).
	if (FontMaterial)
	{
		NewText->SetMaterial(0, FontMaterial);
	}

	// Set damage text BEFORE register so geometry is built correctly
	int32 DisplayDamage = FMath::RoundToInt(DamageAmount);
	NewText->SetText(FText::FromString(FString::Printf(TEXT("%d"), DisplayDamage)));

	// Register the component so it gets a render proxy and material
	NewText->RegisterComponent();

	// Get the color for this damage number
	FLinearColor DamageColor = GetColorForHealthPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));

	// Apply color.  We try both paths:
	// 1. SetTextRenderColor — works with the default UE font material.
	// 2. Dynamic material "Color" parameter — works with custom font materials that expose that param.
	NewText->SetTextRenderColor(DamageColor.ToFColor(true));

	// Create a dynamic material instance so we can drive the "Color" vector parameter at runtime
	// (needed for the fade-out effect in Tick and for custom font materials).
	if (UMaterialInterface* BaseMat = NewText->GetMaterial(0))
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, NewText);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(FName("Color"), DamageColor);
			DynMat->SetVectorParameterValue(FName("TextColor"), DamageColor);
			DynMat->SetScalarParameterValue(FName("Opacity"), 1.0f);
			NewText->SetMaterial(0, DynMat);
		}
	}

	// Store in active list
	FFloatingDamageNumber NewNumber;
	NewNumber.TextComponent = NewText;
	NewNumber.Lifetime = 0.0f;
	NewNumber.MaxLifetime = NumberLifetime;
	NewNumber.InitialLocation = SpawnLoc;
	ActiveNumbers.Add(NewNumber);
}

void UDamageNumberComponent::ShowDeathMarker()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	bIsShowingDeath = true;

	// Create death text above enemy
	if (!DeathTextComponent)
	{
		DeathTextComponent = NewObject<UTextRenderComponent>(Owner);
		if (DeathTextComponent)
		{
			FVector DeathLoc = Owner->GetActorLocation() + FVector(0.0f, 0.0f, DeathMarkerHeightOffset);
			DeathTextComponent->SetWorldLocation(DeathLoc);
			DeathTextComponent->SetHorizontalAlignment(EHTA_Center);
			DeathTextComponent->SetVerticalAlignment(EVRTA_TextCenter);
			DeathTextComponent->SetWorldSize(TextSize * 1.5f);
			DeathTextComponent->SetAbsolute(true, true, true);
			DeathTextComponent->SetText(FText::FromString(TEXT("X")));
			DeathTextComponent->SetTextRenderColor(DeathColor.ToFColor(true));
			// Apply custom font to death marker as well
			if (DamageFont)
			{
				DeathTextComponent->SetFont(DamageFont);
			}
			DeathTextComponent->RegisterComponent();
		}
	}

	// Auto-hide after a while
	GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [this]()
	{
		if (DeathTextComponent)
		{
			DeathTextComponent->SetVisibility(false);
		}
		bIsShowingDeath = false;
	}, 3.0f, false);
}

void UDamageNumberComponent::ResetCombo()
{
	// Clean up all active numbers
	for (int32 i = ActiveNumbers.Num() - 1; i >= 0; --i)
	{
		CleanupNumber(i);
	}
	ActiveNumbers.Empty();

	bIsShowingDeath = false;
	if (DeathTextComponent)
	{
		DeathTextComponent->DestroyComponent();
		DeathTextComponent = nullptr;
	}
	GetWorld()->GetTimerManager().ClearTimer(DeathTimerHandle);
}

void UDamageNumberComponent::CleanupNumber(int32 Index)
{
	if (ActiveNumbers.IsValidIndex(Index))
	{
		if (ActiveNumbers[Index].TextComponent)
		{
			ActiveNumbers[Index].TextComponent->DestroyComponent();
		}
		ActiveNumbers.RemoveAt(Index);
	}
}

FLinearColor UDamageNumberComponent::GetColorForHealthPercent(float HealthPercent) const
{
	if (HealthPercent >= HighToMidThreshold)
	{
		float Range = 1.0f - HighToMidThreshold;
		float Alpha = (Range > 0.0f) ? (HealthPercent - HighToMidThreshold) / Range : 1.0f;
		return FMath::Lerp(MidHealthColor, FullHealthColor, Alpha);
	}
	else if (HealthPercent >= MidToLowThreshold)
	{
		float Range = HighToMidThreshold - MidToLowThreshold;
		float Alpha = (Range > 0.0f) ? (HealthPercent - MidToLowThreshold) / Range : 0.0f;
		return FMath::Lerp(ZeroHealthColor, MidHealthColor, Alpha);
	}
	else
	{
		return ZeroHealthColor;
	}
}
