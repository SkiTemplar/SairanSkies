# Behavior Tree Tasks - Referencia Técnica
## Sistema de Enemigos SairanSkies

---

## 📋 Resumen de Tareas Implementadas

El Behavior Tree del sistema de enemigos cuenta con 10 tareas especializadas que manejan diferentes aspectos del comportamiento de IA:

### Grupo: Patrulla
1. **BTTask_FindPatrolPoint** - Selecciona siguiente punto
2. **BTTask_MoveToLocation** - Movimiento genérico
3. **BTTask_WaitAtPatrolPoint** - Espera en punto
4. **BTTask_IdleBehavior** - Comportamiento durante espera

### Grupo: Combate
5. **BTTask_ChaseTarget** - Persecución
6. **BTTask_ApproachForAttack** - Aproximación a rango
7. **BTTask_PositionForAttack** - Posicionamiento táctico
8. **BTTask_AttackTarget** - Ejecución de ataque

### Grupo: Especial
9. **BTTask_PerformTaunt** - Burlas/Taunt
10. **BTTask_Investigate** - Investigación

---

## 🎯 Tareas de Patrulla

### BTTask_FindPatrolPoint

**Propósito:** Determinar el siguiente punto de patrulla

**Entrada:**
- `PatrolPath` (Actor referencia)
- `PatrolIndex` (int en Blackboard)

**Salida:**
- `TargetLocation` (FVector en Blackboard)
- Retorna: `Success`

**Lógica:**
```cpp
// Pseudocódigo
if (!PatrolPath) return Failure;

int NextIndex = (PatrolIndex + 1) % PatrolPath->PatrolPoints.Num();
TargetLocation = PatrolPath->PatrolPoints[NextIndex]->GetActorLocation();
Blackboard->SetValue("PatrolIndex", NextIndex);
return Success;
```

**Configuración en Blueprint:** Seleccionar el `PatrolPath` asignado al enemigo

---

### BTTask_MoveToLocation

**Propósito:** Mover el enemigo a una ubicación específica

**Entrada:**
- `TargetLocation` (FVector en Blackboard)
- AIController->SimpleMoveToLocation()

**Salida:**
- Retorna: `Success` cuando alcanza la ubicación (aceptance radius)
- Retorna: `InProgress` mientras se está moviendo
- Retorna: `Failure` si no puede llegar

**Lógica:**
```cpp
// Pseudocódigo
FVector Target = Blackboard->GetValue<FVector>("TargetLocation");
float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Target);

if (Distance <= AcceptanceRadius) {
    return EBTNodeResult::Succeeded;
}

if (!AIController->HasPath()) {
    AIController->SimpleMoveToLocation(Target);
}

return EBTNodeResult::InProgress;
```

**Parámetros Configurables:**
- `AcceptanceRadius` - Radio para considerar "alcanzado" (100-200 unidades)
- `bStopOnOverflow` - Detener si no hay ruta

---

### BTTask_WaitAtPatrolPoint

**Propósito:** Esperar en un punto de patrulla con comportamientos naturales

**Entrada:**
- Timer interno

**Salida:**
- Retorna: `Success` cuando termina la espera

**Comportamientos:**
- Duración aleatoria: 2-4 segundos (configurable)
- 15% probabilidad de iniciar pausa aleatoria
- 40% probabilidad de mirar alrededor
- Si detecta jugador → interrumpe inmediatamente
- Intenta conversar con aliados cercanos

**Lógica:**
```cpp
// Pseudocódigo
void ExecuteTask() {
    WaitTimer = FMath::RandRange(MinWaitTime, MaxWaitTime);
    
    // Intentar iniciar conversación
    if (CanStartConversation()) {
        AEnemyBase* Partner = FindNearbyEnemyForConversation();
        if (Partner) TryStartConversation(Partner);
    }
}

void TickTask(float DeltaTime) {
    if (Enemy->GetCurrentTarget()) {
        // Interrumpir si detecta jugador
        return Success;
    }
    
    WaitTimer -= DeltaTime;
    if (WaitTimer <= 0) {
        return Success;
    }
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `MinWaitTime` - Espera mínima (1.0-2.0 segundos)
- `MaxWaitTime` - Espera máxima (3.0-4.0 segundos)
- `bCheckForConversation` - Activar sistema de conversación
- `bLookAroundDuringWait` - Mirar alrededor durante espera

---

### BTTask_IdleBehavior

**Propósito:** Pausas aleatorias con animaciones naturales durante la espera

**Entrada:**
- Timer para pausa

**Salida:**
- Retorna: `Success` cuando termina la pausa

**Comportamientos:**
- Duración: 1-3 segundos (configurable)
- Reproduce montaje de "looking around" (si existe)
- Puede incluir sonidos de idle
- Se interrumpe si ve al jugador

**Lógica:**
```cpp
// Pseudocódigo
void ExecuteTask() {
    if (FMath::Rand() < 0.15f) {  // 15% probabilidad
        bIsPausing = true;
        PauseDuration = FMath::RandRange(MinPause, MaxPause);
        
        if (bLookAroundDuringPause) {
            Enemy->StartLookAround();
        }
    }
}

void TickTask(float DeltaTime) {
    if (Enemy->GetCurrentTarget()) {
        return Success;  // Interrumpir
    }
    
    PauseTimer += DeltaTime;
    if (PauseTimer >= PauseDuration) {
        Enemy->StopLookAround();
        Enemy->EndRandomPause();
        return Success;
    }
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `MinPauseDuration` - Pausa mínima (0.5-1.5 segundos)
- `MaxPauseDuration` - Pausa máxima (2.0-3.5 segundos)
- `bLookAroundDuringPause` - Mirar alrededor (true/false)
- `bUseEnemyConfig` - Usar configuración del enemigo

---

## ⚔️ Tareas de Combate

### BTTask_ChaseTarget

**Propósito:** Perseguir al jugador detectado

**Entrada:**
- `TargetActor` (AActor en Blackboard)
- `TargetLocation` (actualizada continuamente)

**Salida:**
- Retorna: `Success` mientras persigue
- Puede retornar `Failure` si pierde el target

**Comportamientos:**
- Alerta a aliados cercanos (radio: 1500 unidades)
- Actualiza LastKnownTargetLocation constantemente
- Cambia velocidad a ChaseSpeedMultiplier (100%)
- Interrumpe conversaciones si estaba en una

**Lógica:**
```cpp
// Pseudocódigo
void ExecuteTask() {
    Enemy->SetEnemyState(EEnemyState::Chasing);
    Enemy->AlertNearbyAllies(CurrentTarget);
}

void TickTask(float DeltaTime) {
    if (!CurrentTarget) {
        return Failure;
    }
    
    FVector TargetLoc = CurrentTarget->GetActorLocation();
    Blackboard->SetValue("TargetLocation", TargetLoc);
    Blackboard->SetValue("DistanceToTarget", GetDistance());
    
    AIController->SimpleMoveToLocation(TargetLoc);
    Enemy->SetChaseSpeed();
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `AlertRadius` - Radio para alertar aliados (1500 unidades por defecto)
- `UpdateInterval` - Frecuencia de actualización de ubicación

---

### BTTask_ApproachForAttack

**Propósito:** Acercarse al rango de ataque

**Entrada:**
- `TargetActor` con distancia > MinAttackDistance

**Salida:**
- Retorna: `Success` cuando entra en rango
- Retorna: `InProgress` mientras se acerca

**Comportamientos:**
- Velocidad: 70% de chase speed (ralentiza la aproximación)
- Se detiene en MinAttackDistance
- Mira continuamente al objetivo
- Puede ser decorado para verificar distancia

**Lógica:**
```cpp
// Pseudocódigo
void TickTask(float DeltaTime) {
    if (!CurrentTarget) return Failure;
    
    float Distance = GetDistanceToTarget();
    
    if (Distance <= MinAttackDistance) {
        return Success;  // En rango
    }
    
    // Aproximarse más lentamente
    float SlowSpeed = ChatSpeedMultiplier * ApproachSpeedMultiplier;
    Enemy->SetMovementSpeed(SlowSpeed);
    
    AIController->SimpleMoveToLocation(
        CurrentTarget->GetActorLocation()
    );
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `ApproachSpeedMultiplier` - Velocidad de aproximación (0.6-0.8)
- `MinAttackDistance` - Distancia mínima (150 unidades)

---

### BTTask_PositionForAttack

**Propósito:** Posicionamiento táctico (strafe) mientras espera turno para atacar

**Entrada:**
- `TargetActor` con distancia correcta
- `CanAttack` disponible en Blackboard

**Salida:**
- Retorna: `Success` al cambiar a atacar

**Comportamientos:**
- Se mantiene a `PositioningDistance` del objetivo (350 unidades)
- Rota alrededor del objetivo (strafe)
- Verifica si puede unirse al ataque (ActiveAttackers < MaxSimultaneousAttackers)
- Si sí → se registra como atacante
- Si no → continúa strafing

**Lógica:**
```cpp
// Pseudocódigo
void TickTask(float DeltaTime) {
    if (!CurrentTarget) return Failure;
    
    float Distance = GetDistanceToTarget();
    
    // Mantener distancia de posicionamiento
    if (Distance > PositioningDistance) {
        // Acercarse
        SimpleMoveToLocation(Target);
    } else if (Distance < PositioningDistance * 0.8f) {
        // Alejarse
        SimpleMoveToLocation(Target - Direction * 100);
    }
    
    // Rotar alrededor del objetivo
    if (!bIsStrafing) {
        StartStrafe(FMath::Rand() > 0.5f);
    }
    
    // Verificar si puede atacar
    if (CanJoinAttack()) {
        RegisterAsAttacker();
        return Success;  // Cambiar a AttackTarget
    }
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `PositioningDistance` - Distancia de strafe (300-400 unidades)
- `MinPositioningTime` - Tiempo mínimo (1.0-1.5 segundos)
- `MaxPositioningTime` - Tiempo máximo (2.5-3.5 segundos)

---

### BTTask_AttackTarget

**Propósito:** Ejecutar el ataque cuerpo a cuerpo

**Entrada:**
- `CanAttack` = TRUE en Blackboard
- `TargetActor` válido
- Decorador verifica CanAttackNow()

**Salida:**
- Retorna: `Success` después de atacar

**Comportamientos:**
- Detiene el movimiento
- Reproduce montaje de ataque (aleatorio)
- Reproduce sonido de ataque
- Aplica daño al objetivo
- Inicia cooldown de 2 segundos
- Se desregistra como atacante después

**Lógica:**
```cpp
// Pseudocódigo
void ExecuteTask() {
    if (!CanAttackNow()) return Failure;
    
    // Detener movimiento
    AIController->StopMovement();
    Enemy->SetEnemyState(EEnemyState::Attacking);
    
    // Reproducir animación
    UAnimMontage* AttackMontage = GetRandomAttackMontage();
    PlayMontage(AttackMontage);
    
    // Sonido
    PlayRandomSound(SoundConfig.AttackSounds);
    
    // Daño
    float AdjustedDamage = BaseDamage * GetAggressionMultiplier();
    ApplyDamage(CurrentTarget, AdjustedDamage);
    
    // Cooldown
    bCanAttack = false;
    AttackCooldownTimer = AttackCooldown;
    
    return Success;
}

void TickTask(float DeltaTime) {
    AttackCooldownTimer -= DeltaTime;
    if (AttackCooldownTimer <= 0) {
        bCanAttack = true;
        UnregisterAsAttacker();
        return Success;
    }
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `BaseDamage` - Daño base (10-15 unidades)
- `AttackCooldown` - Cooldown entre ataques (1.5-2.5 segundos)
- `AttackMontages[]` - Array de montajes

---

## 🌟 Tareas Especiales

### BTTask_PerformTaunt

**Propósito:** Ejecutar burla/taunt (solo si es apropiado)

**Entrada:**
- `ShouldTaunt()` retorna true (configurable por subclase)
- Decorador verifica condiciones

**Salida:**
- Retorna: `Success` después de taunting

**Comportamientos:**
- Solo se ejecuta si hay suficientes aliados
- Aumenta la alerta de otros enemigos
- Detiene el movimiento
- Puede reproducir animación especial
- Duración variable

**Lógica:**
```cpp
// Pseudocódigo
void PerformTaunt() {
    if (TimeSinceLastTaunt < TauntCooldown) return;
    
    SetEnemyState(EEnemyState::Taunting);
    TimeSinceLastTaunt = 0.0f;
    
    // Alerta más agresivamente a los aliados
    if (CurrentTarget) {
        AlertNearbyAllies(CurrentTarget);
        AlertNearbyAllies(CurrentTarget);  // Doble alerta
    }
    
    // Reproducir taunt si existe
    if (TauntMontage) {
        PlayMontage(TauntMontage);
    }
    
    // Detener movimiento
    StopMovement();
}

bool ShouldTaunt() const {
    if (TimeSinceLastTaunt < TauntCooldown) return false;
    if (!IsInCombat()) return false;
    
    float Probability = TauntProbability;
    
    // Más probable si hay aliados
    if (HasEnoughAlliesForAggression()) {
        Probability *= 1.5f;
    }
    
    return FMath::Rand() < Probability;
}
```

**Parámetros Configurables (NormalEnemy específicamente):**
- `TauntProbability` - Probabilidad (0.0-1.0) por defecto 0.3
- `TauntCooldown` - Cooldown entre taunts (5.0 segundos)
- `MinAlliesForAggression` - Aliados para activar (2)

---

### BTTask_Investigate

**Propósito:** Investigar última ubicación conocida del jugador

**Entrada:**
- `LastKnownTargetLocation` en Blackboard
- Estado: `Investigating`

**Salida:**
- Retorna: `Success` al terminar investigación
- Retorna a `Patrolling` si no encuentra nada

**Comportamientos:**
- Va a LastKnownTargetLocation
- Busca en radio de InvestigationRadius (500 unidades)
- Duración: 10 segundos
- Si ve al jugador → vuelve a ChaseTarget
- Si no → vuelve a Patrolling

**Lógica:**
```cpp
// Pseudocódigo
void ExecuteTask() {
    Enemy->SetEnemyState(EEnemyState::Investigating);
    InvestigationTimer = InvestigationTime;  // 10 segundos
}

void TickTask(float DeltaTime) {
    // Si detecta al jugador nuevamente
    if (CanSeeTarget()) {
        return Success;  // ChaseTarget tomará control
    }
    
    // Moverse hacia última ubicación conocida
    SimpleMoveToLocation(LastKnownLocation);
    
    InvestigationTimer -= DeltaTime;
    if (InvestigationTimer <= 0) {
        return Success;  // Volver a Patrolling
    }
    
    return InProgress;
}
```

**Parámetros Configurables:**
- `InvestigationTime` - Duración (8.0-12.0 segundos)
- `InvestigationRadius` - Radio de búsqueda (400-600 unidades)

---

## 🔄 Flujo de Ejecución Completo

### Ciclo Patrol → Chase → Combat

```
INICIO (Idle)
    ↓
PATROLLING
├─ FindPatrolPoint
├─ MoveToLocation (a punto)
└─ WaitAtPatrolPoint (2-4s)
    ├─ Puede entrar IdleBehavior
    ├─ Intenta conversar
    └─ Verifica Target
    
DETECTOR JUGADOR (TargetActor != nullptr)
    ↓
CHASING
├─ Alerta a aliados
├─ Actualiza LastKnownLocation
└─ Persigue continuamente
    
EN RANGO (Distance <= MaxAttackDistance)
    ↓
    ├─ ¿CanAttack == true? ─→ ATTACKING
    │                           ├─ AttackTarget
    │                           └─ Cooldown (2s)
    │
    └─ ¿CanJoinAttack()? ──→ POSITIONING
                               ├─ PositionForAttack
                               └─ Strafe/Espera
                               
PIERDE TARGET (5s sin ver)
    ↓
INVESTIGATING
├─ MoveToLocation (LastKnownLocation)
├─ Busca en el área (10s)
└─ Si no ve → PATROLLING
```

---

## 📊 Matriz de Cambios de Estado

| Estado | Entrada | Tarea | Salida |
|--------|---------|-------|--------|
| Idle | Spawn | FindPatrolPoint | Patrolling |
| Patrolling | Punto alcanzado | WaitAtPatrolPoint | Patrolling |
| Patrolling | Jugador detectado | ChaseTarget | Chasing |
| Chasing | En rango | ApproachForAttack | Positioning |
| Positioning | Turno disponible | AttackTarget | Attacking |
| Attacking | Cooldown fin | ChaseTarget | Chasing |
| Chasing | 5s sin ver | Investigate | Investigating |
| Investigating | Tiempo fin | FindPatrolPoint | Patrolling |

---

## 🛠️ Debugging en Behavior Tree

### Para inspeccionar en tiempo real:
1. Abrir Behavior Tree viewer (Window → AI Debugging)
2. Seleccionar enemigo en el juego
3. Ver qué tarea se está ejecutando (nodo activo resaltado)
4. Ver valores del Blackboard

### Logs útiles a buscar:
```
"ChaseTarget: Enemy persiguiendo"
"AttackTarget: Aplicando daño"
"PositionForAttack: Esperando turno"
"Investigate: Buscando en área"
"WaitAtPatrolPoint: Esperando" 
```

---

*Documento técnico del Sistema de Enemigos - SairanSkies*
*Última actualización: 2024*

