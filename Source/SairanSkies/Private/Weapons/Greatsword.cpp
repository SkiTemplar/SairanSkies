// SairanSkies - Greatsword Implementation

#include "Weapons/Greatsword.h"

AGreatsword::AGreatsword()
{
	// Greatsword specific settings
	// Large rectangular sword (placeholder)
	WeaponSize = FVector(15.0f, 5.0f, 180.0f); // Width, Depth, Height (length of blade)
	WeaponColor = FLinearColor(0.6f, 0.6f, 0.7f, 1.0f); // Steel gray

	// Note: Attachment positions are now controlled by SceneComponents on the character:
	// - WeaponHandAttachPoint (for held position)
	// - WeaponBackAttachPoint (for sheathed position)
	// - WeaponBlockAttachPoint (for blocking stance)
	// These can be adjusted in the editor without recompiling
}

void AGreatsword::BeginPlay()
{
	Super::BeginPlay();
}
