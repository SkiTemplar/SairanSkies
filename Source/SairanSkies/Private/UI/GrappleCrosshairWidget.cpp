// SairanSkies - Grapple Crosshair Widget Implementation

#include "UI/GrappleCrosshairWidget.h"
#include "Components/Image.h"

void UGrappleCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start with both hidden
	if (CrosshairRed)
	{
		CrosshairRed->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CrosshairGreen)
	{
		CrosshairGreen->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UGrappleCrosshairWidget::SetTargetValid(bool bIsValid)
{
	if (bIsValid)
	{
		// Valid target - show green, hide red
		if (CrosshairGreen)
		{
			CrosshairGreen->SetVisibility(ESlateVisibility::Visible);
		}
		if (CrosshairRed)
		{
			CrosshairRed->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		// Invalid target - show red, hide green
		if (CrosshairRed)
		{
			CrosshairRed->SetVisibility(ESlateVisibility::Visible);
		}
		if (CrosshairGreen)
		{
			CrosshairGreen->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UGrappleCrosshairWidget::ShowCrosshair()
{
	SetVisibility(ESlateVisibility::Visible);
	// Default to invalid (red) until we detect a valid target
	SetTargetValid(false);
}

void UGrappleCrosshairWidget::HideCrosshair()
{
	SetVisibility(ESlateVisibility::Hidden);
	
	if (CrosshairRed)
	{
		CrosshairRed->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CrosshairGreen)
	{
		CrosshairGreen->SetVisibility(ESlateVisibility::Hidden);
	}
}
