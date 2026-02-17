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
| `EnemyState` | Int | Estado actual del enemigo (0=Idle, 1=Patrolling, 2=Investigating, 3=Chasing, 4=Positioning, 5=Attacking, 6=Taunting, 7=Conversing, 8=Dead) |
| `CanSeeTarget` | Bool | Si puede ver al objetivo actualmente |
| `PatrolIndex` | Int | Índice actual del punto de patrulla |
| `ShouldTaunt` | Bool | Si debería hacer taunt |
| `NearbyAllies` | Int | Número de aliados cercanos |
| `DistanceToTarget` | Float | Distancia al objetivo |
| `SuspicionLevel` | Float | Nivel de sospecha (0-1) |
| `IsAlerted` | Bool | Si está en estado de alerta |
| `IsInPause` | Bool | Si está en pausa aleatoria |
| `IsConversing` | Bool | Si está en conversación con otro enemigo |
| `ConversationPartner` | Object (Actor) | El enemigo con quien está conversando |

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

Enemy|Animation (Montajes):
  - AttackMontages: [Array de montajes de ataque]
  - HeavyAttackMontages: [Array de ataques pesados]
  - HitReactionMontages: [Array de reacciones a golpes]
  - DeathMontage: Montaje de muerte
  - TauntMontages: [Array de provocaciones]
  - ConversationGestureMontages: [Array de gestos para conversación]
  - IdleMontages: [Array de idles variados]
  - LookAroundMontage: Montaje de mirar alrededor

Enemy|Sound:
  - AttackSounds: [Array de sonidos de ataque]
  - HitSounds: [Array de sonidos de impacto]
  - DeathSounds: [Array de sonidos de muerte]
  - PainSounds: [Array de sonidos de dolor]
  - TauntVoices: [Array de voces de taunt]
  - AlertVoices: [Array de voces de alerta]
  - ConversationVoices: [Array de voces de conversación]
  - LaughSounds: [Array de risas]
  - FootstepSounds: [Array de pisadas]
  - VoiceVolume: 1.0
  - SFXVolume: 1.0
  - MinTimeBetweenVoices: 3.0

Enemy|VFX:
  - HitEffect: Efecto Niagara de impacto
  - BloodEffect: Efecto Niagara de sangre
  - DeathEffect: Efecto Niagara de muerte
  - AlertEffect: Efecto de alerta (!)
  - ConfusionEffect: Efecto de confusión (?)

Enemy|Mesh:
  - BodyMesh: Skeletal mesh del cuerpo
  - WeaponMeshes: [Array de meshes de armas]
  - ArmorVariants: [Array de variantes de armadura]
  - BodyMaterials: [Array de materiales]
  - DamageMaterial: Material al recibir daño

Enemy|Sockets:
  - RightHandSocket: "hand_r_socket"
  - LeftHandSocket: "hand_l_socket"
  - HeadSocket: "head_socket"
  - VoiceSocket: "head"
  - HitEffectSocket: "pelvis"

Enemy|Conversation:
  - SamePointRadius: 150
  - TimeBeforeConversation: 2.0
  - MinConversationDuration: 5.0
  - MaxConversationDuration: 15.0
  - ChanceToGesture: 0.4
  - ChanceToLaugh: 0.2
  - bLookAtPartner: true
  - ConversationCooldown: 30.0
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
| `Conversing` | **NUEVO** Conversando con otro enemigo |
| `Dead` | Muerto |

---

## 💬 Sistema de Conversación entre Enemigos

### Descripción

Cuando dos enemigos se encuentran en el mismo punto de patrulla y están parados, pueden entrar en conversación. Esta característica añade vida y realismo al mundo.

### Cómo Funciona

```
┌─────────────────────────────────────────────────────────────────┐
│              FLUJO DE CONVERSACIÓN                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. Enemigo A llega a punto de patrulla                         │
│  2. Enemigo B ya está en el mismo punto (o llega)               │
│  3. Ambos están parados por X segundos (TimeBeforeConversation) │
│  4. Se detectan mutuamente (dentro de SamePointRadius)          │
│  5. Enemigo A inicia conversación (TryStartConversation)        │
│  6. Enemigo B se une (JoinConversation)                         │
│  7. Ambos entran en estado "Conversing"                         │
│  8. Se miran mutuamente (Look At)                               │
│  9. Gestos y voces aleatorias durante la conversación           │
│  10. Tras la duración, terminan y vuelven a patrullar           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Configuración en Blueprint

En el Blueprint del enemigo, configura `ConversationConfig`:

| Propiedad | Valor por Defecto | Descripción |
|-----------|-------------------|-------------|
| `ConversationDetectionRadius` | 200 | Radio para detectar otro enemigo |
| `SamePointRadius` | 150 | Distancia para considerar "mismo punto" |
| `TimeBeforeConversation` | 2.0 | Segundos parados antes de conversar |
| `MinConversationDuration` | 5.0 | Duración mínima de conversación |
| `MaxConversationDuration` | 15.0 | Duración máxima de conversación |
| `GestureInterval` | 2.0 | Segundos entre gestos |
| `ChanceToGesture` | 0.4 | Probabilidad de hacer gesto |
| `ChanceToLaugh` | 0.2 | Probabilidad de reír |
| `bLookAtPartner` | true | Mirar al compañero |
| `bCanBeInterrupted` | true | Se puede interrumpir por jugador |
| `ConversationCooldown` | 30.0 | Cooldown antes de otra conversación |

### Assets Necesarios

Para que las conversaciones funcionen correctamente, configura:

**AnimationConfig (Conversación):**
- `ConversationIdleMontages[]` - Montajes de idle durante conversación
- `ConversationGestureMontages[]` - Montajes de gestos (señalar, encogerse de hombros)
- `ConversationStartMontage` - Al iniciar conversación
- `ConversationEndMontage` - Al terminar conversación

**SoundConfig (Conversación):**
- `ConversationVoices[]` - Voces de conversación genéricas
- `LaughSounds[]` - Sonidos de risa
- `AgreementSounds[]` - Sonidos de asentimiento

### Eventos de Blueprint

| Evento | Cuándo se dispara |
|--------|-------------------|
| `OnConversationStartedEvent(Partner)` | Al iniciar conversación |
| `OnConversationEndedEvent()` | Al terminar conversación |
| `OnConversationGesture()` | Al hacer un gesto |

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

**1. Verificar AIPerceptionStimuliSource en el jugador:**
- En el Blueprint del jugador, añade un componente `AIPerceptionStimuliSource`
- En el componente:
  - `Auto Register as Source`: **true**
  - `Register as Source for Senses`: **AISense_Sight** (importante!)

**2. Verificar logs de debug:**
- Abre Output Log: Window → Developer Tools → Output Log
- Deberías ver al iniciar:
  - `SetupPerceptionSystem: Sight configured - Radius: X, Angle: Y`
  - `SetupPerceptionSystem: BP_NormalEnemy perception system ready`
- Cuando el jugador está en rango:
  - `AIController OnPerception: BP_NormalEnemy detected BP_ThirdPersonCharacter (Success: YES)`

**3. Si no se detecta nada:**
- El sistema ahora usa **Team IDs** para determinar quién detectar
- Los enemigos son Team 1, el jugador debería ser Team 0 o sin team (Neutral)
- El sistema detecta Enemies y Neutrals por defecto

**4. Verificar percepción visual:**
- En el editor, selecciona el enemigo
- Ve a Debug → AI Debugging
- Activa "Perception" para ver los radios de detección

**5. Problemas comunes:**
- El jugador está fuera del `SightRadius` configurado
- El jugador está fuera del `PeripheralVisionAngle`
- Hay obstáculos bloqueando la línea de visión (Line of Sight)
- El componente `AIPerceptionStimuliSource` no tiene el sense correcto activado

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

## 🎮 Funcionamiento Completo del Enemigo

### Diagrama de Estados

```
                    ┌──────────────────────────────────────────────────────────┐
                    │                         SPAWN                            │
                    └──────────────────────────────┬───────────────────────────┘
                                                   │
                                                   ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                    IDLE / PATROLLING                     │
                    │                                                          │
                    │  • Recorre PatrolPath (si existe)                        │
                    │  • Pausas aleatorias (ChanceToStopDuringPatrol)          │
                    │  • Mira alrededor (ChanceToLookAround)                   │
                    │  • Velocidad variable (PatrolSpeedVariation)             │
                    └────────────┬─────────────────────────────────────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │   ¿Detecta al jugador?   │
                    └────────────┬────────────┘
                                 │ SÍ (Sospecha > 0.7)
                                 ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                      CHASING                             │
                    │                                                          │
                    │  • Velocidad aumentada (ChaseSpeedMultiplier)            │
                    │  • Persigue al jugador hasta PositioningDistance        │
                    │  • Alerta a aliados cercanos (AlertNearbyAllies)         │
                    │  • Actualiza LastKnownTargetLocation                     │
                    └────────────┬─────────────────────────────────────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │  ¿En rango de combate?   │
                    └────────────┬────────────┘
                                 │ SÍ
                                 ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                    POSITIONING                           │
                    │                                                          │
                    │  • Mantiene distancia (PositioningDistance)              │
                    │  • Strafe lateral (ChanceToStrafe)                       │
                    │  • Duración: MinPositioningTime - MaxPositioningTime     │
                    │  • Evaluación: ¿Hacer taunt? ¿Atacar?                    │
                    └────────────┬─────────────────────────────────────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
    ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
    │    TAUNTING     │ │ APPROACH+ATTACK │ │    HESITATE     │
    │                 │ │                 │ │                 │
    │ • Con aliados   │ │ • Se acerca     │ │ • Sin aliados   │
    │ • Probabilidad  │ │ • MaxAttackDist │ │ • Más cauteloso │
    │ • Animación     │ │ • Aplica daño   │ │                 │
    └────────┬────────┘ └────────┬────────┘ └────────┬────────┘
             │                   │                   │
             └───────────────────┴───────────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │  ¿Perdió al jugador?     │
                    └────────────┬────────────┘
                                 │ SÍ (LoseSightTime)
                                 ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │                   INVESTIGATING                          │
                    │                                                          │
                    │  • Va a LastKnownTargetLocation                          │
                    │  • Busca en área (InvestigationRadius)                   │
                    │  • Mira alrededor buscando                               │
                    │  • Puede mostrar confusión (ChanceToShowConfusion)       │
                    │  • Duración: InvestigationTime                           │
                    └────────────┬─────────────────────────────────────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │  ¿Tiempo agotado?        │
                    └────────────┬────────────┘
                                 │ SÍ
                                 ▼
                    ┌──────────────────────────────────────────────────────────┐
                    │               RETURN TO PATROL                           │
                    │                                                          │
                    │  • Vuelve al PatrolPath                                  │
                    │  • Sospecha decae gradualmente                           │
                    │  • Reanuda comportamiento normal                         │
                    └──────────────────────────────────────────────────────────┘
```

### Propiedades Modulares del Enemigo

El sistema está diseñado para ser completamente modular. Todas las propiedades son accesibles desde Blueprint:

```cpp
// ===== ACCESO A STATS =====
Enemy->MaxHealth                           // Vida máxima
Enemy->GetHealthPercent()                  // Porcentaje de vida (0-1)
Enemy->CurrentHealth                       // Vida actual (protected, usar GetHealthPercent)
Enemy->IsDead()                            // ¿Está muerto?

// ===== CONFIGURACIÓN DE COMBATE =====
Enemy->CombatConfig.BaseDamage             // Daño base
Enemy->CombatConfig.AttackCooldown         // Cooldown entre ataques
Enemy->CombatConfig.MinAttackDistance      // Distancia mínima de ataque
Enemy->CombatConfig.MaxAttackDistance      // Distancia máxima de ataque
Enemy->CombatConfig.PositioningDistance    // Distancia de posicionamiento
Enemy->CombatConfig.MinPositioningTime     // Tiempo mínimo posicionándose
Enemy->CombatConfig.MaxPositioningTime     // Tiempo máximo posicionándose
Enemy->CombatConfig.AllyDetectionRadius    // Radio para detectar aliados
Enemy->CombatConfig.MinAlliesForAggression // Aliados mínimos para ser agresivo

// ===== CONFIGURACIÓN DE PERCEPCIÓN =====
Enemy->PerceptionConfig.SightRadius        // Radio de visión
Enemy->PerceptionConfig.PeripheralVisionAngle // Ángulo de visión
Enemy->PerceptionConfig.HearingRadius      // Radio de audición
Enemy->PerceptionConfig.ProximityRadius    // Radio de detección por proximidad
Enemy->PerceptionConfig.LoseSightTime      // Tiempo para perder al objetivo
Enemy->PerceptionConfig.InvestigationTime  // Tiempo de investigación
Enemy->PerceptionConfig.InvestigationRadius // Radio de investigación

// ===== CONFIGURACIÓN DE PATRULLA =====
Enemy->PatrolConfig.PatrolSpeedMultiplier  // Multiplicador de velocidad al patrullar
Enemy->PatrolConfig.ChaseSpeedMultiplier   // Multiplicador de velocidad al perseguir
Enemy->PatrolConfig.WaitTimeAtPatrolPoint  // Tiempo de espera en cada punto
Enemy->PatrolConfig.PatrolPointAcceptanceRadius // Radio de aceptación

// ===== CONFIGURACIÓN DE COMPORTAMIENTO =====
Enemy->BehaviorConfig.ChanceToStopDuringPatrol // Probabilidad de pausar
Enemy->BehaviorConfig.ChanceToLookAround   // Probabilidad de mirar alrededor
Enemy->BehaviorConfig.ReactionTimeMin/Max  // Tiempo de reacción
Enemy->BehaviorConfig.SuspicionThresholdChase // Umbral para perseguir

// ===== ESTADO ACTUAL =====
Enemy->GetEnemyState()                     // Estado actual (EEnemyState)
Enemy->IsInCombat()                        // ¿Está en combate?
Enemy->GetCurrentTarget()                  // Objetivo actual
Enemy->GetDistanceToTarget()               // Distancia al objetivo
Enemy->GetSuspicionLevel()                 // Nivel de sospecha (0-1)
Enemy->IsAlerted()                         // ¿Está en alerta?

// ===== MÉTODOS DE ACCIÓN =====
Enemy->SetTarget(Actor, SenseType)         // Establecer objetivo
Enemy->Attack()                            // Ejecutar ataque
Enemy->TakeDamageFromSource(Damage, Source, Controller) // Recibir daño
Enemy->Die(Controller)                     // Morir
Enemy->SetEnemyState(NewState)             // Cambiar estado
Enemy->AlertNearbyAllies(Target)           // Alertar aliados
```

### Eventos Disponibles (Delegates)

```cpp
OnEnemyStateChanged(EEnemyState OldState, EEnemyState NewState)
OnPlayerDetected(AActor* Player, EEnemySenseType SenseType)
OnPlayerLost()
OnEnemyDeath(AController* InstigatorController)
OnRandomPauseStarted()    // Blueprint Implementable Event
OnRandomPauseEnded()      // Blueprint Implementable Event
OnLookAroundStarted()     // Blueprint Implementable Event
OnSuspicionChanged(float NewLevel, float OldLevel) // Blueprint Implementable Event
OnShowConfusion()         // Blueprint Implementable Event
```

---

## 🗡️ Sistema Nemesis Simplificado

### Concepto Adaptado para SairanSkies

Una versión simplificada del Sistema Nemesis de Shadow of Mordor, adaptada a un juego con **mini-enemigos** (enemigos comunes) y un **jefe final** (boss).

---

### 🎯 Estructura de Enemigos

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESTRUCTURA DEL JUEGO                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌───────────────┐                          │
│                      │  JEFE FINAL   │  ← Programación especial │
│                      │   (BOSS)      │     Múltiples fases      │
│                      └───────┬───────┘     Sistema Nemesis      │
│                              │                                  │
│         ┌────────────────────┼────────────────────┐             │
│         │                    │                    │             │
│         ▼                    ▼                    ▼             │
│   ┌───────────┐        ┌───────────┐        ┌───────────┐       │
│   │   MINI    │        │   MINI    │        │   MINI    │       │
│   │ ENEMIGO 1 │        │ ENEMIGO 2 │        │ ENEMIGO 3 │       │
│   │ (Grunt)   │        │ (Grunt)   │        │ (Grunt)   │       │
│   └───────────┘        └───────────┘        └───────────┘       │
│         │                    │                    │             │
│         └────────────────────┴────────────────────┘             │
│                              │                                  │
│                    Enemigos genéricos                           │
│                    Sin memoria/identidad                        │
│                    Spawneo infinito                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

### 👾 Mini-Enemigos (Grunts)

Los mini-enemigos son **enemigos genéricos** sin identidad única. Usan el sistema actual de `EnemyBase`.

#### Características

| Aspecto | Descripción |
|---------|-------------|
| **Identidad** | Sin nombre, genéricos |
| **Comportamiento** | Patrullar, detectar, perseguir, atacar |
| **Memoria** | NO recuerdan encuentros |
| **Spawneo** | Infinito, respawnean |
| **Dificultad** | Baja individualmente, peligrosos en grupo |

#### Tipos de Mini-Enemigos

| Tipo | Comportamiento | Stats |
|------|---------------|-------|
| **Normal** | Equilibrado | 100 HP, daño medio |
| **Rápido** | Veloz pero débil | 50 HP, daño bajo, velocidad alta |
| **Pesado** | Lento pero fuerte | 200 HP, daño alto, velocidad baja |
| **A Distancia** | Ataca desde lejos | 75 HP, proyectiles |

---

### 👹 Jefe Final (Boss) - Sistema Nemesis

El **jefe final** es el único enemigo con el sistema Nemesis completo. Es un enemigo **único y memorable** que evoluciona según las interacciones con el jugador.

#### Características del Boss

| Aspecto | Descripción |
|---------|-------------|
| **Identidad** | Nombre único + Título dinámico |
| **Memoria** | Recuerda TODOS los encuentros |
| **Evolución** | Cambia según la historia |
| **Fases** | Múltiples fases de combate |
| **Persistencia** | Datos guardados entre sesiones |

---

### 🎭 Identidad del Boss

El jefe tiene una identidad que puede ser **fija** (diseñada) o **procedural**.

#### Opción A: Identidad Fija (Recomendado para juego narrativo)

```
Nombre:     "Kael"
Título:     "el Destructor" (puede cambiar según eventos)
Historia:   Rival del protagonista desde el inicio
```

#### Opción B: Identidad Semi-Procedural

| Componente | Base | Evolución |
|------------|------|-----------|
| **Nombre** | Fijo: "Kael" | No cambia |
| **Título** | Inicial: "el Destructor" | Cambia según eventos (ver abajo) |

#### Evolución del Título

| Evento | Nuevo Título |
|--------|--------------|
| Mata al jugador 1 vez | "el Asesino de [Nombre Jugador]" |
| Mata al jugador 3+ veces | "el Verdugo" |
| El jugador lo derrota | "el Derrotado" → "el Vengador" |
| Sobrevive a fuego | "el Quemado" |
| Escapa del jugador | "el Cobarde" → "el Superviviente" |

---

### 📜 Sistema de Memoria del Boss

El boss recuerda cada encuentro con el jugador:

#### Datos que Recuerda

| Dato | Efecto |
|------|--------|
| **Veces que mató al jugador** | +Poder, +Confianza, diálogos burlones |
| **Veces que fue derrotado** | +Cautela o +Ira, cicatrices visibles |
| **Cómo fue derrotado** | Puede ganar resistencia a ese tipo de daño |
| **Si el jugador huyó** | Diálogos despectivos, menos respeto |
| **Tiempo desde último encuentro** | "Ha pasado mucho tiempo..." |

#### Estados de Relación

```
┌─────────────────────────────────────────────────────────────────┐
│                   RELACIÓN BOSS ↔ JUGADOR                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  DESPRECIO ──────► RIVALIDAD ──────► OBSESIÓN ──────► RESPETO   │
│      │                 │                 │                │     │
│      │                 │                 │                │     │
│  "Eres un             "Empiezas         "¡NO PUEDES      "Eres  │
│   insecto"             a molestar"       ESCAPAR!"        digno"│
│                                                                 │
│  • Primeros           • Múltiples       • Muchos         • Boss │
│    encuentros           encuentros        encuentros       casi │
│  • El jugador         • Victorias       • Obsesionado      muere│
│    es débil             y derrotas        contigo              │
│                         mezcladas                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

### ⚔️ Evolución del Boss

#### Fortalezas Ganadas

El boss puede ganar fortalezas basadas en eventos:

| Evento | Fortaleza Ganada |
|--------|-----------------|
| Sobrevive a ataque de fuego | **Resistencia al Fuego** (-50% daño) |
| El jugador usa mucho sigilo | **Alerta Mejorada** (detecta sigilo) |
| Derrotado por combos | **Rompe-Combos** (interrumpe cadenas) |
| Mata al jugador con ataque X | **Maestro de X** (ese ataque es más fuerte) |

#### Debilidades (Fijas o Descubribles)

| Debilidad | Cómo Descubrirla | Efecto |
|-----------|------------------|--------|
| **Punto Débil** | Observar animaciones | x2 daño en cierta parte |
| **Miedo a X** | Intel de mini-enemigos | Huye/stun temporal |
| **Patrón Predecible** | Múltiples encuentros | Jugador aprende el timing |

---

### 🎬 Sistema de Fases del Boss

El combate final tiene **múltiples fases**, cada una más difícil:

```
┌─────────────────────────────────────────────────────────────────┐
│                    FASES DEL BOSS FINAL                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐        │
│  │   FASE 1    │────►│   FASE 2    │────►│   FASE 3    │        │
│  │   100-70%   │     │   70-30%    │     │   30-0%     │        │
│  └─────────────┘     └─────────────┘     └─────────────┘        │
│        │                   │                   │                │
│        ▼                   ▼                   ▼                │
│  • Ataques básicos   • Nuevos ataques    • Desesperado          │
│  • Aprende al        • Llama refuerzos   • Todos los ataques    │
│    jugador           • Más agresivo      • Más rápido           │
│  • Diálogo inicial   • Diálogo medio     • Diálogo final        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### Contenido por Fase

| Fase | HP | Comportamiento | Diálogo Ejemplo |
|------|-----|----------------|-----------------|
| **Fase 1** | 100-70% | Cauteloso, prueba al jugador | "Veamos de qué estás hecho" |
| **Fase 2** | 70-30% | Agresivo, usa más habilidades | "¡No eres rival para mí!" |
| **Fase 3** | 30-0% | Desesperado, todo o nada | "¡Si caigo, TÚ CAES CONMIGO!" |

---

### 💬 Diálogos Contextuales del Boss

#### Por Número de Encuentros

| Encuentro | Diálogo |
|-----------|---------|
| **1º encuentro** | "¿Quién eres tú? Otro héroe que cree que puede detenerme." |
| **2º encuentro** | "Volviste... Creí que habías aprendido la lección." |
| **3º+ encuentro** | "Tú otra vez. Esto empieza a ser personal." |
| **Muchos encuentros** | "¡SIEMPRE TÚ! ¡No importa cuántas veces, sigues volviendo!" |

#### Por Resultado Anterior

| Situación | Diálogo |
|-----------|---------|
| **Boss ganó antes** | "¿Ya olvidaste cómo te derroté? Puedo refrescarte la memoria." |
| **Jugador ganó antes** | "La última vez tuviste suerte. Esta vez será diferente." |
| **Jugador huyó antes** | "¿Vienes a huir de nuevo? No te culpo, es lo más inteligente." |
| **Boss casi muere antes** | "Casi me matas... CASI. No cometeré ese error de nuevo." |

#### Por Estado del Boss

| Estado | Diálogo |
|--------|---------|
| **Con cicatrices** | "¿Ves estas marcas? TÚ me las hiciste. Y por cada una, te haré pagar." |
| **Más fuerte** | "Cada vez que caes, me hago más fuerte. ¿No lo entiendes?" |
| **Fase final (poca vida)** | "¡NO! ¡No termina así! ¡NO PUEDE TERMINAR ASÍ!" |

---

### 💀 Sistema de "Muerte" y Retorno

#### ¿Puede el Boss Volver?

Para un juego con boss final, hay dos opciones:

**Opción A: Boss Recurrente (Recomendado)**
```
• El boss aparece varias veces durante el juego
• Cada derrota → escapa y vuelve más fuerte
• El jugador lo derrota "de verdad" solo al final
• Cada encuentro: cicatrices, más poder, más odio
```

**Opción B: Boss Una Vez**
```
• El boss solo aparece al final
• La memoria se basa en mini-encuentros previos (cutscenes, menciones)
• Una pelea épica, muerte permanente
```

#### Cicatrices Visuales (Si vuelve)

| Causa de "Derrota" | Cicatriz | Efecto Visual |
|-------------------|----------|---------------|
| Daño de fuego | Quemaduras | Piel quemada, armadura derretida |
| Daño físico | Cortes | Vendajes, parches de metal |
| Caída | Huesos rotos | Cojea, usa bastón/muleta |
| Casi ahogado | Trauma | Tos, respiración agitada |

---

### 🎮 Loop de Juego Simplificado

```
┌─────────────────────────────────────────────────────────────────┐
│              LOOP DE JUEGO - NEMESIS SIMPLIFICADO               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 1. EXPLORAR NIVEL                                        │   │
│  │    • Combatir mini-enemigos                              │   │
│  │    • Encontrar recursos/mejoras                          │   │
│  │    • Descubrir información sobre el Boss                 │   │
│  └──────────────────────────────┬───────────────────────────┘   │
│                                 │                               │
│                                 ▼                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 2. ENCUENTRO CON BOSS (Opcional / Por progresión)        │   │
│  │    • El Boss aparece (emboscada, arena, evento)          │   │
│  │    • Combate con fases                                   │   │
│  │    • Victoria: Boss escapa, jugador progresa             │   │
│  │    • Derrota: Boss se burla, jugador reaparece           │   │
│  └──────────────────────────────┬───────────────────────────┘   │
│                                 │                               │
│                                 ▼                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 3. CONSECUENCIAS                                         │   │
│  │    • Boss evoluciona (nuevo título, fortaleza, cicatriz) │   │
│  │    • Diálogos cambian según resultado                    │   │
│  │    • La dificultad se ajusta                             │   │
│  └──────────────────────────────┬───────────────────────────┘   │
│                                 │                               │
│                                 ▼                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 4. COMBATE FINAL                                         │   │
│  │    • Todas las cicatrices visibles                       │   │
│  │    • Diálogo épico basado en toda la historia            │   │
│  │    • 3 fases de combate                                  │   │
│  │    • Victoria final = Fin del juego                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

### 📊 Comparativa: Mini-Enemigos vs Boss

| Aspecto | Mini-Enemigos | Boss Final |
|---------|---------------|------------|
| **Cantidad** | Muchos (spawneo) | 1 único |
| **Identidad** | Genéricos | Nombre + Título |
| **Memoria** | No | Sí, recuerda todo |
| **Evolución** | No | Sí, gana poder/cicatrices |
| **Dificultad** | Baja-Media | Alta (múltiples fases) |
| **Programación** | `EnemyBase` existente | Sistema especial de Boss |
| **Diálogos** | Sonidos genéricos | Líneas contextuales |
| **Muerte** | Permanente | Puede volver (opcional) |

---

### 🎨 Assets Necesarios

#### Para Mini-Enemigos (Sistema Actual)

| Asset | Cantidad | Notas |
|-------|----------|-------|
| Modelos | 3-4 variantes | Normal, Rápido, Pesado, Distancia |
| Animaciones | Set básico | Idle, Walk, Run, Attack, Death |
| Sonidos | Genéricos | Gruñidos, golpes, muerte |

#### Para Boss Final (Sistema Nemesis)

| Asset | Cantidad | Notas |
|-------|----------|-------|
| Modelo base | 1 | Alto detalle |
| Cicatrices | 4-6 | Quemaduras, cortes, etc. |
| Animaciones | Set completo | Incluyendo fases y ataques especiales |
| Diálogos | 20-30 líneas | Por contexto (victoria, derrota, fases) |
| Música | 2-3 tracks | Por fase del combate |
| VFX | 5-10 | Ataques especiales, transiciones de fase |

---

### ✅ Implementación por Prioridad

#### Fase 1: Prototipo (~1 semana)
- ☐ Mini-enemigos funcionando (ya existe con EnemyBase)
- ☐ Boss básico con 1 fase
- ☐ Sistema de memoria simple (veces derrotado/victorioso)

#### Fase 2: Core (~2 semanas)
- ☐ Boss con 3 fases
- ☐ Diálogos contextuales básicos (5-10 líneas)
- ☐ 2-3 fortalezas ganables
- ☐ UI mostrando nombre/título del Boss

#### Fase 3: Polish (~2+ semanas)
- ☐ Sistema de cicatrices visuales
- ☐ Diálogos completos
- ☐ Evolución de título
- ☐ Persistencia entre sesiones
- ☐ Audio/VFX especiales

---

### 💡 Consejos para Implementación

1. **El Boss es ÚNICO**: Invierte tiempo en hacerlo memorable
2. **Los diálogos son clave**: Una buena línea vale más que 100 stats
3. **Feedback visual**: Las cicatrices deben ser OBVIAS
4. **Escalada de tensión**: Cada encuentro debe sentirse más épico
5. **La derrota del jugador no es castigo**: Es oportunidad para que el Boss evolucione

---

### 📝 Ejemplo de Progresión del Boss

```
ENCUENTRO 1 (Nivel 3):
├── Nombre: "Kael el Destructor"
├── Estado: Desprecio
├── Diálogo: "¿Otro héroe? Qué aburrido."
├── Resultado: Boss gana
└── Evolución: +1 Poder

ENCUENTRO 2 (Nivel 5):
├── Nombre: "Kael, Asesino de Héroes"
├── Estado: Confiado
├── Diálogo: "¿Volviste por más? Creí que habías aprendido."
├── Resultado: Jugador gana (Boss escapa)
└── Evolución: +Cicatriz (corte en cara), +Ira

ENCUENTRO 3 (Nivel 7):
├── Nombre: "Kael el Marcado"
├── Estado: Furioso
├── Diálogo: "¿Ves esto? TÚ me lo hiciste. Pagarás."
├── Resultado: Boss gana
└── Evolución: +2 Poder, +Resistencia a espadas

ENCUENTRO FINAL (Nivel 10):
├── Nombre: "Kael el Eterno"
├── Estado: Obsesión
├── Diálogo: "Esto termina AHORA. Uno de los dos no sale vivo."
├── Resultado: Victoria del jugador (muerte permanente)
└── FIN DEL JUEGO
```

---

*Documento generado para el proyecto SairanSkies*

