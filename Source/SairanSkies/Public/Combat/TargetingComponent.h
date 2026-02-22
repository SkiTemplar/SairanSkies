// SairanSkies - Targeting Component (Arkham-style auto targeting)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class ASairanCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTargetingComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== TARGETING FUNCTIONS ==========
	
	/** Find the best target in range based on distance and direction */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* FindBestTarget();

	/** Get all valid targets in range */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	TArray<AActor*> GetAllTargetsInRange();

	/** Snap the character to the target (Arkham-style instant approach) */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SnapToTarget(AActor* Target);

	/** Check if an actor is a valid target */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsValidTarget(AActor* PotentialTarget) const;

	/** Check if target is visible (line of sight) */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool HasLineOfSightToTarget(AActor* Target) const;

	// ========== SETTINGS ==========
	
	/** Maximum distance to detect enemies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	float TargetingRadius = 1000.0f; // 10 meters

	/** Maximum distance for snap-to-enemy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	float MaxSnapDistance = 800.0f;

	/** Time to complete snap movement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	float SnapDuration = 0.15f;

	/** How much to weight direction vs distance (0 = only distance, 1 = only direction) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionWeight = 0.6f;

	/** Minimum dot product to consider target in "forward" direction */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	float MinDirectionDot = -0.3f;

	/** Distance to stop from target when snapping */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	float SnapStopDistance = 150.0f;

	/** Tag that enemies must have to be targeted */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Targeting")
	FName EnemyTag = FName("Enemy");

	// ========== STATE ==========
	
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	AActor* CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	bool bIsSnapping = false;

protected:
	UPROPERTY()
	ASairanCharacter* OwnerCharacter;

private:
	/** Calculate target score (higher = better target) */
	float CalculateTargetScore(AActor* Target) const;

	/** Interpolate snap movement */
	void UpdateSnapMovement(float DeltaTime);

	FVector SnapStartLocation;
	FVector SnapEndLocation;
	float SnapElapsedTime;
	FRotator SnapTargetRotation;
};
