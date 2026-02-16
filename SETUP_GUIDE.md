# SairanSkies - Guía de Configuración del Sistema de Combate

## Resumen del Sistema Implementado

Se ha implementado un sistema de combate estilo Batman Arkham con las siguientes características:
- **Movimiento**: Caminar, correr, doble salto con gravedad mejorada, dash direccional (8 direcciones)
- **Cámara**: Tercera persona desde el hombro, se aleja al correr
- **Combate**: Ataque ligero, ataque fuerte, ataque cargado (mantener 2s), parry/block estilo Sekiro
- **Targeting**: Auto-targeting de enemigos en radio de 10m con snap instantáneo al atacar (corregido para ataques aéreos)
- **Arma**: Espadón que se puede enfundar/desenfundar (diagonal en espalda estilo God of War)
- **Feedback de impacto**: Hitstop, camera shake, knockback, sistema de partículas y sonidos

### Cambios Recientes:
- ✅ **Visual del personaje**: Ahora usa un mesh de cápsula (cilindro) en vez de cubo
- ✅ **Posición del arma**: Mejorada para verse correctamente en mano (empuñadura hacia abajo)
- ✅ **Arma en espalda**: Ahora se guarda en diagonal estilo God of War
- ✅ **Gravedad del doble salto**: Aumentada para caída más rápida y responsiva
- ✅ **Debug de hit detection**: Solo dibuja un círculo, no múltiples cada frame
- ✅ **Feedback de impacto**: Knockback, hitstop, camera shake, slots para partículas/sonido
- ✅ **Bug de ataque aéreo**: Arreglado - ahora el personaje baja al nivel del enemigo
- ✅ **Posición de bloqueo**: Al sostener parry, el arma se pone en posición defensiva estilo Sekiro

---

## PASO 1: Crear Input Actions

En el Content Browser, haz clic derecho → Input → Input Action y crea los siguientes:

### IA_Move (Input Action)
- Value Type: `Axis2D (Vector2D)`
- Description: Movimiento del personaje

### IA_Look (Input Action)
- Value Type: `Axis2D (Vector2D)`
- Description: Control de cámara

### IA_Jump (Input Action)
- Value Type: `Digital (bool)`
- Description: Saltar / Doble salto

### IA_Sprint (Input Action)
- Value Type: `Digital (bool)`
- Description: Correr (mantener)

### IA_Dash (Input Action)
- Value Type: `Digital (bool)`
- Description: Dash direccional

### IA_LightAttack (Input Action)
- Value Type: `Digital (bool)`
- Description: Ataque ligero

### IA_HeavyAttack (Input Action)
- Value Type: `Digital (bool)`
- Triggers: Añadir "Hold" con Hold Time Threshold = 0.0 (detecta inicio y fin del hold)
- Description: Ataque fuerte / cargado

### IA_Parry (Input Action)
- Value Type: `Digital (bool)`
- Description: Parry/Bloqueo

### IA_SwitchWeapon (Input Action)
- Value Type: `Digital (bool)`
- Description: Cambiar arma (enfundar/desenfundar)

---

## PASO 2: Crear Input Mapping Context

Crea un nuevo Input Mapping Context: `IMC_Default`

### Mapeos de Teclado y Ratón (PC):

| Action | Key | Modifiers |
|--------|-----|-----------|
| IA_Move | W | Swizzle Input (YXZ), Negate (para Y) |
| IA_Move | S | Swizzle Input (YXZ) |
| IA_Move | A | Negate |
| IA_Move | D | - |
| IA_Look | Mouse XY 2D Axis | - |
| IA_Jump | Space Bar | - |
| IA_Sprint | Left Shift | - |
| IA_Dash | Left Ctrl | - |
| IA_LightAttack | Left Mouse Button | - |
| IA_HeavyAttack | Right Mouse Button | - |
| IA_Parry | Q | - |
| IA_SwitchWeapon | Tab | - |

### Mapeos de Gamepad:

| Action | Key | Modifiers |
|--------|-----|-----------|
| IA_Move | Gamepad Left Thumbstick 2D Axis | Dead Zone (0.2) |
| IA_Look | Gamepad Right Thumbstick 2D Axis | Dead Zone (0.15), Scalar (1.5) |
| IA_Jump | Gamepad Face Button Right (B/Circle) | - |
| IA_Sprint | Left Thumbstick Button (L3) | - |
| IA_Dash | Gamepad Face Button Bottom (A/X) | - |
| IA_LightAttack | Gamepad Right Shoulder (R1/RB) | - |
| IA_HeavyAttack | Gamepad Right Trigger (R2/RT) | - |
| IA_Parry | Gamepad Left Shoulder (L1/LB) | - |
| IA_SwitchWeapon | Gamepad Face Button Top (Y/Triangle) | - |

---

## PASO 3: Crear Blueprint del Personaje

1. Clic derecho en Content Browser → Blueprint Class → Seleccionar `SairanCharacter` como padre
2. Nombrar: `BP_SairanCharacter`

### Configuración del Blueprint:

1. Abrir BP_SairanCharacter
2. En el panel de Details, buscar las secciones:

#### Sección "Input":
- **Default Mapping Context**: Asignar `IMC_Default`
- **Move Action**: Asignar `IA_Move`
- **Look Action**: Asignar `IA_Look`
- **Jump Action**: Asignar `IA_Jump`
- **Sprint Action**: Asignar `IA_Sprint`
- **Dash Action**: Asignar `IA_Dash`
- **Light Attack Action**: Asignar `IA_LightAttack`
- **Heavy Attack Action**: Asignar `IA_HeavyAttack`
- **Parry Action**: Asignar `IA_Parry`
- **Switch Weapon Action**: Asignar `IA_SwitchWeapon`

#### Sección "Weapon":
- **Weapon Class**: Asignar `BP_Greatsword` (lo crearemos en el siguiente paso)

---

## PASO 4: Crear Blueprint del Arma (Espadón)

1. Clic derecho en Content Browser → Blueprint Class → Seleccionar `Greatsword` como padre
2. Nombrar: `BP_Greatsword`

### Configuración:
- El arma viene preconfigurada como un rectángulo largo (placeholder)
- Puedes ajustar `Weapon Size` en Details para cambiar el tamaño
- Ajusta `Hand Socket Name` y `Back Socket Name` según tu skeleton

---

## PASO 5: Crear Blueprint del Enemigo (para pruebas)

1. Clic derecho en Content Browser → Blueprint Class → Seleccionar `EnemyBase` como padre
2. Nombrar: `BP_TestEnemy`

### IMPORTANTE:
El enemigo ya tiene la etiqueta "Enemy" configurada automáticamente, que es necesaria para el sistema de targeting.

### Configuración opcional:
- **Max Health**: Vida del enemigo (default: 100)
- **Attack Windup Duration**: Duración de la ventana de parry cuando ataca

---

## PASO 6: Configurar el Game Mode

1. Crea un Blueprint de `SairanGameMode` o usa la clase C++ directamente
2. En Project Settings → Maps & Modes:
   - **Default GameMode**: Asignar tu Game Mode
   - **Default Pawn Class**: `BP_SairanCharacter`

---

## PASO 7: Probar el Sistema

1. Coloca un `Player Start` en tu nivel
2. Coloca algunos `BP_TestEnemy` alrededor (a menos de 10 metros)
3. Dale Play y prueba:

### Controles PC:
- **WASD**: Moverse
- **Ratón**: Mirar
- **Espacio**: Saltar (doble tap para doble salto)
- **Shift**: Correr
- **Ctrl**: Dash (en la dirección del movimiento)
- **Click Izq**: Ataque ligero (combos de hasta 4 hits)
- **Click Der**: Ataque fuerte (tap) / Ataque cargado (mantener 2s)
- **Q (mantener)**: Parry/Block (posición defensiva mientras se mantiene, parry en el instante de presionar)
- **Tab**: Enfundar/Desenfundar arma

### Controles Gamepad:
- **Stick Izq**: Moverse
- **Stick Der**: Mirar
- **B/Circle**: Saltar
- **L3**: Correr
- **A/X**: Dash
- **R1/RB**: Ataque ligero
- **R2/RT**: Ataque fuerte/cargado
- **L1/LB (mantener)**: Parry/Block (posición defensiva mientras se mantiene)
- **Y/Triangle**: Enfundar/Desenfundar

---

## Características del Sistema de Combate

### Sistema de Targeting (Estilo Batman Arkham)
- Detecta enemigos automáticamente en radio de 10 metros
- Prioriza enemigos basándose en:
  - Distancia (más cerca = mayor prioridad)
  - Dirección del input (hacia donde apuntas)
  - Línea de visión
- Al atacar, el personaje hace "snap" hacia el enemigo más cercano

### Sistema de Combos
- Ataque ligero tiene combo de 4 hits
- Input buffering permite encadenar ataques fluidamente
- Combo se reinicia después de 1.5 segundos sin atacar

### Sistema de Ataque Cargado
- Mantener R2/RT durante 2 segundos para máximo daño
- El daño escala durante la carga
- A los 2 segundos alcanza el máximo y se mantiene hasta soltar

### Sistema de Parry (Estilo Sekiro)
- Ventana de parry de 0.3 segundos
- Cooldown de 0.5 segundos entre parrys
- Los enemigos con `bIsInAttackWindup = true` pueden ser parreados

---

## Valores Configurables (en BP_SairanCharacter)

### Movimiento:
- Walk Speed: 400
- Run Speed: 800
- Dash Distance: 600
- Dash Duration: 0.25s
- Dash Cooldown: 0.4s
- Max Jumps: 2

### Cámara:
- Default Camera Distance: 350
- Running Camera Distance: 450
- Camera Zoom Speed: 5

### Combate (en Combat Component):
- Light Attack Damage: 20
- Heavy Attack Damage: 40
- Charged Attack Damage: 80
- Charge Time For Max Damage: 2s
- Parry Window Duration: 0.3s
- Max Light Combo: 4

### Targeting:
- Targeting Radius: 1000 (10 metros)
- Max Snap Distance: 800
- Snap Duration: 0.15s

### Hit Feedback (nuevo):
- Hitstop Duration: 0.05s (breve pausa al impactar)
- Camera Shake Intensity: 1.0
- Knockback Force: 500
- Charged Knockback Force: 1000
- Hit Particle System: (asignar en Blueprint)
- Hit Sound: (asignar en Blueprint)

### Gravedad (nuevo):
- Normal Gravity Scale: 1.5
- Falling Gravity Scale: 2.5 (para caídas más rápidas)

---

## Próximos Pasos (Opcionales)

1. **Añadir animaciones**: Crear Animation Montages y asignarlos en Combat Component
2. **Efectos visuales**: Añadir VFX de impacto usando Niagara
3. **Feedback háptico**: Añadir vibración de mando al golpear
4. **Sonidos**: Añadir SFX de espadazos e impactos
5. **UI**: Mostrar combo counter, barra de vida del enemigo

---

## Estructura de Archivos Creados

```
Source/SairanSkies/
├── Public/
│   ├── Character/
│   │   └── SairanCharacter.h
│   ├── Combat/
│   │   ├── CombatComponent.h
│   │   └── TargetingComponent.h
│   ├── Weapons/
│   │   ├── WeaponBase.h
│   │   └── Greatsword.h
│   ├── Animation/
│   │   ├── SairanAnimInstance.h
│   │   ├── AN_EnableHitDetection.h
│   │   └── AN_ComboWindow.h
│   ├── Enemies/
│   │   └── EnemyBase.h
│   └── Core/
│       └── SairanGameMode.h
├── Private/
│   ├── Character/
│   │   └── SairanCharacter.cpp
│   ├── Combat/
│   │   ├── CombatComponent.cpp
│   │   └── TargetingComponent.cpp
│   ├── Weapons/
│   │   ├── WeaponBase.cpp
│   │   └── Greatsword.cpp
│   ├── Animation/
│   │   ├── SairanAnimInstance.cpp
│   │   ├── AN_EnableHitDetection.cpp
│   │   └── AN_ComboWindow.cpp
│   ├── Enemies/
│   │   └── EnemyBase.cpp
│   └── Core/
│       └── SairanGameMode.cpp
```
