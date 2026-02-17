# Sistema de Gancho (Grappling Hook) - Guía de Configuración

## 📋 Resumen

El sistema de gancho permite al jugador apuntar a cualquier punto del escenario (dentro del rango máximo de 30 metros) y propulsarse hacia él usando **física real** (como Batman Arkham). El gancho se "retrae" aplicando fuerza continua al personaje.

### Características:
- **Física real**: Impulsos físicos que respetan colisiones
- **Rotación de personaje**: El personaje sigue la cámara al apuntar
- **Crosshair UI**: Mira roja (inválido) / verde (válido)
- **Trayectoria natural**: 15° de offset para evitar colisionar con el punto de enganche
- **Cámara bloqueada**: Durante el impulso la cámara se lockea (sensación de ser arrastrado)
- **Efecto de partículas**: Trail de partículas desde el personaje durante el impulso

---

## 🎮 Controles

| Plataforma | Control | Acción |
|------------|---------|--------|
| **Mando** | L2 (LT) mantener | Entrar en modo apuntado |
| **Mando** | L2 (LT) soltar | Disparar gancho al punto apuntado |
| **PC** | F mantener | Entrar en modo apuntado |
| **PC** | F soltar | Disparar gancho al punto apuntado |

---

## ⚙️ Configuración en Editor

### Paso 1: Crear Input Action

1. Ve a `Content/Progra/InputActions/`
2. Click derecho → **Input** → **Input Action**
3. Nombra el archivo: `IA_Grapple`
4. Abre el asset y configura:
   - **Value Type**: `Digital (bool)` o `Axis1D (float)`
   - **Triggers**: Ninguno (usamos Started/Completed)

### Paso 2: Añadir al Input Mapping Context

1. Abre `Content/Progra/InputMappingContext/IMC_Default`
2. Añade un nuevo mapping:
   - **Input Action**: `IA_Grapple`
3. Añade los siguientes bindings:

#### Para Mando:
- **Gamepad Left Trigger (L2/LT)**

#### Para Teclado:
- **Tecla F**

### Paso 3: Crear Blueprint del Gancho Visual (Opcional pero recomendado)

1. En Content Browser, click derecho → **Blueprint Class**
2. Selecciona `GrappleHookActor` como padre
3. Nombra el archivo: `BP_GrappleHook`
4. Abre el Blueprint y ajusta las propiedades visuales:
   - `HookSize`: Tamaño de la punta del gancho
   - `HandleSize`: Tamaño del mango
   - `GrappleColor`: Color del gancho

### Paso 4: Asignar en Blueprint del Personaje

1. Abre el Blueprint del personaje (ej: `BP_SairanCharacter`)
2. En el panel de detalles, busca la sección **Input**
3. Asigna `IA_Grapple` al campo **Grapple Action**
4. En la sección **Grapple | Visual**:
   - Asigna `BP_GrappleHook` al campo **Grapple Hook Class**

### Paso 5: Crear Widget Blueprint del Crosshair

1. Content Browser → Click derecho → **User Interface** → **Widget Blueprint**
2. En la ventana de selección, busca y selecciona `GrappleCrosshairWidget` como clase padre
3. Nombra el archivo: `WBP_GrappleCrosshair`
4. Abre el Widget Blueprint y diseña:

```
Estructura requerida:
├── Canvas Panel (root)
│   ├── CrosshairRed (Image - nombre EXACTO)
│   │   ├── Anchor: Center
│   │   ├── Position: (0, 0)
│   │   ├── Size: (32, 32) o el tamaño que prefieras
│   │   └── Color: Rojo
│   │
│   └── CrosshairGreen (Image - nombre EXACTO)
│       ├── Anchor: Center
│       ├── Position: (0, 0)
│       ├── Size: (32, 32)
│       └── Color: Verde
```

**IMPORTANTE**: Los nombres de las imágenes deben ser exactamente `CrosshairRed` y `CrosshairGreen` para que el binding funcione.

### Paso 6: Asignar Widget en el Personaje

1. Abre `BP_SairanCharacter`
2. Selecciona el componente **GrappleComponent**
3. En **Grapple | UI** → asigna `WBP_GrappleCrosshair` a **Crosshair Widget Class**

### Paso 7: Crear Sistema de Partículas (Opcional pero recomendado)

1. Content Browser → Click derecho → **FX** → **Niagara System**
2. Crea un sistema de partículas que salga del personaje (trail/estela)
3. Nombra: `NS_GrappleTrail`
4. Abre `BP_SairanCharacter` → Selecciona **GrappleComponent**
5. En **Grapple | Visuals** → asigna `NS_GrappleTrail` a **Grapple Trail Particles**

**Sugerencias para el efecto:**
- Emisión continua desde el origen (personaje)
- Color brillante (azul/cyan)
- Velocidad inicial 0
- Lifetime corto (0.5-1 segundo)
- Tamaño pequeño-mediano

---

## 🔧 Parámetros Configurables

Todos los parámetros se pueden ajustar en el **GrappleComponent** del personaje:

### Settings

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| `MaxGrappleRange` | 3000 (30m) | Rango máximo del gancho |
| `GrapplePullSpeed` | 2500 | Velocidad de propulsión |
| `GrappleAngleOffset` | 15° | Ángulo hacia abajo para evitar colisión |
| `MidpointReleaseDistance` | 100 (1m) | Distancia horizontal para soltar |

### Camera

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| `AimingCameraDistance` | 150 | Distancia de cámara al apuntar |
| `CameraZoomSpeed` | 8.0 | Velocidad de transición de cámara |
| `AimingCameraOffset` | (0, 70, 30) | Offset de cámara al apuntar |

### Visuals/Debug

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| `ValidTargetColor` | Verde | Color cuando hay objetivo válido |
| `InvalidTargetColor` | Rojo | Color cuando no hay objetivo |
| `bShowDebug` | false | Mostrar líneas de debug |

---

## 🎯 Cómo Funciona

### Flujo de la Mecánica:

```
1. IDLE (Inactivo)
   ↓ [Presiona L2/F]
2. AIMING (Apuntando)
   - Cámara se acerca al hombro
   - Personaje rota con la cámara
   - Crosshair aparece (rojo/verde según validez)
   - Trace continuo desde el centro de pantalla
   ↓ [Suelta L2/F con objetivo válido]
3. PULLING (Propulsión)
   - Impulso físico hacia el punto
   - **Cámara se bloquea** (sensación de ser arrastrado)
   - **Trail de partículas activo** desde el personaje
   - Fuerza continua (gancho retrayéndose)
   - Dirección ajustada 15° hacia abajo
   ↓ [Pasa el punto medio horizontal]
4. RELEASING (Liberación)
   - **Cámara se desbloquea** (control restaurado)
   - **Partículas se detienen**
   - Física normal toma el control (Falling)
   - El jugador mantiene momentum reducido
   - Cae naturalmente hacia el destino
   ↓ [Aterriza]
5. IDLE (Vuelve a estado inicial)
```

### Cálculo del Punto Medio:

El gancho suelta al jugador cuando la **distancia horizontal** (X,Y) al punto de enganche es menor a `MidpointReleaseDistance` (1 metro por defecto). Esto permite que el jugador:

1. Se acerque al punto de enganche
2. Pase por debajo/al lado del punto
3. Continue el momentum en una trayectoria parabólica natural
4. Aterrice en el destino deseado

---

## 📡 Eventos Blueprint

Puedes bindear estos eventos en Blueprint para efectos visuales/sonidos:

| Evento | Cuándo se dispara |
|--------|-------------------|
| `OnGrappleAimStart` | Al empezar a apuntar |
| `OnGrappleAimEnd` | Al dejar de apuntar (sin disparar) |
| `OnGrappleFired` | Al disparar el gancho (incluye ubicación) |
| `OnGrappleComplete` | Al soltar en el punto medio |

### Ejemplo de uso en Blueprint:

```
Event OnGrappleFired (TargetLocation)
├── Spawn Niagara System at TargetLocation
├── Play Sound "Grapple_Fire"
└── Spawn Rope Visual (from hand to target)
```

---

## 🎨 Futuras Mejoras (Post-Demo)

### Visual del Gancho:
- [ ] Mesh del gancho en mano izquierda
- [ ] Cuerda visual conectando mano con objetivo
- [ ] Partículas al impactar
- [ ] Animación de brazos

### Crosshair/Mira:
- [ ] Widget de mira en centro de pantalla
- [ ] Cambio de color según validez del objetivo
- [ ] Indicador de rango

### Gameplay:
- [ ] Puntos de gancho específicos (actores grappleable)
- [ ] Cooldown del gancho
- [ ] Combo con otros movimientos (gancho + ataque aéreo)

---

## 🐛 Solución de Problemas

### El gancho no se activa:
1. Verifica que `IA_Grapple` esté asignado en el Blueprint del personaje
2. Verifica que el mapping esté en `IMC_Default`
3. Asegúrate de que el personaje puede realizar acciones (`CanPerformAction()`)

### La cámara no cambia:
- Verifica que el `CameraBoom` esté configurado correctamente
- Comprueba que `bCameraTransitioning` se active

### El personaje atraviesa paredes:
- Aumenta `GrappleAngleOffset` para ir más bajo
- Reduce `GrapplePullSpeed` para más control
- Considera añadir collision checks durante el vuelo

### El jugador no suelta en el punto correcto:
- Ajusta `MidpointReleaseDistance` según necesidades
- El valor actual (100 = 1 metro) funciona bien para la mayoría de casos

---

## ✅ Checklist de Configuración

- [ ] Crear `IA_Grapple` Input Action
- [ ] Añadir binding de L2/LT para mando
- [ ] Añadir binding de F para teclado
- [ ] Asignar `IA_Grapple` en el Blueprint del personaje
- [ ] (Opcional) Activar `bShowDebug` para testing
- [ ] (Opcional) Ajustar parámetros según feedback de gameplay

---

**Compilación:** ✅ Sin errores  
**Estado:** Listo para testing
