# Sistema de Enemigos - SairanSkies
## Guía de Implementación y Montaje

---

## 📁 Estructura de Archivos

### Clases Base
- `EnemyTypes.h` - Enums, estructuras de configuración y delegates
- `EnemyBase.h/.cpp` - Clase base abstracta para todos los enemigos
- `PatrolPath.h/.cpp` - Actor para definir rutas de patrulla
- `EnemyAIController.h/.cpp` - Controlador de IA para enemigos

### Enemigos Implementados
- `NormalEnemy.h/.cpp` - Enemigo básico de combate cuerpo a cuerpo

### BTTaskNodes (Tareas del Behavior Tree)
- `BTTask_FindPatrolPoint` - Encuentra el siguiente punto de patrulla
- `BTTask_MoveToLocation` - Mueve al enemigo a una ubicación
- `BTTask_WaitAtPatrolPoint` - Espera en un punto de patrulla
- `BTTask_ChaseTarget` - Persigue al objetivo
- `BTTask_ApproachForAttack` - Se acerca al rango de ataque
- `BTTask_PositionForAttack` - Se posiciona (strafe/espera turno)
- `BTTask_AttackTarget` - Ejecuta el ataque
- `BTTask_PerformTaunt` - Ejecuta una burla
- `BTTask_IdleBehavior` - Comportamiento de pausa/espera
- `BTTask_Investigate` - Investiga última ubicación conocida

### BTDecorators
- `BTDecorator_HasTarget` - Verifica si tiene un objetivo
- `BTDecorator_CheckEnemyState` - Verifica el estado actual

### BTServices
- `BTService_UpdateEnemyState` - Actualiza el estado en el Blackboard

---

## 🎮 Diagrama de Estados del Enemigo

```
                         ┌───────────────────┐
                         │      SPAWN        │
                         └─────────┬─────────┘
                                   │
                                   ▼
              ┌────────────────────────────────────────┐
              │             PATROLLING                 │
              │  • Recorre PatrolPath                  │
              │  • Velocidad variable (±10%)           │
              │  • Pausas aleatorias (15% prob.)       │
              │  • Mira alrededor durante pausas       │
              └────┬───────────────────────────────────┘
                   │
         ┌─────────���─────────┐
         │                   │
         ▼                   ▼
┌─────────────────┐  ┌─────────────────────────────────┐
│   CONVERSING    │  │     ¿Detecta al jugador?        │
│  • Con otro     │  └────────────────┬────────────────┘
│    enemigo      │                   │ SÍ
│  • 5-12 seg     │                   ▼
│  • Gestos       │  ┌────────────────────────────────────────┐
│  • Se miran     │  │              CHASING                   │
└────────┬────────┘  │  • Persigue al jugador                 │
         │           │  • Velocidad máxima                    │
         │           │  • Alerta a aliados cercanos           │
         └───────────┤  • Interrumpe conversación si la hay   │
                     └──────────────────┬─────────────────────┘
                                        │
                           ┌────────────▼────────────┐
                           │  ¿En rango de combate?  │
                           └────────────┬────────────┘
                                        │ SÍ
                                        ▼
                           ┌────────────────────────────┐
                           │  ¿Puede unirse al ataque?  │
                           │  (ActiveAttackers < 3)     │
                           └──────┬─────────────┬───────┘
                                  │ NO          │ SÍ
                                  ▼             ▼
              ┌─────────────────────┐  ┌─────────────────────┐
              │    POSITIONING      │  │     ATTACKING       │
              │  • Strafe lateral   │  │  • Se acerca        │
              │  • Espera su turno  │  │  • Ejecuta ataque   │
              │  • Mira al jugador  │  │  • Aplica daño      │
              │  • Cambia dirección │  │  • Cooldown 2 seg   │
              └���─────────┬──────────┘  └──────────┬──────────┘
                         │                        │
                         └───────────┬────────────┘
                                     │
                        ┌────────────▼────────────┐
                        │  ¿Perdió al jugador?    │
                        │  (5 seg sin ver)        │
                        └────────────┬────────────┘
                                     │ SÍ
                                     ▼
              ┌────────────────────────────────────────┐
              │            INVESTIGATING               │
              │  • Va a última ubicación conocida      │
              │  • Busca en el área                    │
              │  • Duración: 10 segundos               │
              └──────────────────┬─────────────────────┘
                                 │ Tiempo agotado
                                 ▼
                        Vuelve a PATROLLING
```

---

## 🧠 Sistema de Coordinación de Ataques

```
┌─────────────────────────────────────────────────────────────────┐
│                  COORDINACIÓN DE ATAQUES                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  MaxSimultaneousAttackers = 3                                   │
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐            │
│  │ENEMIGO 1│  │ENEMIGO 2│  │ENEMIGO 3│  │ENEMIGO 4│            │
│  │ATACANDO │  │ATACANDO │  │ATACANDO │  │ STRAFE  │ ← Espera   │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘            │
│       ▲            ▲            ▲            │                  │
│       └────────────┴────────────┴────────────┘                  │
│                ActiveAttackers.Num() = 3                        │
│                                                                 │
│  Cuando uno termina de atacar → Se desregistra                  │
│  El enemigo 4 detecta hueco → RegisterAsAttacker() → Ataca     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 💬 Sistema de Conversación entre Enemigos

Cuando dos enemigos están en el mismo punto de patrulla por X tiempo, pueden conversar:

```
┌─────────────────────────────────────────────────────────────────┐
│              FLUJO DE CONVERSACIÓN                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. Enemigo A llega a punto de patrulla                         │
│  2. Enemigo B también está esperando                            │
│  3. Ambos esperan 3+ segundos (TimeBeforeConversation)          │
│  4. Están dentro de ConversationRadius (200 unidades)           │
│  5. Ninguno está en cooldown de conversación                    │
│  6. Enemigo A inicia → TryStartConversation(B)                  │
│  7. Ambos entran en estado CONVERSING                           │
│  8. Se miran mutuamente                                         │
│  9. Hacen gestos aleatorios + sonidos                           │
│  10. Duración: 5-12 segundos                                    │
│  11. Terminan → Cooldown de 30 segundos                         │
│  12. Vuelven a PATROLLING                                       │
│                                                                 │
│  ⚠️ Si detectan al jugador → Interrumpen inmediatamente        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Comportamientos Naturales (AAA-Style)

### Durante Patrulla:
| Comportamiento | Probabilidad | Descripción |
|---------------|-------------|-------------|
| **Pausas aleatorias** | 15% | Se detiene 1-3 segundos |
| **Mirar alrededor** | 40% | Durante pausas, mira ±90° |
| **Variación de velocidad** | ±10% | Cada vez que cambia de punto |

### Durante Espera en Punto:
- Cuenta el tiempo esperando (`TimeWaitingAtPoint`)
- Si hay otro enemigo cerca esperando → Intenta conversar
- Si no → Puede mirar alrededor aleatoriamente

### Durante Combate:
- Si no puede atacar (3 ya atacando) → Hace strafe
- Strafe cambia de dirección cada 1.5 segundos
- Siempre mira al jugador mientras hace strafe

---

## ⚙️ Variables de Configuración

### Combat Config
| Variable | Valor | Descripción |
|----------|-------|-------------|
| `MinAttackDistance` | 150 | Distancia mínima para atacar |
| `MaxAttackDistance` | 200 | Distancia máxima para atacar |
| `PositioningDistance` | 350 | Distancia de strafe/espera |
| `MinPositioningTime` | 1.0 | Tiempo mínimo en positioning |
| `MaxPositioningTime` | 3.0 | Tiempo máximo en positioning |
| `BaseDamage` | 10 | Daño por ataque |
| `AttackCooldown` | 2.0 | Cooldown entre ataques |
| `AllyDetectionRadius` | 1500 | Radio para alertar aliados |
| `MaxSimultaneousAttackers` | 3 | Máximo de enemigos atacando a la vez |

### Perception Config
| Variable | Valor | Descripción |
|----------|-------|-------------|
| `SightRadius` | 2000 | Radio de visión |
| `PeripheralVisionAngle` | 90 | Ángulo de visión |
| `HearingRadius` | 1000 | Radio de audición |
| `ProximityRadius` | 300 | Radio de detección automática |
| `LoseSightTime` | 5.0 | Tiempo para perder objetivo |
| `InvestigationTime` | 10.0 | Duración de investigación |

### Patrol Config
| Variable | Valor | Descripción |
|----------|-------|-------------|
| `PatrolSpeedMultiplier` | 0.5 | 50% velocidad al patrullar |
| `ChaseSpeedMultiplier` | 1.0 | 100% velocidad al perseguir |
| `WaitTimeAtPatrolPoint` | 2.0 | Tiempo mínimo de espera |
| `MaxWaitTimeAtPatrolPoint` | 4.0 | Tiempo máximo de espera |
| `PatrolPointAcceptanceRadius` | 100 | Radio para "llegar" |

### Behavior Config (Natural)
| Variable | Valor | Descripción |
|----------|-------|-------------|
| `ChanceToStrafe` | 0.5 | 50% probabilidad de strafe |
| `StrafeDuration` | 1.5 | Duración de cada strafe |
| `StrafeSpeed` | 200 | Velocidad de strafe |
| `ChanceToPauseDuringPatrol` | 0.15 | 15% prob. de pausar |
| `MinPauseDuration` | 1.0 | Duración mínima de pausa |
| `MaxPauseDuration` | 3.0 | Duración máxima de pausa |
| `ChanceToLookAround` | 0.4 | 40% prob. mirar alrededor |
| `MaxLookAroundAngle` | 90 | Ángulo máximo de giro |
| `LookAroundSpeed` | 60 | Velocidad de rotación |
| `PatrolSpeedVariation` | 0.1 | ±10% variación velocidad |

### Conversation Config
| Variable | Valor | Descripción |
|----------|-------|-------------|
| `ConversationRadius` | 200 | Radio para detectar compañero |
| `TimeBeforeConversation` | 3.0 | Segundos esperando antes de conversar |
| `MinConversationDuration` | 5.0 | Duración mínima |
| `MaxConversationDuration` | 12.0 | Duración máxima |
| `ConversationCooldown` | 30.0 | Cooldown entre conversaciones |
| `ChanceToGesture` | 0.3 | Probabilidad de gesto |
| `GestureInterval` | 2.5 | Intervalo entre gestos |

---

## 🎬 Assets Necesarios

### Animaciones
| Asset | Tipo | Descripción |
|-------|------|-------------|
| `AttackMontages[]` | Array | Montajes de ataque |
| `HitReactionMontages[]` | Array | Reacciones a golpes |
| `DeathMontage` | Single | Montaje de muerte |
| `LookAroundMontage` | Single | Mirar alrededor (idle) |
| `ConversationGestures[]` | Array | Gestos durante conversación |

### Sonidos
| Asset | Tipo | Descripción |
|-------|------|-------------|
| `AttackSounds[]` | Array | Sonidos de ataque |
| `PainSounds[]` | Array | Sonidos de dolor |
| `DeathSounds[]` | Array | Sonidos de muerte |
| `AlertSounds[]` | Array | Sonidos al detectar jugador |
| `ConversationSounds[]` | Array | Voces de conversación |
| `LaughSounds[]` | Array | Risas durante conversación |

### VFX
| Asset | Tipo | Descripción |
|-------|------|-------------|
| `HitEffect` | Niagara | Efecto al recibir golpe |
| `DeathEffect` | Niagara | Efecto al morir |

---

## 🌳 Behavior Tree - Estructura Completa

### Tareas Implementadas (BTTaskNode):

1. **BTTask_FindPatrolPoint** - Busca el siguiente punto de patrulla
   - Obtiene el siguiente índice del PatrolPath
   - Establece TargetLocation en el Blackboard
   
2. **BTTask_MoveToLocation** - Mueve al enemigo hacia una ubicación
   - Lee TargetLocation del Blackboard
   - Usa MoveTo del AIController
   - Retorna Success cuando llega
   
3. **BTTask_WaitAtPatrolPoint** - Espera en un punto de patrulla
   - Duración aleatoria (2-4 segundos por defecto)
   - Puede iniciar pausa aleatoria (15% probabilidad)
   - Busca compañeros para conversar
   - Se interrumpe si detecta jugador
   
4. **BTTask_ChaseTarget** - Persigue al objetivo
   - Usa MoveTo hacia la ubicación del jugador
   - Actualiza LastKnownTargetLocation continuamente
   - Alerta a aliados cercanos
   - Cambia a Chasing state
   
5. **BTTask_Investigate** - Investiga la última ubicación conocida
   - Va a LastKnownTargetLocation
   - Duración: 10 segundos
   - Si ve al jugador → vuelve a ChaseTarget
   - Si no → vuelve a Patrol
   
6. **BTTask_ApproachForAttack** - Se acerca para el rango de ataque
   - Mueve hacia el objetivo a velocidad variable
   - Se detiene en MinAttackDistance
   - Prepara para atacar
   
7. **BTTask_PositionForAttack** - Posicionamiento táctico (strafe)
   - Se mantiene a PositioningDistance
   - Rota alrededor del objetivo
   - Espera su turno (MaxSimultaneousAttackers = 3)
   - Si puede atacar → intenta registrarse
   
8. **BTTask_AttackTarget** - Ejecuta el ataque
   - Aplica daño al objetivo
   - Reproduce montaje de ataque
   - Reproduce sonido de ataque
   - Cooldown de 2 segundos
   
9. **BTTask_PerformTaunt** - Ejecuta una burla (taunt)
   - Solo si ShouldTaunt() retorna true
   - Alerta a más aliados
   - Duracion variable
   - Para el movimiento
   
10. **BTTask_IdleBehavior** - Comportamiento de pausa/espera
    - Pausa aleatoria durante espera
    - Puede mirar alrededor
    - Se interrumpe si ve jugador
    - Duracion: 1-3 segundos

### Decoradores Implementados:

1. **BTDecorator_HasTarget** - Verifica si CurrentTarget no es null
2. **BTDecorator_CheckEnemyState** - Verifica el estado actual (Idle, Patrolling, Chasing, etc.)

### Servicios Implementados:

1. **BTService_UpdateEnemyState** - Se ejecuta cada 0.25s
   - Actualiza TargetActor en el Blackboard
   - Actualiza TargetLocation
   - Actualiza CanSeeTarget
   - Actualiza DistanceToTarget
   - Actualiza CanAttack (disponibilidad de atacantes)
   - Actualiza IsInPause y IsConversing

---

## 🌳 Behavior Tree - Diagrama Visual Completo

```
                              ┌─────────────────┐
                              │      ROOT       │
                              │   (Selector)    │
                              └────────┬────────┘
                                       │
                    ┌──────────────────┼──────────────────┐
                    │                  │                  │
                    ▼                  ▼                  ▼
         ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
         │ Service Update   │ │  Investigation   │ │  Combat          │
         │ (Cada 0.25s)     │ │  [State==Invest] │ │  [HasTarget]     │
         └──────────────────┘ └────────┬─────────┘ └────────┬─────────┘
                                       │                     │
                                       ▼                     ▼
                           ┌───────────────────────┐  ┌──────────────┐
                           │  MoveToLocation      │  │  Selector    │
                           │  (LastKnownLocation) │  │ "Attack Mode"│
                           └───────────────────────┘  └──────┬───────┘
                                                              │
                                     ┌────────────────────────┼────────────────────────┐
                                     │                        │                        │
                                     ▼                        ▼                        ▼
                           ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
                           │ AttackTarget     │   │ ApproachForAttack│   │ PositionForAttack│
                           │ [Decorator:      │   │                  │   │ (Strafe/Wait)    │
                           │  CanAttack=TRUE] │   │ [Decorator:      │   │                  │
                           │                  │   │  Distance <      │   │ [Decorator:      │
                           │ • Ataca          │   │  PositioningDist]│   │  CanJoinAttack]  │
                           │ • Aplica daño    │   │                  │   │                  │
                           │ • Cooldown 2s    │   │ • Se acerca      │   │ • Rota alrededor │
                           │                  │   │ • Se detiene     │   │ • Espera turno   │
                           │                  │   │ • Prepara        │   │                  │
                           │                  │   │                  │   │                  │
                           └──────────────────┘   └──────────────────┘   └──────────────────┘
                                                              │
                                     ┌────────────────────────┴────────────────────────┐
                                     │                                                 │
                                     ▼                                                 ▼
                           ┌──────────────────┐                        ┌──────────────────┐
                           │ ChaseTarget      │                        │ PerformTaunt     │
                           │                  │                        │ [Decorator:      │
                           │ • Persigue       │                        │  ShouldTaunt]    │
                           │ • Alerta aliados │                        │                  │
                           │ • Actualiza      │                        │ • Hace burla      │
                           │   Last Location  │                        │ • Alerta más     │
                           │                  │                        │ • Pause movemnt  │
                           └──────────────────┘                        └──────────────────┘


                    ┌─────────────────────────────────────────────────────┐
                    │  Patrol [Selector - Si NO HasTarget]               │
                    │  (Se ejecuta cuando no está en combate)             │
                    └────────────────────────┬────────────────────────────┘
                                             │
                    ┌────────────────────────┼────────────────────────┐
                    │                        │                        │
                    ▼                        ▼                        ▼
         ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
         │ FindPatrolPoint  │    │ MoveToLocation   │    │ WaitAtPatrolPoint│
         │                  │    │                  │    │                  │
         │ • Obtiene índice │    │ • Mueve a punto  │    │ • Espera 2-4 seg │
         │ • Incrementa idx │    │ • PathFollowing  │    │ • Pausa aleatoria│
         │ • Set Blackboard │    │ • Lee Blackboard │    │ • Busca compañero│
         │                  │    │                  │    │ • Mira alrededor │
         │                  │    │                  │    │ • Se interrumpe  │
         │                  │    │                  │    │   si ve jugador  │
         └──────────────────┘    └──────────────────┘    └──────────────────┘
                                             │
                                             ▼
                                  ┌──────────────────┐
                                  │ IdleBehavior     │
                                  │ (Opcional durante│
                                  │  WaitAtPoint)    │
                                  │                  │
                                  │ • Pausa 1-3 seg │
                                  │ • Mira alrededor │
                                  │ • Hace gestos    │
                                  │ • Sonidos        │
                                  └──────────────────┘
```

### Flujo de Ejecución:

1. **Cada frame:**
   - Service_UpdateEnemyState actualiza el Blackboard
   - Lee TargetActor, DistanceToTarget, CanSeeTarget, etc.

2. **En Patrol:**
   - FindPatrolPoint → MoveToLocation → WaitAtPatrolPoint (loop)
   - Durante espera puede entrar en IdleBehavior

3. **Al detectar jugador:**
   - HasTarget = TRUE
   - Cambia a Combat
   - ChaseTarget persigue

4. **En rango de combate:**
   - ApproachForAttack OR PositionForAttack
   - Espera su turno en coordinación

5. **Para atacar:**
   - AttackTarget (solo si CanAttack = TRUE)
   - PerformTaunt (ocasionalmente)

6. **Si pierde al jugador:**
   - Pasa a Investigation
   - Busca en el área
   - Si no encuentra → vuelve a Patrol

---

## ✅ Resumen de Tareas del Behavior Tree

| Tarea | Entrada | Salida | Descripción |
|-------|---------|--------|-------------|
| **FindPatrolPoint** | PatrolPath | TargetLocation | Selecciona siguiente punto |
| **MoveToLocation** | TargetLocation | Success/Failure | Movimiento a destino |
| **WaitAtPatrolPoint** | Timer | Success | Espera en punto |
| **IdleBehavior** | Timer | Success | Pausa con animaciones |
| **ChaseTarget** | TargetActor | Success | Persigue jugador |
| **ApproachForAttack** | TargetActor | Success | Se acerca a rango |
| **PositionForAttack** | TargetActor | Success | Strafe/espera turno |
| **AttackTarget** | TargetActor | Success | Ejecuta ataque |
| **PerformTaunt** | Probability | Success | Hace burla |
| **Investigate** | LastKnownLocation | Success | Busca en área |


---

## ✅ Blackboard Keys

| Key | Tipo | Descripción |
|-----|------|-------------|
| `TargetActor` | Object | El jugador detectado |
| `TargetLocation` | Vector | Ubicación objetivo |
| `EnemyState` | Int | Estado actual (enum) |
| `CanSeeTarget` | Bool | Si puede ver al objetivo |
| `PatrolIndex` | Int | Índice del punto de patrulla |
| `DistanceToTarget` | Float | Distancia al objetivo |
| `CanAttack` | Bool | Si puede unirse al ataque |
| `IsInPause` | Bool | Si está en pausa aleatoria |
| `IsConversing` | Bool | Si está conversando |

---

## 🔧 Pasos de Configuración en Unreal Editor

### Paso 1: Crear el Blackboard
1. Content Browser → Click derecho → Artificial Intelligence → Blackboard
2. Nombrar: `BB_Enemy`
3. Añadir las Keys listadas arriba con los tipos correctos

### Paso 2: Crear el Behavior Tree
1. Content Browser → Click derecho → Artificial Intelligence → Behavior Tree
2. Nombrar: `BT_NormalEnemy`
3. Asignar el Blackboard `BB_Enemy`
4. Montar la estructura según el diagrama

### Paso 3: Crear PatrolPath
1. Place Actors → Buscar "PatrolPath"
2. Colocar en el nivel
3. En el panel Details, añadir puntos en `PatrolPoints` array
4. Usar el widget 3D para posicionar los puntos

### Paso 4: Crear Blueprint del Enemigo
1. Click derecho → Blueprint Class
2. Seleccionar `NormalEnemy` como clase padre
3. Nombrar: `BP_NormalEnemy`
4. Abrir el Blueprint
5. Configurar las variables en Class Defaults:
   - Asignar `BehaviorTree` = `BT_NormalEnemy`
   - Asignar `PatrolPath` (se puede hacer en el nivel)
   - Configurar CombatConfig, PerceptionConfig, etc.

### Paso 5: Configurar el Jugador para ser Detectado
1. Abrir el Blueprint del jugador
2. Añadir componente `AIPerceptionStimuliSource`
3. En el componente, configurar:
   - `Auto Register as Source` = true
   - Añadir `AISense_Sight` a los senses registrados

### Paso 6: Configurar NavMesh
1. Place Actors → Volumes → Nav Mesh Bounds Volume
2. Escalar para cubrir el área jugable
3. Build → Build Paths (o presionar P para visualizar)

---

## 📝 Eventos Blueprint Disponibles

| Evento | Cuándo se dispara |
|--------|-------------------|
| `OnEnemyStateChanged(Old, New)` | Al cambiar de estado |
| `OnPlayerDetected(Player, SenseType)` | Al detectar jugador |
| `OnPlayerLost()` | Al perder al jugador |
| `OnEnemyDeath(Instigator)` | Al morir |
| `OnConversationStarted(Partner)` | Al iniciar conversación |
| `OnConversationEnded()` | Al terminar conversación |
| `OnRandomPauseStarted()` | Al iniciar pausa aleatoria |
| `OnRandomPauseEnded()` | Al terminar pausa aleatoria |
| `OnLookAroundStarted()` | Al empezar a mirar alrededor |
| `OnConversationGesture()` | Al hacer gesto en conversación |

---

## 🎯 Resumen del Comportamiento Inteligente

1. **Patrulla natural** - Velocidad variable, pausas aleatorias, mira alrededor
2. **Conversaciones** - Dos enemigos en el mismo punto conversan automáticamente
3. **Detección con alertas** - Un enemigo ve al jugador → alerta a todos los cercanos
4. **Persecución coordinada** - Todos van hacia el jugador
5. **Ataques por turnos** - Máximo 3 atacando a la vez, el resto hace strafe
6. **Strafe inteligente** - Rodean al jugador, cambian dirección periódicamente
7. **Investigación** - Si pierden al jugador, van a buscarlo antes de volver a patrullar
8. **Modular** - Todas las variables son configurables desde Blueprint

---

## ⚠️ Troubleshooting

### El enemigo no se mueve
- Verificar que hay NavMesh en el nivel (presionar P para visualizar)
- Verificar que PatrolPath tiene puntos asignados
- Verificar que BehaviorTree está asignado

### El enemigo no detecta al jugador
- Verificar que el jugador tiene `AIPerceptionStimuliSource`
- Verificar que `SightRadius` es suficientemente grande
- Revisar Output Log para mensajes de debug

### Los enemigos no conversan
- Verificar que están dentro de `ConversationRadius` (200 unidades)
- Verificar que ambos esperan `TimeBeforeConversation` segundos (3)
- Verificar que `ConversationCooldownTimer` ha expirado

### Los ataques no funcionan
- Verificar que `MaxSimultaneousAttackers` permite atacantes
- Verificar `MinAttackDistance` y `MaxAttackDistance`
- Revisar logs: "registered as attacker" / "unregistered as attacker"

---

*Documento generado para el proyecto SairanSkies*

