# Enemy AI — Behaviour Tree & Blackboard (Sistema de Dos Círculos)

> **Última actualización:** Febrero 2026  
> **Sistema:** Modelo de Dos Círculos (Outer Circle + Inner Circle)

---

## 1. Concepto — Dos Círculos

```
                    ┌──────────────────────────────────────┐
                    │         OUTER CIRCLE (~5m)           │
                    │                                      │
                    │    🧟 taunt    🧟 sway              │
                    │         ┌──────────────┐             │
                    │         │ INNER CIRCLE │             │
                    │    🧟   │   (ataque)   │   🧟       │
                    │  feint  │   🧟→⚔️🧑    │  esperando │
                    │         │              │             │
                    │         └──────────────┘             │
                    │    🧟 reposicionándose               │
                    │                                      │
                    └──────────────────────────────────────┘
```

| Zona           | Radio         | Qué hacen                                          |
|----------------|---------------|-----------------------------------------------------|
| **Fuera**      | > 500 cm      | Persiguen al jugador (Chase)                        |
| **Outer Circle** | ~420–580 cm | Esperan, tauntean, fingen avanzar, sway lateral    |
| **Inner Circle** | 100–200 cm  | Atacan (máx 2 simultáneos). Combo por distancia    |

---

## 2. Blackboard — `BB_EnemyDefault`

| Key                | Tipo           | Para qué                                        |
|--------------------|----------------|--------------------------------------------------|
| `TargetActor`      | Object (Actor) | Jugador detectado                                |
| `TargetLocation`   | Vector         | Posición destino (target o patrulla)             |
| `EnemyState`       | Int            | Estado actual (Idle/Patrol/Chase/Outer/Inner/Attack...) |
| `CanSeeTarget`     | Bool           | ¿Ve al jugador?                                  |
| `PatrolIndex`      | Int            | Punto de patrulla actual                         |
| `DistanceToTarget` | Float          | Distancia al jugador (centro a centro)           |
| `CanAttack`        | Bool           | Está en inner circle + cooldown listo            |

---

## 3. Estados del Enemigo (`EEnemyState`)

```
Idle → Patrolling → [detecta jugador] → Chasing → OuterCircle → InnerCircle → Attacking
                                              ↑                      │              │
                                              │                      ↓              ↓
                                              │                 OuterCircle ← (probabilidad)
                                              │                      │
                                              └──────────────────────┘ (jugador se aleja)
```

| Estado         | Valor | Descripción                                         |
|----------------|-------|-----------------------------------------------------|
| `Idle`         | 0     | Sin hacer nada                                      |
| `Patrolling`   | 1     | Siguiendo PatrolPath                                |
| `Investigating`| 2     | Yendo a última posición conocida                    |
| `Chasing`      | 3     | Persiguiendo al jugador hasta outer circle          |
| `OuterCircle`  | 4     | En el círculo exterior, taunting/feints             |
| `InnerCircle`  | 5     | Dentro del círculo interior, preparado para atacar  |
| `Attacking`    | 6     | Ejecutando combo de ataque                          |
| `Conversing`   | 7     | Conversando con otro enemigo                        |
| `Dead`         | 8     | Muerto                                              |

---

## 4. Flujo Completo

```
1. Enemigo patrullando...
2. Detecta al jugador (vista/oído/alerta de aliado)
3. CHASE: persigue hasta llegar al outer circle (~5m)
4. OUTER CIRCLE:
   - Se posiciona en el anillo exterior con golden-angle
   - Hace sway lateral (se balancea)
   - Hace taunts (amaga avanzar hacia el jugador y retrocede)
   - Cada 0.5s comprueba si hay hueco en el inner circle
   - Si el jugador se mueve, reacciona con delay aleatorio (0.4–1.5s)
5. INNER CIRCLE (cuando GroupCombatManager le da permiso):
   - Se acerca a distancia aleatoria (100–200 cm del jugador)
   - Elige combo probabilístico según distancia
   - Ejecuta combo completo (windup → strike → recovery → gap → ...)
   - Rotación debug alternante por golpe (+25°/-25°)
   - Al terminar:
     * 25% probabilidad: se queda en inner circle → ataca de nuevo
     * 75% probabilidad: vuelve al outer circle → otro enemigo avanza
```

---

## 5. Árbol Completo — Behavior Tree

```
ROOT
│
└─ SELECTOR (prioridad de arriba a abajo)
   │
   ├─ [SERVICE] Update Enemy State                    ← cada 0.25s
   │  Actualiza BB: TargetActor, EnemyState,
   │  CanSeeTarget, DistanceToTarget, CanAttack
   │
   │
   │  ╔═══════════════════════════════════════════════════════════╗
   │  ║  ① COMBATE — Dos Círculos                               ║
   │  ╚═══════════════════════════════════════════════════════════╝
   │
   ├─ SEQUENCE "Combat"
   │  │
   │  ├─ [DECORATOR] Has Target                       ← ¿tiene target?
   │  │  Observer Aborts: Both                          si pierde target, aborta
   │  │
   │  ├─ TASK  Chase Target                           ← persigue hasta ~5m
   │  │        Llega al OuterCircleRadius (500 cm)
   │  │        Velocidad: ChaseSpeedMultiplier (0.45)
   │  │
   │  ├─ TASK  Outer Circle Behavior                  ← espera/tauntea
   │  │        CircleSpeedMultiplier: 0.25
   │  │        TauntChancePerSecond: 0.12
   │  │        TauntLungeDistance: 150
   │  │        SwayAmplitude: 40
   │  │        InnerCircleCheckInterval: 0.5
   │  │        RepositionInterval: 4.0 ± 1.5
   │  │        → Sale con Succeeded cuando hay hueco
   │  │          en el inner circle
   │  │
   │  └─ TASK  Attack Target                          ← ataque autónomo
   │           Approach → WindUp → Strike → Recovery
   │           → ComboGap → (siguiente golpe o Finished)
   │           WindUpDuration: 0.35
   │           StrikeDuration: 0.25
   │           RecoveryDuration: 0.5
   │           ComboGapDuration: 0.35
   │           DamageVariance: ±15%
   │           DebugRotationDegrees: ±25°
   │           DebugLungeDistance: 60
   │           → Al terminar: 25% se queda inner,
   │             75% vuelve a outer (loop al nodo anterior)
   │
   │
   │  ╔═══════════════════════════════════════════════════════════╗
   │  ║  ② INVESTIGACIÓN                                        ║
   │  ╚═══════════════════════════════════════════════════════════╝
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
   │  ╔═══════════════════════════════════════════════════════════╗
   │  ║  ③ PATRULLA (con conversación entre enemigos)           ║
   │  ╚═══════════════════════════════════════════════════════════╝
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

---

## 6. Tasks C++ — Referencia Rápida

### `BTTask_ChaseTarget`
- **Qué hace:** Persigue al jugador hasta llegar al `OuterCircleRadius` (~500 cm)
- **Velocidad:** `ChaseSpeedMultiplier` (0.45 × 200 = 90 cm/s)
- **Termina:** `Succeeded` cuando `DistanceToTarget ≤ OuterCircleRadius`

### `BTTask_OuterCircleBehavior` ⭐ NUEVO
- **Qué hace:** Comportamiento en el círculo exterior
- **Movimiento:** Se posiciona en el anillo con golden-angle, sway lateral
- **Taunts:** Amaga avanzar (~150 cm) hacia el jugador y retrocede
- **Reacción al jugador:** Si el jugador se mueve >150 cm, reacciona con delay (0.4–1.5s)
- **Termina:** `Succeeded` cuando `GroupCombatManager.RequestInnerCircleEntry()` devuelve `true`

### `BTTask_AttackTarget`
- **Qué hace:** Ataque completo dentro del inner circle
- **Fases:** Approach → WindUp → Strike → Recovery → ComboGap → Finished
- **Combo por distancia:**
  - Normaliza distancia (0=cerca, 1=lejos)
  - Combo[0] tiene más peso cuando CERCA
  - Combo[N-1] tiene más peso cuando LEJOS
  - Más golpes cuanto más cerca
- **Debug visual:** Rotación alternante ±25° por golpe + lunge 60 cm
- **Al terminar:** 25% se queda en inner, 75% vuelve a outer

---

## 7. GroupCombatManager — Coordinación

| Método                        | Qué hace                                              |
|-------------------------------|-------------------------------------------------------|
| `RegisterCombatEnemy(E)`      | Registra enemigo → va al outer circle                 |
| `UnregisterCombatEnemy(E)`    | Saca al enemigo de todo                               |
| `RequestInnerCircleEntry(E)`  | ¿Hay hueco? → mueve de outer a inner                  |
| `OnAttackFinished(E, bStay)`  | Tras atacar: se queda o sale. Devuelve siguiente atacante |
| `ForceToOuterCircle(E)`       | Fuerza vuelta al outer (knockback, etc.)              |
| `GetOuterCirclePosition(E,T)` | Posición en el anillo exterior (golden-angle)         |
| `GetInnerCircleAttackPosition(E,T)` | Posición random en rango de ataque              |

**Configuración:**
- `MaxInnerCircleEnemies`: 2 (se hereda de `CombatConfig.MaxSimultaneousAttackers`)
- `InnerCircleCooldown`: 2.0s antes de poder re-entrar

---

## 8. Configuración del Enemigo (`FEnemyCombatConfig`)

| Parámetro                      | Default | Descripción                                 |
|--------------------------------|---------|---------------------------------------------|
| `OuterCircleRadius`            | 500 cm  | Radio del círculo exterior                   |
| `OuterCircleVariation`         | 80 cm   | Variación ± del radio exterior               |
| `MinAttackPositionDist`        | 100 cm  | Distancia mín. al atacar                     |
| `MaxAttackPositionDist`        | 200 cm  | Distancia máx. al atacar                     |
| `ChanceToStayInnerAfterAttack` | 0.25    | Probabilidad de quedarse tras atacar          |
| `PlayerMoveReactionDelayMin`   | 0.4 s   | Delay mín. de reacción al movimiento         |
| `PlayerMoveReactionDelayMax`   | 1.5 s   | Delay máx. de reacción al movimiento         |
| `BaseDamage`                   | 10.0    | Daño base por golpe                          |
| `AttackCooldown`               | 2.0 s   | Cooldown entre ataques                       |
| `MaxSimultaneousAttackers`     | 2       | Máx. enemigos en inner circle                |

---

## 9. Velocidades

| Estado         | Multiplier | Velocidad real (base=200) |
|----------------|-----------|---------------------------|
| Patrulla       | 0.20      | 40 cm/s                   |
| Persecución    | 0.45      | 90 cm/s                   |
| Outer Circle   | 0.25      | 50 cm/s                   |
| Inner/Attack   | 0.50      | 100 cm/s                  |

---

## 10. Algoritmo de Selección de Combo

```
Dado:
  N = número de montajes de ataque
  dist = distancia actual al jugador
  T = normalizar(dist, MinAttackPositionDist, MaxAttackPositionDist) → [0, 1]

Para cada combo i (0..N-1):
  idealT(i) = i / (N-1)          ← 0=cerca, 1=lejos
  peso(i) = max(0.1, 1.0 - |idealT - T|)
  peso(i) *= (1.0 + (1.0 - idealT) * 0.3)   ← boost para combos cercanos

Selección: random ponderado por pesos

Número de golpes:
  closeWeight = 1.0 - T
  meanHits = lerp(1, N, closeWeight)
  totalHits = round(meanHits ± 0.5)
```

**Ejemplo con 3 montajes y T=0.2 (cerca):**
```
combo[0]: idealT=0.0, peso=max(0.1, 1-0.2)=0.8  × 1.3 = 1.04  ← FAVORITO
combo[1]: idealT=0.5, peso=max(0.1, 1-0.3)=0.7  × 1.15= 0.81
combo[2]: idealT=1.0, peso=max(0.1, 1-0.8)=0.2  × 1.0 = 0.20
→ combo[0] tiene ~50% probabilidad, combo[2] ~10%
→ totalHits ≈ 3 golpes (cerca = más agresivo)
```

**Ejemplo con 3 montajes y T=0.9 (lejos):**
```
combo[0]: idealT=0.0, peso=max(0.1, 1-0.9)=0.1  × 1.3 = 0.13
combo[1]: idealT=0.5, peso=max(0.1, 1-0.4)=0.6  × 1.15= 0.69
combo[2]: idealT=1.0, peso=max(0.1, 1-0.1)=0.9  × 1.0 = 0.90  ← FAVORITO
→ combo[2] tiene ~52% probabilidad, combo[0] ~8%
→ totalHits ≈ 1 golpe (lejos = ataque rápido)
```

---

## 11. Cómo montar en el Editor de UE5

### Crear el Blackboard
1. Content Browser → clic derecho → Artificial Intelligence → Blackboard
2. Nombre: `BB_EnemyDefault`
3. Añadir las 7 keys de la tabla de arriba (tipos exactos)

### Crear el Behavior Tree
1. Content Browser → clic derecho → Artificial Intelligence → Behavior Tree
2. Nombre: `BT_Enemy`
3. Asignar `BB_EnemyDefault` como Blackboard Asset
4. Montar el árbol siguiendo la estructura de la sección 5

### Asignar al enemigo
1. Abrir `BP_NormalEnemy` (Blueprint hijo de `ANormalEnemy`)
2. En Details → `Behavior Tree` → seleccionar `BT_Enemy`
3. Verificar que `AIControllerClass` = `AEnemyAIController`

### Notas importantes
- El `SERVICE Update Enemy State` va en el **nodo raíz SELECTOR** (no en un sequence)
- Los `DECORATOR` van en el **primer nodo** de cada Sequence
- `Has Target` en Combat: condición normal. En Patrol: condición **invertida**
- El Sequence "Combat" funciona como loop: Chase → Outer → Attack → (si vuelve a outer, el Sequence falla y se re-ejecuta)

---

## 12. ⚠️ Tasks obsoletos — NO usar

| Task                        | Estado      | Reemplazado por                    |
|-----------------------------|-------------|-------------------------------------|
| `BTTask_CircleTarget`       | ❌ DEPRECATED | `BTTask_OuterCircleBehavior`       |
| `BTTask_WaitForAttackTurn`  | ❌ ELIMINADO  | Integrado en `BTTask_OuterCircleBehavior` + `GroupCombatManager` |

**En el BT debes usar:**
- ✅ `Chase Target` → para perseguir
- ✅ `Outer Circle Behavior` → para esperar/tauntear en el círculo exterior
- ✅ `Attack Target` → para atacar en el inner circle

**NO uses `Circle Target (Flank)`** — es el sistema antiguo y no funciona con el nuevo `GroupCombatManager`.

