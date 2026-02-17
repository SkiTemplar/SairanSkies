// SairanSkies - Grapple Crosshair Widget

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrappleCrosshairWidget.generated.h"

class UImage;

/**
 * Widget for displaying the grapple hook crosshair.
 * Shows red when target is invalid, green when valid.
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

protected:
	virtual void NativeConstruct() override;

	/** The red crosshair image (invalid target) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CrosshairRed;

	/** The green crosshair image (valid target) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* CrosshairGreen;
};
