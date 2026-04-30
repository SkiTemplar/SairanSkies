// SairanSkies - Ultimate Laser Attack Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UltimateComponent.generated.h"

class USoundBase;
class UNiagaraSystem;
class UNiagaraComponent;
class ASairanCharacter;

/**
 * Gestiona la barra de XP ultimate y el ataque láser.
 *
 * Flujo:
 *  1. El jugador mata enemigos → AEnemyBase::Die() llama AddXP().
 *  2. Cuando la barra está llena, el jugador pulsa el botón Ultimate.
 *  3. El personaje queda paralizado (puede rotar con la cámara) 5 s.
 *  4. Cada DamageInterval segundos se dispara un Line Trace desde el ojo
 *     aplicando DamageMin–DamageMax a lo que toca.
 *  5. Al terminar, se restaura el movimiento y la barra se vacía.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIRANSKIES_API UUltimateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUltimateComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ── Config ──────────────────────────────────────────────────────────────

	/** XP total necesaria para llenar la barra */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="1.0"))
	float MaxXP = 100.0f;

	/** XP otorgada por cada kill de enemigo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="1.0"))
	float XPPerKill = 25.0f;   // 4 kills = barra llena

	/** Duración del láser en segundos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="1.0", ClampMax="30.0"))
	float LaserDuration = 5.0f;

	/** Intervalo entre ticks de daño — 0.33 = 3 impactos/segundo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="0.05", ClampMax="1.0"))
	float DamageInterval = 0.33f;

	/** Daño mínimo por tick (~3 ataques pesados/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="0.0"))
	float DamageMin = 35.0f;

	/** Daño máximo por tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="0.0"))
	float DamageMax = 55.0f;

	/** Alcance del láser en cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate",
		meta=(ClampMin="100.0"))
	float LaserRange = 3000.0f;

	// ── SFX ─────────────────────────────────────────────────────────────────

	/** Sonido al activar el ultimate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|SFX")
	USoundBase* ActivateSound = nullptr;

	/** Sonido de loop del láser */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|SFX")
	USoundBase* LaserLoopSound = nullptr;

	/** Sonido al finalizar */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|SFX")
	USoundBase* DeactivateSound = nullptr;

	// ── VFX ─────────────────────────────────────────────────────────────────

	/** Niagara que se lanza en el punto de impacto del láser */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|VFX")
	UNiagaraSystem* LaserImpactVFX = nullptr;

	/** Niagara beam que va desde el personaje hasta el punto de impacto.
	 *  Debe ser un sistema de tipo Ribbon/Beam con parámetros de usuario
	 *  "BeamStart" y "BeamEnd" de tipo Vector. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|VFX")
	UNiagaraSystem* LaserBeamVFX = nullptr;

	/** Nombre del parámetro de usuario Niagara para el origen del rayo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|VFX")
	FName LaserBeamStartParam = FName("BeamStart");

	/** Nombre del parámetro de usuario Niagara para el destino del rayo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ultimate|VFX")
	FName LaserBeamEndParam = FName("BeamEnd");

	// ── Estado (solo lectura) ────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Ultimate")
	float CurrentXP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ultimate")
	bool bLaserActive = false;

	// ── Interfaz ────────────────────────────────────────────────────────────

	/** Añade XP (llamar cuando muere un enemigo) */
	UFUNCTION(BlueprintCallable, Category = "Ultimate")
	void AddXP(float Amount);

	/** Intenta activar el ultimate (requiere barra llena) */
	UFUNCTION(BlueprintCallable, Category = "Ultimate")
	void TryActivate();

	/** Porcentaje de XP [0..1] para la HUD */
	UFUNCTION(BlueprintPure, Category = "Ultimate")
	float GetXPPercent() const
	{
		return MaxXP > 0.0f ? FMath::Clamp(CurrentXP / MaxXP, 0.0f, 1.0f) : 0.0f;
	}

	/** Verdadero cuando la barra está llena y no hay láser activo */
	UFUNCTION(BlueprintPure, Category = "Ultimate")
	bool IsReady() const { return CurrentXP >= MaxXP && !bLaserActive; }

private:
	float LaserTimer  = 0.0f;
	float DamageTimer = 0.0f;
	float StoredMaxWalkSpeed = 0.0f;

	/** Componente Niagara activo durante el láser para el rayo continuo */
	UPROPERTY()
	class UNiagaraComponent* LaserBeamComponent = nullptr;

	void FireLaserTick();
	void Deactivate();
	void UpdateLaserBeam(const FVector& Origin, const FVector& End);
};
