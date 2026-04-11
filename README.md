# ECS-based Game Engine

**Author:** Jēkabs Vidrusks  
**Format:** Desktop C++ application (Windows)

![ECS](https://github.com/jekabsv/ECS/blob/master/ECS.jpg)

---

## Table of Contents

- [Description](#description)
- [Features](#features)
- [High-Level Architecture](#high-level-architecture)
- [Requirements](#requirements)
- [Technologies Used](#technologies-used)
- [Roadmap](#roadmap)

## Description

This project is a **desktop C++ game engine** built around a **custom archetype-based Entity Component System (ECS)**.  
The engine is designed to be modular, data-oriented, and suitable for both **runtime gameplay** and a future **editor + preview workflow**.

---

## Features

### Custom Archetype ECS
- Cache-friendly component storage
- Dynamic add/remove of components
- Signature-based archetype matching
- Built-in and user-defined components
- Built-in and user-defined systems

### Rendering (SDL3)
- Textured sprites (`SimpleSprite`)
- Geometry rendering (`SDL_RenderGeometry`)

### State Machine
- Stack-based state system
- Explicit state lifecycle:
  - initialize
  - process input
  - update
  - physics
  - render
  - terminate / pause

### Asset Manager
- Centralized asset access
- Mesh storage
- BMP texture loading
- Asset cache for reuse

### Input Handling (Basic)
- Keyboard input via SDL
- Translation of raw input into actions

---

## High-Level Architecture

### 1. Editor / World Creation
- Editor starts.
- User chooses an existing game world or creates a new one.
- Objects, scripts, and data are added.
- World data is stored as **Game World Data (`.json`)**.
- Preview mode runs the game in a WASM-compatible configuration.
- On save, world data is written to non-volatile storage.

### 2. Game Startup
- Game is opened.
- World data (`.json`) is read.
- World initialization creates entities and components.
- Initial state (`StartState`) is pushed into the state machine.
- Assets required by the state are loaded.

### 3. State Machine
- Manages active states using a stack.
- States can:
  - Initialize
  - Process input
  - Update logic
  - Update physics
  - Render
- States may push, replace, or terminate other states.

### 4. Input Flow
- User input is collected.
- Raw input is translated into actions.
- Actions are written into input components in the ECS.
- Input is processed during the state update phase.

### 5. ECS Execution
- ECS receives processing requests from active states.
- ECS:
  - Finds matching archetypes via component signatures
  - Runs all systems matching those archetypes
- Systems operate on built-in and user-defined components.

### 6. Systems & Events
- Systems may dispatch events.
- Event dispatcher resolves events.
- Event cache is cleared each frame.
- Systems react to events without tight coupling.

### 7. Rendering & Assets
- Render systems query ECS for renderable entities.
- Asset manager provides textures and meshes.
- Asset cache prevents duplicate loading.
- Renderer outputs the final frame.

### 8. Saving
- On save:
  - World state is serialized
  - Data is written to non-volatile storage
  - Data can later be reloaded by the editor or runtime

---

## Requirements

- **Windows**
- **Visual Studio 2022** (v143 toolset)
- **C++20**
- **SDL 3.2.x**
  - Headers and libraries must be installed locally

---

## ToDo

### 1. PhysicSystem
- add BoxCollider, SphereCollider, CapsuleCollider, MeshCollider, etc..
- RigidBody - add gravity, angular velocity, Various constraints like freeze position/rotation axes etc..
- Add Physics Materials — assigned to colliders that define friction and bounciness
- Add Joints
- Add Querries like Raycast, OverlapBox, SphereCast, etc..

### 2. UI
- extend UI system

### 3. Core Game Loop
- Deterministic update order:  
  **Input → Update → Physics → Render**

### 4. Rendering Pipeline
- Camera abstraction.
- Shader abstraction.
- Batch rendering for performance.

### 5. Resource Management
- Improved asset lifetime control.
- Explicit load/unload policies.

### 6. Debugging & Tooling
- Debug overlays (FPS, input states).
- Runtime diagnostics and validation tools.

### 7. Consitency
- Change everything to have the same naming, logic conventions
- Review InputComponent, preferably removed

### 8. ECS
-Update system groups
  - add .before(System) .after(System) to System Builder
  - remove swap remove


## Technologies Used

- **C++20**
- **Visual Studio 2022**
- **SDL 3**
- **SDL_TTF**
- **Emscripten** (WASM builds / preview workflow)
- **JavaScript**
- **ES Modules (mjs)**
- **Node.js**
- **TypeScript**
- **React**
- **Tailwind CSS**
- **JSON**
- **Git**
