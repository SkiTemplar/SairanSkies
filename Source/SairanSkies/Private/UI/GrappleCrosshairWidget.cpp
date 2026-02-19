// SairanSkies - Grapple Crosshair Widget Implementation

#include "UI/GrappleCrosshairWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Blueprint/WidgetLayoutLibrary.h"

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

	// Start at center
	ResetToCenter();
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
	ResetToCenter();
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

void UGrappleCrosshairWidget::SetScreenPosition(FVector2D ScreenPosition)
{
	if (!CrosshairContainer || !CrosshairCanvas)
	{
		return;
	}

	// Get the canvas panel slot for the crosshair container
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CrosshairContainer->Slot);
	if (!CanvasSlot)
	{
		return;
	}

	// Convert viewport coordinates to DPI-scaled widget coordinates
	float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	if (DPIScale <= 0.0f) DPIScale = 1.0f;

	FVector2D WidgetPosition = ScreenPosition / DPIScale;

	// Set the position (anchored at 0,0 top-left, alignment centers it)
	CanvasSlot->SetPosition(WidgetPosition);
}

void UGrappleCrosshairWidget::ResetToCenter()
{
	if (!CrosshairContainer || !CrosshairCanvas)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CrosshairContainer->Slot);
	if (!CanvasSlot)
	{
		return;
	}

	// Get viewport size and set to center
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	if (DPIScale <= 0.0f) DPIScale = 1.0f;

	FVector2D CenterPosition = (ViewportSize * 0.5f) / DPIScale;
	CanvasSlot->SetPosition(CenterPosition);
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
}

