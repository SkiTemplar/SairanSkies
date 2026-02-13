// SairanSkies - Main Game Mode Implementation

#include "Core/SairanGameMode.h"
#include "Character/SairanCharacter.h"

ASairanGameMode::ASairanGameMode()
{
	// Set default pawn class
	DefaultPawnClass = ASairanCharacter::StaticClass();
}
