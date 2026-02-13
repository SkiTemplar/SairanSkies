// SairanSkies - Greatsword Weapon

#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponBase.h"
#include "Greatsword.generated.h"

UCLASS()
class SAIRANSKIES_API AGreatsword : public AWeaponBase
{
	GENERATED_BODY()

public:
	AGreatsword();

protected:
	virtual void BeginPlay() override;
};
