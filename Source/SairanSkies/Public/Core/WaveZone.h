// SairanSkies - Zona de Oleadas

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveZone.generated.h"

class UBoxComponent;
class AEnemyBase;

/**
 * Actor que define una zona de combate de oleadas.
 *
 * Comportamiento:
 *  - El jugador entra en el BoxTrigger → comienza la oleada 1 (BaseEnemyCount enemigos).
 *  - Cada oleada duplica el número de enemigos (10, 20, 40, 80...).
 *  - La siguiente oleada no empieza hasta que todos los enemigos de la actual hayan muerto.
 *  - El jugador sale → todos los enemigos se destruyen y la zona se reinicia a la oleada 0.
 *  - El jugador vuelve a entrar → comienza de nuevo desde la oleada 1.
 *
 * Configuración mínima en el nivel:
 *  1. Colocar AWaveZone en el nivel.
 *  2. Asignar EnemyClass (BP del enemigo a spawnear).
 *  3. Ajustar el tamaño del BoxTrigger para cubrir el área de combate.
 */
UCLASS()
class SAIRANSKIES_API AWaveZone : public AActor
{
	GENERATED_BODY()

public:
	AWaveZone();

protected:
	virtual void BeginPlay() override;

public:
	// ── Componentes ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* ZoneTrigger;

	// ── Config ───────────────────────────────────────────────────────────────

	/** Clase de enemigo a spawnear (asignar en el nivel) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Wave")
	TSubclassOf<AEnemyBase> EnemyClass;

	/** Enemigos en la oleada 1 — se duplica cada oleada (10, 20, 40…) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave",
		meta=(ClampMin="1"))
	int32 BaseEnemyCount = 10;

	/**
	 * Número máximo de oleadas (0 = infinito).
	 * Si se alcanza, la zona se marca como completada.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave",
		meta=(ClampMin="0"))
	int32 MaxWaves = 0;

	/** Radio horizontal máximo de spawn alrededor del centro de la zona */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave",
		meta=(ClampMin="100.0"))
	float SpawnRadius = 600.0f;

	/** Tiempo entre spawns individuales dentro de una oleada (stagger) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave",
		meta=(ClampMin="0.0"))
	float SpawnStagger = 0.25f;

	/** Altura desde la que se lanza el trace de suelo para buscar punto de spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float SpawnTraceHeight = 300.0f;

	/** Sonido al comenzar una nueva oleada */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wave|SFX")
	USoundBase* WaveStartSound = nullptr;

	/** Sonido al completar todas las oleadas (solo si MaxWaves > 0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wave|SFX")
	USoundBase* AllWavesClearedSound = nullptr;

	// ── Estado ───────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Wave")
	int32 CurrentWave = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wave")
	bool bPlayerInZone = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wave")
	bool bAllWavesCleared = false;

	// ── Callback de muerte (enlazado dinámicamente a cada enemigo) ────────────

	UFUNCTION()
	void OnEnemyDied(AController* InstigatorController);

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void StartWave(int32 WaveIndex);
	void SpawnNextEnemy();
	void ResetZone();
	bool FindSpawnLocation(FVector& OutLocation) const;

	int32 EnemiesAliveCount    = 0;
	int32 EnemiesToSpawnQueue  = 0;

	UPROPERTY()
	TArray<AEnemyBase*> ActiveEnemies;

	FTimerHandle SpawnStaggerTimer;
};
