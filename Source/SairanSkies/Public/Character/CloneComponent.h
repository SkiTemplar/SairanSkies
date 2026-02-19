// SairanSkies - Clone/Teleport Component (It Takes Two style)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CloneComponent.generated.h"

class ASairanCharacter;
class UNiagaraSystem;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class ECloneState : uint8
{
	Inactive,		// No clone placed
	CloneActive		// Clone exists, press again to teleport
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClonePlaced, FVector, CloneLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeleported, FVector, FromLocation, FVector, ToLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCloneExpired);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UCloneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCloneComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MAIN FUNCTIONS ==========

	/** Handle clone input: place clone or teleport to it */
	UFUNCTION(BlueprintCallable, Category = "Clone")
	void HandleCloneInput();

	/** Place a clone at the current position */
	UFUNCTION(BlueprintCallable, Category = "Clone")
	bool PlaceClone();

	/** Teleport to the clone position and destroy it */
	UFUNCTION(BlueprintCallable, Category = "Clone")
	void TeleportToClone();

	/** Remove the clone without teleporting */
	UFUNCTION(BlueprintCallable, Category = "Clone")
	void DestroyClone();

	// ========== STATE QUERIES ==========

	UFUNCTION(BlueprintPure, Category = "Clone")
	bool IsCloneActive() const { return CurrentState == ECloneState::CloneActive; }

	UFUNCTION(BlueprintPure, Category = "Clone")
	ECloneState GetCloneState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Clone")
	float GetCloneTimeRemaining() const;

	/** Get distance from player to clone */
	UFUNCTION(BlueprintPure, Category = "Clone")
	float GetDistanceToClone() const;

	/** Check if player is within TP range of clone */
	UFUNCTION(BlueprintPure, Category = "Clone")
	bool IsWithinTeleportRange() const;

	// ========== SETTINGS ==========

	/** How long the clone stays before expiring (seconds) - backup safety, set high */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Settings")
	float CloneDuration = 300.0f;

	/** Max distance (cm) from clone to teleport when weapon is DRAWN (in combat) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Settings")
	float MaxTeleportDistanceCombat = 2000.0f; // 20m

	/** Max distance (cm) from clone to teleport when weapon is SHEATHED (out of combat) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Settings")
	float MaxTeleportDistanceExplore = 5000.0f; // 50m

	/** Force applied backward when placing clone (acts as a short dash) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Settings")
	float PushBackForce = 800.0f;

	/** Minimum distance from ceiling to allow clone placement (units) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Settings")
	float MinCeilingClearance = 200.0f;

	/** Opacity of the clone (0-1, lower = more transparent) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Visuals")
	float CloneOpacity = 0.4f;

	/** Material to apply to the clone for ghost/translucent effect (optional, creates dynamic if null) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Visuals")
	UMaterialInterface* CloneGhostMaterial;

	// ========== VFX ==========

	/** Particle effect when placing the clone */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|VFX")
	UNiagaraSystem* CloneSpawnVFX;

	/** Particle effect when teleporting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|VFX")
	UNiagaraSystem* TeleportVFX;

	// ========== SFX ==========

	/** Sound when placing the clone */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|SFX")
	USoundBase* ClonePlaceSound;

	/** Sound when teleporting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|SFX")
	USoundBase* TeleportSound;

	// ========== DEBUG ==========

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clone|Debug")
	bool bShowDebug = false;

	// ========== EVENTS ==========

	UPROPERTY(BlueprintAssignable, Category = "Clone|Events")
	FOnClonePlaced OnClonePlaced;

	UPROPERTY(BlueprintAssignable, Category = "Clone|Events")
	FOnTeleported OnTeleported;

	UPROPERTY(BlueprintAssignable, Category = "Clone|Events")
	FOnCloneExpired OnCloneExpired;

	// ========== STATE ==========

	UPROPERTY(BlueprintReadOnly, Category = "Clone")
	ECloneState CurrentState = ECloneState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Clone")
	FVector CloneLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Clone")
	FRotator CloneRotation = FRotator::ZeroRotator;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

private:
	/** Check if the current position is valid for clone placement */
	bool IsValidPlacementPosition() const;

	/** Spawn the visual clone actor */
	void SpawnCloneVisual();

	/** Play VFX at a location */
	void PlayVFXAtLocation(UNiagaraSystem* System, const FVector& Location);

	/** Play SFX at a location */
	void PlaySFXAtLocation(USoundBase* Sound, const FVector& Location);

	/** Called when clone timer expires */
	void OnCloneTimerExpired();

	/** The visual clone actor in the world */
	UPROPERTY()
	AActor* CloneActor = nullptr;

	FTimerHandle CloneTimerHandle;
	float CloneStartTime = 0.0f;
};
