# Enemy AI — Behaviour Tree & Blackboard

---

## 1. Blackboard — `BB_EnemyDefault`

| Key                | Tipo           | Para qué                              |
|--------------------|----------------|----------------------------------------|
| `TargetActor`      | Object (Actor) | Jugador detectado                      |
| `TargetLocation`   | Vector         | Posición destino (target o patrulla)   |
| `EnemyState`       | Int            | Estado actual del enemigo              |
| `CanSeeTarget`     | Bool           | ¿Ve al jugador?                        |
| `PatrolIndex`      | Int            | Punto de patrulla actual               |
| `DistanceToTarget` | Float          | Distancia al jugador                   |
| `CanAttack`        | Bool           | Cooldown listo + hay hueco de atacante |

---

## 2. Lógica

```
¿Tiene target?
  SÍ → Chase Target (persigue con chance de pausa baja)
       → Llega al rango → Attack Target (task autónomo):
         1. Se acerca al jugador
         2. Espera turno si hay muchos atacando
         3. Elige combo por distancia (cerca=más golpes)
         4. Ejecuta combo con rotación debug alternante
         5. Termina → vuelve a Chasing

  NO → ¿Investigando?
         SÍ → Ir a última posición conocida
         NO → Patrullar
```

---

## 3. Árbol Completo

```
ROOT
│
└─ SELECTOR
   │
   ├─ [SERVICE] Update Enemy State
   │  Interval: 0.25  |  RandomDeviation: 0.05
   │
   │
   │  ╔═══════════════════════════════════════════════════╗
   │  ║  ① COMBATE                                       ║
   │  ╚═══════════════════════════════════════════════════╝
   │
   ├─ SEQUENCE "Combat"
   │  │
   │  ├─ [DECORATOR] Has Target
   │  │  Observer Aborts: Both
   │  │
   │  ├─ TASK  Chase Target                  ← persigue hasta rango
   │  │        (usa GetDistanceToTarget entre centros)
   │  │
   │  └─ TASK  Attack Target                 ← TODO en uno:
   │           (approach + wait turn +          acercarse, esperar,
   │            combo + rotación debug)         atacar, volver
   │
   │
   │  ╔═══════════════════════════════════════════════════╗
   │  ║  ② INVESTIGACIÓN                                 ║
   │  ╚═══════════════════════════════════════════════════╝
   │
   ├─ SEQUENCE "Investigate"
   │  │
   │  ├─ [DECORATOR] Check Enemy State
   │  │  StateToCheck: Investigating
   │  │  Observer Aborts: Both
   │  │
   │  └─ TASK  Investigate
   │           InvestigationPoints: 3
   │           WaitTimeAtPoint: 2.0
   │
   │
   │  ╔═══════════════════════════════════════════════════╗
   │  ║  ③ PATRULLA                                      ║
   │  ╚═══════════════════════════════════════════════════╝
   │
   └─ SEQUENCE "Patrol"
      │
      ├─ [DECORATOR] Has Target  (Inverse Condition ✓)
      │  Observer Aborts: Both
      │
      ├─ TASK  Find Patrol Point
      ├─ TASK  Move To Location
      │        AcceptanceRadius: 100
      │        bUsePatrolSpeed: true
      ├─ TASK  Idle Behavior
      │        bUseEnemyConfig: true
      └─ TASK  Wait At Patrol Point
               bUseEnemyWaitTime: true
               bLookAround: true
               bCheckForConversation: true
```

### Cómo funciona:

**Con target (rama ①):**
1. `Has Target` = true → entra en Sequence "Combat"
2. `Chase Target` persigue hasta `MaxAttackDistance` → `Succeeded`
3. `Attack Target` toma el control completo:
   - **Approach**: se acerca al 80% de MaxAttackDistance
   - **WaitTurn**: espera mirando al jugador si ya hay 3 atacantes
   - **WindUp→Strike→Recovery**: ejecuta cada golpe con rotación debug
   - **Finished**: se desregistra, estado→Chasing → la Sequence termina → vuelve a ①

**Sin target (rama ③):**
- Patrulla en loop: find → move → pause → wait → repeat

**Investigando (rama ②):**
- Va a última posición → busca alrededor → si no encuentra → patrulla

---

## 4. Attack Target — Detalle

Task autónomo con 8 fases internas:

```
WAIT COOLDOWN → APPROACH → WAIT TURN → [WindUp → Strike → Recovery → ComboGap] × N → FINISHED
```

### Fases:

| Fase         | Duración | Qué pasa                                          |
|--------------|----------|---------------------------------------------------|
| WaitCooldown | variable | Espera quieto mirando al jugador hasta que el cooldown termine |
| Approach     | variable | Se acerca al jugador (MoveToActor)                |
| WaitTurn     | max 5s   | Espera si ya hay 3+ enemigos atacando             |
| WindUp       | 0.3s     | Anticipación antes del golpe                       |
| Strike       | 0.3s     | Aplica daño + montaje + rotación ±25° + lunge 50u |
| Recovery     | 0.4s     | Pausa post-golpe                                   |
| ComboGap     | 0.4s     | Pausa entre golpes del combo                       |
| Finished     | instant  | UnregisterAttacker, estado→Chasing                 |

### Combo por distancia:

El número de golpes depende de la distancia al jugador cuando empieza el combo:

```
Distancia cerca (≈ MinAttackDistance)  → combo largo (todos los montajes)
Distancia lejos (≈ MaxAttackDistance)  → combo corto (1 golpe)
```

Se usa el array `AnimationConfig.AttackMontages` del enemigo:
- **0 montajes** → siempre 1 golpe sin animación
- **3 montajes** → cerca: 3 golpes, lejos: 1-2 golpes

### Debug visual — Rotación + Lunge:

Cada golpe produce dos efectos visuales:
1. **Rotación ±25°**: alterna dirección por golpe (golpe 1: +25°, golpe 2: -25°, etc.)
2. **Lunge 50 unidades**: el enemigo avanza hacia el jugador al golpear, luego vuelve a su posición

Esto se ve incluso sin montajes de animación — el enemigo "gira y embiste" cada golpe.
Se restaura todo después de cada Strike.

### Parámetros del Task:

| Propiedad           | Default | Descripción                        |
|---------------------|---------|------------------------------------|
| WindUpDuration      | 0.3     | Anticipación antes del golpe       |
| StrikeDuration      | 0.3     | Duración del golpe                 |
| RecoveryDuration    | 0.4     | Pausa post-golpe                   |
| ComboGapDuration    | 0.4     | Pausa entre golpes                 |
| DamageVariance      | 0.15    | ±15% varianza de daño              |
| MaxWaitTurnTime     | 5.0     | Timeout esperando turno            |
| DebugRotationDegrees| 25.0    | Grados de rotación debug por golpe |
| DebugLungeDistance  | 50.0    | Unidades que avanza al golpear     |

---

## 5. Montaje en el Editor

### 5.1 Assets
1. Content Browser → AI → **Blackboard** → `BB_EnemyDefault` → crear las 7 keys
2. Content Browser → AI → **Behavior Tree** → `BT_EnemyDefault`

### 5.2 Montar

**Root → Selector:**
1. Añadir **Selector**
2. Service: **Update Enemy State**

**① Combat (1er hijo):**
1. **Sequence**
2. Decorator: **Has Target** → Aborts `Both`
3. Task 1: **Chase Target**
4. Task 2: **Attack Target**

**② Investigate (2do hijo):**
1. **Sequence**
2. Decorator: **Check Enemy State** → `Investigating` → Aborts `Both`
3. Task: **Investigate**

**③ Patrol (3er hijo):**
1. **Sequence**
2. Decorator: **Has Target** → **Inverse ✓** → Aborts `Both`
3. Tasks: **Find Patrol Point** → **Move To Location** → **Idle Behavior** → **Wait At Patrol Point**

### 5.3 Asignar al Enemigo
- `BehaviorTree` → `BT_EnemyDefault`
- `PatrolPath` → actor PatrolPath del nivel
- `AnimationConfig.AttackMontages` → **añadir 2-3 montajes** (define el combo)

---

## 6. Config del Enemigo

### CombatConfig
| Propiedad               | Default | Qué controla                  |
|-------------------------|---------|-------------------------------|
| MinAttackDistance        | 100     | Distancia cercana (más combo) |
| MaxAttackDistance        | 150     | Distancia máxima para atacar  |
| BaseDamage              | 10      | Daño base por golpe           |
| AttackCooldown          | 1.5     | Segundos entre combos         |
| MaxSimultaneousAttackers| 3       | Cuántos atacan a la vez       |

### PatrolConfig
| Propiedad             | Default |
|-----------------------|---------|
| PatrolSpeedMultiplier | 0.25    |
| ChaseSpeedMultiplier  | 0.5     |

### PerceptionConfig
| Propiedad      | Default |
|----------------|---------|
| SightRadius    | 2000    |
| LoseSightTime  | 5.0     |
| HearingRadius  | 1000    |

### Velocidad Base
| BaseMaxWalkSpeed | 350 | Velocidad máxima absoluta |
Patrol real = 350 × 0.25 = **87.5**
Chase real = 350 × 0.5 = **175**

---

## 7. Piezas C++

### Tasks usadas en el BT
| Clase                       | Nombre en Editor     | Qué hace                                     |
|-----------------------------|----------------------|-----------------------------------------------|
| `UBTTask_ChaseTarget`       | Chase Target         | Persigue hasta MaxAttackDistance               |
| `UBTTask_AttackTarget`      | Attack Target        | Approach + wait turn + combo + debug rotation |
| `UBTTask_FindPatrolPoint`   | Find Patrol Point    | Siguiente punto de patrulla                   |
| `UBTTask_MoveToLocation`    | Move To Location     | Moverse al TargetLocation del BB              |
| `UBTTask_IdleBehavior`      | Idle Behavior        | Pausa aleatoria entre puntos                  |
| `UBTTask_WaitAtPatrolPoint` | Wait At Patrol Point | Esperar + mirar alrededor                     |
| `UBTTask_Investigate`       | Investigate          | Buscar en última posición conocida            |


### Service
| `UBTService_UpdateEnemyState` | Update Enemy State | Selector raíz |

### Decorators
| Tipo        | Nombre            | Dónde va               | Config                          |
|-------------|-------------------|------------------------|---------------------------------|
| C++ custom  | Has Target        | Sequence "Combat"      | Aborts: Both                    |
| C++ custom  | Has Target (inv)  | Sequence "Patrol"      | Inverse ✓, Aborts: Both         |
| C++ custom  | Check Enemy State | Sequence "Investigate" | State: Investigating, Aborts: Both |
