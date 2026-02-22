// SairanSkies - Combo Window Notify Implementation

#include "Animation/AN_ComboWindow.h"
#include "Character/SairanCharacter.h"
#include "Combat/CombatComponent.h"

void UAN_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	// Combo window open - inputs can be buffered
	// This is handled automatically by CombatComponent's input buffering
}

void UAN_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// Combo window closed
	// If no input was buffered, the combo might end
}
