# Sistema de Enemigos - SairanSkies
## Guía de Implementación y Montaje

---

## 📁 Estructura de Archivos Creados

### Clases Base
- `EnemyTypes.h` - Enums y estructuras de configuración
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
- `BTTask_PositionForAttack` - Se posiciona a distancia antes de atacar
- `BTTask_ApproachForAttack` - Se acerca para atacar
- `BTTask_AttackTarget` - Ejecuta el ataque
- `BTTask_PerformTaunt` - Realiza un taunt
- `BTTask_Investigate` - Investiga la última ubicación conocida

### BTDecorators (Condiciones del Behavior Tree)
- `BTDecorator_CheckEnemyState` - Verifica el estado actual del enemigo
- `BTDecorator_HasTarget` - Verifica si tiene un objetivo

### BTServices
- `BTService_UpdateEnemyState` - Actualiza el estado en el Blackboard

---

## 🔧 Pasos de Configuración en Unreal Editor

### Paso 1: Compilar el Proyecto
1. Abre el proyecto en Visual Studio
2. Compila en modo Development Editor
3. Abre Unreal Engine

### Paso 2: Crear el Blackboard

1. En el Content Browser: **Click derecho → Artificial Intelligence → Blackboard**
2. Nombrar: `BB_Enemy`
3. Añadir las siguientes Keys (IMPORTANTE: Las claves DEBEN tener exactamente estos nombres):

| Nombre | Tipo | Descripción |
|--------|------|-------------|
| `TargetActor` | Object (Actor) | El jugador detectado |
| `TargetLocation` | Vector | Ubicación objetivo (patrulla o último conocimiento del jugador) |
| `EnemyState` | Int | Estado actual del enemigo (0=Idle, 1=Patrolling, 2=Investigating, 3=Chasing, 4=Positioning, 5=Attacking, 6=Taunting, 7=Dead) |
| `CanSeeTarget` | Bool | Si puede ver al objetivo actualmente |
| `PatrolIndex` | Int | Índice actual del punto de patrulla |
| `ShouldTaunt` | Bool | Si debería hacer taunt |
| `NearbyAllies` | Int | Número de aliados cercanos |
| `DistanceToTarget` | Float | Distancia al objetivo |
| `SuspicionLevel` | Float | Nivel de sospecha (0-1) |
| `IsAlerted` | Bool | Si está en estado de alerta |
| `IsInPause` | Bool | Si está en pausa aleatoria |

**ℹ️ NOTA:** El estado del enemigo se almacena como Int para mayor compatibilidad.

### Paso 3: Crear el Behavior Tree

1. En el Content Browser: **Click derecho → Artificial Intelligence → Behavior Tree**
2. Nombrar: `BT_NormalEnemy`
3. Asignar el Blackboard `BB_Enemy`

### Paso 4: Estructura del Behavior Tree

**IMPORTANTE**: La secuencia de patrullaje debe estar en un **Sequence** node, no en nodos separados.

```
[ROOT]
└── [Selector] - Nodo raíz
    │
    ├── [Service: UpdateEnemyState] (añadir al Selector raíz)
    │
    ├── [Sequence] "Combat" ─ [Decorator: HasTarget]
    │   │
    │   ├── [Task: ChaseTarget]           ← Persigue al objetivo
    │   │
    │   ├── [Selector] "Attack Decision"
    │   │   │
    │   │   ├── [Sequence] "Taunt" ─ [Decorator: CheckEnemyState = Taunting]
    │   │   │   └── [Task: PerformTaunt]  ← Provoca al jugador
    │   │   │
    │   │   └── [Sequence] "Attack Flow"
    │   │       ├── [Task: PositionForAttack]   ← Se posiciona (strafe)
    │   │       ├── [Task: ApproachForAttack]   ← Se acerca rápido
    │   │       └── [Task: AttackTarget]        ← Ejecuta el ataque
    │
    ├── [Sequence] "Investigation" ─ [Decorator: CheckEnemyState = Investigating]
    │   └── [Task: Investigate]           ← Investiga última ubicación
    │
    └── [Sequence] "Patrol" ─ [Decorator: HasTarget (Inverse Check = TRUE)]
        │
        │   ⚠️ IMPORTANTE: Estos nodos deben estar en SECUENCIA
        │
        ├── [Task: FindPatrolPoint]       ← Encuentra el siguiente punto
        ├── [Task: IdleBehavior]          ← (Opcional) Pausas naturales
        ├── [Task: MoveToLocation]        ← Se mueve al punto
        └── [Task: WaitAtPatrolPoint]     ← Espera en el punto
```

### Lista Completa de Nodos Disponibles

| Tipo | Nombre | Descripción |
|------|--------|-------------|
| **Task** | `FindPatrolPoint` | Encuentra el siguiente punto de patrulla |
| **Task** | `MoveToLocation` | Mueve al enemigo a una ubicación |
| **Task** | `WaitAtPatrolPoint` | Espera en un punto de patrulla |
| **Task** | `IdleBehavior` | Pausas aleatorias durante patrulla (AAA-style) |
| **Task** | `ChaseTarget` | Persigue al objetivo |
| **Task** | `PositionForAttack` | Se posiciona a distancia (strafe) |
| **Task** | `ApproachForAttack` | Se acerca rápidamente para atacar |
| **Task** | `AttackTarget` | Ejecuta el ataque |
| **Task** | `PerformTaunt` | Realiza un taunt/provocación |
| **Task** | `Investigate` | Investiga la última ubicación conocida |
| **Decorator** | `HasTarget` | Verifica si tiene un objetivo |
| **Decorator** | `CheckEnemyState` | Verifica el estado actual del enemigo |
| **Service** | `UpdateEnemyState` | Actualiza el estado en el Blackboard |

**Configuración paso a paso:**

1. **Selector raíz**: Click derecho en ROOT → Add Composite → Selector
2. **Service en Selector**: Click derecho en el Selector → Add Service → UpdateEnemyState
3. **Sequence "Combat"**: Click derecho en Selector → Add Composite → Sequence
4. **Decorator HasTarget**: 
   - Click derecho en Sequence "Combat" → Add Decorator → HasTarget
5. **Añadir Tasks de combate al Sequence "Combat"**:
   - Click derecho en Sequence → Add Task → ChaseTarget
   - Añadir Selector hijo para decisión de ataque
   - Dentro: PositionForAttack → ApproachForAttack → AttackTarget
6. **Sequence "Investigation"**: Click derecho en Selector → Add Composite → Sequence
7. **Decorator CheckEnemyState = Investigating**:
   - Click derecho en Sequence "Investigation" → Add Decorator → CheckEnemyState
   - En Details: State To Check = Investigating
8. **Sequence "Patrol"**: Click derecho en Selector → Add Composite → Sequence
9. **Decorator HasTarget (inverso)**: 
   - Click derecho en Sequence "Patrol" → Add Decorator → HasTarget
   - En Details: **Inverse Condition = TRUE** (para que solo patrulle cuando NO hay target)
10. **Añadir Tasks al Sequence "Patrol"**:
    - Click derecho en Sequence → Add Task → FindPatrolPoint
    - Click derecho en Sequence → Add Task → IdleBehavior (opcional)
    - Click derecho en Sequence → Add Task → MoveToLocation
    - Click derecho en Sequence → Add Task → WaitAtPatrolPoint

**El flujo correcto de patrulla es:**
```
FindPatrolPoint (establece TargetLocation) 
    → IdleBehavior (pausa natural, opcional)
    → MoveToLocation (se mueve) 
    → WaitAtPatrolPoint (espera) 
    → [Sequence completa, vuelve a empezar]
```

**El flujo correcto de combate es:**
```
ChaseTarget (persigue hasta rango)
    → PositionForAttack (strafe lateral)
    → ApproachForAttack (se acerca rápido)
    → AttackTarget (golpea)
    → [Vuelve a evaluar]
```

### Paso 5: Crear el PatrolPath en el Nivel

1. En el nivel, **Place Actors → All Classes → PatrolPath**
2. Selecciona el actor PatrolPath
3. En el panel de Detalles, expande **Patrol**
4. Añade puntos al array `PatrolPoints` usando el widget 3D
5. Configura:
   - `bLoopPatrol`: Si debe volver al inicio al terminar
   - `bPingPongPatrol`: Si debe ir y volver

### Paso 6: Crear Blueprint del Enemigo Normal

1. En Content Browser: **Click derecho → Blueprint Class**
2. Selecciona `NormalEnemy` como padre
3. Nombrar: `BP_NormalEnemy`
4. Configura en el Blueprint:

#### Pestaña Class Defaults:
```
Enemy|AI:
  - Behavior Tree: BT_NormalEnemy

Enemy|Combat:
  - Min Attack Distance: 100
  - Max Attack Distance: 150
  - Positioning Distance: 300
  - Min Positioning Time: 1.5
  - Max Positioning Time: 3.5
  - Base Damage: 10
  - Attack Cooldown: 1.5
  - Ally Detection Radius: 1500
  - Min Allies For Aggression: 2

Enemy|Perception:
  - Sight Radius: 2000
  - Peripheral Vision Angle: 75
  - Hearing Radius: 1000
  - Proximity Radius: 250
  - Lose Sight Time: 5
  - Investigation Time: 10
  - Investigation Radius: 400

Enemy|Patrol:
  - Patrol Speed Multiplier: 0.4
  - Chase Speed Multiplier: 1.0
  - Wait Time At Patrol Point: 2
  - Max Wait Time At Patrol Point: 4
  - Patrol Point Acceptance Radius: 100
  - Random Patrol: false

Enemy|Behavior (AAA Natural Behavior):
  - Chance To Stop During Patrol: 0.15
  - Min Random Pause Duration: 0.5
  - Max Random Pause Duration: 2.0
  - Look Around Speed: 60
  - Max Look Around Angle: 90
  - Chance To Look Around: 0.4
  - Patrol Speed Variation: 0.15
  - Reaction Time Min: 0.2
  - Reaction Time Max: 0.6
  - Suspicion Threshold Investigate: 0.3
  - Suspicion Threshold Chase: 0.7
  - Suspicion Build Up Rate: 0.5
  - Suspicion Decay Rate: 0.2

Enemy|Stats:
  - Max Health: 100
```

### Paso 7: Colocar Enemigos en el Nivel

1. Arrastra `BP_NormalEnemy` al nivel
2. En el panel de Detalles:
   - Asigna un `PatrolPath` existente
3. Repite para múltiples enemigos

### Paso 8: Configurar NavMesh

1. **Place Actors → Volumes → Nav Mesh Bounds Volume**
2. Escala el volumen para cubrir toda el área jugable
3. **Build → Build Paths** (o presiona P para visualizar)

### Paso 9: Configurar el Jugador como Detectable

El jugador debe ser detectable por el sistema de percepción:

1. En el Blueprint del jugador, añade un componente `AIPerceptionStimuliSource`
2. En el componente, habilita:
   - `Auto Register as Source`: true
   - `Register as Source for Senses`: Sight, Hearing

---

## 🎮 Estados del Enemigo

| Estado | Descripción |
|--------|-------------|
| `Idle` | Sin actividad, esperando |
| `Patrolling` | Patrullando entre puntos |
| `Investigating` | Investigando última ubicación conocida |
| `Chasing` | Persiguiendo al jugador |
| `Positioning` | Posicionándose a distancia para atacar |
| `Attacking` | Ejecutando ataque |
| `Taunting` | Provocando al jugador |
| `Dead` | Muerto |

---

## 🎯 Sistema de Comportamiento Natural (AAA-Style)

### Sistema de Sospecha Gradual

En lugar de detectar instantáneamente al jugador, el enemigo usa un sistema de sospecha gradual:

```
0.0 ──────────────── 0.3 ──────────────── 0.7 ──────────────── 1.0
 │                    │                    │                    │
 └── Tranquilo       └── Alerta          └── Investiga        └── Persigue
```

**Flujo de detección:**
1. **Ver al jugador** → La sospecha aumenta gradualmente (`SuspicionBuildUpRate`)
2. **Umbral 0.3** → Estado de alerta (animación de alerta)
3. **Umbral 0.7** → Comienza investigación
4. **Umbral 1.0** → Persecución total
5. **Perder de vista** → La sospecha decae gradualmente (`SuspicionDecayRate`)

**Tiempo de reacción:** Al alcanzar el umbral de persecución, hay un pequeño delay (0.2-0.6 seg) antes de reaccionar, simulando el tiempo que tarda en "procesar" la información.

### Comportamiento Idle Natural

Durante la patrulla, el enemigo realiza acciones que lo hacen parecer más natural:

| Comportamiento | Descripción |
|---------------|-------------|
| **Pausas aleatorias** | Se detiene brevemente durante la patrulla |
| **Mirar alrededor** | Gira la cabeza en direcciones aleatorias |
| **Velocidad variable** | Pequeña variación en la velocidad de patrulla |
| **Esperas variadas** | Tiempo de espera diferente en cada punto de patrulla |

### Blueprint Events para Animaciones

El sistema expone eventos para conectar animaciones en Blueprint:

| Evento | Cuándo se dispara |
|--------|-------------------|
| `OnRandomPauseStarted` | Al iniciar una pausa aleatoria |
| `OnRandomPauseEnded` | Al terminar una pausa aleatoria |
| `OnLookAroundStarted` | Al empezar a mirar alrededor |
| `OnSuspicionChanged` | Cuando el nivel de sospecha cambia significativamente |
| `OnShowConfusion` | Durante investigación (para animación de confusión) |

### BTTask: IdleBehavior

Nuevo nodo de Behavior Tree para pausas aleatorias:

```
[Task: IdleBehavior]
├── PauseChance: 0.3 (30% probabilidad de pausar)
├── MinPauseDuration: 0.5s
├── MaxPauseDuration: 2.0s
├── bLookAroundDuringPause: true
└── bUseEnemyConfig: true (usa BehaviorConfig del enemigo)
```

Úsalo entre `FindPatrolPoint` y `MoveToLocation` para pausas durante la patrulla.

---

## 🧠 Lógica de Comportamiento

### Flujo Principal
```
1. PATRULLA
   ↓ (detecta jugador)
2. PERSECUCIÓN
   ↓ (llega a distancia de posicionamiento)
3. POSICIONAMIENTO (strafe, espera)
   ↓ (tiempo cumplido)
4. APROXIMACIÓN (se acerca rápido)
   ↓ (en rango de ataque)
5. ATAQUE
   ↓ (cooldown)
   → Vuelve a 2 o 3
```

### Pérdida del Objetivo
```
PERSECUCIÓN/COMBATE
   ↓ (pierde visión por X segundos)
INVESTIGACIÓN (busca en área)
   ↓ (tiempo de investigación termina)
PATRULLA (vuelve a ruta)
```

### Coordinación de Aliados
- Al detectar jugador → Alerta a enemigos cercanos
- Más aliados = Más agresivo (menos distancia, menos espera)
- Pocos aliados = Más cauteloso (más distancia, más espera)
- Taunt más probable con aliados cerca

---

## 🔌 Extensibilidad

### Crear un Nuevo Tipo de Enemigo

1. Crea una nueva clase C++ heredando de `AEnemyBase`:

```cpp
UCLASS()
class AChargerEnemy : public AEnemyBase
{
    GENERATED_BODY()
    
public:
    virtual void Attack() override;
    // Implementa carga a distancia
};
```

2. Override los métodos virtuales según necesites:
   - `Attack()` - Lógica de ataque personalizada
   - `PerformTaunt()` - Taunt personalizado
   - `ShouldTaunt()` - Condiciones de taunt
   - `OnStateEnter()` / `OnStateExit()` - Comportamiento por estado
   - `HandleCombatBehavior()` - Lógica de combate por frame

### Crear Nuevos BTTaskNodes

1. Hereda de `UBTTaskNode`
2. Implementa `ExecuteTask()` como mínimo
3. Para tareas con duración, implementa `TickTask()` y retorna `InProgress`

---

## ⚠️ Troubleshooting

### El enemigo no se mueve
- Verifica que hay un NavMesh válido
- Comprueba que el PatrolPath tiene puntos
- Asegúrate de que el Behavior Tree está asignado

### El enemigo no detecta al jugador
- Verifica que el jugador tiene `AIPerceptionStimuliSource`
- Comprueba los radios de percepción
- Asegúrate de que el AIController está configurado

### El enemigo se queda en un estado
- Añade logs en los BTTaskNodes para debug
- Verifica las condiciones de los Decorators
- Comprueba que el Blackboard está configurado correctamente

### El enemigo llega al primer punto pero no continúa

**Verificar en Output Log** (Window → Developer Tools → Output Log):
- Deberías ver: `FindPatrolPoint: Enemy going to point X/Y at (location)`
- Luego: `MoveToLocation: Enemy starting move to (location)`
- Luego: `MoveToLocation: Enemy reached destination`
- Luego: `WaitAtPatrolPoint: Enemy starting wait for X seconds`
- Luego: `WaitAtPatrolPoint: Enemy finished waiting`
- Y volver a FindPatrolPoint

**Si no ves estos mensajes:**
1. **Verifica que MoveToLocation termina**: Si se queda en "Moving", puede ser problema de NavMesh
2. **Verifica el NavMesh**: `P` en el editor para visualizar. El área debe estar verde.
3. **Verifica el AcceptanceRadius**: En MoveToLocation node, valor por defecto es 100. Puede ser muy grande o muy pequeño.

**Solución rápida - Verificar NavMesh:**
1. En el nivel, ve a Window → World Settings
2. Busca "Navigation" y verifica que hay un NavMesh configurado
3. Presiona `P` en el viewport para ver el NavMesh (área verde = navegable)
4. Si no hay NavMesh, añade un `NavMeshBoundsVolume` que cubra tu nivel

---

## 📝 Notas Adicionales

- **Animaciones**: Conectar en Blueprint usando los eventos `OnEnemyStateChanged`
- **Efectos**: Usar los delegates para spawnar VFX/SFX
- **UI**: El health percent está disponible via `GetHealthPercent()`
- **Damage**: Implementa `TakeDamageFromSource()` o usa el sistema de daño de UE

---

## 🗂️ Enemigos Futuros (No Implementados)

### ChargerEnemy
- Detecta a distancia X
- Carga contra el jugador con fuerza
- Override `Attack()` con lógica de carga

### SpawnerEnemy
- Se mantiene a distancia
- Cada X tiempo spawnea enemigos normales
- Nuevo BTTask para spawn

---

*Documento para SairanSkies - Sistema de Animación de Enemigos*

---

## 📚 Documentación Relacionada

- **[Animation_Setup_Guide.md](Animation_Setup_Guide.md)** - Guía completa para configurar Animation Blueprints con el sistema de Look At, Turn In Place, y transiciones suaves.

---

*Documento generado para el proyecto SairanSkies*

