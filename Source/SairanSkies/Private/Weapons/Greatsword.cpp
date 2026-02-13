// SairanSkies - Greatsword Implementation

#include "Weapons/Greatsword.h"

AGreatsword::AGreatsword()
{
	// Greatsword specific settings
	// Large rectangular sword (placeholder)
	WeaponSize = FVector(15.0f, 5.0f, 180.0f); // Width, Depth, Height (length of blade)
	WeaponColor = FLinearColor(0.6f, 0.6f, 0.7f, 1.0f); // Steel gray

	// Attachment offsets (adjust based on your skeleton)
	HandAttachOffset = FVector(0, 0, 0);
	HandAttachRotation = FRotator(0, 0, -90); // Blade pointing forward

	BackAttachOffset = FVector(-20, 10, 30);
	BackAttachRotation = FRotator(45, 90, 0); // Diagonal on back, God of War style
}

void AGreatsword::BeginPlay()
{
	Super::BeginPlay();
}
