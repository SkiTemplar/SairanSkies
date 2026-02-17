# Behavior Tree Visual - Referencia Completa
## Sistema de Enemigos SairanSkies

---

## 🎯 Composición del Behavior Tree

El Behavior Tree está compuesto por:
- **1 ROOT (Selector)** - Nodo raíz que selecciona qué rama ejecutar
- **3 Ramas Principales:**
  1. Service Update (ejecución continua)
  2. Investigation Branch
  3. Combat/Patrol Branch (selector principal)

---

## 📊 Diagrama Detallado por Niveles

### NIVEL 0: ROOT

```
┌──────────────────────────────────────────────────┐
│                    ROOT NODE                     │
│                   (SELECTOR)                     │
│  "¿En qué rama debo ejecutar al enemigo?"       │
└────────────────────────┬─────────────────────────┘
                         │
     ┌───────────────────┼───────────────────┐
     │                   │                   │
     ▼                   ▼                   ▼
   BRANCH 1            BRANCH 2            BRANCH 3
  (Service)        (Investigation)      (Patrol/Combat)
```

---

## 🔧 BRANCH 1: Service Update

```
┌────────────────────────────────────────────────────┐
│     BTService_UpdateEnemyState                    │
│     (Se ejecuta cada frame - cada 0.25s)          │
├────────────────────────────────────────────────────┤
│                                                    │
│  Actualiza el Blackboard con:                     │
│  • TargetActor ← CurrentTarget                    │
│  • TargetLocation ← Target.Location               │
│  • CanSeeTarget ← LineTrace al target             │
│  • DistanceToTarget ← Distance(this, target)      │
│  • CanAttack ← (ActiveAttackers < Max) && !Dead   │
│  • IsInPause ← bIsInRandomPause                   │
│  • IsConversing ← (State == Conversing)           │
│  • EnemyState ← CurrentState (enum value)         │
│                                                    │
│  ⚠️ Costo: BAJO - Operaciones rápidas             │
│                                                    │
└────────────────────────────────────────────────────┘
```

### Variables Actualizadas en Blackboard:

| Variable | Tipo | Frecuencia | Propósito |
|----------|------|-----------|----------|
| `TargetActor` | Object | Continuo | Objetivo principal |
| `TargetLocation` | Vector | Continuo | Ubicación del objetivo |
| `CanSeeTarget` | Boolean | Continuo | Línea de visión |
| `DistanceToTarget` | Float | Continuo | Distancia euclidiana |
| `CanAttack` | Boolean | Continuo | Disponibilidad de ataque |
| `IsInPause` | Boolean | Continuo | Estado de pausa |
| `IsConversing` | Boolean | Continuo | Estado de conversación |
| `EnemyState` | Integer | Continuo | Estado actual (enum) |

---

## 🔍 BRANCH 2: Investigation

```
┌──────────────────────────────────────────────────┐
│          INVESTIGATION BRANCH                    │
│     [Decorator: CheckEnemyState]                │
│     "¿Estado == Investigating?"                 │
├──────────────────────────────────────────────────┤
│                                                  │
│            SI (Investigating)                    │
│                   │                              │
│                   ▼                              │
│        ┌───────────────────┐                    │
│        │ BTTask_Investigate│                    │
│        │                   │                    │
│        │ • Va a ubicación  │                    │
│        │   última conocida │                    │
│        │ • Busca 10 seg    │                    │
│        │ • Si ve jugador   │                    │
│        │   → Chasing       │                    │
│        │ • Si no encuentra │                    │
│        │   → Patrolling    │                    │
│        │                   │                    │
│        └───────────────────┘                    │
│                   │                              │
│              Success/Failure                    │
│                   │                              │
│        Vuelve a Patrol o Chase                 │
│                                                  │
└──────────────────────────────────────────────────┘

ENTRADA (LastKnownTargetLocation, InvestigationTime = 10s)
SALIDA (Success → NextBranch)
```

---

## ⚔️ BRANCH 3: Patrol/Combat

```
┌─────────────────────────────────────────────────────────┐
│         MAIN SELECTOR: Patrol vs Combat                │
│         "¿Tenemos un objetivo detectado?"              │
└─────────────────────────────────────────────────────────┘
                        │
        ┌───────────────┴───────────────┐
        │                               │
        ▼                               ▼
   NO TARGET                      HAS TARGET
   (Default)                       (Combat)
        │                               │
        ▼                               ▼
   ┌─────────────┐              ┌──────────────────┐
   │   PATROL    │              │ COMBAT SEQUENCE  │
   │  SELECTOR   │              │   (Sequence)     │
   └──────┬──────┘              └────────┬─────────┘
          │                              │
          │         ┌────────────────────┼────────────────────┐
          │         │                    │                    │
          ▼         ▼                    ▼                    ▼
     ┌────────────────────────┐   ┌──────────────┐  ┌──────────────┐
     │ Patrol Sequence        │   │Chase Target  │  │Taunt Selector│
     │ (Loop)                 │   │[Decorator:   │  │(Ocasional)   │
     │                        │   │ HasTarget]   │  │[Decorator:   │
     │ 1. FindPatrolPoint ────┤───┤              │  │ ShouldTaunt] │
     │ 2. MoveToLocation  ────┤───┤ • Persigue   │  │              │
     │ 3. WaitAtPatrolPoint ──┤───┤ • Actualiza  │  │ • Burla       │
     │    ├─ IdleBehavior     │   │   Location   │  │ • Alerta      │
     │    ├─ Conversación     │   │ • Alerta a   │  │   más        │
     │    └─ Loop             │   │   aliados    │  │              │
     │                        │   │ • Ataque     │  │              │
     │                        │   │   opcional   │  │              │
     └────────────────────────┘   └──────┬───────┘  └──────┬───────┘
                                         │                 │
                                  ┌──────▼────────────────▼───┐
                                  │  ATTACK SELECTOR          │
                                  │  "¿Qué hacer en combate?" │
                                  └─────────┬────────────────┘
                                            │
                  ┌─────────────────────────┼─────────────────────────┐
                  │                         │                         │
                  ▼                         ▼                         ▼
         ┌────────────────┐      ┌────────────────┐      ┌──────────────────┐
         │ AttackTarget   │      │ApproachForAttack│      │PositionForAttack │
         │ [Decorator:    │      │[Decorator:     │      │[Decorator:       │
         │  CanAttack=T]  │      │ InRange]       │      │ CanJoinAttack]   │
         │                │      │                │      │                  │
         │ • Ataca        │      │ • Se acerca    │      │ • Strafe         │
         │ • Daño         │      │ • Velocidad    │      │ • Espera turno   │
         │ • Cooldown 2s  │      │   reducida     │      │ • Mira target    │
         │ • Sonido       │      │ • Se prepara   │      │                  │
         │                │      │                │      │                  │
         └────────────────┘      └────────────────┘      └──────────────────┘
```

---

## 🔄 PATROL LOOP DETALLADO

```
START: Enemy está en Patrol
│
├─ [Service] Actualiza Blackboard (cada frame)
│   └─ TargetActor = null → continúa Patrol
│
├─ [Sequence] Patrol
│  │
│  ├─ Task 1: FindPatrolPoint
│  │  └─ Obtiene siguiente punto de PatrolPath
│  │     └─ Establece TargetLocation
│  │
│  ├─ Task 2: MoveToLocation
│  │  └─ AIController->SimpleMoveToLocation(TargetLocation)
│  │  └─ Espera hasta alcanzar (AcceptanceRadius)
│  │  └─ Success cuando llega
│  │
│  └─ Task 3: WaitAtPatrolPoint
│     │
│     ├─ Espera 2-4 segundos aleatoriamente
│     │
│     ├─ Durante espera:
│     │  ├─ 15% prob → IdleBehavior (pausa 1-3s)
│     │  ├─ 40% prob → StartLookAround (mira ±90°)
│     │  └─ Intenta conversar con aliados
│     │
│     └─ Si ve jugador → interrumpe inmediatamente
│
└─ LOOP → Vuelve a FindPatrolPoint (siguiente punto)

⚠️ Si durante cualquier momento:
   - TargetActor != null → SALE A CHASE
   - Estado cambia → ajusta comportamiento
```

---

## ⚔️ COMBAT SEQUENCE DETALLADO

```
START: Enemy detectó jugador
│
├─ Service actualiza Blackboard
│  └─ TargetActor = Jugador
│  └─ CanSeeTarget = true/false (línea de visión)
│  └─ DistanceToTarget = distancia actual
│
├─ Chase Target (Continuo mientras persigue)
│  ├─ SimpleMoveToLocation(Target.Location)
│  ├─ AlertNearbyAllies(Target)
│  ├─ Actualiza LastKnownTargetLocation
│  └─ SetEnemyState(Chasing)
│
└─ [Selector] Attack Decision
   │
   ├─ ¿DistanceToTarget > MaxAttackDistance?
   │  └─ Task: ApproachForAttack
   │     ├─ Mueve más lentamente (70% velocidad)
   │     ├─ Se detiene en MinAttackDistance
   │     └─ Success → Attack Decision
   │
   ├─ ¿DistanceToTarget en rango [Min, Max]?
   │  └─ ¿CanAttack == TRUE?
   │     │
   │     ├─ SI: Task AttackTarget
   │     │  ├─ Detiene movimiento
   │     │  ├─ PlayMontage(AttackAnimation)
   │     │  ├─ PlaySound(AttackSound)
   │     │  ├─ ApplyDamage(Target)
   │     │  ├─ Inicia cooldown 2 segundos
   │     │  └─ UnregisterAsAttacker → Success
   │     │
   │     └─ NO: Task PositionForAttack
   │        ├─ Mantiene distancia PositioningDistance
   │        ├─ StartStrafe (rota alrededor)
   │        ├─ Mira continuamente al target
   │        ├─ Verifica CanJoinAttack()
   │        │  ├─ SI → RegisterAsAttacker → Success
   │        │  └─ NO → continúa strafing → InProgress
   │        └─ Si ve que puede atacar → Success
   │
   └─ Loop vuelve a Decision basado en distancia/disponibilidad

⚠️ Durante Combat:
   - Si TargetActor = null (5s sin ver) → Investigate
   - Si Estado cambia (muerte, etc) → ajusta
   - Si ShouldTaunt() = true → ejecuta PerformTaunt
```

---

## 🚨 EVENTOS DE INTERRUPCIÓN

```
DURANTE PATROL:
├─ OnPlayerDetected() → TargetActor asignado
│  └─ Selector detecta TargetActor != null
│     └─ Cambia a COMBAT
│
├─ OnPlayerLost() → TargetActor = null después 5s
│  └─ LastKnownTargetLocation guardada
│     └─ Cambia a INVESTIGATING
│
├─ OnConversationPartnerFound() → Intenta conversar
│  └─ SetEnemyState(Conversing)
│     └─ Pausa, mira a compañero, hace gestos
│
└─ OnEnemyDeath() → IsDead = true
   └─ Todas las tareas retornan Failure
      └─ BT detiene ejecución

DURANTE COMBAT:
├─ OnPlayerLost() → después 5 segundos
│  └─ LastKnownTargetLocation = última ubicación
│     └─ TargetActor = null
│        └─ Selector cambia a INVESTIGATING
│
├─ AttackSuccessful() → Daño aplicado
│  └─ AttackCooldown inicia
│     └─ Próximo ataque disponible en 2s
│
├─ OnAlertFromAlly() → Otro enemigo ve jugador
│  └─ Recibe notificación
│     └─ Puede cambiar TargetActor
│        └─ Persigue a la nueva ubicación
│
└─ OnTauntDecision() → ShouldTaunt() = true
   └─ Ejecuta PerformTaunt
      └─ Alerta adicional a aliados
         └─ Modifica estrategia
```

---

## 📈 Diagrama de Flujo - Estados de Retorno

```
Cada tarea retorna uno de estos valores:

┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   SUCCESS   │     │ IN_PROGRESS  │     │   FAILURE   │
├─────────────┤     ├──────────────┤     ├─────────────┤
│ • Terminó   │     │ • Continúa   │     │ • No pudo   │
│ • Continúa  │     │   ejecutándose     │ • Error     │
│   siguiente │     │ • Invocada   │     │ • Intenta   │
│   tarea     │     │   tick cada  │     │   alternativa
│ • Fin rama  │     │   frame      │     │ • O fallida │
└─────────────┘     └──────────────┘     └─────────────┘

Ejemplo:
MoveToLocation:
├─ En progreso → InProgress (continúa cada frame)
├─ Alcanzó destino → Success (siguiente tarea)
└─ Bloqueado/muere → Failure (reintentar)

AttackTarget:
├─ Atacando montage → InProgress
├─ Cooldown fin → Success
└─ Sin target → Failure
```

---

## 🎲 Probabilidades y Tiempos

### En Patrol:
```
┌───────────────────────┬──────┬─────────────────┐
│ Evento                │ Prob │ Duración        │
├───────────────────────┼──────┼─────────────────┤
│ Pausa aleatoria       │ 15%  │ 1-3 seg         │
│ Mirar alrededor       │ 40%  │ ~1-2 seg        │
│ Espera en punto       │ 100% │ 2-4 seg         │
│ Intento conversación  │ Var  │ 5-12 seg (si OK)│
└───────────────────────┴──────┴─────────────────┘
```

### En Combat:
```
┌───────────────────────┬──────┬─────────────────┐
│ Evento                │ Prob │ Duración        │
├───────────────────────┼──────┼─────────────────┤
│ Taunt (si aplica)     │ 30%  │ ~1-2 seg        │
│ Ataque (si pueden)    │ Max3 │ ~1 seg + 2s CD  │
│ Strafe (si esperan)   │ 100% │ 1.5 seg/turno   │
│ Pierde objetivo       │ -    │ 5 seg + invest. │
└───────────────────────┴──────┴─────────────────┘
```

---

## 🔐 Condiciones Decoradores

### Decoradores que Protegen Tareas:

```
1. AttackTarget
   ├─ [Decorator] CanAttack == TRUE
   │  └─ Solo ataca si no hay cooldown
   ├─ [Decorator] HasTarget
   │  └─ Solo ataca si existe TargetActor
   └─ [Decorator] InRange
      └─ Solo ataca si distancia en [Min, Max]

2. ApproachForAttack
   ├─ [Decorator] HasTarget
   │  └─ Necesita objetivo
   ├─ [Decorator] Distance > MinAttackDistance
   │  └─ Solo si está lejos
   └─ [Decorator] CanSeeTarget
      └─ Solo si hay línea de visión

3. PositionForAttack
   ├─ [Decorator] CanJoinAttack
   │  └─ ActiveAttackers < MaxSimultaneousAttackers
   ├─ [Decorator] InRange
   │  └─ Está en rango combate
   └─ [Decorator] HasTarget
      └─ Tiene objetivo válido

4. ChaseTarget
   ├─ [Decorator] HasTarget
   │  └─ Solo persigue si hay target
   └─ [Decorator] NOT InRange
      └─ No está en rango ataque

5. Investigation
   ├─ [Decorator] CheckEnemyState == Investigating
   │  └─ Solo investiga en ese estado
   └─ [Decorator] NOT HasTarget
      └─ Perdió al objetivo
```

---

## 📊 Resumen de Estructura

```
TOTAL: 1 Root + 10 Tasks + 2 Decorators + 1 Service

Profundidad máxima: 5 niveles
Ramas paralelas: 3 (Service, Investigation, Combat/Patrol)
Selectors: 4 (Root, Combat Decision, Attack Decision, Patrol)
Sequences: 3 (Patrol, Combat, Investigation)

Costo CPU: BAJO (servicios optimizados, sin búsquedas costosas)
Escalabilidad: Hasta 30+ enemigos sin problemas
```

---

*Diagrama Visual - Sistema de Enemigos SairanSkies*
*Referencia completa del Behavior Tree implementado*

