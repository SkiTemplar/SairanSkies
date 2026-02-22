// SairanSkies - Grapple Hook Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleComponent.generated.h"

class ASairanCharacter;
class USpringArmComponent;
class UStaticMeshComponent;
class USceneComponent;
class AGrappleHookActor;
class UGrappleCrosshairWidget;
class APlayerController;

UENUM(BlueprintType)
enum class EGrappleState : uint8
{
	Idle,          // No grapple active
	Aiming,        // Player is aiming (L2/F held)
	Firing,        // Grapple has been fired (hook traveling)
	Pulling,       // Player is being pulled towards target
	Releasing      // Player has passed midpoint, in freefall
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleAimStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleAimEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleFired, FVector, TargetLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleComplete);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGrappleComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MAIN FUNCTIONS ==========
	
	/** Start aiming the grapple hook (L2/F pressed) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void StartAiming();

	/** Stop aiming without firing (L2/F released without valid target) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void StopAiming();

	/** Fire the grapple hook to the current aim point */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void FireGrapple();

	/** Cancel an active grapple */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void CancelGrapple();

	// ========== STATE QUERIES ==========
	
	UFUNCTION(BlueprintPure, Category = "Grapple")
	bool IsAiming() const { return CurrentState == EGrappleState::Aiming; }

	UFUNCTION(BlueprintPure, Category = "Grapple")
	bool IsGrappling() const { return CurrentState == EGrappleState::Pulling || CurrentState == EGrappleState::Releasing; }

	UFUNCTION(BlueprintPure, Category = "Grapple")
	bool HasValidTarget() const { return bHasValidTarget; }

	UFUNCTION(BlueprintPure, Category = "Grapple")
	FVector GetAimTargetLocation() const { return AimTargetLocation; }

	UFUNCTION(BlueprintPure, Category = "Grapple")
	EGrappleState GetGrappleState() const { return CurrentState; }

	// ========== SETTINGS ==========
	
	/** Maximum range of the grapple hook (30 meters = 3000 units) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	float MaxGrappleRange = 3000.0f;

	/** Tag that objects must have to be grappleable */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	FName GrappleTag = FName("Grapple");

	/** If true, only actors with GrappleTag can be grappled. If false, any surface works (tag still gives aim assist priority) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	bool bRequireGrappleTag = true;

	// ========== AIM ASSIST ==========

	/** Screen-space radius (in pixels) for aim assist detection area */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|AimAssist")
	float AimAssistScreenRadius = 250.0f;

	/** How quickly the aim snaps to the target (higher = faster) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|AimAssist")
	float AimAssistStickySpeed = 18.0f;

	/** How quickly the crosshair moves on screen (higher = faster) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|AimAssist")
	float CrosshairLerpSpeed = 15.0f;

	/** Currently soft-locked grapple target actor */
	UPROPERTY(BlueprintReadOnly, Category = "Grapple|AimAssist")
	AActor* CurrentSoftLockTarget = nullptr;

	/** Speed at which the player is pulled towards the target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	float GrapplePullSpeed = 2500.0f;

	/** Angle offset below target to avoid collision (in degrees) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	float GrappleAngleOffset = 15.0f;

	/** Distance threshold to consider "passing midpoint" - when to release (in units, 100 = 1 meter) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	float MidpointReleaseDistance = 100.0f;

	/** Duration of velocity dampening after grapple release (in seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	float DampeningDuration = 1.0f;

	/** Percentage of horizontal velocity to reduce during dampening (0.0 - 1.0, where 0.8 = 80%) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DampeningFactor = 0.8f;

	/** Camera distance when aiming grapple (closer to shoulder) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Camera")
	float AimingCameraDistance = 150.0f;

	/** Camera zoom speed when transitioning */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Camera")
	float CameraZoomSpeed = 8.0f;

	/** Socket offset when aiming (over the shoulder view) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Camera")
	FVector AimingCameraOffset = FVector(0.0f, 70.0f, 30.0f);

	/** Collision channel to trace against for grapple targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Settings")
	TEnumAsByte<ECollisionChannel> GrappleTraceChannel = ECC_Visibility;

	// ========== VISUAL SETTINGS ==========
	
	/** Color of the crosshair when valid target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Visuals")
	FLinearColor ValidTargetColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

	/** Color of the crosshair when no valid target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Visuals")
	FLinearColor InvalidTargetColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	/** Niagara particle system for grapple trail (from character during pull) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Visuals")
	class UNiagaraSystem* GrappleTrailParticles;

	/** Show debug visualization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Debug")
	bool bShowDebug = false;

	// ========== VISUAL HOOK ==========
	
	/** Class of the grapple hook actor to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Visual")
	TSubclassOf<AGrappleHookActor> GrappleHookClass;

	/** Reference to the spawned grapple hook actor */
	UPROPERTY(BlueprintReadOnly, Category = "Grapple|Visual")
	AGrappleHookActor* GrappleHookActor;

	// ========== CROSSHAIR UI ==========
	
	/** Widget class for the grapple crosshair */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|UI")
	TSubclassOf<UGrappleCrosshairWidget> CrosshairWidgetClass;

	/** Reference to the crosshair widget instance */
	UPROPERTY(BlueprintReadOnly, Category = "Grapple|UI")
	UGrappleCrosshairWidget* CrosshairWidget;

	// ========== PARTICLE EFFECTS ==========
	
	/** Reference to the spawned grapple trail particle component */
	UPROPERTY(BlueprintReadOnly, Category = "Grapple|Visuals")
	class UNiagaraComponent* GrappleTrailComponent;

	// ========== EVENTS ==========
	
	UPROPERTY(BlueprintAssignable, Category = "Grapple|Events")
	FOnGrappleAimStart OnGrappleAimStart;

	UPROPERTY(BlueprintAssignable, Category = "Grapple|Events")
	FOnGrappleAimEnd OnGrappleAimEnd;

	UPROPERTY(BlueprintAssignable, Category = "Grapple|Events")
	FOnGrappleFired OnGrappleFired;

	UPROPERTY(BlueprintAssignable, Category = "Grapple|Events")
	FOnGrappleComplete OnGrappleComplete;

	// ========== STATE ==========
	
	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	EGrappleState CurrentState = EGrappleState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	FVector GrappleTargetPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	FVector AimTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	bool bHasValidTarget = false;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

private:
	// Core functions
	void UpdateAiming(float DeltaTime);
	void UpdatePulling(float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void UpdateCharacterRotation(float DeltaTime);
	FHitResult PerformAimTrace();
	void SetState(EGrappleState NewState);
	void ResetGrapple();
	
	// Aim assist - find best grappleable target in screen area
	AActor* FindBestGrappleTarget();
	
	// Calculate the adjusted target direction (15 degrees below actual target)
	FVector CalculateGrappleDirection() const;
	
	// Check if we've passed the midpoint
	bool HasPassedMidpoint() const;

	// Grapple hook actor management
	void SpawnGrappleHookActor();
	void UpdateGrappleHookVisual();

	// Crosshair UI management
	void CreateCrosshairWidget();
	void ShowCrosshair();
	void HideCrosshair();
	void UpdateCrosshair(float DeltaTime);

	// Particle effects management
	void StartGrappleTrailParticles();
	void StopGrappleTrailParticles();

	// Camera lock management
	void LockCamera();
	void UnlockCamera();

	// Velocity dampening
	void UpdateVelocityDampening(float DeltaTime);

	// Camera state
	float OriginalCameraDistance;
	FVector OriginalCameraOffset;
	float TargetCameraDistance;
	FVector TargetCameraOffset;
	bool bOriginalUsePawnControlRotation;
	bool bOriginalEnableCameraLag;

	// Character rotation state
	bool bOriginalOrientRotationToMovement;

	// Gravity state
	float OriginalGravityScale;

	// Grapple state
	FVector GrappleStartPoint;
	FVector GrappleMidpoint;
	float InitialDistanceToTarget;
	bool bCameraTransitioning = false;

	// Post-grapple velocity dampening state
	bool bIsDampeningVelocity = false;
	float DampeningTimeRemaining = 0.0f;

	// Crosshair smooth tracking state
	FVector2D CurrentCrosshairPos = FVector2D::ZeroVector;
	bool bCrosshairInitialized = false;
};
