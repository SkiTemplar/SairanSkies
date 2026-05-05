# 🎯 Guía de Configuración del VFX del Láser Ultimate

## 📋 Resumen

El sistema de Ultimate Laser en SairanSkies funciona con dos componentes VFX principales:

1. **LaserBeamVFX** — Rayo continuo que viaja desde el jugador hasta donde impacta
2. **LaserImpactVFX** — Explosión en el punto de impacto

---

## 🔧 Configuración del Sistema Niagara (LaserBeamVFX)

### Requisitos del Sistema Niagara

El VFX de rayo **DEBE** ser un sistema Niagara con los siguientes parámetros de usuario:

#### **1. Parámetro: `BeamStart` (Vector)**
- **Tipo:** Vector User Parameter
- **Valor Default:** (0, 0, 0)
- **Propósito:** Posición donde comienza el rayo (posición del jugador)
- **Actualización:** Cada frame por el código

#### **2. Parámetro: `BeamEnd` (Vector)**
- **Tipo:** Vector User Parameter
- **Valor Default:** (0, 0, 300)
- **Propósito:** Posición donde termina el rayo (punto de impacto o máximo alcance)
- **Actualización:** Cada frame por el código

### Cómo Crear el Sistema Niagara en el Editor

1. **Click derecho** en Content Browser
2. **FX → Niagara System → New Niagara System**
3. Nombre: `NS_LaserBeam`

#### **En el Emitter:**

**Emitter Update (Module):**
```
Los módulos de update deben estar vacíos o tener solo lógica básica.
```

**Spawn Rate:**
```
Set Spawn Rate = 100 (o según sea necesario)
```

**Particle Update (Modules):**

1. **Ribbon Renderer** (o Beam Renderer si lo tiene)
   - **Ribbon Width Mode:** Use Ribbon Width Attribute
   - **Width:** 8-16 cm
   - **Material:** Crear material láser (ver más abajo)

2. **Module: Curve Position**
   ```
   Position = Lerp(BeamStart, BeamEnd, Particle.NormalizedAge)
   ```
   Esto hace que las partículas viajen desde el inicio hasta el fin del rayo.

3. **Module: Color**
   ```
   Color = (0.0, 0.5, 1.0, 1.0)  // Azul cian brillante
   Alpha = 1.0 - (Particle.NormalizedAge ^ 2)  // Fade al final
   ```

**Renderer (Niagara Ribbon Renderer):**
- **Ribbon Width:** 12.0
- **Tie Ribbon Ends:** Enabled
- **Material:** Material con shader personalizado

---

## 🎨 Crear Material de Láser

### En Unreal Engine:

1. **Click derecho** en Content Browser
2. **Material**
3. Nombre: `M_LaserBeam`

#### **Setup del Material Graph:**

```
Base Color:
  - Componente azul-cian del color: (0.0, 1.0, 1.0)
  - Usar Emissive para el brillo

Emissive Color:
  - Color: (0.0, 1.0, 1.0) * Brightness Parameter (default 2.0)
  
Roughness: 0.1 (muy pulido)
Metallic: 1.0 (reflectivo)

Opacity: 
  - Blend Mode: Additive o Translucent
  - Opacity = 0.8
```

### Alternativa Simple:
```
Use the default BasicShapeMaterial y aplica un tinte azul-cian con multiplicación.
```

---

## 🔌 Cómo Funciona el Código (C++)

### En `UUltimateComponent::TryActivate()`

```cpp
if (LaserBeamVFX)
{
    // 1. Calcular posición inicial y final del láser
    FVector Origin;
    FVector End;
    FHitResult Hit;
    const bool bHit = TraceLaser(Origin, End, Hit);
    
    // 2. Spawnear el sistema Niagara EN LA POSICIÓN INICIAL
    LaserBeamComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        LaserBeamVFX,
        Origin,  // ← Se coloca en el origen del rayo
        GetLaserBeamRotation(Origin, bHit ? Hit.ImpactPoint : End),
        FVector(1.0f),
        false,
        false
    );
    
    // 3. Activar el componente
    if (LaserBeamComponent)
    {
        LaserBeamComponent->Activate(true);
        UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);
    }
}
```

### En `UUltimateComponent::TickComponent()` (cada frame)

```cpp
void UUltimateComponent::TickComponent(...)
{
    // ... código de timer ...
    
    // IMPORTANTE: Cada frame se actualiza la posición final del láser
    FVector Origin;
    FVector End;
    FHitResult Hit;
    const bool bHit = TraceLaser(Origin, End, Hit);
    
    // Llamar SIEMPRE a UpdateLaserBeam con las nuevas posiciones
    UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);
    
    // ... resto del código ...
}
```

### En `UUltimateComponent::UpdateLaserBeam()`

```cpp
void UUltimateComponent::UpdateLaserBeam(const FVector& Origin, const FVector& End)
{
    if (!LaserBeamComponent) return;

    // Actualizar la posición del componente Niagara
    LaserBeamComponent->SetWorldLocation(Origin);
    LaserBeamComponent->SetWorldRotation(GetLaserBeamRotation(Origin, End));
    
    // CRÍTICO: Enviar los parámetros al shader cada frame
    LaserBeamComponent->SetVectorParameter(LaserBeamStartParam, Origin);
    LaserBeamComponent->SetVectorParameter(LaserBeamEndParam, End);
}
```

---

## ✅ Checklist de Configuración en el Editor

- [ ] **LaserBeamVFX** está asignado en el blueprint de UltimateComponent
- [ ] El sistema Niagara tiene los parámetros **`BeamStart`** y **`BeamEnd`** (Vector User Parameters)
- [ ] El **Emitter** usa un módulo que interpola entre BeamStart y BeamEnd
- [ ] El **Renderer** es Ribbon o un custom renderer para tipo beam
- [ ] El **Material** del láser es brillante (Emissive, Additive o translúcido)
- [ ] **LaserBeamRotationOffset** está configurado correctamente (default: Yaw = -90°)
  - Esto alinea el eje del rayo con la dirección de disparo
  - Ajustarlo si el rayo sale "de lado" o girado

---

## 🎬 Diagrama de Flujo

```
[TryActivate]
    ↓
[SpawnSystemAtLocation] ← LaserBeamVFX spawneado en Origin
    ↓
[TickComponent] (cada frame)
    ├─ TraceLaser(Origin, End) ← Calcula dónde va el rayo
    ├─ UpdateLaserBeam(Origin, End) ← ACTUALIZA PARÁMETROS
    │   ├─ SetVectorParameter("BeamStart", Origin)
    │   └─ SetVectorParameter("BeamEnd", End)
    ├─ FireLaserTick() ← Aplica daño cada DamageInterval
    │   ├─ LineTrace
    │   ├─ ApplyDamage
    │   └─ SpawnImpactVFX
    └─ Si LaserTimer <= 0: Deactivate()
```

---

## 🐛 Troubleshooting

### Problema: El rayo no se ve
**Solución:**
1. Verificar que `LaserBeamVFX` está asignado en el blueprint
2. Habilitar `bShowLaserDebug = true` en el editor para ver líneas de debug
3. Verificar que el sistema Niagara **tiene partículas spawneando** (check Spawn Rate)
4. El material debe ser **Translucent** o **Additive** para verse en la escena

### Problema: El rayo no se mueve/actualiza
**Solución:**
1. Verificar que `LaserBeamComponent` no es nullptr en UpdateLaserBeam()
2. Los parámetros de usuario en Niagara deben coincidir EXACTAMENTE:
   - En código: `LaserBeamStartParam` y `LaserBeamEndParam`
   - En Niagara: "BeamStart" y "BeamEnd"
3. Si cambias los nombres, actualiza los FName en el inspector

### Problema: El rayo aparece pero no sigue la cámara
**Solución:**
1. Verificar que en `FireLaserTick()` se usa `FollowCamera->GetForwardVector()`
2. Asegurarse de que `Character->FollowCamera` está inicializado

### Problema: El impacto VFX aparece en posición incorrecta
**Solución:**
1. Verificar que `LaserImpactVFX` está asignado
2. El impacto debe spawnearse en `Hit.ImpactPoint` (ya se hace)
3. Si sale "dentro" del enemigo, es porque el rayo está atravesando
   - Aumentar ligeramente el offset forward del laser point

---

## 📊 Parámetros Editables en Inspector

```cpp
// En UUltimateComponent:
UPROPERTY(EditDefaultsOnly, Category = "Ultimate|VFX")
UNiagaraSystem* LaserBeamVFX = nullptr;

UPROPERTY(EditDefaultsOnly, Category = "Ultimate|VFX")
UNiagaraSystem* LaserImpactVFX = nullptr;

UPROPERTY(EditDefaultsOnly, Category = "Ultimate|VFX")
FName LaserBeamStartParam = FName("BeamStart");

UPROPERTY(EditDefaultsOnly, Category = "Ultimate|VFX")
FName LaserBeamEndParam = FName("BeamEnd");

UPROPERTY(EditAnywhere, Category = "Ultimate|VFX")
FRotator LaserBeamRotationOffset = FRotator(0.0f, -90.0f, 0.0f);

UPROPERTY(EditAnywhere, Category = "Ultimate|Debug")
bool bShowLaserDebug = false;
```

---

## 🎮 Testing en el Editor

1. Abre el nivel de prueba
2. Mata 4 enemigos (1 kill = 25 XP, 4 kills = 100 XP = barra llena)
3. Presiona el botón Ultimate (debería tener la barra llena y decir "LISTA")
4. El rayo debe salir desde el personaje
5. Habilita `bShowLaserDebug = true` para ver líneas de debug
6. El rayo debe:
   - Seguir la dirección de la cámara
   - Actualizar su final según los enemigos/geometría que toque
   - Hacer daño a los enemigos en su camino
   - Mostrar impacto VFX donde golpea

---

## 📝 Mejoras Futuras

- Añadir distorsión en el espacio-tiempo alrededor del rayo
- Múltiples rayos saltando entre enemigos
- Reducción de daño según la distancia
- Rotación continua del rayo (efecto de "barrida")
- Sonido de barrido mientras el láser está activo


