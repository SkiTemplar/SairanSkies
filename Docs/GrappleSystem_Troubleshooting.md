# 🔧 Troubleshooting - Sistema de Gancho

## ⚠️ Problema: Cámara no se bloquea y partículas no aparecen

### ✅ Solución Implementada

He corregido el código. Las funciones `LockCamera()`, `UnlockCamera()`, `StartGrappleTrailParticles()` y `StopGrappleTrailParticles()` no se estaban llamando correctamente.

**Archivos corregidos:**
- `GrappleComponent.cpp` - Añadidas todas las llamadas faltantes
- `GrappleComponent.cpp` - Implementadas todas las funciones al final del archivo

---

## 📋 Checklist de Verificación

### 1. Compilar el Proyecto

```
En Unreal Engine:
- Ctrl + Shift + B
O
- Botón "Compile" en la barra de herramientas
```

**Espera a que diga "Compile Complete" sin errores.**

---

### 2. Verificar Asignación del Sistema de Partículas

1. Abre `BP_SairanCharacter`
2. Selecciona el componente **GrappleComponent**
3. En el panel de detalles, busca la categoría **Grapple | Visuals**
4. Verifica que **Grapple Trail Particles** tiene asignado tu Niagara System

**Si está vacío:**
- Crea un Niagara System primero (ver instrucciones abajo)
- O usa uno existente del proyecto
- Asígnalo al campo

---

### 3. Activar Debug Mode

Para verificar que las funciones se están llamando:

1. En `BP_SairanCharacter` → `GrappleComponent`
2. Busca **Grapple | Debug**
3. Activa **bShowDebug** = `TRUE`

**Mensajes que deberías ver al jugar:**

```
Al disparar el gancho:
- "Grapple: Camera locked" (naranja)
- "Grapple: Particle trail started" (magenta)

Al pasar el punto medio:
- "Grapple: Camera unlocked" (naranja)
- "Grapple: Particle trail stopped" (magenta)
```

**Si ves estos mensajes de error:**
- "Grapple: No particle system or owner!" → Sistema de partículas no asignado
- "Grapple: Failed to spawn particles!" → Sistema de partículas inválido o corrupto
- "Grapple: No camera boom!" → Problema con el CameraBoom del personaje

---

### 4. Crear Niagara System (Si no tienes uno)

#### Paso A: Crear el Sistema

1. Content Browser → Click derecho
2. **FX** → **Niagara System**
3. Selecciona "New system from selected emitters"
4. O usa el template **Fountain**
5. Nombra: `NS_GrappleTrail`

#### Paso B: Configurar Emisor

Abre `NS_GrappleTrail` y configura:

**IMPORTANTE - Emitter State (para emisión continua):**
```
Emitter State:
- Loop Behavior: Infinite  <-- MUY IMPORTANTE
- Inactive Response: Complete
- Life Cycle Mode: Self
```

**Spawn Rate:**
```
Spawn Rate:
- Spawn Rate: 50-100 (partículas por segundo)
```

**Particle Spawn:**
```
Lifetime Mode: Random
- Min: 0.5
- Max: 1.0
```

**Initialize Particle:**
```
Color Mode: Direct Set
- Color: Azul/Cyan brillante (0, 0.5, 1, 1)

Sprite Size Mode: Uniform
- Uniform Sprite Size: 10.0

Velocity Mode: Random Range
- Minimum: (-10, -10, -10)
- Maximum: (10, 10, 10)
```

**Particle Update:**
```
Drag: 5.0 (para que las partículas se queden flotando)

Color Over Life:
- Gradient: De color sólido a transparente
- Alpha al final: 0 (fade out)
```

**Renderer:**
```
Sprite Renderer:
- Sprite Alignment: Velocity Aligned
- Facing Mode: Face Camera
```

#### Paso C: Guardar y Asignar

1. Guarda el sistema
2. Asígnalo en `GrappleComponent` → **Grapple Trail Particles**

---

### 5. Verificar la Cámara en el Personaje

Para que el bloqueo funcione, el personaje debe tener el CameraBoom configurado correctamente:

1. Abre `BP_SairanCharacter`
2. Selecciona el componente **CameraBoom**
3. Verifica estas propiedades:

```
Camera Settings:
- Use Pawn Control Rotation: TRUE (por defecto)
- Enable Camera Lag: TRUE (por defecto)
- Enable Camera Rotation Lag: TRUE (por defecto)
```

Estos valores deben estar en `TRUE` **antes** de jugar. El código los guarda al inicio y los cambia temporalmente durante el gancho.

---

### 6. Probar en el Juego

#### Test 1: Cámara Bloqueada

1. Apunta con el gancho (L2/F)
2. Dispara a un objetivo válido
3. **Durante el impulso:**
   - Mueve el ratón/stick derecho
   - La cámara **NO DEBERÍA MOVERSE**
4. Al pasar el punto medio:
   - La cámara debería **volver a responder** al input

#### Test 2: Partículas

1. Dispara el gancho
2. **Durante el impulso:**
   - Deberías ver partículas saliendo del personaje
   - Forman un trail/estela detrás
3. Al pasar el punto medio:
   - Las partículas **DEJAN DE GENERARSE**
   - Las existentes se disipan naturalmente

---

## 🐛 Problemas Comunes

### La cámara se mueve durante el impulso

**Causas posibles:**
1. El código no se compiló correctamente
2. `bShowDebug` no muestra "Camera locked"
3. El CameraBoom tiene valores incorrectos guardados

**Solución:**
1. Recompila el proyecto
2. Activa debug y verifica mensajes
3. Reinicia el nivel (para que BeginPlay se ejecute)

---

### Las partículas no aparecen

**Causas posibles:**
1. Sistema de partículas no asignado
2. Sistema de partículas corrupto o mal configurado
3. El sistema se desactiva inmediatamente

**Solución:**

**Verifica el debug:**
```
Si ves "No particle system or owner!" → Asigna el sistema
Si ves "Failed to spawn particles!" → El sistema está corrupto, créalo de nuevo
Si ves "Particle trail started" → El sistema SÍ se activa, revisa su configuración
```

**Revisa el Niagara System:**
1. Abre el sistema
2. Verifica que el **Emitter State** esté en "Active"
3. Verifica que **Spawn Rate** sea > 0
4. Verifica que **Lifetime** sea > 0

---

### Las partículas aparecen pero se ven mal

**Ajustes recomendados:**

**Demasiado densas:**
- Reduce **Spawn Rate** a 30-50

**Desaparecen muy rápido:**
- Aumenta **Lifetime** a 1.0-2.0

**No dejan estela:**
- Activa **Velocity Aligned** en el Sprite Renderer
- O usa un **Ribbon Renderer** en lugar de Sprite

**Color muy apagado:**
- Aumenta **Color** brightness
- Desactiva **Color Over Life** si no quieres fade

---

## 📊 Verificación Final

Antes de dar por terminado, verifica:

- [ ] Compilación sin errores
- [ ] Sistema de partículas asignado en GrappleComponent
- [ ] Debug activado y muestra mensajes correctos
- [ ] Al disparar: "Camera locked" y "Particle trail started"
- [ ] Al soltar: "Camera unlocked" y "Particle trail stopped"
- [ ] La cámara NO responde al input durante el impulso
- [ ] Las partículas aparecen y forman una estela
- [ ] Todo se desactiva correctamente al pasar el punto medio

---

## 💡 Tips Adicionales

### Para un mejor efecto visual:

**Partículas tipo Cable:**
1. Usa **Niagara Ribbon Renderer** en lugar de Sprite
2. Configura el ribbon para que vaya del personaje al punto de enganche
3. Añade física simulada para que se curve

**Sonido:**
1. Añade un sonido al disparar el gancho
2. Añade un sonido de "cable tensándose" durante el impulso
3. Añade un "whoosh" al pasar el punto medio

**Cámara Shake:**
1. Al disparar, añade un ligero camera shake
2. Durante el impulso, un shake constante sutil
3. Al soltar, un shake más fuerte

---

**Si sigues teniendo problemas después de estos pasos, avísame con los mensajes de debug que ves.**
