// SairanSkies - Procedural Limbs Component Implementation

#include "Character/ProceduralLimbsComponent.h"
#include "Character/SairanCharacter.h"
#include "Combat/GrappleComponent.h"
#include "Weapons/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Character/UltimateComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"

// ── UE5 BasicShapes dimensions at scale 1.0 ─────────────────────────────────
//   Sphere   : radius  50 (diameter 100)
//   Cone     : height 100 (from base at -50Z to tip at +50Z), base radius 50
// ────────────────────────────────────────────────────────────────────────────
static constexpr float SPHERE_UNIT_RADIUS    = 50.0f;
static constexpr float CONE_UNIT_HEIGHT      = 100.0f; // tip at +Z, base at -Z
static constexpr float CONE_UNIT_BASE_RADIUS = 50.0f;

UProceduralLimbsComponent::UProceduralLimbsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// ============================================================
//  BeginPlay — create all mesh components
// ============================================================
void UProceduralLimbsComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASairanCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
	UStaticMesh* ConeMesh   = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone"));

	if (!SphereMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralLimbs: Cannot load /Engine/BasicShapes/Sphere"));
		return;
	}

	// Fallback: if no Cone asset, use sphere for feet
	UStaticMesh* FootMesh = ConeMesh ? ConeMesh : SphereMesh;

	// ── Body ──────────────────────────────────────────────────
	BodySphere = MakeSphereComp(TEXT("LimbBody"), BodyRadius, SphereMesh, BodyMaterial);

	// ── Hands (spheres) ───────────────────────────────────────
	RightHand = MakeSphereComp(TEXT("LimbRightHand"), HandRadius, SphereMesh, LimbMaterial);
	LeftHand  = MakeSphereComp(TEXT("LimbLeftHand"),  HandRadius, SphereMesh, LimbMaterial);

	// ── Feet (cones, tip-down) ────────────────────────────────
	RightFoot = MakeConeComp(TEXT("LimbRightFoot"), FootConeRadius, FootConeHeight, FootMesh, LimbMaterial);
	LeftFoot  = MakeConeComp(TEXT("LimbLeftFoot"),  FootConeRadius, FootConeHeight, FootMesh, LimbMaterial);

	// ── Seed initial positions to rest so there is no snap on first frame ──
	const FVector    Loc = OwnerCharacter->GetActorLocation();
	const FQuat      Rot = OwnerCharacter->GetActorQuat();

	BodyPos      = Loc + FVector(0.0f, 0.0f, BodyZOffset);
	RightHandPos = Loc + Rot.RotateVector(RightHandRestOffset);
	LeftHandPos  = Loc + Rot.RotateVector(LeftHandRestOffset);
	RightFootPos = Loc + Rot.RotateVector(RightFootRestOffset);
	LeftFootPos  = Loc + Rot.RotateVector(LeftFootRestOffset);

	// ── SKM_Tash_model: Poseable mesh driven from procedural positions ──
	if (bDriveSkeletalMesh)
	{
		USkeletalMesh* SKMesh = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/Meshes/SKM_Tash_model.SKM_Tash_model"));

		if (SKMesh)
		{
			TashMesh = NewObject<UPoseableMeshComponent>(GetOwner(), TEXT("TashMesh"));
			TashMesh->SetSkinnedAssetAndUpdate(SKMesh);
			TashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TashMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			TashMesh->CastShadow = true;
			TashMesh->RegisterComponent();
			TashMesh->AttachToComponent(
				OwnerCharacter->GetRootComponent(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			// Copiar el Z-offset del mesh nativo (pies al nivel del suelo)
			TashMesh->SetRelativeLocation(OwnerCharacter->GetMesh()->GetRelativeLocation());
			// Rotación solo desde MeshYawOffset — ajústalo en BP si el modelo sigue girado
			// (prueba -90 si está girado a la derecha, +90 si está a la izquierda)
			TashMesh->SetRelativeRotation(FRotator(0.0f, MeshYawOffset, 0.0f));

			// Ocultar el Mesh nativo para evitar el doble render
			OwnerCharacter->GetMesh()->SetVisibility(false, false);

			GetOwner()->AddInstanceComponent(TashMesh);

			// ── Log de huesos reales (ver en Output Log al hacer Play) ────────────
			if (const USkinnedAsset* Asset = TashMesh->GetSkinnedAsset())
			{
				const FReferenceSkeleton& RefSkel = Asset->GetRefSkeleton();
				UE_LOG(LogTemp, Warning,
					TEXT("ProceduralLimbs: SKM_Tash_model tiene %d huesos:"), RefSkel.GetNum());
				for (int32 i = 0; i < RefSkel.GetNum(); ++i)
				{
					UE_LOG(LogTemp, Log, TEXT("  Hueso[%d]: %s"), i,
						*RefSkel.GetBoneName(i).ToString());
				}
			}

			UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: TashMesh creado con SKM_Tash_model"));

			// Ocultar las geometrías placeholder si el diseñador lo pide
			if (bHideStaticMeshes)
			{
				if (BodySphere) BodySphere->SetVisibility(false, true);
				if (RightHand)  RightHand->SetVisibility(false, true);
				if (LeftHand)   LeftHand->SetVisibility(false, true);
				if (RightFoot)  RightFoot->SetVisibility(false, true);
				if (LeftFoot)   LeftFoot->SetVisibility(false, true);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ProceduralLimbs: No se encontró /Game/Meshes/SKM_Tash_model — usando static meshes."));
		}
	}

	bInitialized = true;
}

// ============================================================
//  TickComponent — lerp tracking + gait oscillation
// ============================================================
void UProceduralLimbsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInitialized || !OwnerCharacter) return;

	// ══════════════════════════════════════════════════════════
	//  DEATH POSE — cuerpo colapsando al suelo
	// ══════════════════════════════════════════════════════════
	if (bDeathPose)
	{
		DeathPoseTimer += DeltaTime;
		const float T     = FMath::Clamp(DeathPoseTimer / DeathCollapseDuration, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseIn(0.0f, 1.0f, T, 2.5f);
		const FQuat CharRot = OwnerCharacter->GetActorQuat();
		const FVector Origin = OwnerCharacter->GetActorLocation();

		// Cuerpo cae a nivel de suelo (origin Z - 65 ≈ rozando el suelo)
		const FVector DeathBody = FVector(Origin.X, Origin.Y, Origin.Z - 65.0f);
		BodyPos = FMath::VInterpTo(BodyPos, DeathBody, DeltaTime, BodyLerpSpeed * (1.0f + 3.0f * Eased));

		// Manos se desploman a los lados, cerca del suelo
		RightHandPos = FMath::VInterpTo(RightHandPos,
			BodyPos + CharRot.RotateVector(FVector(25.0f,  65.0f, -28.0f)),
			DeltaTime, HandLerpSpeed * 2.5f);
		LeftHandPos = FMath::VInterpTo(LeftHandPos,
			BodyPos + CharRot.RotateVector(FVector(25.0f, -65.0f, -28.0f)),
			DeltaTime, HandLerpSpeed * 2.5f);

		// Pies se separan ligeramente
		RightFootPos = FMath::VInterpTo(RightFootPos,
			BodyPos + CharRot.RotateVector(FVector(15.0f,  38.0f, -62.0f)),
			DeltaTime, FootLerpSpeed * 2.0f);
		LeftFootPos = FMath::VInterpTo(LeftFootPos,
			BodyPos + CharRot.RotateVector(FVector(15.0f, -38.0f, -62.0f)),
			DeltaTime, FootLerpSpeed * 2.0f);

		if (BodySphere) BodySphere->SetWorldLocation(BodyPos);
		if (RightHand)  RightHand->SetWorldLocation(RightHandPos);
		if (LeftHand)   LeftHand->SetWorldLocation(LeftHandPos);
		if (RightFoot)  RightFoot->SetWorldLocation(RightFootPos);
		if (LeftFoot)   LeftFoot->SetWorldLocation(LeftFootPos);

		if (TashMesh && bDriveSkeletalMesh) DriveSkeletalBones(0.0f);
		return;  // skip normal update
	}

	// ── Speed ratio 0→1 (walk→run) ───────────────────────────
	const float Speed      = OwnerCharacter->GetCharacterMovement()->Velocity.Size2D();
	const float MaxSpeed   = FMath::Max(OwnerCharacter->RunSpeed, 1.0f);
	const float SpeedRatio = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f);

	// ── Advance gait timer ───────────────────────────────────
	GaitTimer += DeltaTime * GaitFrequency * SpeedRatio;

	// ── Dash roll angle (avanza mientras dasha, reset al terminar) ──────────
	{
		const bool bNowDashing = (OwnerCharacter->CurrentState == ECharacterState::Dashing);
		if (bNowDashing)
			DashRollAngle += DeltaTime * FMath::DegreesToRadians(DashRollSpeed);
		else if (bWasDashing)
			DashRollAngle = 0.0f;
		bWasDashing = bNowDashing;
	}

	// ── Body bob ─────────────────────────────────────────────
	const float Bob = FMath::Sin(GaitTimer * 2.0f) * BodyBobAmplitude * SpeedRatio;

	// ── VInterpTo — fast lerp, no positional lag ─────────────
	BodyPos      = FMath::VInterpTo(BodyPos,      GetBodyTarget(Bob),     DeltaTime, BodyLerpSpeed);
	RightHandPos = FMath::VInterpTo(RightHandPos, GetRightHandTarget(),   DeltaTime, HandLerpSpeed);
	LeftHandPos  = FMath::VInterpTo(LeftHandPos,  GetLeftHandTarget(),    DeltaTime, HandLerpSpeed);
	RightFootPos = FMath::VInterpTo(RightFootPos, GetRightFootTarget(),   DeltaTime, FootLerpSpeed);
	LeftFootPos  = FMath::VInterpTo(LeftFootPos,  GetLeftFootTarget(),    DeltaTime, FootLerpSpeed);

	// ── Apply positions ───────────────────────────────────────
	if (BodySphere) BodySphere->SetWorldLocation(BodyPos);
	if (RightHand)  RightHand->SetWorldLocation(RightHandPos);
	if (LeftHand)   LeftHand->SetWorldLocation(LeftHandPos);

	// Feet: position + rotation (tip-down cone with gait tilt)
	if (RightFoot)
	{
		RightFoot->SetWorldLocation(RightFootPos);
		RightFoot->SetWorldRotation(GetFootRotation(GaitTimer,         SpeedRatio));
	}
	if (LeftFoot)
	{
		LeftFoot->SetWorldLocation(LeftFootPos);
		LeftFoot->SetWorldRotation(GetFootRotation(GaitTimer + PI,     SpeedRatio));
	}

	// ── Skeletal mesh (Tash final character) ─────────────────
	if (TashMesh && bDriveSkeletalMesh)
	{
		DriveSkeletalBones(SpeedRatio);
	}

	// ── Debug ─────────────────────────────────────────────────
	if (bShowDebug)
	{
		DrawDebugSphere(GetWorld(), BodyPos,      BodyRadius,     12, FColor::White,  false, -1.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), RightHandPos, HandRadius,      8, FColor::Yellow, false, -1.0f, 0, 1.0f);
		DrawDebugSphere(GetWorld(), LeftHandPos,  HandRadius,      8, FColor::Yellow, false, -1.0f, 0, 1.0f);
		DrawDebugSphere(GetWorld(), RightFootPos, FootConeRadius,  8, FColor::Cyan,   false, -1.0f, 0, 1.0f);
		DrawDebugSphere(GetWorld(), LeftFootPos,  FootConeRadius,  8, FColor::Cyan,   false, -1.0f, 0, 1.0f);
	}
}

// ============================================================
//  Target positions
// ============================================================

FVector UProceduralLimbsComponent::GetBodyTarget(float BobOffset) const
{
	// Durante el dash el cuerpo baja: la figura se encoge en bola
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
		return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, BodyZOffset - 22.0f);

	return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, BodyZOffset + BobOffset);
}

FVector UProceduralLimbsComponent::GetRightHandTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Origin  = OwnerCharacter->GetActorLocation();
	const FQuat   CharRot = OwnerCharacter->GetActorQuat();
	const FVector FwdDir  = OwnerCharacter->GetActorForwardVector();
	const FVector Base    = Origin + CharRot.RotateVector(RightHandRestOffset);

	// ── Ultimate: ambas manos apuntan hacia el láser ──────────
	if (OwnerCharacter->UltimateComponent && OwnerCharacter->UltimateComponent->bLaserActive)
	{
		const FVector LaserDir = OwnerCharacter->FollowCamera
			? OwnerCharacter->FollowCamera->GetForwardVector()
			: FwdDir;
		return Origin + LaserDir * 90.0f + CharRot.RotateVector(FVector(0.0f, 22.0f, 0.0f));
	}

	// ── Dash: bola rodando — mano derecha orbita en plano Forward-Up ────────
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
	{
		const FVector Fwd   = OwnerCharacter->GetActorForwardVector();
		const FVector Right = OwnerCharacter->GetActorRightVector();
		return BodyPos
			+ Right * 14.0f
			+ Fwd             * (DashBallRadius * FMath::Cos(DashRollAngle))
			+ FVector::UpVector * (DashBallRadius * FMath::Sin(DashRollAngle));
	}

	// ── Armed: arma siempre gana sobre cualquier estado de combate ──────────
	if (OwnerCharacter->bIsWeaponDrawn && OwnerCharacter->EquippedWeapon)
	{
		const FVector WeaponLoc = OwnerCharacter->EquippedWeapon->GetActorLocation();
		return WeaponLoc - OwnerCharacter->EquippedWeapon->GetActorUpVector() * (WeaponHandSeparation * 0.5f);
	}

	// ── Unarmed state overrides ───────────────────────────────
	if (OwnerCharacter->CurrentState == ECharacterState::Jumping)
		return Base + FVector(0.0f, 0.0f, 28.0f);

	if (OwnerCharacter->CurrentState == ECharacterState::Attacking)
		return Base + FwdDir * 35.0f;

	if (OwnerCharacter->CurrentState == ECharacterState::Parrying)
		return Base + FwdDir * 18.0f + FVector(0.0f, 0.0f, 18.0f);

	// ── Unarmed idle: reposo + balanceo ──────────────────────
	const float Swing = FMath::Sin(GaitTimer + PI) * IdleHandSwingAmplitude;
	return Base + FwdDir * Swing;
}

FVector UProceduralLimbsComponent::GetLeftHandTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Origin  = OwnerCharacter->GetActorLocation();
	const FQuat   CharRot = OwnerCharacter->GetActorQuat();
	const FVector FwdDir  = OwnerCharacter->GetActorForwardVector();
	const FVector Base    = Origin + CharRot.RotateVector(LeftHandRestOffset);

	// ── Ultimate: ambas manos al frente ───────────────────────
	if (OwnerCharacter->UltimateComponent && OwnerCharacter->UltimateComponent->bLaserActive)
	{
		const FVector LaserDir = OwnerCharacter->FollowCamera
			? OwnerCharacter->FollowCamera->GetForwardVector()
			: FwdDir;
		return Origin + LaserDir * 90.0f + CharRot.RotateVector(FVector(0.0f, -22.0f, 0.0f));
	}

	// ── Dash: mano izquierda orbita en fase opuesta (PI) ────────
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
	{
		const FVector Fwd   = OwnerCharacter->GetActorForwardVector();
		const FVector Right = OwnerCharacter->GetActorRightVector();
		return BodyPos
			- Right * 14.0f
			+ Fwd             * (DashBallRadius * FMath::Cos(DashRollAngle + PI))
			+ FVector::UpVector * (DashBallRadius * FMath::Sin(DashRollAngle + PI));
	}

	// ── Armed: agarre secundario siempre sigue el arma ─────────
	if (OwnerCharacter->bIsWeaponDrawn && OwnerCharacter->EquippedWeapon)
	{
		const FVector WeaponLoc = OwnerCharacter->EquippedWeapon->GetActorLocation();
		return WeaponLoc + OwnerCharacter->EquippedWeapon->GetActorUpVector() * (WeaponHandSeparation * 0.5f);
	}

	// ── Grapple (sin arma): mano izquierda sigue el anclaje ──
	if (OwnerCharacter->GrappleComponent &&
		(OwnerCharacter->GrappleComponent->IsGrappling() || OwnerCharacter->GrappleComponent->IsAiming())
		&& OwnerCharacter->GrappleHandAttachPoint)
	{
		return OwnerCharacter->GrappleHandAttachPoint->GetComponentLocation();
	}

	// ── Unarmed state overrides ───────────────────────────────
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
		return Base - FwdDir * 40.0f;

	if (OwnerCharacter->CurrentState == ECharacterState::Jumping)
		return Base + FVector(0.0f, 0.0f, 28.0f);

	if (OwnerCharacter->CurrentState == ECharacterState::Attacking)
		return Base + FwdDir * 15.0f + FVector(0.0f, 0.0f, 10.0f);

	if (OwnerCharacter->CurrentState == ECharacterState::Parrying)
		return Base + FwdDir * 18.0f + FVector(0.0f, 0.0f, 18.0f);

	// ── Unarmed idle ──────────────────────────────────────────
	const float Swing = FMath::Sin(GaitTimer) * IdleHandSwingAmplitude;
	return Base + FwdDir * Swing;
}

FVector UProceduralLimbsComponent::GetRightFootTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Base     = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(RightFootRestOffset);
	const FVector FwdDir   = OwnerCharacter->GetActorForwardVector();

	// ── Salto: pies se recogen hacia arriba ───────────────────
	if (OwnerCharacter->CurrentState == ECharacterState::Jumping)
		return Base + FVector(0.0f, 0.0f, 38.0f);

	// ── Dash: pie derecho orbita 90° por delante de la mano derecha ──────────
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
	{
		const FVector Fwd   = OwnerCharacter->GetActorForwardVector();
		const FVector Right = OwnerCharacter->GetActorRightVector();
		const float   Angle = DashRollAngle + PI * 0.5f;
		return BodyPos
			+ Right * 8.0f
			+ Fwd             * (DashBallRadius * FMath::Cos(Angle))
			+ FVector::UpVector * (DashBallRadius * FMath::Sin(Angle));
	}

	// ── Normal: gait oscillation ──────────────────────────────
	const float SpeedRatio = FMath::Clamp(
		OwnerCharacter->GetCharacterMovement()->Velocity.Size2D() / FMath::Max(OwnerCharacter->RunSpeed, 1.0f),
		0.0f, 1.0f);
	const float Swing = FMath::Sin(GaitTimer) * FootSwingAmplitude * SpeedRatio;
	return Base + FwdDir * Swing;
}

FVector UProceduralLimbsComponent::GetLeftFootTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Base   = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(LeftFootRestOffset);
	const FVector FwdDir = OwnerCharacter->GetActorForwardVector();

	// ── Salto: pies se recogen ────────────────────────────────
	if (OwnerCharacter->CurrentState == ECharacterState::Jumping)
		return Base + FVector(0.0f, 0.0f, 38.0f);

	// ── Dash: pie izquierdo orbita 90° por detrás de la mano izquierda ───────
	if (OwnerCharacter->CurrentState == ECharacterState::Dashing)
	{
		const FVector Fwd   = OwnerCharacter->GetActorForwardVector();
		const FVector Right = OwnerCharacter->GetActorRightVector();
		const float   Angle = DashRollAngle - PI * 0.5f;
		return BodyPos
			- Right * 8.0f
			+ Fwd             * (DashBallRadius * FMath::Cos(Angle))
			+ FVector::UpVector * (DashBallRadius * FMath::Sin(Angle));
	}

	// ── Normal: gait (fase opuesta a pie derecho) ─────────────
	const float SpeedRatio = FMath::Clamp(
		OwnerCharacter->GetCharacterMovement()->Velocity.Size2D() / FMath::Max(OwnerCharacter->RunSpeed, 1.0f),
		0.0f, 1.0f);
	const float Swing = FMath::Sin(GaitTimer + PI) * FootSwingAmplitude * SpeedRatio;
	return Base + FwdDir * Swing;
}

// ============================================================
//  Foot rotation: cone tip-down + forward/back tilt with gait
// ============================================================
FRotator UProceduralLimbsComponent::GetFootRotation(float GaitPhase, float SpeedRatio) const
{
	// The UE5 Cone mesh default: tip at +Z, base at -Z.
	// To flip tip-down we pitch 180°.
	// On top of that, tilt forward when swinging forward and back when trailing.
	const float TiltDeg = FMath::Sin(GaitPhase) * FootTiltAngle * SpeedRatio;

	// Character facing yaw so the cone faces the same direction as the character
	const float CharYaw = OwnerCharacter ? OwnerCharacter->GetActorRotation().Yaw : 0.0f;

	// Pitch=180 flips cone, then TiltDeg tilts in locomotion direction
	return FRotator(180.0f + TiltDeg, CharYaw, 0.0f);
}

// ============================================================
//  Factory helpers
// ============================================================
UStaticMeshComponent* UProceduralLimbsComponent::MakeSphereComp(const FString& Name,
	float WorldRadius, UStaticMesh* Mesh, UMaterialInterface* Mat) const
{
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(GetOwner(), *Name);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Comp->bCastDynamicShadow = true;
	Comp->CastShadow         = true;
	if (Mat) Comp->SetMaterial(0, Mat);

	// Scale so the sphere's world radius equals WorldRadius
	const float Scale = WorldRadius / SPHERE_UNIT_RADIUS;
	Comp->SetWorldScale3D(FVector(Scale));

	Comp->RegisterComponent();
	Comp->AttachToComponent(OwnerCharacter->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);
	GetOwner()->AddInstanceComponent(Comp);
	return Comp;
}

UStaticMeshComponent* UProceduralLimbsComponent::MakeConeComp(const FString& Name,
	float WorldBaseRadius, float WorldHeight, UStaticMesh* Mesh, UMaterialInterface* Mat) const
{
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(GetOwner(), *Name);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Comp->bCastDynamicShadow = true;
	Comp->CastShadow         = true;
	if (Mat) Comp->SetMaterial(0, Mat);

	// Scale: XY controls base radius, Z controls height
	const float ScaleXY = WorldBaseRadius / CONE_UNIT_BASE_RADIUS;
	const float ScaleZ  = WorldHeight     / CONE_UNIT_HEIGHT;
	Comp->SetRelativeScale3D(FVector(ScaleXY, ScaleXY, ScaleZ));

	Comp->RegisterComponent();
	Comp->AttachToComponent(OwnerCharacter->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);
	GetOwner()->AddInstanceComponent(Comp);
	return Comp;
}

// ============================================================
//  Death pose
// ============================================================

void UProceduralLimbsComponent::EnterDeathPose()
{
	bDeathPose     = true;
	DeathPoseTimer = 0.0f;
}

void UProceduralLimbsComponent::ExitDeathPose()
{
	bDeathPose     = false;
	DeathPoseTimer = 0.0f;

	// Re-seed positions from respawn location so no snap
	if (OwnerCharacter)
	{
		const FVector    Loc = OwnerCharacter->GetActorLocation();
		const FQuat      Rot = OwnerCharacter->GetActorQuat();
		BodyPos      = Loc + FVector(0.0f, 0.0f, BodyZOffset);
		RightHandPos = Loc + Rot.RotateVector(RightHandRestOffset);
		LeftHandPos  = Loc + Rot.RotateVector(LeftHandRestOffset);
		RightFootPos = Loc + Rot.RotateVector(RightFootRestOffset);
		LeftFootPos  = Loc + Rot.RotateVector(LeftFootRestOffset);
	}
}

// ============================================================
//  Skeletal mesh IK driver
// ============================================================

void UProceduralLimbsComponent::DriveSkeletalBones(float SpeedRatio)
{
	if (!TashMesh || !OwnerCharacter) return;

	const FQuat CharRot = OwnerCharacter->GetActorQuat();

	// ── Centro (cuerpo) ───────────────────────────────────────────────────────
	TashMesh->SetBoneLocationByName(BoneName_Root, BodyPos, EBoneSpaces::WorldSpace);
	TashMesh->SetBoneRotationByName(BoneName_Root,
		OwnerCharacter->GetActorRotation(), EBoneSpaces::WorldSpace);

	// ── Brazo derecho ─────────────────────────────────────────────────────────
	{
		const FVector Shoulder  = BodyPos + CharRot.RotateVector(RightShoulderOffset);
		const FVector ElbowHint = CharRot.RotateVector(FVector(-0.25f, 1.0f, -0.15f)).GetSafeNormal();
		const FVector Elbow     = SolveTwoBoneIK(Shoulder, RightHandPos,
			UpperArmLength, LowerArmLength, ElbowHint);

		TashMesh->SetBoneLocationByName(BoneName_ShoulderR, Shoulder,     EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_ElbowR,    Elbow,        EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_HandR,     RightHandPos, EBoneSpaces::WorldSpace);

		if (!Shoulder.Equals(Elbow))
			TashMesh->SetBoneRotationByName(BoneName_ShoulderR,
				(Elbow - Shoulder).ToOrientationRotator(), EBoneSpaces::WorldSpace);
		if (!Elbow.Equals(RightHandPos))
			TashMesh->SetBoneRotationByName(BoneName_ElbowR,
				(RightHandPos - Elbow).ToOrientationRotator(), EBoneSpaces::WorldSpace);
	}

	// ── Brazo izquierdo ───────────────────────────────────────────────────────
	{
		const FVector Shoulder  = BodyPos + CharRot.RotateVector(LeftShoulderOffset);
		const FVector ElbowHint = CharRot.RotateVector(FVector(-0.25f, -1.0f, -0.15f)).GetSafeNormal();
		const FVector Elbow     = SolveTwoBoneIK(Shoulder, LeftHandPos,
			UpperArmLength, LowerArmLength, ElbowHint);

		TashMesh->SetBoneLocationByName(BoneName_ShoulderL, Shoulder,    EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_ElbowL,    Elbow,       EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_HandL,     LeftHandPos, EBoneSpaces::WorldSpace);

		if (!Shoulder.Equals(Elbow))
			TashMesh->SetBoneRotationByName(BoneName_ShoulderL,
				(Elbow - Shoulder).ToOrientationRotator(), EBoneSpaces::WorldSpace);
		if (!Elbow.Equals(LeftHandPos))
			TashMesh->SetBoneRotationByName(BoneName_ElbowL,
				(LeftHandPos - Elbow).ToOrientationRotator(), EBoneSpaces::WorldSpace);
	}

	// ── Pierna derecha ────────────────────────────────────────────────────────
	{
		const FVector Hip      = BodyPos + CharRot.RotateVector(RightHipOffset);
		const FVector KneeHint = CharRot.RotateVector(FVector(1.0f, 0.2f, 0.0f)).GetSafeNormal();
		const FVector Knee     = SolveTwoBoneIK(Hip, RightFootPos,
			UpperLegLength, LowerLegLength, KneeHint);

		TashMesh->SetBoneLocationByName(BoneName_KneeR, Knee,         EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_FootR, RightFootPos, EBoneSpaces::WorldSpace);

		if (!Knee.Equals(RightFootPos))
			TashMesh->SetBoneRotationByName(BoneName_KneeR,
				(RightFootPos - Knee).ToOrientationRotator(), EBoneSpaces::WorldSpace);
		TashMesh->SetBoneRotationByName(BoneName_FootR,
			GetFootRotation(GaitTimer, SpeedRatio), EBoneSpaces::WorldSpace);
	}

	// ── Pierna izquierda ──────────────────────────────────────────────────────
	{
		const FVector Hip      = BodyPos + CharRot.RotateVector(LeftHipOffset);
		const FVector KneeHint = CharRot.RotateVector(FVector(1.0f, -0.2f, 0.0f)).GetSafeNormal();
		const FVector Knee     = SolveTwoBoneIK(Hip, LeftFootPos,
			UpperLegLength, LowerLegLength, KneeHint);

		TashMesh->SetBoneLocationByName(BoneName_KneeL, Knee,        EBoneSpaces::WorldSpace);
		TashMesh->SetBoneLocationByName(BoneName_FootL, LeftFootPos, EBoneSpaces::WorldSpace);

		if (!Knee.Equals(LeftFootPos))
			TashMesh->SetBoneRotationByName(BoneName_KneeL,
				(LeftFootPos - Knee).ToOrientationRotator(), EBoneSpaces::WorldSpace);
		TashMesh->SetBoneRotationByName(BoneName_FootL,
			GetFootRotation(GaitTimer + PI, SpeedRatio), EBoneSpaces::WorldSpace);
	}
}

// ============================================================
//  Solver analítico de IK de 2 huesos (ley de cosenos)
// ============================================================

FVector UProceduralLimbsComponent::SolveTwoBoneIK(
	const FVector& Root, const FVector& Tip,
	float Upper, float Lower,
	const FVector& HintDir) const
{
	FVector AC = Tip - Root;
	float   D  = AC.Size();

	const float MaxReach = Upper + Lower - 0.5f;
	const float MinReach = FMath::Abs(Upper - Lower) + 0.5f;
	D = FMath::Clamp(D, MinReach, MaxReach);

	// Ley de cosenos — ángulo en Root entre Root→Mid y Root→Tip
	float CosA = (Upper*Upper + D*D - Lower*Lower) / (2.0f * Upper * D);
	CosA = FMath::Clamp(CosA, -1.0f, 1.0f);
	const float SinA = FMath::Sqrt(FMath::Max(0.0f, 1.0f - CosA*CosA));

	const FVector DirAC = AC.GetSafeNormal();

	// Componente de HintDir perpendicular a DirAC (define el plano de doblado)
	FVector Perp = HintDir - (HintDir | DirAC) * DirAC;
	if (Perp.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		Perp = FVector::UpVector - (FVector::UpVector | DirAC) * DirAC;
		if (Perp.SizeSquared() < KINDA_SMALL_NUMBER)
			Perp = FVector::RightVector;
	}
	Perp.Normalize();

	return Root + Upper * (CosA * DirAC + SinA * Perp);
}

// ============================================================
//  Hit Flash (jugador se pone rojo al recibir daño)
// ============================================================

void UProceduralLimbsComponent::StartHitFlash()
{
	// Cachear materiales originales la primera vez de TODOS los meshes
	if (!bMaterialsCached)
	{
		OriginalMaterials.Empty();
		
		// Si tenemos TashMesh, cachear de ahí
		if (TashMesh && TashMesh->GetNumMaterials() > 0)
		{
			for (int32 i = 0; i < TashMesh->GetNumMaterials(); i++)
			{
				OriginalMaterials.Add(TashMesh->GetMaterial(i));
			}
			UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Cached %d materials from TashMesh"), OriginalMaterials.Num());
		}
		// Si no, cachear de las primitivas
		else if (BodySphere && BodySphere->GetNumMaterials() > 0)
		{
			for (int32 i = 0; i < BodySphere->GetNumMaterials(); i++)
			{
				OriginalMaterials.Add(BodySphere->GetMaterial(i));
			}
			UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Cached %d materials from BodySphere"), OriginalMaterials.Num());
		}
		bMaterialsCached = true;
	}

	// Crear instancia de material de flash si no existe
	if (!FlashMaterialInstance)
	{
		UMaterial* BaseMaterial = LoadObject<UMaterial>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!BaseMaterial)
		{
			// Fallback: intentar otro material básico
			BaseMaterial = LoadObject<UMaterial>(nullptr, 
				TEXT("/Engine/EngineMaterials/WorldGridMaterial"));
		}
		
		if (BaseMaterial)
		{
			FlashMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (FlashMaterialInstance)
			{
				FlashMaterialInstance->SetVectorParameterValue(FName("Color"), HitFlashColor);
				UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Created flash material instance"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ProceduralLimbs: Could not load base material for hit flash"));
		}
	}

	// Aplicar material de flash a todos los meshes
	if (FlashMaterialInstance)
	{
		if (TashMesh)
		{
			for (int32 i = 0; i < TashMesh->GetNumMaterials(); i++)
			{
				TashMesh->SetMaterial(i, FlashMaterialInstance);
			}
			UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Applied flash to %d TashMesh materials"), TashMesh->GetNumMaterials());
		}
		else
		{
			// Aplicar a todas las primitivas si no hay TashMesh
			if (BodySphere) 
			{
				BodySphere->SetMaterial(0, FlashMaterialInstance);
				UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Applied flash to BodySphere"));
			}
			if (RightHand) RightHand->SetMaterial(0, FlashMaterialInstance);
			if (LeftHand) LeftHand->SetMaterial(0, FlashMaterialInstance);
			if (RightFoot) RightFoot->SetMaterial(0, FlashMaterialInstance);
			if (LeftFoot) LeftFoot->SetMaterial(0, FlashMaterialInstance);
		}
	}

	// Programar restauración de materiales
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
		World->GetTimerManager().SetTimer(
			HitFlashTimerHandle, this,
			&UProceduralLimbsComponent::StopHitFlash,
			HitFlashDuration, false);
		
		UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Hit flash timer set for %.2f seconds"), HitFlashDuration);
	}
}

void UProceduralLimbsComponent::StopHitFlash()
{
	UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: StopHitFlash called. bMaterialsCached=%d, OriginalMaterials.Num()=%d"), 
		bMaterialsCached, OriginalMaterials.Num());

	if (!bMaterialsCached || OriginalMaterials.Num() == 0) 
	{
		UE_LOG(LogTemp, Warning, TEXT("ProceduralLimbs: No cached materials to restore!"));
		return;
	}

	// Restaurar materiales en TashMesh
	if (TashMesh)
	{
		for (int32 i = 0; i < OriginalMaterials.Num() && i < TashMesh->GetNumMaterials(); i++)
		{
			if (OriginalMaterials[i])
			{
				TashMesh->SetMaterial(i, OriginalMaterials[i]);
				UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Restored TashMesh material slot %d"), i);
			}
		}
	}
	else if (BodySphere)
	{
		// Restaurar en primitivas
		if (BodySphere->GetNumMaterials() > 0 && OriginalMaterials.Num() > 0)
		{
			if (OriginalMaterials[0])
			{
				BodySphere->SetMaterial(0, OriginalMaterials[0]);
				UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Restored BodySphere material"));
			}
		}
		// También restaurar en otros componentes
		if (RightHand && OriginalMaterials.Num() > 0)
			RightHand->SetMaterial(0, OriginalMaterials[0]);
		if (LeftHand && OriginalMaterials.Num() > 0)
			LeftHand->SetMaterial(0, OriginalMaterials[0]);
		if (RightFoot && OriginalMaterials.Num() > 0)
			RightFoot->SetMaterial(0, OriginalMaterials[0]);
		if (LeftFoot && OriginalMaterials.Num() > 0)
			LeftFoot->SetMaterial(0, OriginalMaterials[0]);
	}

	UE_LOG(LogTemp, Log, TEXT("ProceduralLimbs: Hit flash ended - materials restored"));
}
