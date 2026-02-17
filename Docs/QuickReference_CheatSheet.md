# 🎯 Quick Reference Card - Behavior Tree System
## SairanSkies Enemy AI - Cheat Sheet

---

## 📋 Las 10 Tareas - Vista Rápida

```
┌─────────────────────────────────────────────────────────────┐
│ PATRULLA                                                    │
├─────────────────────────────────────────────────────────────┤
│ 1. FindPatrolPoint → Siguiente punto                        │
│ 2. MoveToLocation  → Ir a ubicación                         │
│ 3. WaitAtPatrolPoint → Esperar 2-4s + pausas/conversación │
│ 4. IdleBehavior → Pausa 1-3s + mirar alrededor             │
│                                                             │
│ COMBATE                                                     │
├─────────────────────────────────────────────────────────────┤
│ 5. ChaseTarget → Perseguir (alerta aliados)                │
│ 6. ApproachForAttack → Acercarse lentamente                │
│ 7. PositionForAttack → Strafe/espera turno (Max 3 ataques) │
│ 8. AttackTarget → Atacar + cooldown 2s                     │
│                                                             │
│ ESPECIAL                                                    │
├─────────────────────────────────────────────────────────────┤
│ 9. PerformTaunt → Burla si ShouldTaunt() = true            │
│ 10. Investigate → Buscar 10s en última ubicación           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔄 Estados del Enemigo

```
Idle → Patrolling → {Detecta} → Chasing
                                    ↓
                    Attacking ← Positioning ← ApproachForAttack
                                    ↑
                              (Max 3 simultáneos)
                                    ↓
                             {Pierde 5s} → Investigating → Patrolling
```

---

## ⚙️ Variables Clave - Para Ajustar

### Combate
```
MinAttackDistance = 150              (Rango mínimo)
MaxAttackDistance = 200              (Rango máximo)
PositioningDistance = 350            (Distancia strafe)
AttackCooldown = 2.0                 (Cooldown entre ataques)
BaseDamage = 10                      (Daño por ataque)
MaxSimultaneousAttackers = 3         (Máximo atacando)
```

### Comportamiento
```
ChanceToPauseDuringPatrol = 0.15    (15% pausa)
MinPauseDuration = 1.0               (Pausa mínima)
MaxPauseDuration = 3.0               (Pausa máxima)
ChanceToLookAround = 0.4             (40% mirar)
StrafeDuration = 1.5                 (Duración strafe)
```

### NormalEnemy
```
TauntProbability = 0.3               (30% taunt)
TauntCooldown = 5.0                  (Cooldown taunt)
MinAlliesForAggression = 2            (Aliados para agredir)
```

### Percepción
```
SightRadius = 2000                   (Rango visión)
HearingRadius = 1000                 (Rango audición)
LoseSightTime = 5.0                  (Perder después 5s)
InvestigationTime = 10.0             (Duración investigación)
AllyDetectionRadius = 1500           (Radio alertas)
```

---

## 🎯 Flujo Rápido

### Patrol
```
1. FindPatrolPoint (obtiene siguiente)
2. MoveToLocation (va)
3. WaitAtPatrolPoint (espera 2-4s)
   └─ 15% → IdleBehavior (pausa 1-3s)
   └─ 40% → StartLookAround
   └─ Busca compañero para conversar
→ Loop
```

### Combat
```
1. ChaseTarget (persigue, alerta)
2. ¿En rango? 
   ├─ No → ApproachForAttack (acerca)
   └─ Sí → 
      ├─ ¿Puede atacar? → AttackTarget (2s cooldown)
      └─ No → PositionForAttack (strafe/espera)
→ ¿Pierde target 5s?
   └─ Investigate (busca 10s)
   └─ Si no encuentra → Patrolling
```

---

## 📊 Probabilidades

| Evento | Probabilidad | Duración |
|--------|-------------|----------|
| Pausa durante patrulla | 15% | 1-3 seg |
| Mirar alrededor | 40% | ~1-2 seg |
| Taunt (NormalEnemy) | 30% | var |
| Strafe en combate | 50% | 1.5 seg |

---

## 🔐 Blackboard Keys Principales

```
TargetActor          (Object)   → Jugador detectado
TargetLocation       (Vector)   → Ubicación objetivo
EnemyState          (Integer)   → Estado actual
CanSeeTarget        (Boolean)   → Línea de visión
DistanceToTarget    (Float)     → Distancia euclidiana
CanAttack           (Boolean)   → Puede atacar
```

---

## 🐛 Debugging Rápido

### Abrir BT Viewer:
1. Window → AI Debugging → Behavior Tree Viewer
2. Seleccionar enemigo en juego
3. Ver tarea activa (nodo resaltado)

### Logs principales:
```
"ChaseTarget: Enemy chasing Player"
"AttackTarget: Enemy attacks for 10 damage"
"PositionForAttack: Waiting for turn"
"Investigate: Enemy searching"
```

### Variables a inspeccionar:
- `CurrentState` - Debe cambiar según flujo
- `CurrentTarget` - Válido en combate
- `LastKnownTargetLocation` - Se actualiza
- `CanAttack` - Respeta cooldown
- `NearbyAlliesCount` - Para taunt

---

## 🎮 Configuración Mínima

1. Crear Blueprint de NormalEnemy
2. Asignar BehaviorTree (BT_NormalEnemy)
3. Asignar PatrolPath
4. Colocar NavMesh en nivel (P para ver)
5. Asignar AIPerceptionStimuliSource al jugador
6. Testear

---

## 🔧 Cambios Comunes

### Enemigo más rápido:
```
ChaseSpeedMultiplier = 1.5  // Default 1.0
```

### Enemigo más fuerte:
```
BaseDamage = 20             // Default 10
```

### Rango de ataque más largo:
```
MaxAttackDistance = 300     // Default 200
```

### Menos pausas:
```
ChanceToPauseDuringPatrol = 0.05  // Default 0.15
```

### Más taunts:
```
TauntProbability = 0.6      // Default 0.3
```

---

## 📈 Rendimiento

- **1-5 enemigos:** Sin impacto
- **5-15 enemigos:** Rendimiento bajo
- **15-30 enemigos:** Monitoreado
- **30+ enemigos:** Considerar optimización

---

## ✅ Checklist Rápido

- [ ] Enemigo patrulla
- [ ] Detecta al jugador
- [ ] Persigue
- [ ] Ataca en rango
- [ ] Cooldown funciona
- [ ] Aliados se alertan
- [ ] Investigación funciona
- [ ] Conversaciones funcionan (si aplica)
- [ ] Taunting funciona (NormalEnemy)

---

## 🆘 SOS - Problemas Comunes

| Problema | Solución |
|----------|----------|
| No se mueve | Verificar NavMesh (P en editor) |
| No detecta | Verificar SightRadius y AIPerceptionStimuliSource |
| No ataca | Verificar CanAttack y distancia |
| Freezea | Monitor CPU/GPU, reducir enemigos |
| No conversa | Verificar ConversationRadius y TimeBeforeConversation |
| Taunt no funciona | Verificar ShouldTaunt() retorna true |

---

## 📚 Documentos Relacionados

- **Guía Completa:** EnemySystem_Setup_Guide.md
- **Referencia Técnica:** BTTasks_Technical_Reference.md
- **Diagramas:** BTVisual_Complete_Reference.md
- **Código:** BTTasks_Code_Examples.md
- **Índice:** README_Index.md

---

*Quick Reference Card - SairanSkies Enemy AI System*
*Imprime y pega en la pared para referencia rápida*

