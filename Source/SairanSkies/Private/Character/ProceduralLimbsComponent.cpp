// SairanSkies - Procedural Limbs Component Implementation

#include "Character/ProceduralLimbsComponent.h"
#include "Character/SairanCharacter.h"
#include "Combat/GrappleComponent.h"
#include "Weapons/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"

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

	// ── Speed ratio 0→1 (walk→run) ───────────────────────────
	const float Speed      = OwnerCharacter->GetCharacterMovement()->Velocity.Size2D();
	const float MaxSpeed   = FMath::Max(OwnerCharacter->RunSpeed, 1.0f);
	const float SpeedRatio = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f);

	// ── Advance gait timer ───────────────────────────────────
	GaitTimer += DeltaTime * GaitFrequency * SpeedRatio;

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
	return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, BodyZOffset + BobOffset);
}

FVector UProceduralLimbsComponent::GetRightHandTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	// ── Armed: right hand at the base/handle of the weapon ───
	if (OwnerCharacter->bIsWeaponDrawn && OwnerCharacter->EquippedWeapon)
	{
		const FVector WeaponLoc = OwnerCharacter->EquippedWeapon->GetActorLocation();
		// Slight separation along the weapon's up vector so hands don't fully overlap
		return WeaponLoc - OwnerCharacter->EquippedWeapon->GetActorUpVector() * (WeaponHandSeparation * 0.5f);
	}

	// ── Unarmed: rest position + small idle arm swing ─────────
	const FVector Base = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(RightHandRestOffset);
	const float Swing  = FMath::Sin(GaitTimer + PI) * IdleHandSwingAmplitude;
	return Base + OwnerCharacter->GetActorForwardVector() * Swing;
}

FVector UProceduralLimbsComponent::GetLeftHandTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	// ── Armed: left hand on weapon (secondary grip, above right) ─
	if (OwnerCharacter->bIsWeaponDrawn && OwnerCharacter->EquippedWeapon)
	{
		const FVector WeaponLoc = OwnerCharacter->EquippedWeapon->GetActorLocation();
		return WeaponLoc + OwnerCharacter->EquippedWeapon->GetActorUpVector() * (WeaponHandSeparation * 0.5f);
	}

	// ── Grappling: track the grapple-hook hand attach point ───
	if (OwnerCharacter->GrappleComponent && OwnerCharacter->GrappleComponent->IsGrappling()
		&& OwnerCharacter->GrappleHandAttachPoint)
	{
		return OwnerCharacter->GrappleHandAttachPoint->GetComponentLocation();
	}
	if (OwnerCharacter->GrappleComponent && OwnerCharacter->GrappleComponent->IsAiming()
		&& OwnerCharacter->GrappleHandAttachPoint)
	{
		return OwnerCharacter->GrappleHandAttachPoint->GetComponentLocation();
	}

	// ── Unarmed: rest + idle swing (opposite phase to right hand) ─
	const FVector Base = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(LeftHandRestOffset);
	const float Swing  = FMath::Sin(GaitTimer) * IdleHandSwingAmplitude;
	return Base + OwnerCharacter->GetActorForwardVector() * Swing;
}

FVector UProceduralLimbsComponent::GetRightFootTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Base    = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(RightFootRestOffset);
	const float SpeedRatio = FMath::Clamp(
		OwnerCharacter->GetCharacterMovement()->Velocity.Size2D() / FMath::Max(OwnerCharacter->RunSpeed, 1.0f),
		0.0f, 1.0f);
	const float Swing = FMath::Sin(GaitTimer) * FootSwingAmplitude * SpeedRatio;
	return Base + OwnerCharacter->GetActorForwardVector() * Swing;
}

FVector UProceduralLimbsComponent::GetLeftFootTarget() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const FVector Base    = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorQuat().RotateVector(LeftFootRestOffset);
	const float SpeedRatio = FMath::Clamp(
		OwnerCharacter->GetCharacterMovement()->Velocity.Size2D() / FMath::Max(OwnerCharacter->RunSpeed, 1.0f),
		0.0f, 1.0f);
	// Opposite phase so feet alternate
	const float Swing = FMath::Sin(GaitTimer + PI) * FootSwingAmplitude * SpeedRatio;
	return Base + OwnerCharacter->GetActorForwardVector() * Swing;
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
