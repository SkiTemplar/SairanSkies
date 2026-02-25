// SairanSkies - Damage Number Widget (UMG-based floating damage number)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * Widget for displaying a single floating damage number.
 * Supports custom fonts via UMG's native font system (no TextRenderComponent issues).
 * Create a Widget Blueprint inheriting from this and add a TextBlock named "DamageText".
 */
UCLASS()
class SAIRANSKIES_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the damage text to display */
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void SetDamageText(const FText& Text);

	/** Set the text color */
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void SetDamageColor(FLinearColor Color);

	/** Set the font size */
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void SetFontSize(int32 Size);

protected:
	/** Bind to a TextBlock named "DamageText" in the Widget Blueprint */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DamageText;
};

