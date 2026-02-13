// SairanSkies - Base Enemy Class for Testing

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDamaged, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDeath);

UCLASS()
class SAIRANSKIES_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Damage handling
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// ========== HEALTH ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	// ========== PARRY ==========
	/** Is this enemy currently in an attackable parry window */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInAttackWindup = false;

	/** Duration of attack windup (parryable window) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
	float AttackWindupDuration = 0.5f;

	// ========== EVENTS ==========
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDeath OnDeath;

	// ========== FUNCTIONS ==========
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartAttackWindup();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndAttackWindup();

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

protected:
	FTimerHandle WindupTimer;
};
