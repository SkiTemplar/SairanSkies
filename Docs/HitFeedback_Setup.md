# 🎮 Guía de Configuración: Hitstop y Camera Shake

## ❗ IMPORTANTE
Este documento explica paso a paso cómo configurar el feedback visual al golpear enemigos.

---

## 🛠️ Parte 1: HITSTOP (Pausa del Juego)

El hitstop ya está implementado en el código y debería funcionar automáticamente.

### ¿Qué hace?
- Pausa el juego brevemente (0.05 segundos por defecto) al golpear
- Crea sensación de impacto
- Se puede ajustar la duración

### Configuración:
1. Abre el Blueprint de tu personaje: `BP_SairanCharacter`
2. Selecciona el componente `CombatComponent` (en la lista de componentes a la izquierda)
3. En el panel de detalles (derecha), busca la categoría **"Combat | Hit Feedback"**
4. Ajusta el valor de **"Hitstop Duration"**:
   - `0.05` = Sutil (recomendado para ataques ligeros)
   - `0.1` = Más notorio
   - `0.15` = Muy impactante (para ataques cargados)

### Debug:
Cuando golpees a un enemigo, deberías ver en el **Output Log**:
```
LogTemp: HITSTOP TRIGGERED - Duration: 0.050000
LogTemp: HITSTOP ENDED - Game speed resumed
```

Si NO ves estos logs, el problema está en que el golpe no se está detectando.

---

## 📹 Parte 2: CAMERA SHAKE (Vibración de Cámara)

Este requiere **crear un Blueprint** de Camera Shake.

### 🔧 Paso 1: Crear el Camera Shake Blueprint

1. **En el Content Browser**, navega a `Content/Progra/` (o donde guardes tus Blueprints)

2. **Clic derecho** en un espacio vacío → **Blueprint Class**

3. En la ventana "Pick Parent Class":
   - **NO selecciones Actor**
   - Expande **"All Classes"** (abajo a la izquierda)
   - En la barra de búsqueda escribe: `CameraShakeBase`
   - Selecciona **"MatineeCameraShakeBase"** (es más fácil de configurar que la nueva)
   - Dale un nombre: `CS_HitShake` (CS = Camera Shake)

4. **Abre el Blueprint** `CS_HitShake`

### 🎛️ Paso 2: Configurar el Camera Shake

En el Blueprint `CS_HitShake`, busca estas propiedades en el panel de detalles:

#### **Oscillation Duration**
- Tipo: `float`
- Valor recomendado: `0.2` (200ms de shake)
- Esto controla cuánto dura la vibración

#### **Location Oscillation** (Vibración de posición)
Expande esta sección:
- **X → Amplitude**: `5.0` (vibración horizontal leve)
- **Y → Amplitude**: `5.0` (vibración lateral leve)
- **Z → Amplitude**: `0.0` (sin vibración vertical)
- **Frequency**: `30.0` (rapidez de la vibración)

#### **Rotation Oscillation** (Vibración de rotación)
Expande esta sección:
- **Pitch → Amplitude**: `0.5` (inclinación arriba/abajo)
- **Yaw → Amplitude**: `0.5` (rotación izquierda/derecha)
- **Roll → Amplitude**: `0.2` (inclinación lateral)
- **Frequency**: `30.0`

#### **FOV Oscillation** (Opcional - cambio de zoom)
- **Amplitude**: `2.0` (ligero zoom in/out)
- **Frequency**: `30.0`

**Guarda y compila** el Blueprint.

### 🔗 Paso 3: Asignar el Camera Shake al Personaje

1. Abre `BP_SairanCharacter`
2. Selecciona el componente **CombatComponent**
3. En el panel de detalles, busca **"Combat | Hit Feedback"**
4. Encuentra la propiedad **"Hit Camera Shake"**
5. En el dropdown, selecciona `CS_HitShake` (el Blueprint que acabas de crear)
6. Ajusta **"Camera Shake Intensity"** si quieres (1.0 por defecto):
   - `0.5` = Sutil
   - `1.0` = Normal
   - `2.0` = Muy intenso

**Compila y guarda** el Blueprint.

---

## 🧪 Parte 3: TESTING

### Prueba en el juego:
1. Presiona **Play**
2. Acércate a un enemigo (debe tener el tag `Enemy`)
3. Ataca con **R1** o **Clic Izquierdo**

### Lo que deberías ver:
- ✅ El juego se pausa brevemente (hitstop)
- ✅ La cámara vibra/tiembla
- ✅ El enemigo retrocede (knockback)

### Debug en el Output Log:
Presiona `Ctrl + Shift + F1` para abrir el Output Log mientras juegas, deberías ver:
```
LogTemp: HITSTOP TRIGGERED - Duration: 0.050000
LogTemp: CAMERA SHAKE TRIGGERED - Intensity: 1.000000
LogTemp: HITSTOP ENDED - Game speed resumed
```

### Si NO funciona:

#### ❌ No veo los logs de Hitstop:
- El golpe no está conectando
- Verifica que el enemigo tiene el tag `Enemy`
- Verifica que estás en rango (10 metros)

#### ❌ Veo el log pero no siento el hitstop:
- Aumenta `Hitstop Duration` a `0.15` para que sea más obvio
- Verifica que no hay otros sistemas que interfieran con el Time Dilation

#### ❌ Veo "No HitCameraShake assigned":
- No has creado o asignado el Camera Shake Blueprint
- Repite los pasos 1-3 de la Parte 2

#### ❌ El camera shake es demasiado sutil:
- Aumenta las **Amplitude** en el `CS_HitShake` Blueprint
- Aumenta la **Camera Shake Intensity** en el CombatComponent

---

## 🎨 Consejos de Diseño

### Para Ataques Ligeros:
- Hitstop: `0.03` - `0.05`
- Shake Intensity: `0.5` - `0.8`

### Para Ataques Pesados:
- Hitstop: `0.08` - `0.12`
- Shake Intensity: `1.2` - `1.5`

### Para Ataques Cargados:
- Hitstop: `0.15` - `0.2`
- Shake Intensity: `2.0` - `3.0`

### Tip Pro:
Puedes crear 3 Camera Shakes diferentes:
- `CS_LightHitShake`
- `CS_HeavyHitShake`
- `CS_ChargedHitShake`

Y cambiarlos dinámicamente en el código según el tipo de ataque.

---

## 📝 Siguientes Pasos Opcionales

### Añadir Partículas de Impacto:
1. Crea un sistema Niagara de chispas/impacto
2. En `BP_SairanCharacter` → `CombatComponent` → **"Hit Particle System"**, asígnalo

### Añadir Sonido de Impacto:
1. Importa un sonido de golpe metálico
2. En `BP_SairanCharacter` → `CombatComponent` → **"Hit Sound"**, asígnalo

---

## ⚠️ Troubleshooting Final

Si después de todo esto sigue sin funcionar, verifica:

1. ¿El proyecto compila sin errores?
   - Ejecuta `Build` → `Build Solution` en Rider/Visual Studio

2. ¿Has guardado TODOS los Blueprints?
   - `BP_SairanCharacter`
   - `CS_HitShake`

3. ¿El enemigo tiene el tag "Enemy"?
   - Selecciona el actor enemigo en el nivel
   - En el panel de detalles, busca "Tags"
   - Debe aparecer `Enemy`

4. ¿Estás en rango de ataque?
   - El targeting funciona hasta 10 metros
   - El hit detection tiene un radio de 1.5 metros

5. ¿Hay algún error en el Output Log?
   - Busca líneas rojas o amarillas
   - Compártelas para ayudarte mejor

---

**¡Ahora deberías tener un combate con feedback satisfactorio!** 🎉
