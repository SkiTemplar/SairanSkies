// SairanSkies - Damage Number Widget Implementation

#include "UI/DamageNumberWidget.h"
#include "Components/TextBlock.h"

void UDamageNumberWidget::SetDamageText(const FText& Text)
{
	if (DamageText)
	{
		DamageText->SetText(Text);
	}
}

void UDamageNumberWidget::SetDamageColor(FLinearColor Color)
{
	if (DamageText)
	{
		DamageText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UDamageNumberWidget::SetFontSize(int32 Size)
{
	if (DamageText)
	{
		FSlateFontInfo FontInfo = DamageText->GetFont();
		FontInfo.Size = Size;
		DamageText->SetFont(FontInfo);
	}
}

