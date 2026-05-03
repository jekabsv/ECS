# Engine Documentation

A walkthrough of every major system in the engine — what each class does, what methods are available, and how to use them together.

---

## Table of Contents

1. [SharedData — The Global Context](#1-shareddata--the-global-context)
2. [ECS World — Entities, Components & Systems](#2-ecs-world--entities-components--systems)
3. [InputSystem — Actions, Bindings & Devices](#3-inputsystem--actions-bindings--devices)
4. [UI Context — Retained-Mode Interface](#4-ui-context--retained-mode-interface)
5. [AnimationSystem — Sprite Frame Animations](#5-animationsystem--sprite-frame-animations)
6. [PhysicsSystem — Collision & Movement](#6-physicssystem--collision--movement)
7. [SpatialIndex — Broad-Phase Queries](#7-spatialindex--broad-phase-queries)
8. [AssetManager — Textures, Fonts, Shaders & Meshes](#8-assetmanager--textures-fonts-shaders--meshes)
9. [Blackboard — Typed Key-Value Store](#9-blackboard--typed-key-value-store)
10. [Logger — Structured Logging](#10-logger--structured-logging)
11. [StateMachine — Game State Stack](#11-statemachine--game-state-stack)
12. [Putting It All Together — A Minimal Game State](#12-putting-it-all-together--a-minimal-game-state)

---

## 1. SharedData — The Global Context

`SharedData` (accessed via `SharedDataRef`, a `shared_ptr<SharedData>`) is passed into every system lambda and every `State`. It owns every engine subsystem.

```cpp
struct SharedData {
    const int GAME_WIDTH  = 1920;
    const int GAME_HEIGHT = 1080;

    StateMachine    state;
    AssetManager    assets;
    InputSystem::System inputs;
    AnimationSystem animation;
    PhysicsSystem   physics;
    SpatialIndex    spatialIndex;
    Blackboard      session;
    Renderer        renderer;
    bool            quit = false;
};
```

Inside a system lambda you receive it as `SharedDataRef _data`. Access any subsystem through it:

```cpp
_data->inputs.IsPressed("jump");
_data->animation.Play(player.anim, "run");
_data->physics.GetContacts(entity);
```

---

## 2. ECS World — Entities, Components & Systems

`ECS::World` (referred to as `ecs` inside a `State`) is the core of the engine. It manages entities, attaches components, and schedules systems.

### Entity Lifecycle

```cpp
// Create a new entity — returns a handle (uint64_t)
Entity e = ecs.Create();

// Destroy an entity and reclaim its ID
ecs.Destroy(e);

// Check whether a handle still refers to a live entity
bool alive = ecs.Alive(e);
```

> Handles are generational. After `Destroy(e)`, the old handle is invalid even if a new entity is created with the same numeric ID.

### Adding & Removing Components

```cpp
// Add a single component with an initial value
ecs.Add<TransformComponent>(e, TransformComponent({ 100.f, 200.f, 0.f }, { 1.f, 1.f }));

// Add multiple components in one call (variadic)
ecs.Add<RigidBody>(e, RigidBody{});
ecs.Add<BoxCollider>(e, BoxCollider(32.f, 32.f, false));

// Remove a component
ecs.Remove<BoxCollider>(e);

// Remove multiple components at once
ecs.Remove<RigidBody, BoxCollider>(e);
```

### Accessing Components

```cpp
// Get a reference — throws/asserts if the component is missing
TransformComponent& tr = ecs.Get<TransformComponent>(e);

// Get a pointer — returns nullptr if missing (safe)
RigidBody* rb = ecs.TryGet<RigidBody>(e);

// Check presence without fetching
bool hasCollider = ecs.Has<BoxCollider>(e);
```

### Querying Entities

`Query` iterates every entity that has **all** of the listed component types:

```cpp
ecs.Query<TransformComponent, RigidBody>([](Entity e, TransformComponent& tr, RigidBody& rb) {
    tr.position.x += rb.vx * dt;
    tr.position.y += rb.vy * dt;
});
```

### Registering Systems

Systems are lambdas registered with a name, a component filter, and a `SystemGroup` that controls when they run each frame.

```cpp
// Signature: (ArchetypeContext ctx, float dt, SharedDataRef data)
ecs.RegisterSystem<TransformComponent, RigidBody>(
    "moveSystem",
    [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data) {
        auto transforms = ctx.Slice<TransformComponent>();
        auto bodies     = ctx.Slice<RigidBody>();
        for (std::size_t i = 0; i < transforms.size(); i++) {
            transforms[i].position.x += bodies[i].vx * dt;
            transforms[i].position.y += bodies[i].vy * dt;
        }
    },
    ECS::SystemGroup::Update
);
```

**Available `SystemGroup` values and their execution order:**

| Group | When it runs |
|---|---|
| `Initialise` | Once per frame, before everything else |
| `PreUpdate` | Before main update |
| `Update` | Main game logic |
| `PostUpdate` | After main update |
| `Physics` | Physics integration step |
| `Render` | Rendering pass |

Inside a system, `ctx.Slice<T>()` returns a contiguous span of component `T` for the current archetype chunk. All slices for the same context are index-aligned.

### Enabling / Disabling Systems

```cpp
ecs.DisableSystem("moveSystem");   // stop running it
ecs.EnableSystem("moveSystem");    // resume
ecs.ToggleSystem("moveSystem");    // flip state
```

### Running Systems Manually

```cpp
// Usually called by the engine, but available if you need manual control
ecs.Run(ECS::SystemGroup::Update, dt);
ecs.Run(ECS::SystemGroup::Render, dt);
```

### SystemBuilder — Read/Write Hints

`RegisterSystem` returns a `SystemBuilder` that lets you declare component read/write access for future parallelism:

```cpp
ecs.RegisterSystem<TransformComponent>("readOnlyMove", fn, ECS::SystemGroup::Update)
   .Read<TransformComponent>()
   .Write<RigidBody>();
```

---

## 3. InputSystem — Actions, Bindings & Devices

The input system maps physical keys/buttons to named **Actions** grouped in **ActionMaps**. All polling is done against action names, not raw key codes.

### Setup — Action Maps & Actions

```cpp
// Create (or get) an ActionMap, then chain actions onto it
_data->inputs.AddActionMap("gameplay")
    .AddAction("jump")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, 0, SDL_SCANCODE_SPACE)
    .AddAction("move")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, 0, SDL_SCANCODE_D, +1.0f)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, 1, SDL_SCANCODE_A, -1.0f)
        .AddProcessor(std::make_unique<ProcessWASD>("wasd"));
```

**`AddBinding` parameters:**
- `BindingType` — `Button` or `Axis`
- `DeviceType` — `Keyboard`, `Mouse`, or `Gamepad`
- `componentIndex` — which slot of the `INPUT_DATA_4` (0–3) this binding writes to
- `key` — SDL scancode / button index
- `scale` *(optional, default 1.0f)* — multiplier applied to the raw value

### Assigning Maps & Devices to Players

```cpp
_data->inputs.AssignMapToPlayer("gameplay");               // player -1 (default)
_data->inputs.AssignDeviceToPlayer(InputSystem::KeyboardHub::Current());
_data->inputs.AssignDeviceToPlayer(InputSystem::MouseHub::Current());
```

Player `-1` is the default single-player slot. Pass an explicit `int` player ID for local multiplayer.

### Polling Action State

```cpp
// Returns Idle / Pressed / Held / Released
InputSystem::ActionState state = _data->inputs.GetActionState("jump");

// Convenience booleans (each checks the default player)
bool pressed  = _data->inputs.IsPressed("jump");   // true only on the frame it was pressed
bool held     = _data->inputs.IsHeld("jump");
bool released = _data->inputs.IsReleased("jump");
bool idle     = _data->inputs.IsIdle("jump");
```

### Reading Axis Data

```cpp
// Returns std::array<float, 4>
auto move = _data->inputs.GetActionAxis("move");
float horizontal = move[0];  // maps to componentIndex 0
float vertical   = move[1];  // maps to componentIndex 1
```

### Raw Device Queries

```cpp
Vec2 mousePos  = _data->inputs.GetMousePosition();
bool leftClick = _data->inputs.GetMouseButton(0);
bool wKey      = _data->inputs.GetKey(SDL_SCANCODE_W);
```

### Modifying Actions at Runtime

```cpp
InputSystem::ActionMap* map = _data->inputs.GetActionMap("gameplay");
map->AddAction("sprint").AddBinding(InputSystem::Button, InputSystem::Keyboard, 0, SDL_SCANCODE_LSHIFT);

// Deactivate an action (it returns Idle and zero data while inactive)
map->GetAction("sprint")->Deactivate();
map->GetAction("sprint")->Activate();

map->RemoveAction("sprint");
_data->inputs.RemoveActionMap("gameplay");
```

---

## 4. UI Context — Retained-Mode Interface

`UI::Context` (stored in `SharedData` as part of the renderer pipeline) builds a **retained node tree** once and then updates and renders it every frame. Nodes are identified by stable `NodeHandle` values.

### Lifecycle

```cpp
// Called once (usually in State::Init via the engine)
ui.Init(renderer, canvasW, canvasH, window, assets);

// Call when the window is resized
ui.Resize(newW, newH);

// Call every frame — processes input, layout, and rendering
UI::InputState inp = /* fill from SDL events */;
ui.Update(inp, dt);
```

### Building the Tree

All `Add*` functions return a `NodeHandle`. Pass a parent handle (or `UI::NULL_HANDLE` for root-level) to nest nodes.

```cpp
NodeHandle panel  = ui.AddContainer(UI::NULL_HANDLE, "mainPanel");
NodeHandle title  = ui.AddLabel("Score: 0", panel, "scoreLabel");
NodeHandle btn    = ui.AddButton("Start", panel, "startBtn");
NodeHandle slider = ui.AddSlider(0.5f, 0.f, 1.f, panel, "volumeSlider");
NodeHandle field  = ui.AddInputField("Type here...", panel, "nameField");
NodeHandle img    = ui.AddImage(StringId("logo"), {0,0,0,0}, panel, "logo");
```

The string `id` argument is optional but enables `Find()` lookups later.

### Looking Up Nodes

```cpp
NodeHandle h = ui.Find("scoreLabel");  // returns NULL_HANDLE if not found
bool exists  = ui.Exists(h);
```

### Polling Interaction

```cpp
// Raw state: None / Hovered / Pressed / Released
UI::InteractionState state = ui.Poll(btn);

// Convenience helpers
if (ui.IsClicked(btn))         { /* button was released this frame */ }
if (ui.SliderChanged(slider))  { float v = ui.GetSliderValue(slider); }
if (ui.InputChanged(field))    { std::string txt = ui.GetInputValue(field); }
```

### Mutating Nodes

```cpp
ui.SetText(title, "Score: 42");
ui.SetVisible(panel, false);
ui.SetEnabled(btn, false);          // greyed out, not interactive
ui.SetSliderValue(slider, 0.8f);
ui.SetInputValue(field, "Player1");
ui.SetImage(img, StringId("newTex"), {0, 0, 64, 64});
```

### Layout — Flex Properties

The layout engine is a CSS-style flexbox. Every node's flex properties can be set after creation:

```cpp
// Size
ui.SetSize(panel, SizeValue::Px(400.f), SizeValue::Px(300.f));
ui.SetSize(panel, SizeValue::Percent(50.f), SizeValue::Auto());

// Direction and alignment
ui.SetFlexDirection(panel, FlexDirection::Column);
ui.SetJustify(panel, JustifyContent::Center);
ui.SetAlignItems(panel, AlignItems::Center);
ui.SetFlexWrap(panel, FlexWrap::Wrap);

// Spacing
ui.SetGap(panel, 8.f);                 // main-axis gap
ui.SetGap(panel, 8.f, 4.f);           // main gap, cross gap
ui.SetPadding(panel, Edges::All(16.f));
ui.SetMargin(btn,   Edges::Axes(4.f, 8.f));   // vertical, horizontal
ui.SetPadding(panel, Edges::TRBL(10.f, 8.f, 10.f, 8.f));

// Flex grow / shrink / basis
ui.SetFlexGrow(btn, 1.f);
ui.SetFlexShrink(btn, 0.f);
ui.SetFlexBasis(btn, SizeValue::Px(120.f));
```

**`SizeValue` factory methods:**

```cpp
SizeValue::Px(200.f)       // fixed pixels
SizeValue::Percent(50.f)   // percentage of parent
SizeValue::Auto()           // auto (shrink-to-content)
```

**`Edges` factory methods:**

```cpp
Edges::All(8.f)                          // uniform
Edges::Axes(vertical, horizontal)
Edges::TRBL(top, right, bottom, left)    // explicit four sides
```

### Per-Node Style Overrides

```cpp
UI::StyleOverride style;
style.background      = Color::RGBA(30, 30, 30, 200);
style.backgroundHover = Color::RGBA(50, 50, 50, 255);
style.foreground      = Color::Hex(0xFFFFFFFF);
style.borderRadius    = 8.f;
style.borderWidth     = 1.f;
style.border          = Color::RGBA(100, 100, 100, 255);
style.textAlign       = TextAlign::Center;
ui.SetStyleOverride(btn, style);
```

### Removing Nodes

```cpp
ui.Remove(panel);           // removes panel and all its descendants
ui.ClearChildren(panel);    // removes all children, keeps panel itself
```

---

## 5. AnimationSystem — Sprite Frame Animations

`AnimationSystem` manages named clips and drives per-entity `AnimationPlayer` components through frame sequences.

### Defining Clips

```cpp
AnimationClip clip;
clip.spritesheet   = StringId("playerSheet");
clip.frameWidth    = 64;
clip.frameHeight   = 64;
clip.startX        = 0;      // pixel X of first frame
clip.stopX         = 448;    // pixel X of last frame (inclusive)
clip.row           = 0;      // which row of the spritesheet
clip.frameDuration = 0.08f;  // seconds per frame

_data->animation.AddClip("player_run_right", clip);

// Shorthand initializer-list form
_data->animation.AddClip("player_idle", { "playerSheet", 64, 64, 192, 192, 1, 0.1f });
```

### Controlling Playback

Each entity that animates carries an `AnimationPlayer` component:

```cpp
ecs.Add<AnimationPlayer>(e, AnimationPlayer{});

// Start playing a clip (loop = true by default)
_data->animation.Play(ecs.Get<AnimationPlayer>(e), "player_run_right");
_data->animation.Play(ecs.Get<AnimationPlayer>(e), "player_idle", false);  // play once

// Stop
_data->animation.Stop(ecs.Get<AnimationPlayer>(e));
```

### Advancing Frames

Call every frame, typically inside an Update system:

```cpp
_data->animation.Update(anim, dt);

// The player exposes the current source rect and spritesheet after each Update
sprite.TextureSRect          = anim.currentRect;
sprite.material.textures[0]  = anim.currentSpritesheet;
```

### Querying State

```cpp
AnimationPlayer& anim = ecs.Get<AnimationPlayer>(e);
bool isPlaying    = anim.playing;
StringId clipName = anim.currentClip;   // compare to know what's active
```

### Removing Clips

```cpp
_data->animation.RemoveClip("player_run_right");  // returns -1 if not found

// Safe lookup before access
const AnimationClip* clip = _data->animation.TryGetClip("player_idle");
if (clip) { /* ... */ }

// Asserting lookup (use when the clip must exist)
const AnimationClip& clip = _data->animation.GetClip("player_idle");
```

---

## 6. PhysicsSystem — Collision & Movement

`PhysicsSystem` registers its own internal ECS systems and exposes a simple enable/disable API. It requires `BoxCollider` and `TransformComponent` (for collision) and `RigidBody` and `TransformComponent` (for movement).

### Setup

```cpp
// Call once in your State::Init — ties the physics system to this world
_data->physics.Tie(ecs);

// Enable/disable subsystems independently
_data->physics.EnableCollisionDetection(true);   // broad + narrow phase
_data->physics.EnableMovement(true);             // velocity integration
_data->physics.EnableSpatialIndexBuild(true);    // only broad-phase rebuild
```

All three are disabled by default after `Tie()`.

### Components

**`BoxCollider`**

```cpp
// BoxCollider(half-width, half-height, isTrigger)
ecs.Add<BoxCollider>(e, BoxCollider(32.f, 32.f, false));

struct BoxCollider {
    float hw, hh;          // half extents in local space
    float offsetX, offsetY;// local offset from transform origin
    bool  isTrigger;       // triggers record contacts but don't block movement
};
```

**`RigidBody`**

```cpp
RigidBody rb;
rb.vx       = 200.f;   // velocity
rb.vy       = 0.f;
rb.drag     = 0.5f;    // linear drag applied per frame
rb.isStatic = false;   // statics are tested against but never moved
ecs.Add<RigidBody>(e, rb);
```

### Reading Collision Results

After `SystemGroup::Update` runs, contact data is available:

```cpp
// Returns a vector of all entities currently overlapping with e (max 8)
const std::vector<ECS::Entity>& contacts = _data->physics.GetContacts(e);
for (ECS::Entity other : contacts) {
    BoxCollider* bc = ecs.TryGet<BoxCollider>(other);
    if (bc && bc->isTrigger) { /* trigger logic */ }
}

// All unique collision pairs this frame
const auto& pairs = _data->physics.GetCollisionPairs();
for (auto [a, b] : pairs) { /* ... */ }
```

---

## 7. SpatialIndex — Broad-Phase Queries

`SpatialIndex` is a grid-based spatial hash. `PhysicsSystem` uses it internally, but you can query it directly for gameplay logic (line-of-sight, aggro ranges, etc.).

```cpp
// Insert an entity's AABB
_data->spatialIndex.InsertRectangle(e, x, y, width, height);

// Query all entities whose AABB overlaps a rectangle
std::vector<ECS::Entity> found;
_data->spatialIndex.QueryRectangle(x, y, width, height, found);

// Query all entities within a circle (approximated as bounding square)
_data->spatialIndex.QueryCircle(cx, cy, radius, found);

// Query all entities at a point
_data->spatialIndex.QueryPoint(px, py, found);

// Pre-allocate to avoid per-frame allocations
_data->spatialIndex.Reserve(entityCount);
```

Each query appends to `found` and returns the number of new entries added.

---

## 8. AssetManager — Textures, Fonts, Shaders & Meshes

`AssetManager` stores all GPU resources identified by `StringId`.

### Surfaces (CPU-side bitmaps)

```cpp
_data->assets.LoadBMPSurface(StringId("playerSurface"), "assets/player.bmp");
SDL_Surface* surf = _data->assets.TryGetSurface(StringId("playerSurface"));
SDL_Surface& surf = _data->assets.GetSurface(StringId("playerSurface")); // asserts
```

### Fonts

```cpp
_data->assets.LoadFont(StringId("uiFont"), "assets/fonts/Roboto.ttf");
TTF_Font* font = _data->assets.TryGetFont(StringId("uiFont"));
TTF_Font& font = _data->assets.GetFont(StringId("uiFont")); // asserts
```

### Shaders

```cpp
_data->assets.LoadShader(
    StringId("spriteVert"),
    "shaders/sprite.vert.spv",
    _data->device,
    ShaderStage::VERTEX,
    /*numSamplers*/ 1
);
const Shader& vs = _data->assets.GetShader(StringId("spriteVert"));
const Shader* vs = _data->assets.TryGetShader(StringId("spriteVert")); // nullable
```

### Meshes

```cpp
MeshVertices verts  = { /* Vertex array */ };
MeshIndices  indices = { 0, 1, 2 };
_data->assets.AddMesh(StringId("quad"), verts, indices);
MeshBase* mesh = _data->assets.GetMesh(StringId("quad")); // nullptr if missing
```

### Materials & Textures (GPU)

```cpp
MaterialBase mat;
MaterialBase::MakeTransparent(mat);   // alpha blended
MaterialBase::MakeOpaque(mat);        // depth write
MaterialBase::MakeAdditive(mat);      // additive blend
MaterialBase::MakeOverlay(mat);       // no depth test
_data->assets.AddMaterial(StringId("spriteMat"), mat);
MaterialBase* mat = _data->assets.GetMaterial(StringId("spriteMat"));

_data->assets.AddTexture(StringId("playerTex"), texture);
TextureBase* tex = _data->assets.TryGetTexture(StringId("playerTex"));
TextureBase& tex = _data->assets.GetTexture(StringId("playerTex")); // asserts
```

---

## 9. Blackboard — Typed Key-Value Store

`Blackboard` (`_data->session`) is a runtime dictionary for sharing arbitrary typed data across systems and states without coupling them directly.

```cpp
// Write (overwrites if key exists)
_data->session.Set(StringId("score"), 0);
_data->session.Set(StringId("playerName"), std::string("Hero"));

// Read — asserts if key or type is wrong
int&  score = _data->session.Get<int>(StringId("score"));
score += 10;

// Safe read — returns nullptr if missing or wrong type
int* scorePtr = _data->session.TryGet<int>(StringId("score"));
if (scorePtr) { /* ... */ }

// Check existence
bool exists = _data->session.Has(StringId("score"));

// Remove a single key
_data->session.Remove(StringId("score"));

// Wipe all entries
_data->session.Clear();
```

---

## 10. Logger — Structured Logging

The logger routes messages through **sinks**. Two built-in sinks are provided: `ConsoleSink` and `FileSink`.

### Getting the Global Logger

```cpp
Logger& log = GlobalLogger();
```

### Adding Sinks

```cpp
log.AddSink(std::make_shared<ConsoleSink>());
log.AddSink(std::make_shared<FileSink>("game.log"));

// Remove a previously added sink
auto sink = std::make_shared<ConsoleSink>();
log.AddSink(sink);
log.RemoveSink(sink);
```

### Logging with Macros

Use the macros — they capture file, line, and function automatically:

```cpp
LOG_DEBUG(GlobalLogger(), "MySystem", "Debug message");   // stripped in Release
LOG_INFO(GlobalLogger(), "MySystem",  "Info message");    // stripped in Release
LOG_WARN(GlobalLogger(), "MySystem",  "Warning message"); // always compiled
LOG_ERROR(GlobalLogger(), "MySystem", "Error message");   // always compiled
```

### Log Levels

```cpp
// Suppress anything below Warning
GlobalLogger().minLevel = LogLevel::Warning;
```

Available levels: `Debug < Info < Warning < Error`.

---

## 11. StateMachine — Game State Stack

`StateMachine` manages a stack of `State` objects. The top state is the active one. States can be pushed, replaced, or popped.

```cpp
// Push a new state on top (the current state is paused, not destroyed)
_data->state.AddState(std::make_unique<Level1>(_data), /*isReplacing*/ false);

// Replace the current state (destroys current, pushes new)
_data->state.AddState(std::make_unique<MainMenu>(_data), /*isReplacing*/ true);

// Pop the top state (resumes the one below, if any)
_data->state.RemoveState();

// The engine calls this each frame to apply pending transitions
_data->state.ProcessStateChanges();

// Get the currently active state
StateRef& active = _data->state.GetActiveState();
```

### Writing a State

```cpp
class MyState : public State {
public:
    using State::State;       // inherit constructor that receives SharedDataRef

    void Init()          override;  // called once when the state is pushed
    void Update(float dt) override; // called every frame while active
    void Render(float dt) override; // called every frame for rendering
    void Pause()         override;  // called when a state is pushed on top
    void Resume()        override;  // called when the state on top is popped
};
```

Inside any `State` method, `_data` (the `SharedDataRef`) and `ecs` (the state's private `ECS::World`) are available directly.

---

## 12. Putting It All Together — A Minimal Game State

Below is a self-contained example that demonstrates every major system working together.

```cpp
// MyState.h
#pragma once
#include "State.h"
#include "SharedDataRef.h"

class MyState : public State {
public:
    using State::State;
    void Init()           override;
    void Update(float dt) override;
    void Render(float dt) override;
private:
    ECS::Entity player = 0;
};
```

```cpp
// MyState.cpp
#include "MyState.h"
#include "Transform.h"
#include "RigidBody.h"
#include "BoxCollider.h"
#include "SimpleSprite.h"

void MyState::Init()
{
    // ---- Assets -------------------------------------------------------
    _data->assets.LoadBMPSurface(StringId("playerSurface"), "assets/player.bmp");

    // ---- Animation ----------------------------------------------------
    _data->animation.AddClip("idle", { "playerSheet", 64, 64, 0, 192, 0, 0.1f });
    _data->animation.AddClip("run",  { "playerSheet", 64, 64, 0, 448, 1, 0.08f });

    // ---- Input --------------------------------------------------------
    _data->inputs.AddActionMap("game")
        .AddAction("move")
            .AddBinding(InputSystem::Button, InputSystem::Keyboard, 0, SDL_SCANCODE_D, +1.f)
            .AddBinding(InputSystem::Button, InputSystem::Keyboard, 0, SDL_SCANCODE_A, -1.f);
    _data->inputs.AssignMapToPlayer("game");
    _data->inputs.AssignDeviceToPlayer(InputSystem::KeyboardHub::Current());

    // ---- Entity -------------------------------------------------------
    player = ecs.Create();
    ecs.Add<TransformComponent>(player, TransformComponent({ 960.f, 540.f, 0.f }, { 1.f, 1.f }));
    ecs.Add<RigidBody>(player, RigidBody{});
    ecs.Add<BoxCollider>(player, BoxCollider(24.f, 32.f, false));
    ecs.Add<AnimationPlayer>(player, AnimationPlayer{});

    _data->animation.Play(ecs.Get<AnimationPlayer>(player), "idle");

    // ---- Physics ------------------------------------------------------
    _data->physics.Tie(ecs);
    _data->physics.EnableCollisionDetection(true);
    _data->physics.EnableMovement(true);

    // ---- Systems ------------------------------------------------------
    ecs.RegisterSystem<TransformComponent, RigidBody, AnimationPlayer>(
        "playerMove",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data) {
            auto transforms = ctx.Slice<TransformComponent>();
            auto bodies     = ctx.Slice<RigidBody>();
            auto anims      = ctx.Slice<AnimationPlayer>();
            auto move       = data->inputs.GetActionAxis("move");

            for (std::size_t i = 0; i < transforms.size(); i++) {
                bodies[i].vx = move[0] * 300.f;
                data->animation.Update(anims[i], dt);

                StringId nextClip = (move[0] != 0.f) ? StringId("run") : StringId("idle");
                if (anims[i].currentClip != nextClip)
                    data->animation.Play(anims[i], nextClip);
            }
        },
        ECS::SystemGroup::Update
    );

    // ---- UI -----------------------------------------------------------
    auto panel = _data->renderer.ui.AddContainer(UI::NULL_HANDLE, "hud");
    _data->renderer.ui.SetFlexDirection(panel, FlexDirection::Row);
    _data->renderer.ui.SetPadding(panel, Edges::All(12.f));
    _data->renderer.ui.AddLabel("Score: 0", panel, "scoreLabel");
}

void MyState::Update(float dt)
{
    ecs.Run(ECS::SystemGroup::Update, dt);
    ecs.Run(ECS::SystemGroup::Physics, dt);
}

void MyState::Render(float dt)
{
    ecs.Run(ECS::SystemGroup::Render, dt);
}
```

---

## Quick Reference

| Task | Method |
|---|---|
| Create entity | `ecs.Create()` |
| Add component | `ecs.Add<T>(e, value)` |
| Read component | `ecs.Get<T>(e)` / `ecs.TryGet<T>(e)` |
| Iterate entities | `ecs.Query<A, B>([](Entity, A&, B&){})` |
| Register system | `ecs.RegisterSystem<A, B>("name", fn, Group)` |
| Add UI node | `ui.AddButton("label", parent, "id")` |
| Poll button | `ui.IsClicked(handle)` |
| Play animation | `animation.Play(player, "clipName")` |
| Advance animation | `animation.Update(player, dt)` |
| Check collision | `physics.GetContacts(entity)` |
| Read input | `inputs.IsPressed("action")` |
| Read axis | `inputs.GetActionAxis("move")[0]` |
| Store global value | `session.Set(StringId("key"), value)` |
| Read global value | `session.Get<T>(StringId("key"))` |
| Load asset | `assets.LoadBMPSurface(id, "path")` |
| Log a warning | `LOG_WARN(GlobalLogger(), "System", "msg")` |
