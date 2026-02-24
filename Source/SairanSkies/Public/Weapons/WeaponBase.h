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

	/**
	 * Normal swing trail VFX asset (white / blue arc).
	 * Spawned dynamically at the start of every attack.
	 * Assign in your Weapon Blueprint.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Trail")
	UNiagaraSystem* NormalSwingTrailFX;

	/**
	 * Blood trail VFX asset (red / dark).
	 * Spawned *on top of* the normal trail when the blade connects with an enemy.
	 * The normal trail is NOT removed — both trails run simultaneously until they expire.
	 * Assign in your Weapon Blueprint.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Trail")
	UNiagaraSystem* BloodSwingTrailFX;

	/** Called at the start of an attack swing — spawns a fresh normal trail. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void ActivateSwingTrail();

	/**
	 * Called at the END of the attack animation.
	 * Stops emission on every active trail so particles die naturally (not cut instantly).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void DeactivateSwingTrail();

	/**
	 * Called when the blade hits an enemy (mid-swing).
	 * Spawns a blood trail WITHOUT touching the normal trail.
	 * Both trails continue until their particles expire.
	 * Safe to call multiple times — only one blood trail is created per swing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Trail")
	void SwitchToBloodTrail();

	// ========== WEAPON SETTINGS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Placeholder")
	FVector WeaponSize = FVector(20.0f, 10.0f, 150.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Placeholder")
	FLinearColor WeaponColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f);

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

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsInBlockingStance() const { return bInBlockingStance; }

protected:
	UFUNCTION()
	void OnHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void SetupPlaceholderMesh();

	bool bInBlockingStance = false;

	/** True once a blood trail has been spawned this swing — prevents duplicates. */
	bool bBloodTrailSpawnedThisSwing = false;

	/**
	 * All Niagara components currently alive for this swing.
	 * Using UPROPERTY so UE GC doesn't collect them while emission is running.
	 * Entries are removed lazily when the component is no longer valid.
	 */
	UPROPERTY()
	TArray<UNiagaraComponent*> ActiveTrailComponents;
};
