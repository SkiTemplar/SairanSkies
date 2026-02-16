# Resumen de Cambios - Sistema de Combate SairanSkies

## Última actualización: 2026-02-16

---

## 🆕 CAMBIOS MÁS RECIENTES

### Attach Points para el Arma (Nuevo Sistema)
- **Archivos**: `SairanCharacter.h/cpp`, `WeaponBase.h/cpp`, `Greatsword.cpp`
- Se eliminaron los vectores/rotaciones hardcodeadas
- Ahora hay 3 **USceneComponents** en el personaje que definen las posiciones del arma:
  - `WeaponHandAttachPoint` - Posición cuando el arma está en mano
  - `WeaponBackAttachPoint` - Posición cuando el arma está enfundada (diagonal en espalda)
  - `WeaponBlockAttachPoint` - Posición cuando está bloqueando/parry
- **Ventaja**: Se pueden ajustar las posiciones directamente en el editor (Blueprint) sin recompilar

### Bug del Snap Aéreo - ARREGLADO
- **Archivo**: `TargetingComponent.cpp`
- **Problema**: Al atacar desde el aire, el personaje a veces atravesaba el suelo o no podía volver a saltar
- **Solución**:
  1. Ahora se hace un **line trace hacia el suelo** para encontrar la superficie
  2. El personaje se coloca justo encima del suelo (respetando la altura de la cápsula)
  3. Se **resetea el contador de saltos** después del snap
  4. Se limpia la velocidad residual para evitar deslizamientos

---

## 🎮 Cambios Visuales

### 1. Visual del Personaje - Cápsula
- **Archivo**: `SairanCharacter.h/cpp`
- El personaje ahora tiene un `VisualMesh` que muestra un cilindro (cápsula) de color azul claro
- Dimensiones: 84 de diámetro x 192 de altura
- Se crea automáticamente en `BeginPlay()` con `SetupVisualMesh()`

### 2. Posición del Arma en Mano
- **Archivo**: `WeaponBase.h`
- `HandAttachRotation` cambiado a `FRotator(0, 0, -90)` 
- La espada ahora se ve correctamente en posición vertical con el filo hacia arriba
- La empuñadura queda hacia abajo, como se sostiene un espadón real

### 3. Arma en Espalda - Diagonal (Estilo God of War)
- **Archivo**: `WeaponBase.h`
- `BackAttachOffset` cambiado a `FVector(-20, 10, 0)`
- `BackAttachRotation` cambiado a `FRotator(-35, 45, 0)`
- El arma ahora se guarda en diagonal en la espalda

---

## 🦘 Mejoras de Movimiento

### 4. Gravedad del Doble Salto - Más Responsivo
- **Archivo**: `SairanCharacter.h/cpp`
- Nuevas propiedades:
  - `NormalGravityScale = 1.5f` (gravedad normal)
  - `FallingGravityScale = 2.5f` (cuando está cayendo)
- Función `UpdateGravityScale()` detecta cuando está cayendo y aumenta la gravedad
- Resultado: Caídas más rápidas y snappy, no se siente "lunar"

---

## ⚔️ Mejoras de Combate

### 5. Debug de Hit Detection - Solo Un Círculo
- **Archivo**: `CombatComponent.h/cpp`
- Nueva propiedad `bShowHitDebug = false` para controlar debug visual
- El debug ahora solo dibuja `ForOneFrame` y solo si no ha habido hit este ataque
- Flag `bHitLandedThisAttack` previene múltiples dibujos de debug

### 6. Feedback de Impacto
- **Archivo**: `CombatComponent.h/cpp`
- **Hitstop**: Pausa de 0.05s usando `SetGlobalTimeDilation`
- **Camera Shake**: Configurable con `HitCameraShake` (TSubclassOf) y `CameraShakeIntensity`
- **Knockback**: 
  - `KnockbackForce = 500` para ataques normales
  - `ChargedKnockbackForce = 1000` para ataques cargados
  - Los enemigos son empujados hacia atrás con `LaunchCharacter`
- **Partículas**: Slot para `UNiagaraSystem* HitParticleSystem`
- **Sonido**: Slot para `USoundBase* HitSound`
- **Evento Blueprint**: `OnHitLanded` delegate para efectos adicionales

### 7. Bug de Ataque Aéreo - ARREGLADO
- **Archivo**: `TargetingComponent.cpp`
- Problema: Al saltar y atacar, el personaje se movía en X pero mantenía su Z, atacando "sobre la cabeza" del enemigo
- Solución: `SnapDestination.Z = TargetLocation.Z` - El snap ahora lleva al jugador AL nivel del enemigo

### 8. Posición de Bloqueo/Parry (Estilo Sekiro)
- **Archivos**: `WeaponBase.h/cpp`, `CombatComponent.h/cpp`, `SairanCharacter.h/cpp`
- Nueva rotación `BlockingRotation = FRotator(0, 45, 0)`
- Función `SetBlockingStance(bool bIsBlocking)`
- El parry ahora es HOLD-based:
  - Al presionar: Activa parry window + posición de bloqueo
  - Mientras mantiene: Arma en posición defensiva
  - Al soltar: Vuelve a posición normal
- Controles: `ParryStart()` y `ParryRelease()` en lugar de solo `Parry()`

---

## 📋 Propiedades Configurables en Blueprint

### Weapon Attach Points (Nuevo - ajustable en editor)
En el Blueprint del personaje, selecciona estos componentes y muévelos/rótalos:
- `WeaponHandAttachPoint` - Arrastra para posicionar el arma en la mano
- `WeaponBackAttachPoint` - Arrastra para posicionar el arma en la espalda
- `WeaponBlockAttachPoint` - Arrastra para posicionar el arma al bloquear

### Combat Component - Hit Feedback
```cpp
HitstopDuration = 0.05f;
CameraShakeIntensity = 1.0f;
TSubclassOf<UCameraShakeBase> HitCameraShake; // Asignar en BP
KnockbackForce = 500.0f;
ChargedKnockbackForce = 1000.0f;
UNiagaraSystem* HitParticleSystem; // Asignar en BP
USoundBase* HitSound; // Asignar en BP
bShowHitDebug = false;
```

### Character - Gravity
```cpp
NormalGravityScale = 1.5f;
FallingGravityScale = 2.5f;
```


---

## 🔧 Cómo Añadir Efectos

### Camera Shake
1. Crear un Blueprint de Camera Shake (clic derecho → Blueprint → CameraShakeBase)
2. Configurar la intensidad y duración del shake
3. En BP_SairanCharacter → Combat Component → Hit Camera Shake, asignar el BP

### Partículas de Impacto
1. Crear un sistema Niagara para el impacto
2. En BP_SairanCharacter → Combat Component → Hit Particle System, asignarlo

### Sonido de Impacto
1. Importar un sonido de impacto
2. En BP_SairanCharacter → Combat Component → Hit Sound, asignarlo

---

## ✅ Compilación Exitosa
El proyecto compila sin errores con UE 5.6.
