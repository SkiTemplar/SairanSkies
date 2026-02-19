// SairanSkies - Checkpoint Component (auto-saves last grounded position)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CheckpointComponent.generated.h"

class ASairanCharacter;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRespawn, FVector, RespawnLocation);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UCheckpointComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCheckpointComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== MAIN FUNCTIONS ==========

	/** Respawn the player at the last safe checkpoint position */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void RespawnAtLastCheckpoint();

	/** Get the current saved checkpoint position */
	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	FVector GetLastCheckpointLocation() const { return LastSafeLocation; }

	// ========== SETTINGS ==========

	/** How often to save the grounded position (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkpoint|Settings")
	float SaveInterval = 0.25f;

	/** Minimum distance from last save to create a new one (avoids saving same spot) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkpoint|Settings")
	float MinSaveDistance = 50.0f;

	// ========== VFX/SFX ==========

	/** VFX played at respawn location */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkpoint|VFX")
	UNiagaraSystem* RespawnVFX;

	/** Sound played on respawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkpoint|SFX")
	USoundBase* RespawnSound;

	// ========== DEBUG ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkpoint|Debug")
	bool bShowDebug = false;

	// ========== EVENTS ==========
	UPROPERTY(BlueprintAssignable, Category = "Checkpoint|Events")
	FOnRespawn OnRespawn;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

private:
	/** The last known safe grounded position */
	FVector LastSafeLocation;
	FRotator LastSafeRotation;

	float SaveTimer = 0.0f;
	bool bHasValidCheckpoint = false;
};
