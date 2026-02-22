// SairanSkies - Custom Animation Instance

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SairanAnimInstance.generated.h"

class ASairanCharacter;

UCLASS()
class SAIRANSKIES_API USairanAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ========== ANIMATION PROPERTIES ==========
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ASairanCharacter* OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsWeaponDrawn = true;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float VerticalVelocity = 0.0f;
};
