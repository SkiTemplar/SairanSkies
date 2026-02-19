// SairanSkies - Damage Number Component Implementation

#include "Enemies/DamageNumberComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UDamageNumberComponent::UDamageNumberComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDamageNumberComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Create the text render component attached to the owner
	TextComponent = NewObject<UTextRenderComponent>(Owner, TEXT("DamageNumberText"));
	if (TextComponent)
	{
		TextComponent->SetupAttachment(Owner->GetRootComponent());
		TextComponent->SetRelativeLocation(FVector(0.0f, -LeftOffset, HeightOffset));
		TextComponent->SetHorizontalAlignment(EHTA_Center);
		TextComponent->SetVerticalAlignment(EVRTA_TextCenter);
		TextComponent->SetWorldSize(TextSize);
		TextComponent->SetTextRenderColor(FColor::Green);
		TextComponent->SetText(FText::GetEmpty());
		TextComponent->SetVisibility(false);
		// Always face camera
		TextComponent->SetAbsolute(false, false, false);
		TextComponent->RegisterComponent();
	}
}

void UDamageNumberComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Make text face the camera
	if (TextComponent && bIsVisible)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		if (CameraManager)
		{
			FVector CameraLocation = CameraManager->GetCameraLocation();
			FVector TextLocation = TextComponent->GetComponentLocation();
			FVector Direction = CameraLocation - TextLocation;
			Direction.Z = 0.0f; // Keep upright
			if (!Direction.IsNearlyZero())
			{
				FRotator LookAtRotation = Direction.Rotation();
				TextComponent->SetWorldRotation(LookAtRotation);
			}
		}
	}
}

void UDamageNumberComponent::AddDamage(float DamageAmount, float HealthPercent)
{
	AccumulatedDamage += DamageAmount;
	LastHealthPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);
	bIsShowingDeath = false;

	UpdateTextDisplay();

	// Reset/extend the display timer
	GetWorld()->GetTimerManager().SetTimer(
		DisplayTimerHandle,
		this,
		&UDamageNumberComponent::OnDisplayTimerExpired,
		DisplayDuration,
		false
	);
}

void UDamageNumberComponent::ShowDeathMarker()
{
	bIsShowingDeath = true;
	AccumulatedDamage = 0.0f;

	if (TextComponent)
	{
		TextComponent->SetText(FText::FromString(TEXT("X")));
		FColor DeathFColor = DeathColor.ToFColor(true);
		TextComponent->SetTextRenderColor(DeathFColor);
		TextComponent->SetWorldSize(TextSize * 1.5f); // Bigger X
		TextComponent->SetVisibility(true);
		bIsVisible = true;
	}

	// Hide after a longer duration for death
	GetWorld()->GetTimerManager().SetTimer(
		DisplayTimerHandle,
		this,
		&UDamageNumberComponent::OnDisplayTimerExpired,
		DisplayDuration * 2.0f,
		false
	);
}

void UDamageNumberComponent::ResetCombo()
{
	AccumulatedDamage = 0.0f;
	LastHealthPercent = 1.0f;
	bIsShowingDeath = false;
	HideText();
	GetWorld()->GetTimerManager().ClearTimer(DisplayTimerHandle);
}

void UDamageNumberComponent::UpdateTextDisplay()
{
	if (!TextComponent) return;

	// Format the accumulated damage as integer
	int32 DisplayDamage = FMath::RoundToInt(AccumulatedDamage);
	FString DamageText = FString::Printf(TEXT("%d"), DisplayDamage);
	TextComponent->SetText(FText::FromString(DamageText));

	// Use the 3-stage color system based on health thresholds
	FLinearColor CurrentColor = GetColorForHealthPercent(LastHealthPercent);
	FColor DisplayColor = CurrentColor.ToFColor(true);
	TextComponent->SetTextRenderColor(DisplayColor);
	TextComponent->SetWorldSize(TextSize);

	TextComponent->SetVisibility(true);
	bIsVisible = true;
}

FLinearColor UDamageNumberComponent::GetColorForHealthPercent(float HealthPercent) const
{
	// 3 stages:
	//   HP >= HighToMidThreshold        -> lerp FullHealthColor (at 100%) to MidHealthColor (at threshold)
	//   MidToLowThreshold <= HP < High  -> lerp MidHealthColor (at high threshold) to ZeroHealthColor (at low threshold)
	//   HP < MidToLowThreshold          -> solid ZeroHealthColor

	if (HealthPercent >= HighToMidThreshold)
	{
		// Stage 1: Full -> Mid (green -> orange)
		// Map [HighToMidThreshold, 1.0] to [0.0, 1.0]
		float Range = 1.0f - HighToMidThreshold;
		float Alpha = (Range > 0.0f) ? (HealthPercent - HighToMidThreshold) / Range : 1.0f;
		return FMath::Lerp(MidHealthColor, FullHealthColor, Alpha);
	}
	else if (HealthPercent >= MidToLowThreshold)
	{
		// Stage 2: Mid -> Low (orange -> red)
		// Map [MidToLowThreshold, HighToMidThreshold] to [0.0, 1.0]
		float Range = HighToMidThreshold - MidToLowThreshold;
		float Alpha = (Range > 0.0f) ? (HealthPercent - MidToLowThreshold) / Range : 0.0f;
		return FMath::Lerp(ZeroHealthColor, MidHealthColor, Alpha);
	}
	else
	{
		// Stage 3: Critical - solid red
		return ZeroHealthColor;
	}
}

void UDamageNumberComponent::HideText()
{
	if (TextComponent)
	{
		TextComponent->SetVisibility(false);
	}
	bIsVisible = false;
}

void UDamageNumberComponent::OnDisplayTimerExpired()
{
	if (bIsShowingDeath)
	{
		HideText();
		bIsShowingDeath = false;
	}
	else
	{
		// Reset accumulated damage when timer expires (combo break)
		AccumulatedDamage = 0.0f;
		HideText();
	}
}
