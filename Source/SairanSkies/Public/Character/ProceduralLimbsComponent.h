// SairanSkies - Procedural Limbs Component (Rayman / Wii-Mii style)
//
// Visual-only character body:
//   • Sphere  → torso
//   • Spheres → hands (follow weapon when armed, idle swing when unarmed)
//   • Cones   → feet (tip pointing down, alternating gait driven by locomotion speed)
//
// No real skeletal animation. Movement is fast VInterpTo tracking + procedural gait oscillation.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralLimbsComponent.generated.h"

class ASairanCharacter;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UProceduralLimbsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProceduralLimbsComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MESH REFERENCES (created at runtime in BeginPlay) ==========

	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Visual")
	UStaticMeshComponent* BodySphere = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Visual")
	UStaticMeshComponent* RightHand = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Visual")
	UStaticMeshComponent* LeftHand = nullptr;

	/** Cone mesh, tip pointing down. */
	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Visual")
	UStaticMeshComponent* RightFoot = nullptr;

	/** Cone mesh, tip pointing down. */
	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Visual")
	UStaticMeshComponent* LeftFoot = nullptr;

	// ========== MATERIALS ==========

	/** Material for the body sphere. Assign in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Material")
	UMaterialInterface* BodyMaterial = nullptr;

	/** Material for hands and feet. Assign in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Material")
	UMaterialInterface* LimbMaterial = nullptr;

	// ========== SIZES ==========

	/** World-space radius of the body sphere (UE sphere mesh = radius 50 at scale 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Size",
		meta=(ClampMin="5.0", ClampMax="150.0"))
	float BodyRadius = 35.0f;

	/** World-space radius of each hand sphere. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Size",
		meta=(ClampMin="2.0", ClampMax="50.0"))
	float HandRadius = 11.0f;

	/** Base-radius of each foot cone (widest part). Tip always points down. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Size",
		meta=(ClampMin="2.0", ClampMax="50.0"))
	float FootConeRadius = 12.0f;

	/** Height of each foot cone (from base to tip). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Size",
		meta=(ClampMin="4.0", ClampMax="100.0"))
	float FootConeHeight = 28.0f;

	// ========== BODY ==========

	/** Height offset of body sphere above the character capsule origin. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Body")
	float BodyZOffset = 35.0f;

	/** Amplitude of the body vertical bob when running (units). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Body")
	float BodyBobAmplitude = 4.0f;

	// ========== HAND OFFSETS (character-local, used when unarmed) ==========

	/** Right hand rest offset relative to character root (unarmed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	FVector RightHandRestOffset = FVector(18.0f, 30.0f, 8.0f);

	/** Left hand rest offset relative to character root (unarmed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	FVector LeftHandRestOffset = FVector(18.0f, -30.0f, 8.0f);

	/** Idle hand swing amplitude (units). Hands sway slightly when unarmed and walking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	float IdleHandSwingAmplitude = 10.0f;

	/** Offset added to each hand along the weapon when armed (0 = same point, positive = separate). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	float WeaponHandSeparation = 15.0f;

	// ========== FOOT OFFSETS (character-local) ==========

	/** Right foot rest offset relative to character root. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|FootOffset")
	FVector RightFootRestOffset = FVector(0.0f, 14.0f, -60.0f);

	/** Left foot rest offset relative to character root. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|FootOffset")
	FVector LeftFootRestOffset = FVector(0.0f, -14.0f, -60.0f);

	// ========== GAIT ==========

	/** Forward/back swing amplitude of feet while walking (units). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="0.0"))
	float FootSwingAmplitude = 24.0f;

	/** Gait frequency (Hz) at maximum run speed. Scales linearly with actual speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="0.1"))
	float GaitFrequency = 3.2f;

	/** Max forward/back tilt (degrees) of the cone when swinging. Makes gait look more natural. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="0.0", ClampMax="45.0"))
	float FootTiltAngle = 18.0f;

	// ========== INTERPOLATION SPEEDS ==========

	/** How fast hands lerp to their target (higher = snappier, no lag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Lerp",
		meta=(ClampMin="1.0"))
	float HandLerpSpeed = 30.0f;

	/** How fast feet lerp to their target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Lerp",
		meta=(ClampMin="1.0"))
	float FootLerpSpeed = 25.0f;

	/** How fast the body sphere lerps vertically (bob). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Lerp",
		meta=(ClampMin="1.0"))
	float BodyLerpSpeed = 40.0f;

	// ========== DEBUG ==========

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Debug")
	bool bShowDebug = false;

private:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter = nullptr;

	// Smoothed world positions
	FVector RightHandPos = FVector::ZeroVector;
	FVector LeftHandPos  = FVector::ZeroVector;
	FVector RightFootPos = FVector::ZeroVector;
	FVector LeftFootPos  = FVector::ZeroVector;
	FVector BodyPos      = FVector::ZeroVector;

	float GaitTimer   = 0.0f;
	bool bInitialized = false;

	// ---- Factory helpers ----
	UStaticMeshComponent* MakeSphereComp(const FString& Name, float WorldRadius,
		UStaticMesh* Mesh, UMaterialInterface* Mat) const;
	UStaticMeshComponent* MakeConeComp(const FString& Name, float WorldBaseRadius,
		float WorldHeight, UStaticMesh* Mesh, UMaterialInterface* Mat) const;

	// ---- Target positions this frame ----
	FVector GetRightHandTarget()  const;
	FVector GetLeftHandTarget()   const;
	FVector GetRightFootTarget()  const;
	FVector GetLeftFootTarget()   const;
	FVector GetBodyTarget(float BobOffset) const;

	// ---- Foot rotation (tip-down + gait tilt) ----
	FRotator GetFootRotation(float GaitPhase, float SpeedRatio) const;
};
