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
class UMaterialInstanceDynamic;
class UPoseableMeshComponent;

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

	/** World-space radius of the body sphere (UE sphere mesh = radius 50 at scale 1).
	 *  Visually matches the sphere hitbox (capsule radius 75). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Size",
		meta=(ClampMin="5.0", ClampMax="150.0"))
	float BodyRadius = 55.0f;

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

	/** Vertical offset of the body sphere from the capsule centre.
	 *  0 = perfectly centred on the hitbox sphere (recommended when BodyRadius ≈ capsule radius). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Body")
	float BodyZOffset = 0.0f;

	/** Amplitude of the body vertical bob when running (units). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Body")
	float BodyBobAmplitude = 4.0f;

	// ========== HAND OFFSETS (character-local, used when unarmed) ==========

	/** Right hand rest offset relative to character root (unarmed).
	 *  Distance from origin must exceed BodyRadius so the hand is visible outside the body sphere.
	 *  (25,70,0) → dist ≈ 74 > radius 55 = clearly outside. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	FVector RightHandRestOffset = FVector(25.0f, 70.0f, 0.0f);

	/** Left hand rest offset relative to character root (unarmed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	FVector LeftHandRestOffset = FVector(25.0f, -70.0f, 0.0f);

	/** Idle hand swing amplitude (units). Hands sway slightly when unarmed and walking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	float IdleHandSwingAmplitude = 10.0f;

	/** Offset added to each hand along the weapon when armed (0 = same point, positive = separate). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HandOffset")
	float WeaponHandSeparation = 15.0f;

	// ========== FOOT OFFSETS (character-local) ==========

	/** Right foot rest offset relative to character root.
	 *  Actor origin is HalfHeight (75) above floor, so Z=-68 puts the foot at 7u above floor
	 *  and clearly below the body sphere bottom (sphere bottom = -55 from origin). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|FootOffset")
	FVector RightFootRestOffset = FVector(5.0f, 20.0f, -68.0f);

	/** Left foot rest offset relative to character root. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|FootOffset")
	FVector LeftFootRestOffset = FVector(5.0f, -20.0f, -68.0f);

	// ========== GAIT ==========

	/** Forward/back swing amplitude of feet while walking (units). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="0.0"))
	float FootSwingAmplitude = 24.0f;

	/** Gait frequency (Hz) at maximum run speed. Scales linearly with actual speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="0.1"))
	float GaitFrequency = 5.5f;

	/** Radio de la bola de dash: distancia de las extremidades al centro del cuerpo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="10.0", ClampMax="120.0"))
	float DashBallRadius = 42.0f;

	/** Velocidad de giro de la bola en °/s (720 = 2 rotaciones/segundo) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Gait",
		meta=(ClampMin="180.0", ClampMax="2160.0"))
	float DashRollSpeed = 720.0f;

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

	// ========== SKELETAL MESH (Tash final character) ==========

	/** PoseableMesh que conduce los huesos de SKM_Tash_model. Se crea en BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "Limbs|Skeletal")
	UPoseableMeshComponent* TashMesh = nullptr;

	/** Activar la conducción del esqueleto Tash desde las posiciones procedurales. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal")
	bool bDriveSkeletalMesh = true;

	/**
	 * Ocultar las StaticMeshes placeholder (esfera + manos + pies cono)
	 * cuando el mesh final está activo.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal")
	bool bHideStaticMeshes = false;

	// ── IK — longitudes de huesos (ajustar según la malla real) ──────────────

	/** Longitud del hueso Hombro→Codo (brazo superior) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK",
		meta=(ClampMin="5.0", ClampMax="200.0"))
	float UpperArmLength = 35.0f;

	/** Longitud del hueso Codo→Mano (antebrazo) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK",
		meta=(ClampMin="5.0", ClampMax="200.0"))
	float LowerArmLength = 35.0f;

	/** Longitud del hueso Rodilla→Pie (pierna inferior) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK",
		meta=(ClampMin="5.0", ClampMax="200.0"))
	float UpperLegLength = 38.0f;

	/** Longitud del hueso centro→Rodilla (muslo virtual) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK",
		meta=(ClampMin="5.0", ClampMax="200.0"))
	float LowerLegLength = 38.0f;

	/** Offset local desde BodyPos hasta el punto de anclaje del hombro derecho */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK")
	FVector RightShoulderOffset = FVector(0.0f, 42.0f, 12.0f);

	/** Offset local desde BodyPos hasta el punto de anclaje del hombro izquierdo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK")
	FVector LeftShoulderOffset = FVector(0.0f, -42.0f, 12.0f);

	/** Offset local desde BodyPos hasta la cadera derecha (origen de la pierna) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK")
	FVector RightHipOffset = FVector(0.0f, 22.0f, -34.0f);

	/** Offset local desde BodyPos hasta la cadera izquierda */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|IK")
	FVector LeftHipOffset = FVector(0.0f, -22.0f, -34.0f);

	// ── Rotación de la mesh (corrige el eje de importación) ─────────────────────

	/**
	 * Yaw aplicado al TashMesh respecto al personaje.
	 * Si la figura aparece girada 90° a la izquierda, pon 90. Si a la derecha, -90.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal",
		meta=(ClampMin="-360.0", ClampMax="360.0"))
	float MeshYawOffset = 0.0f;

	// ── Nombres de huesos (ajustar al esqueleto real de SKM_Tash_model) ─────────
	// En BeginPlay se imprime en el Output Log la lista completa de huesos reales.

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_Root = TEXT("Centro");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_ShoulderR = TEXT("Hombro_der");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_ElbowR = TEXT("Codo_der");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_HandR = TEXT("Mano_der");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_ShoulderL = TEXT("Hombro_izq");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_ElbowL = TEXT("Codo_izq");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_HandL = TEXT("Mano_izq");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_KneeR = TEXT("Rodilla_der");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_FootR = TEXT("Pie_der");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_KneeL = TEXT("Rodilla_izq");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Skeletal|BoneNames")
	FName BoneName_FootL = TEXT("Pie_izq");

	// ========== DEBUG ==========

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|Debug")
	bool bShowDebug = false;

	// ========== DEATH POSE ==========

	/** El cuerpo se desploma progresivamente al suelo. Llamar desde HandleDeath(). */
	UFUNCTION(BlueprintCallable, Category = "Limbs|Death")
	void EnterDeathPose();

	/** Restaura el estado normal tras el respawn. */
	UFUNCTION(BlueprintCallable, Category = "Limbs|Death")
	void ExitDeathPose();

	// ========== HIT FLASH ==========

	/** Color del flash cuando el jugador recibe daño */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HitFlash")
	FLinearColor HitFlashColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	/** Duración del flash en segundos */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Limbs|HitFlash",
		meta=(ClampMin="0.01", ClampMax="1.0"))
	float HitFlashDuration = 0.1f;

	/** Activa el flash de golpe (llamar desde SairanCharacter::TakeDamage). */
	UFUNCTION(BlueprintCallable, Category = "Limbs|HitFlash")
	void StartHitFlash();

private:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter = nullptr;

	// Smoothed world positions
	FVector RightHandPos = FVector::ZeroVector;
	FVector LeftHandPos  = FVector::ZeroVector;
	FVector RightFootPos = FVector::ZeroVector;
	FVector LeftFootPos  = FVector::ZeroVector;
	FVector BodyPos      = FVector::ZeroVector;

	float GaitTimer     = 0.0f;
	float DashRollAngle = 0.0f;   // radianes, acumula durante el dash
	bool  bWasDashing   = false;
	bool  bInitialized  = false;

	// Death collapse
	bool  bDeathPose       = false;
	float DeathPoseTimer   = 0.0f;
	static constexpr float DeathCollapseDuration = 0.7f;

	// Hit flash
	void StopHitFlash();
	FTimerHandle HitFlashTimerHandle;
	UPROPERTY()
	TArray<UMaterialInterface*> OriginalMaterials;
	UPROPERTY()
	UMaterialInstanceDynamic* FlashMaterialInstance = nullptr;
	bool bMaterialsCached = false;

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

	// ---- Skeletal mesh helpers ----
	void DriveSkeletalBones(float SpeedRatio);

	/**
	 * Solver analítico de IK de 2 huesos (ley de cosenos).
	 * Devuelve la posición world del joint intermedio (codo/rodilla).
	 * @param Root     Posición world del hueso raíz (hombro/cadera).
	 * @param Tip      Posición world del efector final (mano/pie).
	 * @param Upper    Longitud del segmento raíz→joint.
	 * @param Lower    Longitud del segmento joint→tip.
	 * @param HintDir  Dirección en la que debe "doblarse" el joint.
	 */
	FVector SolveTwoBoneIK(const FVector& Root, const FVector& Tip,
		float Upper, float Lower, const FVector& HintDir) const;
};
