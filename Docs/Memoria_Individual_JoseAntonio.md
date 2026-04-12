<style>
body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; max-width: 950px; margin: 0 auto; padding: 40px 20px; color: #2c3e50; line-height: 1.7; }
h1 { color: #1a1a2e; border-bottom: 3px solid #e94560; padding-bottom: 12px; font-size: 2em; }
h2 { color: #16213e; border-left: 4px solid #e94560; padding-left: 12px; margin-top: 40px; }
h3 { color: #0f3460; margin-top: 24px; }
h4 { color: #533483; }
table { width: 100%; border-collapse: collapse; margin: 20px 0; }
th, td { border: 1px solid #ddd; padding: 10px 14px; text-align: left; }
th { background-color: #1a1a2e; color: white; }
tr:nth-child(even) { background-color: #f8f9fa; }
code { background: #f0f0f0; padding: 2px 6px; border-radius: 3px; font-size: 0.9em; }
pre { background: #1e1e2e; color: #cdd6f4; padding: 16px; border-radius: 8px; overflow-x: auto; font-size: 0.85em; line-height: 1.5; }
pre code { background: none; color: inherit; padding: 0; }
.highlight { background: #fff3cd; padding: 12px 16px; border-left: 4px solid #ffc107; margin: 16px 0; border-radius: 4px; }
.info { background: #d1ecf1; padding: 12px 16px; border-left: 4px solid #17a2b8; margin: 16px 0; border-radius: 4px; }
.section-box { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 8px; padding: 16px 20px; margin: 16px 0; }
.component-tag { display: inline-block; background: #e94560; color: white; padding: 2px 8px; border-radius: 4px; font-size: 0.8em; margin-right: 4px; }
.file-tag { display: inline-block; background: #0f3460; color: white; padding: 2px 8px; border-radius: 4px; font-size: 0.8em; }
</style>

# Sairan Skies — Memoria Individual

**Autor:** José Antonio Mota Lucas  
**Motor:** Unreal Engine 5.6 | **Lenguaje:** C++  
**Fecha:** Febrero 2026

---

## 1. Resumen de Responsabilidades

Mi parte del proyecto abarca la totalidad del **personaje jugable**, su **sistema de combate**, todas sus **habilidades**, las **armas**, la **interfaz de usuario** y sistemas de soporte como la **zona de muerte** y los **números de daño flotantes** sobre los enemigos.

### Archivos desarrollados

| Carpeta | Archivos |
|---|---|
| `Character/` | `SairanCharacter.h/.cpp`, `CloneComponent.h/.cpp`, `CheckpointComponent.h/.cpp` |
| `Combat/` | `CombatComponent.h/.cpp`, `GrappleComponent.h/.cpp`, `TargetingComponent.h/.cpp` |
| `Weapons/` | `WeaponBase.h/.cpp`, `Greatsword.h/.cpp`, `GrappleHookActor.h/.cpp`, `WeaponLerpComponent.h/.cpp` |
| `UI/` | `PlayerHUDWidget.h/.cpp`, `DamageNumberWidget.h/.cpp`, `EnemyHealthBarWidget.h/.cpp`, `GrappleCrosshairWidget.h/.cpp` |
| `Animation/` | `AN_ComboWindow.h/.cpp`, `AN_EnableHitDetection.h/.cpp` |
| `Core/` | `DeathZone.h/.cpp` |
| `Enemies/` | `DamageNumberComponent.h/.cpp` |
| `AI/Tasks/` | `BTTask_CircleTarget.h/.cpp` |

---

## 2. Personaje Principal — `ASairanCharacter`

<span class="file-tag">SairanCharacter.h</span> <span class="file-tag">SairanCharacter.cpp</span>

El personaje es la clase central del proyecto. Hereda de `ACharacter` y utiliza el **Enhanced Input System** de UE5 para soportar tanto teclado/ratón como mando de forma nativa.

### 2.1 Arquitectura por Componentes

La decisión de diseño principal fue **no sobrecargar la clase del personaje**. En su lugar, cada sistema mayor vive en su propio `UActorComponent`:

```
ASairanCharacter
├── UCombatComponent        → Combate (ataques, parry, bloqueo)
├── UTargetingComponent     → Auto-targeting estilo Arkham
├── UGrappleComponent       → Sistema de gancho
├── UCloneComponent         → Clon / Teletransporte
├── UCheckpointComponent    → Guardado de posición segura
└── UWeaponLerpComponent    → Interpolación de la espada en combos
```

Esta separación permite:
- **Modularidad**: cada componente puede desarrollarse y testearse de forma aislada.
- **Serialización independiente**: cada componente expone sus propias variables editables en el Editor.
- **Bajo acoplamiento**: los componentes se comunican a través del personaje, pero no dependen entre sí directamente.

### 2.2 Movimiento

El movimiento está diseñado para sentirse ágil y responsivo:

| Acción | Implementación |
|---|---|
| **Andar** | `MaxWalkSpeed = 400` — orientación al movimiento activada |
| **Correr** | `MaxWalkSpeed = 800` — cámara se aleja suavemente. En mando: toggle con L3, se cancela al reducir input del stick |
| **Doble salto** | Contador `CurrentJumpCount` con máximo configurable. El segundo salto resetea la velocidad vertical para consistencia |
| **Dash** | `LaunchCharacter` en la dirección del input (8 direcciones). Solo en suelo. Otorga **i-frames** (invulnerabilidad) |
| **Gravedad variable** | `FallingGravityScale = 2.5` cuando se cae, `NormalGravityScale = 1.5` al subir. Evita sensación de "luna" |

#### Código clave — Sprint Toggle (Mando)

```cpp
void ASairanCharacter::SprintStart(const FInputActionValue& Value)
{
    bSprintToggleActive = true;
    StartSprint();
}

// En Move(): si el stick baja del 80%, se cancela el toggle
if (bSprintToggleActive)
{
    float InputMagnitude = MovementInput.Size();
    if (InputMagnitude < SprintCancelThreshold)
    {
        bSprintToggleActive = false;
        StopSprint();
    }
}
```

Este sistema permite que en mando, pulsar L3 active el sprint hasta que el jugador reduzca el stick, mientras que en PC funciona con mantener o pulsar Shift normalmente, según lo que se prefiera.

### 2.3 Cámara

La cámara usa un `USpringArmComponent` con:
- **Lag suave** (`CameraLagSpeed = 10`) para sensación cinematográfica.
- **Colisión con geometría del mundo** pero **no con actores** (evita que enemigos empujen la cámara hacia el jugador).
- **Zoom dinámico**: se aleja al correr, se acerca al apuntar con el gancho, se aleja en el dash.

### 2.4 Puntos de Anclaje del Arma

Se crearon **componentes vacíos** (`USceneComponent`) como puntos de referencia editables desde el Editor para posicionar el arma:

| Punto | Uso |
|---|---|
| `WeaponHandAttachPoint` | Arma en la mano (combate) |
| `WeaponBackAttachPoint` | Arma en la espalda diagonal (enfundada) |
| `WeaponBlockAttachPoint` | Arma en posición de bloqueo/parry |
| `GrappleHandAttachPoint` | Gancho en la mano izquierda |
| `WeaponIdlePoint` | Posición de reposo del arma |
| `LightAttackPoint1-5` | 5 posiciones del combo de ataque ligero |
| `HeavyAttackPoint1-2` | 2 posiciones del ataque pesado (arriba → abajo) |

Esta aproximación reemplaza vectores hardcodeados, permitiendo ajustar posiciones visualmente en el Editor.

### 2.5 Vida y Muerte

El personaje tiene un sistema de salud con `TakeDamage` sobrescrito que enruta el daño a través del `CombatComponent` para evaluar parry/bloqueo antes de aplicarlo. Al morir, se desactiva el input durante `RespawnDelay` segundos y se respawnea con vida completa en el último checkpoint.

---

## 3. Sistema de Combate — `UCombatComponent`

<span class="component-tag">COMPONENTE</span> <span class="file-tag">CombatComponent.h</span>

### 3.1 Tipos de Ataque

| Tipo | Descripción | Daño base |
|---|---|---|
| **Ligero** | Combo de hasta 5 golpes encadenados. Si se completa, entra en cooldown de recuperación | 20 |
| **Pesado** | Golpe único potente con tap rápido | 40 |
| **Cargado** | Mantener R2/click derecho. A los 2s alcanza el daño máximo y se mantiene hasta soltar | 80 |

Cada golpe tiene una **varianza aleatoria** de ±2 puntos (`DamageVariance`), para feedback más orgánico.

#### Código clave — Varianza de Daño

```cpp
float UCombatComponent::ApplyDamageVariance(float BaseDamage) const
{
    float Variation = FMath::RandRange(-DamageVariance, DamageVariance);
    return FMath::Max(1.0f, BaseDamage + Variation);
}
```

### 3.2 Sistema de Combo

El combo ligero soporta hasta `MaxLightCombo = 4` golpes encadenados con buffer de input. Si se agotan todos los golpes, entra en un estado de `bComboExhausted` durante `ComboRecoveryCooldown` segundos donde no se puede atacar.

El combo se resetea si pasan `ComboResetTime` segundos sin atacar.

### 3.3 Parry y Bloqueo (estilo Sekiro)

Al pulsar L1/Q, se abre una **ventana de parry** de `ParryWindowDuration = 0.3s`. Durante esta ventana:

- **Parry perfecto**: deflecta el 100% del daño. Activa **slow-motion** estilo God of War:
  1. Hitstop de `0.08s` (congelación total).
  2. Slow-mo de `0.5s` al 30% de velocidad.
- **Bloqueo normal**: si la ventana de parry ha pasado pero se sigue manteniendo el botón, se reduce parte del daño.

El parry solo funciona en un **ángulo de 180° frontal**. Ataques por la espalda siempre conectan.

#### Código clave — HandleIncomingDamage

```cpp
bool UCombatComponent::HandleIncomingDamage(float IncomingDamage, AActor* Attacker, float& OutDamageApplied)
{
    // Parry perfecto: ventana activa
    if (bIsInParryWindow)
    {
        OutDamageApplied = 0.0f;
        PlayParryFeedback(true);
        TriggerHitstop(EAttackType::Light);   // Freeze frame
        StartPerfectParrySlowMo();            // → Slow-mo
        return true;
    }
    // Bloqueo normal: reduce daño parcialmente
    if (bIsHoldingBlock)
    {
        OutDamageApplied = IncomingDamage * BlockDamageReduction;
        PlayParryFeedback(false);
        return false;
    }
    OutDamageApplied = IncomingDamage;
    return false;
}
```

### 3.4 Hit Detection

La detección de golpes utiliza un **sphere trace** delante del personaje con:
- `HitDetectionRadius = 80` unidades
- `HitDetectionForwardOffset = 100` unidades
- `HitDetectionHeightOffset = 50` unidades (evita tocar el suelo)
  
Esta diseñado mayoritariamente para la demo, ya que en versión final se adaptara al modelo del arma en vez de ser una simple esfera.

### 3.5 Feedback de Impacto

Cada golpe exitoso produce:

| Efecto | Detalle |
|---|---|
| **Hitstop** | Pausa de `0.05s` — congela brevemente el juego |
| **Camera Shake** | Sacudida configurable con `TSubclassOf<UCameraShakeBase>` |
| **Knockback** | Empuja al enemigo hacia atrás (`KnockbackForce = 500`) |
| **VFX de impacto** | Niagara particles en el punto de golpe |
| **SFX** | Sonido de impacto serializado |
| **Hit Flash en enemigo** | El enemigo parpadea blanco completo (`HitFlashDuration = 0.1s`) estilo Blasphemous |
| **Números de daño** | Se muestran flotando sobre el enemigo con color según vida restante |

### 3.6 Trail de Espada (Lies of P)

El arma tiene un sistema de trail dual:

1. **Trail normal** (`NormalSwingTrailFX`): se activa al inicio de cada swing.
2. **Trail de sangre** (`BloodSwingTrailFX`): se añade *encima* del trail normal cuando la espada conecta con un enemigo.

Ambos trails coexisten hasta que sus partículas expiran naturalmente. No se eliminan al cambiar de trail.

```cpp
void AWeaponBase::SwitchToBloodTrail()
{
    if (bBloodTrailSpawnedThisSwing || !BloodSwingTrailFX) return;
    bBloodTrailSpawnedThisSwing = true;
    // Se añade SIN eliminar el trail normal
    UNiagaraComponent* BloodComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
        BloodSwingTrailFX, WeaponMesh, NAME_None, ...);
    ActiveTrailComponents.Add(BloodComp);
}
```

Actualmente no es posible ver el uso en la demo debido a la falta del modelo final del arma al que aplicarle los trails, pero es un avance de cara a uso en el proyecto completo.

---

## 4. Targeting — `UTargetingComponent`

<span class="component-tag">COMPONENTE</span> <span class="file-tag">TargetingComponent.h</span>

Sistema de auto-targeting inspirado en **Batman Arkham**:

1. Al pulsar ataque, busca enemigos en un radio de `1000` unidades (10m).
2. Puntúa cada enemigo por **distancia** (40%) y **dirección** respecto al input del jugador (60%).
3. Si hay un enemigo válido y visible (line of sight), el personaje hace **snap** instantáneo (`SnapDuration = 0.15s`) hasta quedar a `SnapStopDistance = 150` unidades.
4. Tras el snap, ejecuta el ataque inmediatamente.

```
Puntuación = (1 - distNorm) × (1 - DirectionWeight) + dotProduct × DirectionWeight
```

Esto garantiza que el combate fluya sin necesidad de apuntar manualmente.

---

## 5. Sistema de Gancho — `UGrappleComponent`

<span class="component-tag">COMPONENTE</span> <span class="file-tag">GrappleComponent.h</span>

### 5.1 Flujo de Uso

```
[Idle] → Pulsar L2/F → [Aiming] → Soltar → [Firing] → [Pulling] → Pasa el punto medio → [Releasing]
```

### 5.2 Estados

| Estado | Descripción |
|---|---|
| `Idle` | Sin gancho activo |
| `Aiming` | Apuntando: la cámara se acerca al hombro, aparece crosshair, el personaje rota con la cámara |
| `Firing` | El gancho ha sido disparado |
| `Pulling` | El jugador es arrastrado hacia el objetivo. Gravedad desactivada. Cámara bloqueada. Trail de partículas activo |
| `Releasing` | Ha pasado el punto medio vertical del objetivo. Impulso se para. Caída libre |

### 5.3 Aim Assist

El gancho implementa aim-assist con **snap suave**:
- Busca actores con tag `Grapple` dentro de un radio de pantalla (`AimAssistScreenRadius = 250px`).
- Si hay un objetivo válido, el crosshair se snapea suavemente al centro del componente con tag.
- Si sale del área, el dessnap también es suave (no brusco).
- Solo se puede enganchar a objetos con el tag `Grapple` dentro de `MaxGrappleRange = 3000` unidades (30m).

### 5.4 Mecánica de Impulso

- Al soltar el trigger, la velocidad Y del personaje se resetea a 0 para normalizar todos los enganches.
- Durante el pull: sin gravedad, cámara bloqueada, trail de partículas continuo.
- Al soltar: **dampening** progresivo del 80% de la velocidad horizontal durante 1 segundo.
- Tras cada gancho: se reinicia el contador de doble salto (solo 1 salto extra).

### 5.5 Crosshair Widget

<span class="file-tag">GrappleCrosshairWidget.h</span>

Widget UMG con dos imágenes superpuestas:
- **Rojo**: visible cuando no hay objetivo válido.
- **Verde**: visible cuando hay un punto enganchable dentro del rango.

---

## 6. Clon y Teletransporte — `UCloneComponent`

<span class="component-tag">COMPONENTE</span> <span class="file-tag">CloneComponent.h</span>

Inspirado en **It Takes Two**:

1. **Pulsar R/Y**: planta un clon translúcido (`CloneOpacity = 0.4`) en tu posición actual. El personaje hace un pequeño dash hacia atrás.
2. **Pulsar de nuevo**: si estás dentro del rango, te teletransportas al clon.

### Reglas

- Solo se puede plantar estando **en el suelo** y con espacio sobre la cabeza (`MinCeilingClearance = 200`).
- Distancia máxima para TP:
  - **En combate** (arma desenfundada): `2000` unidades (20m)
  - **Fuera de combate**: `5000` unidades (50m)
- Si te sales de la distancia máxima, el clon se destruye automáticamente.
- Cada activación tiene VFX y SFX independientes para plantar y para teletransportarse.
- Tras teleportarse se suprime el sonido de aterrizaje.

---

## 7. Arma — `AWeaponBase` + `AGreatsword`

<span class="file-tag">WeaponBase.h</span> <span class="file-tag">Greatsword.h</span>

### 7.1 Estructura

- `UStaticMeshComponent` para el mesh del espadón.
- `UBoxComponent` (`HitCollision`) para detección de colisión del golpe.
- Sistema de trail dual (Lies of P).
- Tres posiciones: mano, espalda (diagonal estilo God of War), bloqueo.

### 7.2 Weapon Lerp — `UWeaponLerpComponent`

<span class="component-tag">COMPONENTE</span> <span class="file-tag">WeaponLerpComponent.h</span>

Controla la interpolación del arma entre puntos durante los combos:

| Estado | Comportamiento |
|---|---|
| `Idle` | Arma en posición de reposo |
| `LightAttacking` | Lerp entre los 5 puntos de combo ligero (velocidad `12.0`) |
| `HeavyAttacking` | Snap rápido: punto 1 (arriba) → punto 2 (abajo) (velocidad `18.0`) |
| `HeavyCharging` | Lerp lento hasta punto 1 durante carga (velocidad `1.5`) |
| `ReturningToIdle` | Retorno suave tras `2s` de inactividad (velocidad `5.0`) |

Los puntos son `USceneComponent` hijos del personaje, editables visualmente en el Editor.
Añadido como sustitutivo previo a las animaciones de cara al proyecto final.

---

## 8. Checkpoint y Zona de Muerte

### 8.1 CheckpointComponent

<span class="component-tag">COMPONENTE</span> <span class="file-tag">CheckpointComponent.h</span>

Guarda automáticamente la última posición segura del jugador cada `0.25s` cuando está tocando el suelo y se ha movido al menos `50` unidades desde el último guardado.

### 8.2 DeathZone

<span class="file-tag">DeathZone.h</span>

Actor con `UBoxComponent` como trigger. Al solapar con el jugador, invoca `CheckpointComponent::RespawnAtLastCheckpoint()` con VFX y SFX de respawn.

El mismo sistema de respawn se usa cuando el jugador muere por daño:
- Se desactiva el input durante `RespawnDelay = 1.5s`.
- Se restaura la vida al máximo.
- Se teletransporta al último checkpoint.

---

## 9. Interfaz de Usuario (UI)

Toda la UI está implementada con **UMG (Unreal Motion Graphics)** usando clases C++ base que se extienden con Widget Blueprints en el Editor.

### 9.1 PlayerHUDWidget

<span class="file-tag">PlayerHUDWidget.h</span>

Barra de vida del jugador. Se actualiza automáticamente en cada `TakeDamage` y al respawnear.

### 9.2 EnemyHealthBarWidget

<span class="file-tag">EnemyHealthBarWidget.h</span>

Barra de vida flotante sobre cada enemigo. Se orienta siempre hacia la cámara del jugador. El color del `ProgressBar` cambia según el porcentaje de vida.

### 9.3 DamageNumberComponent + DamageNumberWidget

<span class="file-tag">DamageNumberComponent.h</span> <span class="file-tag">DamageNumberWidget.h</span>

Sistema de números de daño flotantes estilo juegos de acción:

- Cada golpe genera un widget individual en el **punto de impacto**.
- Los números flotan hacia arriba y se desvanecen en `1s`.
- **Color por salud restante** con tres tramos configurables:
  - Verde (>60% HP) → Naranja (25-60%) → Rojo (<25%)
  - Transición gradiente entre tramos.
- Al morir el enemigo aparece una **X** roja.
- El tamaño del texto es serializable (`TextFontSize`).
- La fuente personalizada se configura directamente en el Widget Blueprint del Editor (TextBlock con font).

### 9.4 GrappleCrosshairWidget

<span class="file-tag">GrappleCrosshairWidget.h</span>

Crosshair dual (rojo/verde) que se mueve suavemente por la pantalla siguiendo el objetivo del gancho. Snap suave al entrar/salir de objetos con tag `Grapple`.

---

## 10. Animation Notifies

### AN_ComboWindow

<span class="file-tag">AN_ComboWindow.h</span>

`AnimNotifyState` que marca la ventana temporal durante una animación de ataque en la que se acepta input para encadenar el siguiente golpe del combo. Permite buffer de input anticipado.

### AN_EnableHitDetection

<span class="file-tag">AN_EnableHitDetection.h</span>

`AnimNotifyState` que activa/desactiva la detección de colisiones del arma durante la porción activa de la animación de ataque. Esto garantiza que los golpes solo registren hits durante el tramo correcto del swing.

Actualmente no se consigue un uso completo hasta que no se añadan animaciones y montages de los diversos combos.

---

## 11. BTTask_CircleTarget

<span class="file-tag">BTTask_CircleTarget.h</span>

Tarea de Behavior Tree para el comportamiento de flanqueo de enemigos. Implementa:

- **Movimiento circular** alrededor del jugador con velocidad configurable.
- **Feints (amagues)**: lunges aleatorios hacia el jugador y retirada, para que el jugador no pueda predecir quién va a atacar.
- **Sway lateral**: oscilación de peso estilo boxeador mientras esperan.
- **Reposicionamiento periódico** cada 2.5s ± variación.
- **Cara al jugador**: los enemigos siempre miran al jugador, nunca le dan la espalda.

> **Nota**: Esta tarea fue posteriormente sustituida por `BTTask_OuterCircleBehavior` como parte de la evolución del sistema de dos círculos, pero se mantiene por compatibilidad, será completamente deprecada para el proyecto final.

---

## 12. Decisiones de Diseño

### ¿Por qué componentes y no herencia?

Unreal Engine favorece la composición sobre la herencia profunda. Un único `ASairanCharacter` con componentes es más mantenible que una cadena de herencia `ABaseCharacter → ACombatCharacter → AGrappleCharacter...`.

### ¿Por qué no GAS (Gameplay Ability System)?

Al ser un prototipo/demo, GAS añade complejidad significativa (Ability System Component, Gameplay Effects, Gameplay Tags) sin beneficio proporcional. El sistema custom con componentes es más directo, más fácil de iterar y suficiente para validar las mecánicas.

### ¿Por qué UMG para números de daño?

Inicialmente se exploraron `TextRenderComponent` y fuentes 3D, pero UMG ofrece soporte nativo y robusto para fuentes personalizadas, escalado y fade, sin los problemas de renderizado que tienen los componentes de texto 3D en UE5.

### ¿Por qué World Subsystem para el GroupCombatManager?

Un `UWorldSubsystem` es el patrón correcto para un sistema global que coordina múltiples actores (enemigos) sin pertenecer a ninguno de ellos. Se crea y destruye automáticamente con el mundo, sin necesidad de instanciarlo manualmente.

---

## 13. Variables Serializables (Principales)

Todas las variables relevantes están expuestas como `UPROPERTY(EditDefaultsOnly)` o `UPROPERTY(EditAnywhere)`, permitiendo ajustar el gameplay desde el Editor sin recompilar:

| Sistema | Variables clave |
|---|---|
| Movimiento | `WalkSpeed`, `RunSpeed`, `DashDistance`, `DashDuration`, `MaxJumps`, `FallingGravityScale` |
| Cámara | `DefaultCameraDistance`, `RunningCameraDistance`, `CameraZoomSpeed` |
| Combate | `LightAttackDamage`, `HeavyAttackDamage`, `ChargedAttackDamage`, `DamageVariance`, `ChargeTimeForMaxDamage` |
| Parry | `ParryWindowDuration`, `PerfectParryHitstopDuration`, `PerfectParrySlowMoDuration`, `PerfectParrySlowMoTimeDilation` |
| Gancho | `MaxGrappleRange`, `GrapplePullSpeed`, `GrappleAngleOffset`, `DampeningDuration`, `DampeningFactor` |
| Clon/TP | `MaxTeleportDistanceCombat`, `MaxTeleportDistanceExplore`, `PushBackForce`, `CloneOpacity` |
| Hit Feedback | `HitstopDuration`, `KnockbackForce`, `CameraShakeIntensity` |
| Números daño | `NumberLifetime`, `FloatUpSpeed`, `TextFontSize`, `HighToMidThreshold`, `MidToLowThreshold` |

---

## 14. SFX y VFX Serializados

Todos los efectos de sonido y partículas están expuestos como propiedades editables. El código no contiene referencias hardcodeadas a assets, facilitando la iteración por parte del equipo de arte:

| Categoría | Propiedades |
|---|---|
| **Movimiento** | `DashSound/VFX`, `JumpSound/VFX`, `DoubleJumpSound/VFX`, `LandSound/VFX`, `WalkFootstepSound`, `RunFootstepSound` |
| **Arma** | `DrawWeaponSound`, `SheathWeaponSound`, `NormalSwingTrailFX`, `BloodSwingTrailFX` |
| **Combate** | `LightAttackSwingSound/VFX`, `HeavyAttackSwingSound/VFX`, `ChargeStartSound`, `ChargeHoldLoopSound`, `ChargedAttackReleaseSound/VFX`, `HitSound`, `HitParticleSystem` |
| **Parry** | `ParryDeflectSound/VFX`, `BlockSound/VFX` |
| **Gancho** | `AimingSound`, `FireSound`, `PullingSound`, `ReleaseSound`, `GrappleTrailParticles` |
| **Clon** | `ClonePlaceSound/VFX`, `TeleportSound/VFX` |
| **Checkpoint** | `RespawnSound/VFX` |

---

## 15. Conclusión

El personaje jugable de **Sairan Skies** implementa un sistema de combate completo y fluido inspirado en títulos AAA, con todas las mecánicas fundamentales implementadas en C++, para más adelante dejar otras funciones secundarias a nuestros compañero de DIPI por Blueprints. La arquitectura modular por componentes permite que cada sistema se desarrolle, ajuste y pruebe de forma independiente, facilitando la iteración rápida que requiere una demo de preproducción.


