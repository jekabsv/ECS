# jekabsv-ecs

A small C++ game/engine prototype built around a custom **archetype-based ECS (Entity Component System)**, **SDL3** rendering, and a simple **state machine**. The project targets Windows (Visual Studio) and contains partial support for Emscripten/WASM.

---

## Features

- Custom **archetype ECS**
  - Cache-friendly component storage
  - Dynamic add/remove of components
  - Signature-based archetype matching
- **SDL3** rendering
  - Textured sprites (`SimpleSprite`)
  - Geometry rendering (`SDL_RenderGeometry`)
- **State machine**
  - Stack-based states (`StartState`, etc.)
- **Asset manager**
  - Mesh storage
  - BMP texture loading
- Basic **input handling** (keyboard via SDL)

---

## Project Structure


---

## Requirements

- **Windows**
- **Visual Studio 2022** (v143 toolset)
- **C++20**
- **SDL 3.2.x**
  - Headers and libraries must be installed locally

---

## Build Instructions (Windows)

1. Install **SDL3**
   - Example path used in the project:
     ```
     C:\Users\<user>\Documents\SDL3-3.2.26\
     ```
2. Open `ECS.sln` in Visual Studio
3. Select configuration:
   - `Debug | x64` or `Release | x64`
4. Build and run

> SDL include and library paths are already set in `ECS.vcxproj` for x64.

---

## Controls (StartState)

| Key | Action |
|----|-------|
| W A S D | Move player |
| Q | Increase scale |
| E | Decrease scale |
| Close window | Quit |

---

## ECS Overview

- **Entity**: `uint32_t`
- **Components**:
  - `RenderComponent`
  - `MeshComponent`
  - `SimpleSprite`
- **Iteration**:
  ```cpp
  ecs.for_each(ecs.CreateMask<RenderComponent>(), callback);
