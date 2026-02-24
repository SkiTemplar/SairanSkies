// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

// Forward declarations
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;
class AEnemyBase;

/**
 * Estados de comportamiento del enemigo
 */
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Patrolling		UMETA(DisplayName = "Patrolling"),
	Investigating	UMETA(DisplayName = "Investigating"),
	Chasing			UMETA(DisplayName = "Chasing"),
	OuterCircle		UMETA(DisplayName = "Outer Circle"),
	InnerCircle		UMETA(DisplayName = "Inner Circle"),
	Attacking		UMETA(DisplayName = "Attacking"),
	Conversing		UMETA(DisplayName = "Conversing"),
	Dead			UMETA(DisplayName = "Dead")
};

/**
 * Tipos de sentidos del enemigo
 */
UENUM(BlueprintType)
enum class EEnemySenseType : uint8
{
	None		UMETA(DisplayName = "None"),
	Sight		UMETA(DisplayName = "Sight"),
	Hearing		UMETA(DisplayName = "Hearing"),
	Damage		UMETA(DisplayName = "Damage"),
	Proximity	UMETA(DisplayName = "Proximity"),
	Alert		UMETA(DisplayName = "Alert From Ally")
};

/**
 * Configuración de combate del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyCombatConfig
{
	GENERATED_BODY()

	// ========== DOS CÍRCULOS ==========

	/** Radio del círculo exterior — distancia donde los enemigos esperan/tauntan */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float OuterCircleRadius = 500.0f;

	/** Variación aleatoria del radio exterior (±) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float OuterCircleVariation = 80.0f;

	/** Distancia mínima para posicionarse al atacar (dentro del inner circle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float MinAttackPositionDist = 100.0f;

	/** Distancia máxima para posicionarse al atacar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float MaxAttackPositionDist = 200.0f;

	/** Probabilidad de quedarse en el inner circle tras atacar (0=siempre sale, 1=siempre se queda) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToStayInnerAfterAttack = 0.25f;

	/** Delay mínimo antes de reaccionar cuando el jugador se aleja */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float PlayerMoveReactionDelayMin = 0.4f;

	/** Delay máximo antes de reaccionar cuando el jugador se aleja */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TwoCircles")
	float PlayerMoveReactionDelayMax = 1.5f;

	// ========== ATAQUE ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MinAttackDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxAttackDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldown = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AllyDetectionRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxSimultaneousAttackers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MinAlliesForAggression = 2;
};

/**
 * Configuración de percepción del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyPerceptionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float PeripheralVisionAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float HearingRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float ProximityRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float LoseSightTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float InvestigationTime = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float InvestigationRadius = 500.0f;
};

/**
 * Configuración de patrulla del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyPatrolConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PatrolSpeedMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChaseSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTimeAtPatrolPoint = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float MaxWaitTimeAtPatrolPoint = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolPointAcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bRandomPatrol = false;
};

/**
 * Configuración de comportamiento natural del enemigo (AAA-style)
 */
USTRUCT(BlueprintType)
struct FEnemyBehaviorConfig
{
	GENERATED_BODY()


	// ========== COMPORTAMIENTO NATURAL ==========
	
	// Probabilidad de pausar brevemente durante patrulla
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToPauseDuringPatrol = 0.15f;

	// Duración de pausas aleatorias
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural")
	float MinPauseDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural")
	float MaxPauseDuration = 3.0f;

	// Probabilidad de mirar alrededor cuando está en idle/pausa
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToLookAround = 0.4f;

	// Ángulo máximo al mirar alrededor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural")
	float MaxLookAroundAngle = 90.0f;

	// Velocidad de rotación al mirar alrededor (grados/segundo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural")
	float LookAroundSpeed = 60.0f;

	// Variación de velocidad durante patrulla (±%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural", meta = (ClampMin = "0.0", ClampMax = "0.3"))
	float PatrolSpeedVariation = 0.1f;
};

/**
 * Configuración de conversación entre enemigos
 */
USTRUCT(BlueprintType)
struct FEnemyConversationConfig
{
	GENERATED_BODY()

	// Radio para detectar otro enemigo para conversar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float ConversationRadius = 200.0f;

	// Tiempo que deben estar juntos antes de conversar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float TimeBeforeConversation = 3.0f;

	// Duración mínima de conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float MinConversationDuration = 5.0f;

	// Duración máxima de conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float MaxConversationDuration = 12.0f;

	// Cooldown antes de otra conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float ConversationCooldown = 30.0f;

	// Probabilidad de hacer gesto durante conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToGesture = 0.3f;

	// Intervalo entre gestos
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float GestureInterval = 2.5f;
};

// ========== DELEGATES ==========

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyStateChanged, EEnemyState, OldState, EEnemyState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerDetected, AActor*, Player, EEnemySenseType, SenseType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, AController*, InstigatorController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationStarted, AEnemyBase*, Partner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);

/**
 * Configuración de animaciones del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyAnimationConfig
{
	GENERATED_BODY()

	// Montajes de ataque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> AttackMontages;

	// Reacción al golpe
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> HitReactionMontages;

	// Muerte
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* DeathMontage = nullptr;

	// Mirar alrededor (idle)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	UAnimMontage* LookAroundMontage = nullptr;

	// Gestos de conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<UAnimMontage*> ConversationGestures;
};

/**
 * Configuración de sonidos del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemySoundConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> AttackSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> PainSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> DeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> AlertSounds;

	// Sonidos de conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<USoundBase*> ConversationSounds;

	// Risas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<USoundBase*> LaughSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float Volume = 1.0f;
};

/**
 * Configuración de efectos visuales del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyVFXConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UNiagaraSystem* HitEffect = nullptr;

	/** Blood splatter VFX spawned from enemy on hit (separate from HitEffect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UNiagaraSystem* BloodVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UNiagaraSystem* DeathEffect = nullptr;
};

