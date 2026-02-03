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

## Roadmap

### 1. InputSystem (current focus)
- Complete keyboard and mouse input handling.
- Track input states: `pressed`, `held`, `released`.
- Handle window focus and input loss correctly.
- Separate input collection from game logic.

### 2. Event / Messaging System
- Central event dispatcher.
- Subscription-based input and system events.
  
### 3. Core Game Loop
- Implement fixed and variable timestep support.
- Define update order:  
  **Input → Update → Physics → Render**

### 4. Scene & Entity System
- Entity creation and destruction.
- Component-based architecture.
- Efficient component access.

### 5. Rendering Pipeline
- Camera abstraction.
- Basic material and shader system.
- Batch rendering for performance.

### 6. Physics & Collision (Minimal)
- Simple collision shapes (AABB).
- Basic collision detection and response.
- Integration with the update loop.

### 7. Resource Management
- Centralized asset loading.
- Resource caching and lifetime control.

### 8. Debugging & Tooling
- Logging system.
- Debug overlays (FPS, input state).
- Runtime diagnostics.
