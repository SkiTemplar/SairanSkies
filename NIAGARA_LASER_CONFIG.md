# 🎯 Configuración Detallada del VFX NS_Ultimate como Rayo Láser

## 🔴 Problema Identificado

Basándome en la imagen que compartiste, el VFX actual:
- ✗ Crece en todas las direcciones (escala)
- ✗ Atraviesa al jugador por atrás
- ✗ No viaja en línea recta desde origin hasta end
- ✗ No usa los parámetros `BeamStart` y `BeamEnd`

## ✅ Solución: Configurar como Rayo Láser Continuo

### PASO 1: Estructura Base del Sistema Niagara

El sistema debe tener **1 emitter principal** que use mallas para crear el efecto visual del rayo.

```
NS_Ultimate
├─ Emitter_LaserBeam
   ├─ Emitter Update
   ├─ Particle Spawn (Burst = bajo, 10-20 partículas)
   ├─ Particle Update
   │  ├─ Initialize Particle
   │  ├─ Update Position (LERP entre BeamStart y BeamEnd)
   │  ├─ Mesh Renderer
   │  └─ Lifetime Manager
   └─ Events (None)
```

---

### PASO 2: Parámetros de Usuario (User Parameters)

Abre el panel **"User Parameters"** en el overview del Niagara y añade:

#### **Parámetro 1: BeamStart**
- **Nombre:** `BeamStart`
- **Tipo:** Vector
- **Valor Default:** `(0, 0, 0)`
- **Descripción:** Posición donde comienza el rayo (ojos del jugador)

#### **Parámetro 2: BeamEnd**
- **Nombre:** `BeamEnd`
- **Tipo:** Vector
- **Valor Default:** `(0, 0, 300)`
- **Descripción:** Posición donde termina el rayo (impacto/alcance máximo)

---

### PASO 3: Configurar el Emitter

#### **3.1 Emitter Properties**

```
Emitter Settings:
├─ Execution State: Active
├─ Fixed Delta Time: Disabled
├─ Loop Behavior: Infinite
└─ Determinism: Deterministic (for testing)
```

#### **3.2 Spawn Rate**

```
Spawn Rate: 20 particles per second
Spawn Burst Override: DISABLED (usa spawn rate normal)
```

**Motivo:** Queremos spawning continuo pero controlado, no explosiones.

---

### PASO 3: Particle Update — Lo CRÍTICO

En la sección **Particle Update**, necesitas estos módulos en **este orden**:

#### **Módulo 1: Initialize Particle**
```
Position: (0, 0, 0)
Velocity: (0, 0, 0)
Lifetime: 2.0 segundos (el rayo dura 5s, pero las partículas viven 2s)
Sprite/Mesh Orientation: aligned to velocity (luego lo cambiamos)
Mass: 1.0
```

#### **Módulo 2: Update Position (CUSTOM)**
```
Este es el CORAZÓN del sistema. Necesitas crear un módulo personalizado o usar:

Position = Lerp(BeamStart, BeamEnd, Age / Lifetime)

Es decir:
- Cuando Age = 0: Position = BeamStart (origen)
- Cuando Age = Lifetime: Position = BeamEnd (final)
- Interpola suavemente entre ambos

Si no puedes crear módulo custom, usa:
- Attribute Reader: Read "BeamStart" user parameter → Store in temp
- Attribute Reader: Read "BeamEnd" user parameter → Store in temp
- Vector Lerp: Lerp(temp_start, temp_end, NormalizedAge) → Position
```

#### **Módulo 3: Color Over Lifetime**
```
Color: (0.0, 1.0, 1.0) - Cian/Azul
Alpha: 1.0 - (NormalizedAge ^ 1.5)  // Fade suave al final
Brightness Multiplier: 2.0
```

#### **Módulo 4: Scale**
```
Uniform Scale: 1.0 (NO CRECER)

Si necesitas ajustar tamaño del rayo:
├─ Scale X: 1.0
├─ Scale Y: 1.0  
└─ Scale Z: 1.0  (importante: altura fija, NO crece)
```

#### **Módulo 5: Mesh Renderer** (ÚLTIMA)
```
Renderer Settings:
├─ Mesh: Cone (o custom mesh láser)
├─ Material: M_LaserBeam (ver abajo)
├─ Alignment: Mesh Alignment Type = Velocity
├─ Scale: 
│  ├─ X: 0.5 (ancho del rayo)
│  ├─ Y: 0.5
│  └─ Z: Depende de la distancia entre BeamStart y BeamEnd
│
└─ Orientation:
   ├─ Use Local Space: NO
   └─ Particle Orientation: Aligned to Velocity
```

---

### PASO 4: Renderer Configuration — Visual

#### **Mesh Renderer Settings**

```
Asset:
├─ Mesh: /Engine/BasicShapes/Cone
├─ Material: M_LaserBeam (ver abajo)
└─ Overwrite Materials: Enabled

Transform:
├─ Use Local Space: NO
├─ Alignment: Mesh Alignment Type = Velocity
└─ Pivot Offset: (0, 0, 50)  // Centra la malla

Lighting:
├─ Cast Shadows: NO (los rayos no caster sombras)
├─ Receive Decals: NO
└─ Sorting Priority: 100
```

---

### PASO 5: Material del Rayo (M_LaserBeam)

Crea este material en Content Browser:

#### **Crear Material**

```
Right Click → Material → M_LaserBeam
```

#### **Graph del Material**

```
Base Color:
  ├─ Constant3Vector: (0.0, 0.8, 1.0)
  └─ Output → Base Color

Emissive Color:
  ├─ Constant3Vector: (0.0, 1.5, 2.0)
  ├─ Multiply por Constant(2.0) → Brightness
  └─ Output → Emissive Color

Roughness:
  ├─ Constant(0.1)
  └─ Output → Roughness

Metallic:
  ├─ Constant(1.0)
  └─ Output → Metallic

Blend Mode: Additive (NO Opaque)
Light Mode: Unlit (si es posible)
```

**Alternativa Simple:**
```
Si no quieres crear material:
1. Usa BasicShapeMaterial
2. Tintea con color cyan multiplicado
3. Añade emisión
```

---

### PASO 6: Ajustes en UltimateComponent.cpp

El código ACTUAL está correcto. Asegúrate de que:

```cpp
// En TryActivate():
LaserBeamComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    GetWorld(),
    LaserBeamVFX,
    Origin,  // ← Posición inicial
    GetLaserBeamRotation(Origin, bHit ? Hit.ImpactPoint : End),
    FVector(1.0f),  // ← Escala uniforme, NO CRECER
    false,
    false
);

// En TickComponent():
UpdateLaserBeam(Origin, bHit ? Hit.ImpactPoint : End);  // ← CADA FRAME

// En UpdateLaserBeam():
LaserBeamComponent->SetVectorParameter(LaserBeamStartParam, Origin);
LaserBeamComponent->SetVectorParameter(LaserBeamEndParam, End);
```

---

## 🔧 Checklist de Configuración

- [ ] NS_Ultimate tiene parámetros de usuario: `BeamStart` y `BeamEnd`
- [ ] El Emitter spawneа 20 partículas por segundo (NO en burst)
- [ ] Módulo de Position lerp correctamente entre BeamStart y BeamEnd
- [ ] Scale está FIJA en 1.0 (no crece)
- [ ] Renderer usa Mesh Alignment = Velocity
- [ ] Material es Additive o translúcido con Emissive
- [ ] El código llama a `UpdateLaserBeam()` cada frame
- [ ] `LaserBeamRotationOffset` es Yaw=-90° (ajusta si hace falta)

---

## 📊 Explicación Visual del Problema → Solución

### ❌ ANTES (Incorrecto)
```
Frame 1:  Pequeña esfera en el jugador
Frame 2:  Esfera más grande
Frame 3:  Esfera aún más grande ← CRECE EN TODAS DIRECCIONES
Frame 4:  Atraviesa al jugador

Causa: Scale/Lifetime creciendo sin control
```

### ✅ DESPUÉS (Correcto)
```
Frame 1:  Partícula en BeamStart (origen)
          Viaja hacia BeamEnd interpolando
Frame 2:  Partícula en mitad del camino
Frame 3:  Partícula cerca del destino
Frame 4:  Partícula desaparece (lifetime alcanzado)

Causa: Position lerpeada, Scale fija = rayo continuo
```

---

## 🎮 Testing

1. **Abre NS_Ultimate en el editor**
2. **Presiona Play en la preview**
3. **Verifica:**
   - [ ] Las partículas viajan en línea recta (no crecen)
   - [ ] No hay "bola" que se expande
   - [ ] Es un rayo continuo desde A hasta B

4. **En el juego:**
   - [ ] Mata 4 enemigos → barra llena
   - [ ] Presiona Ultimate
   - [ ] El rayo debe viajar correctamente

---

## 🐛 Si aún no funciona

### Síntoma: El rayo sigue creciendo
**Causa probable:** Módulo de Scale está creciendo
- Busca cualquier módulo que modifique Scale
- Desactívalo o cámbialo a valor fijo

### Síntoma: El rayo no se actualiza cada frame
**Causa probable:** UpdateLaserBeam() no se llama
- Verifica que TickComponent() está activo
- Comprueba logs: `bShowLaserDebug = true`

### Síntoma: Las partículas no se ven
**Causa probable:** Material no es emisivo/additive
- Verifica Blend Mode del material
- Comprueba que no está oculto (visibility)

---

## 📝 Notas de Implementación

El sistema se basa en:
1. **Spawning continuo** de partículas → rayo continuo
2. **Posición interpolada** → viaja desde origen a final
3. **Scale fija** → no crece
4. **Material brillante** → se ve bien

Esto crea un **rayo láser realista** que:
- Recorre el camino completo cada frame
- Se actualiza suavemente
- No atraviesa al jugador
- Responde a nuevos objetivos en tiempo real


