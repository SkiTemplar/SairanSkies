# 📚 Documentación del Sistema de Enemigos - Índice Completo
## SairanSkies - Enemy AI System

---

## 📖 Documentos Disponibles

### 1. **EnemySystem_Setup_Guide.md** (Guía Principal)
   - Estructura de archivos del sistema
   - Diagrama de estados del enemigo
   - Sistema de coordinación de ataques
   - Sistema de conversación
   - Comportamientos naturales
   - Variables de configuración detalladas
   - Assets necesarios
   - Pasos de configuración en Unreal Editor
   - Eventos Blueprint disponibles
   - Troubleshooting

### 2. **BTTasks_Technical_Reference.md** (Referencia Técnica)
   - Resumen de todas las 10 tareas implementadas
   - Descripción técnica de cada tarea
   - Entrada y salida de cada tarea
   - Lógica pseudocódigo
   - Parámetros configurables
   - Matriz de cambios de estado
   - Debugging en Behavior Tree

### 3. **BTVisual_Complete_Reference.md** (Diagrama Visual)
   - Estructura visual completa del Behavior Tree
   - Diagrama por niveles
   - Flujo de ejecución detallado
   - Estados de retorno
   - Probabilidades y tiempos
   - Condiciones de decoradores
   - Eventos de interrupción
   - Resumen de estructura

### 4. **BTTasks_Code_Examples.md** (Ejemplos de Código)
   - Estructura base de una BTTask
   - Ejemplos reales implementados
   - Patrón de errores comunes
   - Patrones de implementación

### 5. **GrappleSystem_Setup.md** (Sistema de Gancho)
   - Controles de PC y mando
   - Configuración de Input Action
   - Parámetros configurables del gancho
   - Flujo de la mecánica (Idle → Aiming → Pulling → Releasing)
   - Eventos Blueprint disponibles
   - Cálculo del punto medio
   - Troubleshooting

### 6. **HitDetection_Changes.md** (Sistema de Detección de Hits)
   - Cambios del sistema de detección
   - Flujo de detección con arma
   - Configuración del HitCollision
   - Problemas resueltos

---

## 🎯 Las 10 Tareas del Behavior Tree

```
┌─────────────────────────────────────────────────────────────────┐
│                    BEHAVIOR TREE TASKS                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  GRUPO PATRULLA (4 tareas)                                      │
│  ├─ BTTask_FindPatrolPoint      - Selecciona siguiente punto   │
│  ├─ BTTask_MoveToLocation        - Movimiento genérico         │
│  ├─ BTTask_WaitAtPatrolPoint     - Espera en punto            │
│  └─ BTTask_IdleBehavior          - Comportamiento en pausa    │
│                                                                 │
│  GRUPO COMBATE (4 tareas)                                       │
│  ├─ BTTask_ChaseTarget           - Persigue al jugador        │
│  ├─ BTTask_ApproachForAttack     - Se aproxima                │
│  ├─ BTTask_PositionForAttack     - Strafe/espera turno       │
│  └─ BTTask_AttackTarget          - Ejecuta ataque             │
│                                                                 │
│  GRUPO ESPECIAL (2 tareas)                                      │
│  ├─ BTTask_PerformTaunt          - Burla/Taunt               │
│  └─ BTTask_Investigate           - Investigación              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🗂️ Localización de Archivos

### Headers (.h)
```
Source/SairanSkies/Public/
├── Enemies/
│   ├── EnemyBase.h
│   ├── EnemyTypes.h
│   └── Types/
│       └── NormalEnemy.h
├── AI/
│   ├── EnemyAIController.h
│   ├── Decorators/
│   │   ├── BTDecorator_CheckEnemyState.h
│   │   └── BTDecorator_HasTarget.h
│   ├── Services/
│   │   └── BTService_UpdateEnemyState.h
│   └── Tasks/
│       ├── BTTask_FindPatrolPoint.h
│       ├── BTTask_MoveToLocation.h
│       ├── BTTask_WaitAtPatrolPoint.h
│       ├── BTTask_ChaseTarget.h
│       ├── BTTask_ApproachForAttack.h
│       ├── BTTask_PositionForAttack.h
│       ├── BTTask_AttackTarget.h
│       ├── BTTask_PerformTaunt.h
│       ├── BTTask_Investigate.h
│       └── BTTask_IdleBehavior.h
```

### Implementación (.cpp)
```
Source/SairanSkies/Private/
├── Enemies/
│   └── Types/
│       └── NormalEnemy.cpp
└── AI/
    ├── Decorators/
    │   ├── BTDecorator_CheckEnemyState.cpp
    │   └── BTDecorator_HasTarget.cpp
    ├── Services/
    │   └── BTService_UpdateEnemyState.cpp
    └── Tasks/
        ├── BTTask_FindPatrolPoint.cpp
        ├── BTTask_MoveToLocation.cpp
        ├── BTTask_WaitAtPatrolPoint.cpp
        ├── BTTask_ChaseTarget.cpp
        ├── BTTask_ApproachForAttack.cpp
        ├── BTTask_PositionForAttack.cpp
        ├── BTTask_AttackTarget.cpp
        ├── BTTask_PerformTaunt.cpp
        ├── BTTask_Investigate.cpp
        └── BTTask_IdleBehavior.cpp
```

---

## 🔍 Guía de Lectura

### Para Implementadores/Desarrolladores:
1. Leer: **EnemySystem_Setup_Guide.md** (visión general)
2. Referencia: **BTTasks_Technical_Reference.md** (especificaciones)
3. Código: **BTTasks_Code_Examples.md** (implementación)
4. Debug: **BTVisual_Complete_Reference.md** (flujos)

### Para Game Designers:
1. Leer: **EnemySystem_Setup_Guide.md** (cómo funciona)
2. Referencia: **EnemySystem_Setup_Guide.md** → Variables de Configuración
3. Tuning: Ajustar valores en Blueprint del enemigo

### Para Debuggers:
1. Referencia: **BTVisual_Complete_Reference.md** (flujos)
2. Logs: **BTTasks_Technical_Reference.md** → Debugging
3. Estructura: **BTVisual_Complete_Reference.md** → Eventos de Interrupción

### Para Entender la IA:
1. Leer: **EnemySystem_Setup_Guide.md** → Diagrama de Estados
2. Visual: **BTVisual_Complete_Reference.md** → Diagrama Completo
3. Flujos: **BTVisual_Complete_Reference.md** → Flujo de Ejecución

---

## ⚙️ Variables Clave del Sistema

### Configuración de Combate
```cpp
struct FEnemyCombatConfig
{
    float MinAttackDistance = 150.0f;          // Rango mínimo
    float MaxAttackDistance = 200.0f;          // Rango máximo
    float PositioningDistance = 350.0f;        // Distancia de strafe
    float MinPositioningTime = 1.0f;           // Tiempo mín. posición
    float MaxPositioningTime = 3.0f;           // Tiempo máx. posición
    float BaseDamage = 10.0f;                  // Daño por ataque
    float AttackCooldown = 2.0f;               // Cooldown entre ataques
    float AllyDetectionRadius = 1500.0f;       // Radio de alertas
    int32 MaxSimultaneousAttackers = 3;        // Máximo atacando
    int32 MinAlliesForAggression = 2;          // Aliados para agredir
};
```

### Configuración de Comportamiento
```cpp
struct FEnemyBehaviorConfig
{
    float ChanceToStrafe = 0.5f;               // 50% prob. strafe
    float StrafeDuration = 1.5f;               // Duración strafe
    float ChanceToPauseDuringPatrol = 0.15f;  // 15% prob. pausa
    float MinPauseDuration = 1.0f;             // Pausa mínima
    float MaxPauseDuration = 3.0f;             // Pausa máxima
    float ChanceToLookAround = 0.4f;          // 40% prob. mirar
    float MaxLookAroundAngle = 90.0f;         // Ángulo máx.
    float LookAroundSpeed = 60.0f;            // Velocidad rotación
    float PatrolSpeedVariation = 0.1f;        // ±10% variación
};
```

---

## 🎮 Estados del Enemigo

```
enum class EEnemyState : uint8
{
    Idle                    // Esperando
    Patrolling              // Patrullando ruta
    Investigating           // Buscando última ubicación
    Chasing                 // Persiguiendo al jugador
    Positioning             // Esperando turno / strafe
    Attacking               // Atacando
    Taunting                // Haciendo burla
    Conversing              // Conversando con aliado
    Dead                    // Muerto
};
```

---

## 🔐 Blackboard Keys

```
Todas las tareas usan estas claves del Blackboard:

TargetActor (Object)              ← Jugador detectado
TargetLocation (Vector)           ← Ubicación objetivo
EnemyState (Integer)              ← Estado actual
CanSeeTarget (Boolean)            ← Línea de visión
PatrolIndex (Integer)             ← Punto de patrulla actual
DistanceToTarget (Float)          ← Distancia al objetivo
CanAttack (Boolean)               ← Puede atacar ahora
IsInPause (Boolean)               ← En pausa aleatoria
IsConversing (Boolean)            ← Conversando
```

---

## 📊 Flujo Principal

```
[SPAWN] → [PATROLLING] → {DETECT PLAYER} → [CHASING]
  ↓                                           ↓
[IDLE]                                    [ATTACKING/POSITIONING]
  ↓                                           ↓
[WAIT]                                    {LOSE PLAYER}
  ↓                                           ↓
[PATROL LOOP] ←────────────────────── [INVESTIGATING]
                                          ↓
                                      {NO FIND}
                                          ↓
                                    [BACK TO PATROL]
```

---

## 🚀 Checklist de Implementación

- [ ] **Creación de Enemigo Blueprint**
  - [ ] Heredar de NormalEnemy
  - [ ] Asignar BehaviorTree (BT_NormalEnemy)
  - [ ] Configurar valores en Class Defaults
  - [ ] Asignar PatrolPath en el nivel

- [ ] **Configuración del Nivel**
  - [ ] Colocar NavMesh Bounds Volume
  - [ ] Buildear paths (P en editor)
  - [ ] Colocar PatrolPath con puntos
  - [ ] Colocar enemigos en el nivel
  - [ ] Asignar PatrolPath a cada enemigo

- [ ] **Configuración del Jugador**
  - [ ] Añadir AIPerceptionStimuliSource
  - [ ] Configurar AISense_Sight
  - [ ] Auto Register as Source = true

- [ ] **Testing**
  - [ ] Enemigo patrulla correctamente
  - [ ] Detecta al jugador
  - [ ] Persigue
  - [ ] Ataca
  - [ ] Aliados se alertan
  - [ ] Enemigos conversan
  - [ ] Investigación funciona

---

## 📞 Referencias Rápidas

### Para Encontrar:
- **Comportamiento de pausa**: BTTask_IdleBehavior + FEnemyBehaviorConfig
- **Coordinación de ataques**: BTTask_PositionForAttack + MaxSimultaneousAttackers
- **Conversaciones**: EnemyBase::TryStartConversation() + FEnemyConversationConfig
- **Alertas de aliados**: EnemyBase::AlertNearbyAllies() + AllyDetectionRadius
- **Taunting**: BTTask_PerformTaunt + ShouldTaunt() en NormalEnemy
- **Investigación**: BTTask_Investigate + LastKnownTargetLocation

### Para Ajustar:
- **Velocidad de ataque**: FEnemyCombatConfig.AttackCooldown
- **Rango de combate**: FEnemyCombatConfig.MinAttackDistance/MaxAttackDistance
- **Visión del enemigo**: FEnemyPerceptionConfig.SightRadius
- **Duración de pausas**: FEnemyBehaviorConfig.MinPauseDuration/MaxPauseDuration
- **Probabilidad de taunt**: ANormalEnemy.TauntProbability
- **Radio de alerta**: FEnemyCombatConfig.AllyDetectionRadius

---

## 🐛 Debugging Útil

### Abrir Behavior Tree Viewer:
1. Window → AI Debugging → Behavior Tree Viewer
2. Seleccionar enemigo en el juego
3. Ver tarea activa (nodo resaltado)

### Logs a buscar:
```
"FindPatrolPoint: Enemy -> Point X"
"ChaseTarget: Enemy chasing Player"
"AttackTarget: Enemy attacks Player for Y damage"
"PositionForAttack: Enemy waiting for turn"
"Investigate: Enemy searching area"
"WaitAtPatrolPoint: Enemy waiting"
```

### Variables a inspeccionar:
- CurrentState - Debe cambiar según flujo
- CurrentTarget - Debe ser válido en combate
- LastKnownTargetLocation - Debe actualizarse
- CanAttack - Debe respetar cooldown
- bCanJoinAttack - Debe verificar límite

---

## 📋 Tabla de Decisión Rápida

| Condición | Tarea Activa | Siguiente |
|-----------|-------------|-----------|
| Sin target, en patrulla | FindPatrolPoint | MoveToLocation |
| Alcanzó punto | MoveToLocation | WaitAtPatrolPoint |
| Esperando en punto | WaitAtPatrolPoint | Idle o Patrol |
| Detectó jugador | ChaseTarget | ApproachForAttack |
| Fuera de rango | ApproachForAttack | PositionForAttack |
| En rango, puede atacar | AttackTarget | (cooldown) |
| En rango, NO puede atacar | PositionForAttack | Espera turno |
| Pierde jugador 5s | Investigate | (luego Patrol) |

---

## 🎓 Conclusión

El sistema de enemigos de SairanSkies es completamente modular, escalable y fácil de depurar.

- **10 tareas especializadas** manejan diferentes aspectos del comportamiento
- **Sistema de coordinación** permite múltiples enemigos actuando juntos
- **Configuración flexible** permite ajustar desde el editor
- **Documentación completa** facilita mantenimiento y extensión

Para más información, consultar los documentos específicos según necesidad.

---

*Documentación Final - Sistema de Enemigos SairanSkies*
*Actualizado: 2024*

