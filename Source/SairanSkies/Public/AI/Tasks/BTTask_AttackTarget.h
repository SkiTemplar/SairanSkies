// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackTarget.generated.h"

class AEnemyBase;
class AEnemyAIController;

/**
 * Fases del ataque — incluye acercarse y esperar turno
 */
UENUM()
enum class EAttackPhase : uint8
{
	WaitCooldown,	// Esperando que termine el cooldown
	Approach,	// Acercándose al jugador
	WaitTurn,	// Esperando hueco de atacante
	WindUp,		// Preparación antes del golpe
	Strike,		// Ejecutando el golpe (aplica daño)
	Recovery,	// Recuperación después del golpe
	ComboGap,	// Pausa entre golpes del combo
	Finished	// Combo completo
};

/**
 * Task de ataque completo y autónomo.
 * 
 * Se acerca al jugador, espera su turno si hay demasiados atacantes,
 * elige un combo según la distancia, ejecuta los golpes con rotación
 * debug alternante, y se desregistra al terminar.
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
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float WindUpDuration = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float StrikeDuration = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float RecoveryDuration = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.1"))
	float ComboGapDuration = 0.4f;

	// ── Daño ──
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DamageVariance = 0.15f;

	// ── Espera ──
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "1.0"))
	float MaxWaitTurnTime = 5.0f;

	// ── Debug visual: rotación alternante por golpe ──
	UPROPERTY(EditAnywhere, Category = "Attack|Debug", meta = (ClampMin = "0.0"))
	float DebugRotationDegrees = 25.0f;

	/** Distancia que el enemigo avanza hacia el target durante Strike (debug lunge) */
	UPROPERTY(EditAnywhere, Category = "Attack|Debug", meta = (ClampMin = "0.0"))
	float DebugLungeDistance = 50.0f;

private:
	EAttackPhase Phase = EAttackPhase::WaitCooldown;
	float PhaseTimer = 0.0f;
	int32 CurrentHit = 0;
	int32 TotalHits = 1;
	float ChosenAttackDist = 0.0f;
	FRotator OriginalRotation = FRotator::ZeroRotator;
	FVector StrikeStartLocation = FVector::ZeroVector;

	void PickComboByDistance(AEnemyBase* Enemy);
	void ApplyDamage(AEnemyBase* Enemy, AActor* Target);
	void ApplyDebugRotation(AEnemyBase* Enemy, bool bRestore);
	void Cleanup(AEnemyBase* Enemy);
};
