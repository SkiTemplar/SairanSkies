# Implementación de BTTasks - Ejemplos de Código
## Sistema de Enemigos SairanSkies

---

## 📝 Estructura Base de una BTTask

Todas las BTTasks heredan de `UBTTaskNode` y deben implementar estos métodos:

```cpp
// Header (.h)
class UBTTask_Example : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Example();
    
protected:
    // Se llama cuando la tarea inicia
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;
    
    // Se llama cada frame si retorna InProgress
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds
    ) override;
    
    // Descripción en el editor
    virtual FString GetStaticDescription() const override;
    
private:
    // Variables privadas de la tarea
    float TimerInternal = 0.0f;
};


// Implementation (.cpp)
UBTTask_Example::UBTTask_Example()
{
    NodeName = TEXT("Example Task");
    bNotifyTick = true;  // Necesita TickTask
}

EBTNodeResult::Type UBTTask_Example::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    // Lógica inicial
    TimerInternal = 0.0f;
    
    // Si termina inmediatamente
    return EBTNodeResult::Succeeded;
    
    // Si necesita continuar en TickTask
    return EBTNodeResult::InProgress;
    
    // Si falla
    return EBTNodeResult::Failed;
}

void UBTTask_Example::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    // Lógica continua cada frame
    TimerInternal += DeltaSeconds;
    
    if (TimerInternal >= 2.0f)
    {
        // Tarea completada
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

FString UBTTask_Example::GetStaticDescription() const
{
    return FString::Printf(TEXT("Example Task"));
}
```

---

## 🎯 Ejemplos Reales de Implementación

### 1. BTTask_FindPatrolPoint

```cpp
// Header
class UBTTask_FindPatrolPoint : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindPatrolPoint();
    
protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    
    virtual FString GetStaticDescription() const override;
};

// Implementation
UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
    NodeName = TEXT("Find Patrol Point");
    bNotifyTick = false;  // No necesita tick
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    // Obtener referencias
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    AEnemyBase* Enemy = AIController->GetControlledEnemy();
    
    if (!Enemy || !Enemy->PatrolPath)
    {
        return EBTNodeResult::Failed;
    }
    
    // Obtener siguiente punto
    const TArray<FVector>& PatrolPoints = 
        Enemy->PatrolPath->PatrolPoints;
    
    if (PatrolPoints.Num() == 0)
    {
        return EBTNodeResult::Failed;
    }
    
    // Leer índice actual del Blackboard
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    int32 CurrentIndex = Blackboard->GetValueAsInt(
        AEnemyBase::BB_PatrolIndex);
    
    // Calcular siguiente índice (circular)
    int32 NextIndex = (CurrentIndex + 1) % PatrolPoints.Num();
    
    // Guardar siguiente ubicación en Blackboard
    Blackboard->SetValue<FVector>(
        AEnemyBase::BB_TargetLocation,
        PatrolPoints[NextIndex]
    );
    
    // Actualizar índice
    Blackboard->SetValue<int32>(
        AEnemyBase::BB_PatrolIndex,
        NextIndex
    );
    
    UE_LOG(LogTemp, Log, 
        TEXT("FindPatrolPoint: %s -> Point %d"),
        *Enemy->GetName(),
        NextIndex
    );
    
    return EBTNodeResult::Succeeded;
}

FString UBTTask_FindPatrolPoint::GetStaticDescription() const
{
    return FString::Printf(
        TEXT("Find next patrol point on PatrolPath")
    );
}
```

---

### 2. BTTask_MoveToLocation

```cpp
// Header
class UBTTask_MoveToLocation : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_MoveToLocation();
    
protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;
    
    virtual FString GetStaticDescription() const override;
    
    UPROPERTY(EditAnywhere, Category = "Movement")
    float AcceptanceRadius = 100.0f;
};

// Implementation
UBTTask_MoveToLocation::UBTTask_MoveToLocation()
{
    NodeName = TEXT("Move To Location");
    bNotifyTick = true;  // Necesita verificar cada frame
}

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    AEnemyBase* Enemy = AIController->GetControlledEnemy();
    if (!Enemy)
    {
        return EBTNodeResult::Failed;
    }
    
    // Obtener ubicación objetivo del Blackboard
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    FVector TargetLocation = Blackboard->GetValueAsVector(
        AEnemyBase::BB_TargetLocation
    );
    
    if (TargetLocation.IsZero())
    {
        return EBTNodeResult::Failed;
    }
    
    // Iniciar movimiento
    AIController->SimpleMoveToLocation(TargetLocation);
    
    return EBTNodeResult::InProgress;  // Continúa en TickTask
}

void UBTTask_MoveToLocation::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // Obtener ubicación objetivo
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    FVector TargetLocation = Blackboard->GetValueAsVector(
        AEnemyBase::BB_TargetLocation
    );
    
    // Calcular distancia
    float Distance = FVector::Dist(
        Enemy->GetActorLocation(),
        TargetLocation
    );
    
    // Verificar si alcanzó el destino
    if (Distance <= AcceptanceRadius)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // Si está bloqueado, fallar después de tiempo
    if (!AIController->HasPath())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    }
}

FString UBTTask_MoveToLocation::GetStaticDescription() const
{
    return FString::Printf(
        TEXT("Move to TargetLocation (radius: %.0f)"),
        AcceptanceRadius
    );
}
```

---

### 3. BTTask_AttackTarget

```cpp
// Header
class UBTTask_AttackTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_AttackTarget();
    
protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;
    
    virtual FString GetStaticDescription() const override;
    
private:
    float CooldownTimer = 0.0f;
};

// Implementation
UBTTask_AttackTarget::UBTTask_AttackTarget()
{
    NodeName = TEXT("Attack Target");
    bNotifyTick = true;  // Controlar cooldown
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy || !Enemy->CanAttackNow())
    {
        return EBTNodeResult::Failed;
    }
    
    AActor* Target = Enemy->GetCurrentTarget();
    if (!Target)
    {
        return EBTNodeResult::Failed;
    }
    
    // Detener movimiento
    if (AIController)
    {
        AIController->StopMovement();
    }
    
    // Cambiar estado
    Enemy->SetEnemyState(EEnemyState::Attacking);
    
    // Reproducir montaje de ataque
    if (UAnimMontage* AttackMontage = Enemy->GetRandomAttackMontage())
    {
        Enemy->GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage);
    }
    
    // Reproducir sonido
    Enemy->PlayRandomSound(Enemy->SoundConfig.AttackSounds);
    
    // Aplicar daño
    float Damage = Enemy->CombatConfig.BaseDamage;
    
    // Si está en NormalEnemy, aplicar multiplicador
    if (ANormalEnemy* NormalEnemy = Cast<ANormalEnemy>(Enemy))
    {
        Damage *= NormalEnemy->GetAggressionMultiplier();
    }
    
    UGameplayStatics::ApplyDamage(
        Target,
        Damage,
        Enemy->GetController(),
        Enemy,
        UDamageType::StaticClass()
    );
    
    // Iniciar cooldown
    CooldownTimer = Enemy->CombatConfig.AttackCooldown;
    
    UE_LOG(LogTemp, Log,
        TEXT("AttackTarget: %s attacks %s for %.0f damage"),
        *Enemy->GetName(),
        *Target->GetName(),
        Damage
    );
    
    return EBTNodeResult::InProgress;  // Espera cooldown
}

void UBTTask_AttackTarget::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // Contar cooldown
    CooldownTimer -= DeltaSeconds;
    
    if (CooldownTimer <= 0.0f)
    {
        // Cooldown terminado
        Enemy->UnregisterAsAttacker();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

FString UBTTask_AttackTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("Attack current target"));
}
```

---

### 4. BTTask_ChaseTarget

```cpp
// Header
class UBTTask_ChaseTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ChaseTarget();
    
protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;
    
    virtual FString GetStaticDescription() const override;
};

// Implementation
UBTTask_ChaseTarget::UBTTask_ChaseTarget()
{
    NodeName = TEXT("Chase Target");
    bNotifyTick = true;  // Actualizar continuo
}

EBTNodeResult::Type UBTTask_ChaseTarget::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy)
    {
        return EBTNodeResult::Failed;
    }
    
    AActor* Target = Enemy->GetCurrentTarget();
    if (!Target)
    {
        return EBTNodeResult::Failed;
    }
    
    // Cambiar estado
    Enemy->SetEnemyState(EEnemyState::Chasing);
    Enemy->SetChaseSpeed();
    
    // Alerta a aliados cercanos
    Enemy->AlertNearbyAllies(Target);
    
    UE_LOG(LogTemp, Log,
        TEXT("ChaseTarget: %s chasing %s"),
        *Enemy->GetName(),
        *Target->GetName()
    );
    
    return EBTNodeResult::InProgress;  // Continúa persiguiendo
}

void UBTTask_ChaseTarget::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    AActor* Target = Enemy->GetCurrentTarget();
    
    if (!Target)
    {
        // Perdió el objetivo
        Enemy->LastKnownTargetLocation = Enemy->GetActorLocation();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // Actualizar ubicación objetivo
    FVector TargetLoc = Target->GetActorLocation();
    OwnerComp.GetBlackboardComponent()->SetValue<FVector>(
        AEnemyBase::BB_TargetLocation,
        TargetLoc
    );
    
    // Mover hacia el objetivo
    if (AIController)
    {
        AIController->SimpleMoveToLocation(TargetLoc);
    }
    
    // Guardar última ubicación conocida
    Enemy->LastKnownTargetLocation = TargetLoc;
}

FString UBTTask_ChaseTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("Chase current target"));
}
```

---

### 5. BTTask_PositionForAttack

```cpp
// Header
class UBTTask_PositionForAttack : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_PositionForAttack();
    
protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;
    
    virtual FString GetStaticDescription() const override;
    
private:
    float PositioningTimer = 0.0f;
};

// Implementation
UBTTask_PositionForAttack::UBTTask_PositionForAttack()
{
    NodeName = TEXT("Position For Attack");
    bNotifyTick = true;  // Control continuo
}

EBTNodeResult::Type UBTTask_PositionForAttack::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy || !Enemy->GetCurrentTarget())
    {
        return EBTNodeResult::Failed;
    }
    
    // Cambiar estado
    Enemy->SetEnemyState(EEnemyState::Positioning);
    
    // Iniciar strafe
    bool bStrafeRight = FMath::Rand() > 0.5f;
    Enemy->StartStrafe(bStrafeRight);
    
    // Iniciar timer de posicionamiento
    PositioningTimer = FMath::RandRange(
        Enemy->CombatConfig.MinPositioningTime,
        Enemy->CombatConfig.MaxPositioningTime
    );
    
    return EBTNodeResult::InProgress;
}

void UBTTask_PositionForAttack::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AEnemyAIController* AIController = 
        Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemyBase* Enemy = AIController ? AIController->GetControlledEnemy() : nullptr;
    
    if (!Enemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    AActor* Target = Enemy->GetCurrentTarget();
    if (!Target)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // Mantener distancia de posicionamiento
    float Distance = FVector::Dist(
        Enemy->GetActorLocation(),
        Target->GetActorLocation()
    );
    
    float PositioningDist = Enemy->CombatConfig.PositioningDistance;
    
    if (Distance > PositioningDist)
    {
        // Acercarse
        AIController->SimpleMoveToLocation(Target->GetActorLocation());
    }
    else if (Distance < PositioningDist * 0.8f)
    {
        // Alejarse un poco
        FVector Direction = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
        AIController->SimpleMoveToLocation(
            Target->GetActorLocation() + Direction * 100.0f
        );
    }
    
    // Actualizar timer
    PositioningTimer -= DeltaSeconds;
    
    // Verificar si puede atacar
    if (Enemy->CanJoinAttack())
    {
        Enemy->RegisterAsAttacker();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // Si tiempo agotado sin poder atacar, termina
    if (PositioningTimer <= 0.0f)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

FString UBTTask_PositionForAttack::GetStaticDescription() const
{
    return FString::Printf(TEXT("Position and strafe, wait for attack turn"));
}
```

---

## 🔧 Patrón de Errores Comunes

### Olvido de nullptr checks:
```cpp
// ❌ MAL - Sin verificación
AEnemyBase* Enemy = AIController->GetControlledEnemy();
Enemy->SetEnemyState(EEnemyState::Chasing);  // CRASH si Enemy = nullptr

// ✅ BIEN - Con verificación
AEnemyBase* Enemy = AIController->GetControlledEnemy();
if (!Enemy) return EBTNodeResult::Failed;
Enemy->SetEnemyState(EEnemyState::Chasing);
```

### Olvido de FinishLatentTask:
```cpp
// ❌ MAL - Se queda en InProgress por siempre
void TickTask(...) {
    if (Condición) {
        bTaskComplete = true;  // Solo cambia variable
        // Nunca termina la tarea
    }
}

// ✅ BIEN - Termina la tarea
void TickTask(...) {
    if (Condición) {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
```

### Blackboard keys incorrectas:
```cpp
// ❌ MAL - String mágico
Blackboard->SetValue<int32>("PatrolIndex", 5);

// ✅ BIEN - Usar constantes de la clase
Blackboard->SetValue<int32>(AEnemyBase::BB_PatrolIndex, 5);
```

---

*Implementación de BTTasks - Sistema de Enemigos SairanSkies*
*Ejemplos reales de código funcional*

