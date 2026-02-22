// SairanSkies - Grapple Crosshair Widget

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrappleCrosshairWidget.generated.h"

class UImage;
class UCanvasPanel;
class UCanvasPanelSlot;
class UOverlay;

/**
 * Widget for displaying the grapple hook crosshair.
 * Shows red when target is invalid, green when valid.
 * Moves to track the aim target on screen.
 */
UCLASS()
class SAIRANSKIES_API UGrappleCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set whether the current target is valid (changes crosshair color) */
	UFUNCTION(BlueprintCallable, Category = "Grapple|UI")
	void SetTargetValid(bool bIsValid);

	/** Show the crosshair */
	UFUNCTION(BlueprintCallable, Category = "Grapple|UI")
	void ShowCrosshair();

	/** Hide the crosshair */
	UFUNCTION(BlueprintCallable, Category = "Grapple|UI")
	void HideCrosshair();

	/** Move the crosshair to a screen position (viewport coordinates) */
	UFUNCTION(BlueprintCallable, Category = "Grapple|UI")
	void SetScreenPosition(FVector2D ScreenPosition);

	/** Reset crosshair to center of screen */
	UFUNCTION(BlueprintCallable, Category = "Grapple|UI")
	void ResetToCenter();

protected:
	virtual void NativeConstruct() override;

	/** Root canvas panel that holds the crosshair images */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UCanvasPanel* CrosshairCanvas;

	/** Container for crosshair images (placed inside canvas) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UOverlay* CrosshairContainer;

	/** The red crosshair image (invalid target) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CrosshairRed;

	/** The green crosshair image (valid target) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CrosshairGreen;
};
