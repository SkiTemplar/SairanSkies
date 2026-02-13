# Estructura de Código - Enemy AI System

## 📁 Estructura de Carpetas

### `/Source/SairanSkies/Public/`

```
AI/
├── EnemyAIController.h
├── Tasks/
│   ├── BTTask_ApproachForAttack.h
│   ├── BTTask_AttackTarget.h
│   ├── BTTask_ChaseTarget.h
│   ├── BTTask_FindPatrolPoint.h
│   ├── BTTask_Investigate.h
│   ├── BTTask_MoveToLocation.h
│   ├── BTTask_PerformTaunt.h
│   ├── BTTask_PositionForAttack.h
│   └── BTTask_WaitAtPatrolPoint.h
├── Decorators/
│   ├── BTDecorator_CheckEnemyState.h
│   └── BTDecorator_HasTarget.h
└── Services/
    └── BTService_UpdateEnemyState.h

Enemies/
├── EnemyBase.h
├── EnemyTypes.h
└── Types/
    └── NormalEnemy.h

Navigation/
└── PatrolPath.h
```

### `/Source/SairanSkies/Private/`

```
AI/
├── EnemyAIController.cpp
├── Tasks/
│   ├── BTTask_ApproachForAttack.cpp
│   ├── BTTask_AttackTarget.cpp
│   ├── BTTask_ChaseTarget.cpp
│   ├── BTTask_FindPatrolPoint.cpp
│   ├── BTTask_Investigate.cpp
│   ├── BTTask_MoveToLocation.cpp
│   ├── BTTask_PerformTaunt.cpp
│   ├── BTTask_PositionForAttack.cpp
│   └── BTTask_WaitAtPatrolPoint.cpp
├── Decorators/
│   ├── BTDecorator_CheckEnemyState.cpp
│   └── BTDecorator_HasTarget.cpp
└── Services/
    └── BTService_UpdateEnemyState.cpp

Enemies/
├── EnemyBase.cpp
└── Types/
    └── NormalEnemy.cpp

Navigation/
└── PatrolPath.cpp
```

## 📝 Descripción de Carpetas

### **AI/**
Contiene todo el sistema de inteligencia artificial para los enemigos.

- **EnemyAIController**: Controlador principal de IA que gestiona el Behavior Tree y el sistema de percepción.

#### **AI/Tasks/**
Tareas del Behavior Tree que ejecutan acciones específicas:
- `BTTask_ApproachForAttack`: Acercarse al objetivo para atacar
- `BTTask_AttackTarget`: Ejecutar un ataque al objetivo
- `BTTask_ChaseTarget`: Perseguir al objetivo
- `BTTask_FindPatrolPoint`: Encontrar el siguiente punto de patrullaje
- `BTTask_Investigate`: Investigar un área específica
- `BTTask_MoveToLocation`: Moverse a una ubicación específica
- `BTTask_PerformTaunt`: Realizar una provocación/taunt
- `BTTask_PositionForAttack`: Posicionarse estratégicamente para atacar
- `BTTask_WaitAtPatrolPoint`: Esperar en un punto de patrullaje

#### **AI/Decorators/**
Decoradores que evalúan condiciones en el Behavior Tree:
- `BTDecorator_CheckEnemyState`: Verifica el estado actual del enemigo
- `BTDecorator_HasTarget`: Verifica si el enemigo tiene un objetivo

#### **AI/Services/**
Servicios que se ejecutan periódicamente en el Behavior Tree:
- `BTService_UpdateEnemyState`: Actualiza los valores del blackboard

### **Enemies/**
Contiene las clases base y tipos de enemigos.

- **EnemyBase**: Clase base abstracta para todos los enemigos
- **EnemyTypes**: Enumeraciones y estructuras de datos compartidas

#### **Enemies/Types/**
Implementaciones específicas de tipos de enemigos:
- `NormalEnemy`: Enemigo básico con comportamiento estándar

### **Navigation/**
Sistema de navegación y patrullaje.

- **PatrolPath**: Define rutas de patrullaje para los enemigos

## 🔧 Uso de Includes

Al referenciar archivos del proyecto, usa las rutas relativas desde `Source/SairanSkies/`:

```cpp
// Ejemplos:
#include "AI/EnemyAIController.h"
#include "Enemies/EnemyBase.h"
#include "Enemies/Types/NormalEnemy.h"
#include "Navigation/PatrolPath.h"
#include "AI/Tasks/BTTask_ChaseTarget.h"
#include "AI/Decorators/BTDecorator_HasTarget.h"
#include "AI/Services/BTService_UpdateEnemyState.h"
```

## 🎯 Beneficios de la Nueva Estructura

1. **Organización Clara**: Cada tipo de componente tiene su propia carpeta
2. **Fácil Navegación**: Encuentras rápidamente lo que buscas
3. **Escalabilidad**: Es fácil agregar nuevos tipos de enemigos o tareas
4. **Mantenibilidad**: El código relacionado está agrupado lógicamente
5. **Colaboración**: Múltiples desarrolladores pueden trabajar sin conflictos

## 📦 Próximos Pasos

Para agregar nuevos elementos:

### Nuevo Tipo de Enemigo
1. Crear `.h` en `Public/Enemies/Types/`
2. Crear `.cpp` en `Private/Enemies/Types/`
3. Heredar de `AEnemyBase`

### Nueva Tarea BT
1. Crear `.h` en `Public/AI/Tasks/`
2. Crear `.cpp` en `Private/AI/Tasks/`
3. Heredar de `UBTTaskNode`

### Nuevo Decorador BT
1. Crear `.h` en `Public/AI/Decorators/`
2. Crear `.cpp` en `Private/AI/Decorators/`
3. Heredar de `UBTDecorator`

### Nuevo Servicio BT
1. Crear `.h` en `Public/AI/Services/`
2. Crear `.cpp` en `Private/AI/Services/`
3. Heredar de `UBTService`

