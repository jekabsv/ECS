# ECS-based game engine

C++ engine prototype built around a custom **archetype-based ECS (Entity Component System)**, **SDL3** rendering, and a **state machine**.

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

## Requirements

- **Windows**
- **Visual Studio 2022** (v143 toolset)
- **C++20**
- **SDL 3.2.x**
  - Headers and libraries must be installed locally

---

