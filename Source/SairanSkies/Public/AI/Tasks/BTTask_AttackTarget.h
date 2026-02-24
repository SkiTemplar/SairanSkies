// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackTarget.generated.h"

class AEnemyBase;
class AEnemyAIController;

/**
 * Fases del ataque dentro del inner circle.
 * El enemigo ya fue admitido al inner circle antes de que este task empiece.
 */
UENUM()
enum class EAttackPhase : uint8
{
	Approach,    // Acercándose a posición de ataque
	WindUp,      // Preparación antes del golpe
	Strike,      // Ejecutando golpe (aplica daño + rotación debug)
	Recovery,    // Recuperación post-golpe
	ComboGap,    // Pausa entre golpes del combo
	Finished     // Combo completo → decide quedarse o salir
};

/**
 * Task de ataque del inner circle.
 *
 * Prerequisito: el GroupCombatManager ya admitió al enemigo al inner circle.
 * 
 * 1. Se acerca a una posición random dentro del rango de ataque
 * 2. Selecciona combo probabilístico según distancia:
 *    - Combo 0 → mayor prob. cuando CERCA
 *    - Combo N → mayor prob. cuando LEJOS
 * 3. Ejecuta golpes con rotación debug alternante
 * 4. Al terminar, decide con probabilidad si quedarse o salir al outer circle
 */
UCLASS()
class SAIRANSKIES_API UBTTask_AttackTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// ── Timing ──
	UPROPERTY(EditAnywhere, Category = "Attack|Timing", meta = (ClampMin = "0.05"))
	float WindUpDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Attack|Timing", meta = (ClampMin = "0.05"))
	float StrikeDuration = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Attack|Timing", meta = (ClampMin = "0.05"))
	float RecoveryDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Attack|Timing", meta = (ClampMin = "0.1"))
	float ComboGapDuration = 0.35f;

	/** Max time allowed to approach attack position before aborting */
	UPROPERTY(EditAnywhere, Category = "Attack|Timing", meta = (ClampMin = "1.0"))
	float MaxApproachTime = 3.0f;

	// ── Damage ──
	UPROPERTY(EditAnywhere, Category = "Attack|Damage", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DamageVariance = 0.15f;

	// ── Debug visual: rotación alternante por golpe ──
	UPROPERTY(EditAnywhere, Category = "Attack|Debug", meta = (ClampMin = "0.0"))
	float DebugRotationDegrees = 25.0f;

	/** Distancia de lunge debug hacia el target durante Strike */
	UPROPERTY(EditAnywhere, Category = "Attack|Debug", meta = (ClampMin = "0.0"))
	float DebugLungeDistance = 60.0f;

private:
	EAttackPhase Phase = EAttackPhase::Approach;
	float PhaseTimer = 0.0f;
	int32 CurrentHit = 0;
	int32 TotalHits = 1;
	int32 ChosenComboIndex = 0;
	FRotator OriginalRotation = FRotator::ZeroRotator;
	FVector StrikeStartLocation = FVector::ZeroVector;
	FVector AttackPosition = FVector::ZeroVector;

	/** Probabilistic combo selection algorithm */
	void PickComboByDistance(AEnemyBase* Enemy);

	/** Apply damage with variance */
	void ApplyDamage(AEnemyBase* Enemy, AActor* Target);

	/** Debug rotation per hit */
	void ApplyDebugRotation(AEnemyBase* Enemy, bool bRestore);

	/** Cleanup on finish/abort */
	void Cleanup(UBehaviorTreeComponent& OwnerComp, AEnemyBase* Enemy);
};
