<style>
body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; max-width: 900px; margin: 0 auto; padding: 40px 20px; color: #2c3e50; line-height: 1.7; }
h1 { color: #1a1a2e; border-bottom: 3px solid #e94560; padding-bottom: 12px; font-size: 2em; }
h2 { color: #16213e; border-left: 4px solid #e94560; padding-left: 12px; margin-top: 40px; }
h3 { color: #0f3460; }
table { width: 100%; border-collapse: collapse; margin: 20px 0; }
th, td { border: 1px solid #ddd; padding: 10px 14px; text-align: left; }
th { background-color: #1a1a2e; color: white; }
tr:nth-child(even) { background-color: #f8f9fa; }
code { background: #f0f0f0; padding: 2px 6px; border-radius: 3px; font-size: 0.9em; }
.highlight { background: #fff3cd; padding: 12px 16px; border-left: 4px solid #ffc107; margin: 16px 0; border-radius: 4px; }
.info { background: #d1ecf1; padding: 12px 16px; border-left: 4px solid #17a2b8; margin: 16px 0; border-radius: 4px; }
img { max-width: 100%; border: 1px solid #ddd; border-radius: 4px; margin: 10px 0; }
</style>

# Sairan Skies — Memoria de Grupo (Programación)

---

## 1. Descripción del Juego

**Sairan Skies** es un juego de acción en tercera persona desarrollado en **Unreal Engine 5.6** con C++. El jugador controla a un personaje equipado con un espadón y diversas habilidades (gancho, teletransporte) en un entorno de plataformas y combate cuerpo a cuerpo.

El combate se inspira en las siguientes referencias:

| Referencia | Elemento adoptado |
|---|---|
| **Batman Arkham** | Targeting automático, snap al enemigo, flujo de combate, sistema de grupo (solo N enemigos atacan simultáneamente) |
| **Sekiro: Shadows Die Twice** | Parry con ventana de tiempo precisa, deflect perfecto con slow-motion |
| **God of War (2018)** | Arma en la espalda en diagonal, freeze frame en parry perfecto, trail de la espada |
| **Ratchet & Clank** | Cámara dinámica tercera persona, sensación de movilidad y agilidad |
| **It Takes Two** | Mecánica de clon/teletransporte |
| **Lies of P** | Trail de espada con cambio a trail de sangre al impactar |

---

## 2. Integrantes (Programación)

| Nombre | Rol |
|---|---|
| **José Antonio Mota Lucas** | Personaje jugable, sistema de combate, habilidades, UI, armas, zona de muerte, contribución a IA de enemigos |
| **Rodrigo Fernández Carnicer** | Sistema de enemigos, IA (Behavior Trees), percepción, patrullas, coordinación grupal |

---

## 3. Arquitectura del Proyecto

El proyecto sigue una **arquitectura basada en componentes** nativa de Unreal Engine, donde el personaje principal (`ASairanCharacter`) actúa como eje central al que se le acoplan componentes modulares independientes.

### 3.1 Diagrama de Módulos

```
┌─────────────────────────────────────────────────────────┐
│                    ASairanCharacter                       │
│  (ACharacter + Enhanced Input + Health + HUD)            │
├──────────┬──────────┬──────────┬──────────┬─────────────┤
│ UCombat  │UTargeting│UGrapple  │UClone    │UCheckpoint  │
│Component │Component │Component │Component │Component    │
├──────────┴──────────┴──────────┴──────────┴─────────────┤
│                  UWeaponLerpComponent                     │
├──────────────────────┬──────────────────────────────────┤
│   AWeaponBase        │     AGreatsword                   │
│  (Mesh + Hit + Trail)│    (hereda WeaponBase)            │
└──────────────────────┴──────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                      AEnemyBase                          │
│  (ACharacter + State Machine + Health)                   │
├──────────┬──────────┬───────────────────────────────────┤
│UDamageNum│UWidget   │  ANormalEnemy (hereda EnemyBase)   │
│Component │Component │                                    │
├──────────┴──────────┴───────────────────────────────────┤
│                  EnemyAIController                        │
│            (Behavior Tree + Perception)                   │
├─────────────────────────────────────────────────────────┤
│              UGroupCombatManager                         │
│         (World Subsystem - Dos Círculos)                 │
└─────────────────────────────────────────────────────────┘

┌──────────────┐  ┌──────────────────┐  ┌────────────────┐
│  ADeathZone  │  │ ASairanGameMode  │  │  APatrolPath   │
└──────────────┘  └──────────────────┘  └────────────────┘
```

### 3.2 Estructura de Carpetas (Source)

```
Source/SairanSkies/
├── Public/
│   ├── AI/              → IA: Controller, GroupCombatManager, Tasks, Services, Decorators
│   ├── Animation/       → Anim Notifies (combo window, hit detection)
│   ├── Character/       → SairanCharacter, CloneComponent, CheckpointComponent
│   ├── Combat/          → CombatComponent, GrappleComponent, TargetingComponent
│   ├── Core/            → GameMode, DeathZone
│   ├── Enemies/         → EnemyBase, NormalEnemy, EnemyTypes, DamageNumberComponent
│   ├── UI/              → HUD, DamageNumberWidget, EnemyHealthBar, GrappleCrosshair
│   └── Weapons/         → WeaponBase, Greatsword, GrappleHookActor, WeaponLerpComponent
└── Private/
    └── (misma estructura con .cpp)
```

---

## 4. División de Tareas

### 4.1 José Antonio Mota Lucas

| Sistema | Descripción |
|---|---|
| **Personaje (SairanCharacter)** | Movimiento completo: andar, correr (toggle mando), doble salto con gravedad variable, dash con i-frames |
| **Sistema de Combate (CombatComponent)** | Ataque ligero (combo 5 golpes), ataque fuerte, ataque cargado (2s), parry/bloqueo con ventana tipo Sekiro, slow-mo en parry perfecto |
| **Targeting (TargetingComponent)** | Auto-targeting estilo Arkham: detección en radio, snap al enemigo más cercano, line-of-sight |
| **Gancho (GrappleComponent)** | Apuntar (L2/F), aim-assist con snap a objetos con tag, impulso físico, bloqueo de cámara, trail de partículas, reinicio de doble salto |
| **Clon/Teletransporte (CloneComponent)** | Plantar clon translúcido, teleportarse a él, distancia por contexto (combate/exploración), VFX/SFX |
| **Arma (WeaponBase + Greatsword)** | Espadón con trail estilo Lies of P (normal + sangre al impactar), posiciones de bloqueo/mano/espalda |
| **Weapon Lerp (WeaponLerpComponent)** | Lerp de la espada entre puntos de combo, timeout de inactividad |
| **UI** | HUD del jugador, barra de vida enemigos, números de daño flotantes (UMG), crosshair del gancho |
| **Zona de Muerte (DeathZone)** | Respawn en último checkpoint con VFX/SFX |
| **Checkpoint (CheckpointComponent)** | Guardado automático de última posición segura |
| **BTTask_CircleTarget** | Contribución al comportamiento de flanqueo con feints |

### 4.2 Rodrigo Fernández Carnicer

| Sistema | Descripción |
|---|---|
| **EnemyBase** | Clase base de enemigos con máquina de estados, sistema de salud, hit flash, coordinación entre aliados |
| **NormalEnemy** | Tipo concreto de enemigo con su configuración |
| **EnemyAIController** | Controlador de IA con Behavior Tree y sistema de percepción |
| **GroupCombatManager** | Subsistema de mundo: modelo de dos círculos (inner/outer) para combate grupal natural |
| **Behavior Tree Tasks** | AttackTarget, ChaseTarget, FindPatrolPoint, IdleBehavior, Investigate, MoveToLocation, OuterCircleBehavior, WaitAtPatrolPoint |
| **EnemyTypes** | Configuraciones (combate, percepción, patrulla, comportamiento natural, conversación) |
| **Sistema de Conversación** | Enemigos conversan entre sí cuando están en idle |
| **Patrullas** | PatrolPath con puntos de patrulla y comportamiento natural |

---

## 5. Controles

### PC

| Acción | Tecla |
|---|---|
| Movimiento | WASD |
| Cámara | Ratón |
| Saltar / Doble salto | Espacio |
| Correr | Shift |
| Dash | Ctrl |
| Ataque ligero | Click izquierdo |
| Ataque fuerte / Cargado | Click derecho (tap / mantener) |
| Parry / Bloqueo | Q |
| Gancho | F (mantener para apuntar, soltar para disparar) |
| Cambiar arma | Flechas izq/der |
| Clon / TP | R |

### Mando

| Acción | Botón |
|---|---|
| Movimiento | Stick izquierdo |
| Cámara | Stick derecho |
| Saltar | B / Círculo |
| Correr (toggle) | L3 |
| Dash | A / Cruz |
| Ataque ligero | R1 / RB |
| Ataque fuerte / Cargado | R2 / RT (tap / mantener) |
| Parry / Bloqueo | L1 / LB |
| Gancho | L2 / LT |
| Cambiar arma | D-Pad izq/der |
| Clon / TP | Y / Triángulo |

---

## 6. Tecnologías Utilizadas

- **Motor**: Unreal Engine 5.6
- **Lenguaje**: C++ (100% código nativo, sin Blueprints de lógica)
- **Input**: Enhanced Input System
- **IA**: Behavior Trees + AI Perception
- **VFX**: Niagara Particle System
- **UI**: UMG (Unreal Motion Graphics)
- **Gestión de grupos**: World Subsystem (`UGroupCombatManager`)


