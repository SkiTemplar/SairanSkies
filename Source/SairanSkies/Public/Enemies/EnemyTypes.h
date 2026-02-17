// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

// Forward declarations
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;
class UParticleSystem;
class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;

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
	Positioning		UMETA(DisplayName = "Positioning"),
	Attacking		UMETA(DisplayName = "Attacking"),
	Taunting		UMETA(DisplayName = "Taunting"),
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

	// Distancia mínima de ataque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MinAttackDistance = 150.0f;

	// Distancia máxima de ataque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxAttackDistance = 200.0f;

	// Distancia de posicionamiento (donde se queda antes de atacar)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float PositioningDistance = 350.0f;

	// Tiempo mínimo en posición antes de atacar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MinPositioningTime = 1.0f;

	// Tiempo máximo en posición antes de atacar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxPositioningTime = 3.0f;

	// Daño base del ataque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 10.0f;

	// Cooldown entre ataques
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldown = 2.0f;

	// Radio para detectar compañeros
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AllyDetectionRadius = 1500.0f;

	// Número mínimo de compañeros para volverse más agresivo
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

	// Radio de visión
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightRadius = 2000.0f;

	// Ángulo de visión periférica
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float PeripheralVisionAngle = 90.0f;

	// Radio de audición
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float HearingRadius = 1000.0f;

	// Radio de proximidad (siempre detecta)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float ProximityRadius = 300.0f;

	// Tiempo para perder al objetivo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float LoseSightTime = 5.0f;

	// Tiempo de investigación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float InvestigationTime = 10.0f;

	// Radio de investigación alrededor del último punto visto
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

	// Velocidad de patrulla (porcentaje del máximo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PatrolSpeedMultiplier = 0.5f;

	// Velocidad de persecución (porcentaje del máximo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChaseSpeedMultiplier = 1.0f;

	// Tiempo de espera en cada punto de patrulla (mínimo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTimeAtPatrolPoint = 2.0f;

	// Tiempo de espera máximo (se elige aleatorio entre min y max)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float MaxWaitTimeAtPatrolPoint = 4.0f;

	// Radio de aceptación para llegar a un punto
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolPointAcceptanceRadius = 100.0f;

	// Si el patrullaje es aleatorio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bRandomPatrol = false;
};

/**
 * Configuración de comportamiento natural/idle del enemigo (AAA-style)
 */
USTRUCT(BlueprintType)
struct FEnemyBehaviorConfig
{
	GENERATED_BODY()

	// ========== IDLE/PATROL BEHAVIOR ==========
	
	// Probabilidad de detenerse brevemente durante la patrulla (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToStopDuringPatrol = 0.15f;

	// Duración mínima de pausa aleatoria
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float MinRandomPauseDuration = 0.5f;

	// Duración máxima de pausa aleatoria
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float MaxRandomPauseDuration = 2.0f;

	// Velocidad de rotación al mirar alrededor (grados/segundo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float LookAroundSpeed = 60.0f;

	// Ángulo máximo de rotación al mirar alrededor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float MaxLookAroundAngle = 90.0f;

	// Probabilidad de mirar en una dirección aleatoria al esperar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToLookAround = 0.4f;

	// Variación aleatoria en velocidad de patrulla (porcentaje)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float PatrolSpeedVariation = 0.15f;

	// ========== ALERT/DETECTION BEHAVIOR ==========

	// Tiempo de reacción antes de perseguir (simula "procesar" la información)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert")
	float ReactionTimeMin = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert")
	float ReactionTimeMax = 0.6f;

	// Nivel de sospecha necesario para investigar (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SuspicionThresholdInvestigate = 0.3f;

	// Nivel de sospecha necesario para perseguir (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SuspicionThresholdChase = 0.7f;

	// Velocidad a la que aumenta la sospecha por segundo (cuando ve al jugador)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert")
	float SuspicionBuildUpRate = 0.5f;

	// Velocidad a la que decae la sospecha por segundo (cuando no ve al jugador)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert")
	float SuspicionDecayRate = 0.2f;

	// ========== COMBAT BEHAVIOR ==========

	// Probabilidad de dudar antes de atacar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToHesitateBeforeAttack = 0.2f;

	// Duración de la duda antes de atacar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float HesitationDuration = 0.5f;

	// Probabilidad de hacer strafe durante el combate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToStrafe = 0.3f;

	// Duración del strafe
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float StrafeDuration = 1.5f;

	// ========== INVESTIGATION BEHAVIOR ==========

	// Número de puntos a investigar antes de rendirse
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation")
	int32 InvestigationPointsToCheck = 3;

	// Tiempo mirando en cada punto de investigación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation")
	float TimeAtEachInvestigationPoint = 2.0f;

	// Probabilidad de rascarse la cabeza / gesto de confusión
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToShowConfusion = 0.3f;
};

// Delegate para notificar cambios de estado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyStateChanged, EEnemyState, OldState, EEnemyState, NewState);

// Delegate para notificar detección del jugador
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerDetected, AActor*, Player, EEnemySenseType, SenseType);

// Delegate para notificar pérdida del jugador
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLost);

// Delegate para notificar muerte
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, AController*, InstigatorController);

// Forward declaration for conversation delegate
class AEnemyBase;

// Delegate para notificar inicio/fin de conversación
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationStarted, AEnemyBase*, ConversationPartner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);


/**
 * Configuración de animaciones del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyAnimationConfig
{
	GENERATED_BODY()

	// ========== MONTAJES DE COMBATE ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> HeavyAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> HitReactionMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* DeathMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* BlockMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* DodgeMontage = nullptr;

	// ========== MONTAJES DE COMPORTAMIENTO ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	TArray<UAnimMontage*> TauntMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	TArray<UAnimMontage*> InvestigateMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	UAnimMontage* ConfusionMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	UAnimMontage* AlertMontage = nullptr;

	// ========== MONTAJES DE CONVERSACIÓN ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<UAnimMontage*> ConversationIdleMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<UAnimMontage*> ConversationGestureMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	UAnimMontage* ConversationStartMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	UAnimMontage* ConversationEndMontage = nullptr;

	// ========== MONTAJES DE IDLE ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	TArray<UAnimMontage*> IdleMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	UAnimMontage* LookAroundMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	UAnimMontage* StretchMontage = nullptr;
};

/**
 * Configuración de sonidos del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemySoundConfig
{
	GENERATED_BODY()

	// ========== SONIDOS DE COMBATE ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> AttackSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> HitSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> DeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<USoundBase*> PainSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	USoundBase* BlockSound = nullptr;

	// ========== SONIDOS DE VOZ ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> TauntVoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> AlertVoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> InvestigateVoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> ConfusionVoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TArray<USoundBase*> SpotPlayerVoices;

	// ========== SONIDOS DE CONVERSACIÓN ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<USoundBase*> ConversationVoices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<USoundBase*> LaughSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversation")
	TArray<USoundBase*> AgreementSounds;

	// ========== SONIDOS DE MOVIMIENTO ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TArray<USoundBase*> FootstepSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	USoundBase* ArmorRustleSound = nullptr;

	// ========== CONFIGURACIÓN DE AUDIO ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float VoiceVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MinTimeBetweenVoices = 3.0f;
};

/**
 * Configuración de efectos visuales del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyVFXConfig
{
	GENERATED_BODY()

	// ========== EFECTOS DE COMBATE (Niagara) ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Niagara")
	UNiagaraSystem* HitEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Niagara")
	UNiagaraSystem* BloodEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Niagara")
	UNiagaraSystem* DeathEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Niagara")
	UNiagaraSystem* BlockEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Niagara")
	UNiagaraSystem* AttackTrailEffect = nullptr;

	// ========== EFECTOS DE ESTADO (Niagara) ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Niagara")
	UNiagaraSystem* AlertEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Niagara")
	UNiagaraSystem* ConfusionEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Niagara")
	UNiagaraSystem* SpotPlayerEffect = nullptr;

	// ========== EFECTOS LEGACY (Cascade) ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy|Cascade")
	UParticleSystem* HitParticle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Legacy|Cascade")
	UParticleSystem* DeathParticle = nullptr;
};

/**
 * Configuración de meshes y materiales del enemigo
 */
USTRUCT(BlueprintType)
struct FEnemyMeshConfig
{
	GENERATED_BODY()

	// ========== SKELETAL MESHES ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal")
	USkeletalMesh* BodyMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal")
	USkeletalMesh* HeadMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal")
	TArray<USkeletalMesh*> ArmorVariants;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal")
	TArray<USkeletalMesh*> WeaponMeshes;

	// ========== STATIC MESHES (Props) ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static")
	UStaticMesh* ShieldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static")
	TArray<UStaticMesh*> AccessoryMeshes;

	// ========== MATERIALES ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	TArray<UMaterialInterface*> BodyMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	TArray<UMaterialInterface*> ArmorMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* DamageMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* DeathMaterial = nullptr;
};

/**
 * Configuración de sockets y attachment points
 */
USTRUCT(BlueprintType)
struct FEnemySocketConfig
{
	GENERATED_BODY()

	// ========== SOCKETS DE ARMAS ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName RightHandSocket = TEXT("hand_r_socket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName LeftHandSocket = TEXT("hand_l_socket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName BackWeaponSocket = TEXT("spine_03_socket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName ShieldSocket = TEXT("shield_socket");

	// ========== SOCKETS DE EFECTOS ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FName HeadSocket = TEXT("head_socket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FName ChestSocket = TEXT("spine_02_socket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FName HitEffectSocket = TEXT("pelvis");

	// ========== SOCKETS DE AUDIO ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FName VoiceSocket = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FName FootLeftSocket = TEXT("foot_l");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FName FootRightSocket = TEXT("foot_r");
};

/**
 * Configuración del sistema de conversación entre enemigos
 */
USTRUCT(BlueprintType)
struct FEnemyConversationConfig
{
	GENERATED_BODY()

	// ========== DISTANCIA Y DETECCIÓN ==========
	
	// Radio para detectar a otro enemigo para conversar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float ConversationDetectionRadius = 200.0f;

	// Distancia mínima para considerar que están "juntos" en un punto
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float SamePointRadius = 150.0f;

	// Tiempo que deben estar parados juntos antes de empezar conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float TimeBeforeConversation = 2.0f;

	// ========== DURACIÓN ==========
	
	// Duración mínima de la conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float MinConversationDuration = 5.0f;

	// Duración máxima de la conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float MaxConversationDuration = 15.0f;

	// Intervalo entre gestos/voces durante la conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duration")
	float GestureInterval = 2.0f;

	// ========== PROBABILIDADES ==========
	
	// Probabilidad de hacer un gesto durante la conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToGesture = 0.4f;

	// Probabilidad de reírse
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToLaugh = 0.2f;

	// Probabilidad de asentir
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChanceToNod = 0.3f;

	// ========== COMPORTAMIENTO ==========
	
	// Si debe mirar al compañero durante la conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	bool bLookAtPartner = true;

	// Si la conversación puede ser interrumpida por detección del jugador
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	bool bCanBeInterrupted = true;

	// Cooldown antes de poder tener otra conversación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float ConversationCooldown = 30.0f;
};

