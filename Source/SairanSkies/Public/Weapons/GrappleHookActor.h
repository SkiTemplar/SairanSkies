// SairanSkies - Grapple Hook Visual Actor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleHookActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * Visual representation of the grapple hook.
 * Spawned when aiming and shows the hook in the character's left hand.
 */
UCLASS()
class SAIRANSKIES_API AGrappleHookActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrappleHookActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ========== COMPONENTS ==========
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grapple")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grapple")
	UStaticMeshComponent* HookMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grapple")
	UStaticMeshComponent* HandleMesh;

	// ========== SETTINGS ==========
	
	/** Size of the hook head */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Placeholder")
	FVector HookSize = FVector(10.0f, 5.0f, 15.0f);

	/** Size of the handle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Placeholder")
	FVector HandleSize = FVector(8.0f, 8.0f, 25.0f);

	/** Color of the grapple hook */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Placeholder")
	FLinearColor GrappleColor = FLinearColor(0.3f, 0.3f, 0.35f, 1.0f);

	// ========== FUNCTIONS ==========
	
	/** Show the hook (when aiming) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void ShowHook();

	/** Hide the hook (when not aiming) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void HideHook();

	/** Set the hook to aim at a specific world location */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void AimAtLocation(const FVector& TargetLocation);

	/** Set if the target is valid (changes color) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void SetTargetValid(bool bIsValid);

private:
	void SetupPlaceholderMesh();

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	FLinearColor ValidColor = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);
	FLinearColor InvalidColor = FLinearColor(0.8f, 0.0f, 0.0f, 1.0f);
};
