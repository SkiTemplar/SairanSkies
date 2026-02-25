// SairanSkies - Damage Number Component Implementation (UMG Widget-based)

#include "Enemies/DamageNumberComponent.h"
#include "UI/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
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

	// Update all active floating numbers
	for (int32 i = ActiveNumbers.Num() - 1; i >= 0; --i)
	{
		FFloatingDamageNumber& Num = ActiveNumbers[i];
		if (!Num.WidgetComponent)
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
		FVector CurrentLoc = Num.WidgetComponent->GetComponentLocation();
		CurrentLoc.Z += FloatUpSpeed * DeltaTime;
		Num.WidgetComponent->SetWorldLocation(CurrentLoc);

		// Fade out opacity over lifetime
		float Alpha = 1.0f - (Num.Lifetime / Num.MaxLifetime);
		if (Num.Widget)
		{
			// Update color with fading alpha
			FLinearColor FadedColor = Num.NumberColor;
			FadedColor.A = Alpha;
			Num.Widget->SetDamageColor(FadedColor);
			Num.Widget->SetRenderOpacity(Alpha);
		}
	}

	// Death marker - nothing special needed, WidgetComponent in Screen space auto-faces camera
}

void UDamageNumberComponent::SpawnDamageNumber(float DamageAmount, float HealthPercent, const FVector& WorldLocation)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Random scatter to avoid overlapping
	FVector SpawnLoc = WorldLocation;
	SpawnLoc.X += FMath::RandRange(-HorizontalScatter, HorizontalScatter);
	SpawnLoc.Y += FMath::RandRange(-HorizontalScatter, HorizontalScatter);
	SpawnLoc.Z += FMath::RandRange(0.0f, HorizontalScatter * 0.5f);

	// Get the color for this damage number
	FLinearColor DamageColor = GetColorForHealthPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));

	// Create a WidgetComponent in Screen space (always faces camera)
	UWidgetComponent* NewWidgetComp = NewObject<UWidgetComponent>(Owner);
	if (!NewWidgetComp) return;

	NewWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	NewWidgetComp->SetDrawSize(WidgetDrawSize);
	NewWidgetComp->SetAbsolute(true, true, true);
	NewWidgetComp->SetWorldLocation(SpawnLoc);
	NewWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewWidgetComp->SetPivot(FVector2D(0.5f, 0.5f));

	// Set widget class if available
	if (DamageNumberWidgetClass)
	{
		NewWidgetComp->SetWidgetClass(DamageNumberWidgetClass);
	}

	NewWidgetComp->RegisterComponent();

	// Configure the widget
	UDamageNumberWidget* DmgWidget = Cast<UDamageNumberWidget>(NewWidgetComp->GetUserWidgetObject());
	if (DmgWidget)
	{
		int32 DisplayDamage = FMath::RoundToInt(DamageAmount);
		DmgWidget->SetDamageText(FText::FromString(FString::Printf(TEXT("%d"), DisplayDamage)));
		DmgWidget->SetDamageColor(DamageColor);
		DmgWidget->SetFontSize(TextFontSize);
	}

	// Store in active list
	FFloatingDamageNumber NewNumber;
	NewNumber.WidgetComponent = NewWidgetComp;
	NewNumber.Widget = DmgWidget;
	NewNumber.Lifetime = 0.0f;
	NewNumber.MaxLifetime = NumberLifetime;
	NewNumber.InitialLocation = SpawnLoc;
	NewNumber.NumberColor = DamageColor;
	ActiveNumbers.Add(NewNumber);
}

void UDamageNumberComponent::ShowDeathMarker()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	bIsShowingDeath = true;

	// Create death widget above enemy
	if (!DeathWidgetComponent)
	{
		FVector DeathLoc = Owner->GetActorLocation() + FVector(0.0f, 0.0f, DeathMarkerHeightOffset);

		DeathWidgetComponent = NewObject<UWidgetComponent>(Owner);
		if (DeathWidgetComponent)
		{
			DeathWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
			DeathWidgetComponent->SetDrawSize(WidgetDrawSize * 1.5f);
			DeathWidgetComponent->SetAbsolute(true, true, true);
			DeathWidgetComponent->SetWorldLocation(DeathLoc);
			DeathWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			if (DamageNumberWidgetClass)
			{
				DeathWidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
			}

			DeathWidgetComponent->RegisterComponent();

			UDamageNumberWidget* DeathWidget = Cast<UDamageNumberWidget>(DeathWidgetComponent->GetUserWidgetObject());
			if (DeathWidget)
			{
				DeathWidget->SetDamageText(FText::FromString(TEXT("X")));
				DeathWidget->SetDamageColor(DeathColor);
				DeathWidget->SetFontSize(TextFontSize * 1.5f);
			}
		}
	}

	// Auto-hide after a while
	GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [this]()
	{
		if (DeathWidgetComponent)
		{
			DeathWidgetComponent->SetVisibility(false);
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
	if (DeathWidgetComponent)
	{
		DeathWidgetComponent->DestroyComponent();
		DeathWidgetComponent = nullptr;
	}
	GetWorld()->GetTimerManager().ClearTimer(DeathTimerHandle);
}

void UDamageNumberComponent::CleanupNumber(int32 Index)
{
	if (ActiveNumbers.IsValidIndex(Index))
	{
		if (ActiveNumbers[Index].WidgetComponent)
		{
			ActiveNumbers[Index].WidgetComponent->DestroyComponent();
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
