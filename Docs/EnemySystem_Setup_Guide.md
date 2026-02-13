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
3. Añadir las siguientes Keys:

| Nombre | Tipo | Descripción |
|--------|------|-------------|
| `TargetActor` | Object (Actor) | El jugador detectado |
| `TargetLocation` | Vector | Última ubicación conocida del objetivo |
| `EnemyState` | Enum (EEnemyState) | Estado actual del enemigo |
| `CanSeeTarget` | Bool | Si puede ver al objetivo actualmente |
| `PatrolIndex` | Int | Índice actual del punto de patrulla |
| `ShouldTaunt` | Bool | Si debería hacer taunt |
| `NearbyAllies` | Int | Número de aliados cercanos |
| `DistanceToTarget` | Float | Distancia al objetivo |
| `PatrolLocation` | Vector | Ubicación del punto de patrulla actual |

### Paso 3: Crear el Behavior Tree

1. En el Content Browser: **Click derecho → Artificial Intelligence → Behavior Tree**
2. Nombrar: `BT_NormalEnemy`
3. Asignar el Blackboard `BB_Enemy`

### Paso 4: Estructura del Behavior Tree

```
[ROOT]
└── [Selector] - Nodo raíz
    │
    ├── [Service: UpdateEnemyState]
    │
    ├── [Sequence] "Combat" ─ [Decorator: HasTarget]
    │   ├── [Selector] "Combat Actions"
    │   │   ├── [Sequence] "Attack Sequence"
    │   │   │   ├── [Task: ChaseTarget] (bUsePositioningDistance = true)
    │   │   │   ├── [Task: PositionForAttack]
    │   │   │   ├── [Task: ApproachForAttack]
    │   │   │   └── [Task: AttackTarget]
    │   │   │
    │   │   └── [Task: PerformTaunt] ─ [Decorator: CheckEnemyState = Taunting]
    │   │
    │   └── [Task: ChaseTarget] (fallback)
    │
    ├── [Sequence] "Investigation" ─ [Decorator: CheckEnemyState = Investigating]
    │   └── [Task: Investigate]
    │
    └── [Sequence] "Patrol" ─ [Decorator: HasTarget (inverse)]
        ├── [Task: FindPatrolPoint]
        ├── [Task: MoveToLocation] (PatrolLocation)
        └── [Task: WaitAtPatrolPoint]
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
  - Patrol Point Acceptance Radius: 100
  - Random Patrol: false

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

*Documento generado para el proyecto SairanSkies*

