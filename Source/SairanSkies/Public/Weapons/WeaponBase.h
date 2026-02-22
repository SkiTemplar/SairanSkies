// SairanSkies - Base Weapon Class (Greatsword)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ASairanCharacter;
class UNiagaraComponent;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Sheathed,
	Drawn,
	Attacking
};

UCLASS()
class SAIRANSKIES_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ========== COMPONENTS ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UBoxComponent* HitCollision;

	// ========== SWING TRAIL (Lies of P style) ==========
	/** Niagara component for the sword trail - stays attached to weapon */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Trail")
	UNiagaraComponent* SwingTrailComponent;

	/** Normal swing trail VFX (white/blue arc) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Trail")
	UNiagaraSystem* NormalSwingTrailFX;

	/** Blood trail VFX (red, activated when hitting an enemy mid-swing) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Trail")
	UNiagaraSystem* BloodSwingTrailFX;

	/** Activate the swing trail (call at start of attack) */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void ActivateSwingTrail();

	/** Deactivate the swing trail (call at end of attack) */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void DeactivateSwingTrail();

	/** Switch from normal trail to blood trail (call on enemy hit) */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void SwitchToBloodTrail();

	// ========== WEAPON SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Placeholder")
	FVector WeaponSize = FVector(20.0f, 10.0f, 150.0f); // Large rectangular sword

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Placeholder")
	FLinearColor WeaponColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f); // Metallic gray


	// ========== STATE ==========
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponState CurrentState = EWeaponState::Drawn;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ASairanCharacter* OwnerCharacter;

	// ========== FUNCTIONS ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipToCharacter(ASairanCharacter* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToHand();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToBack();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToBlockPosition();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EnableHitCollision();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DisableHitCollision();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponState(EWeaponState NewState);

	/** Set weapon to blocking/parry stance */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetBlockingStance(bool bIsBlocking);

	/** Get if weapon is in blocking stance */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsInBlockingStance() const { return bInBlockingStance; }

protected:
	UFUNCTION()
	void OnHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void SetupPlaceholderMesh();

	bool bInBlockingStance = false;
	bool bIsBloodTrailActive = false;
};
