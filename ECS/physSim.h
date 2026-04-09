//#pragma once
//#include "State.h"
//#include "SharedDataRef.h"
//#include <vector>
//#include <cmath>
//
//// ── Shape types ──────────────────────────────────────────────────────────────
//enum class SimShapeType { Circle, Rect };
//
//// ── Per-body component (stored in ECS) ───────────────────────────────────────
//struct SimBody
//{
//    SimShapeType shape = SimShapeType::Rect;
//    float        hw = 30.0f;   // half-width  (rect) or radius (circle)
//    float        hh = 30.0f;   // half-height (rect); unused for circle
//    float        mass = 1.0f;
//    float        restitution = 0.5f;
//
//    // physics state
//    float vx = 0.0f, vy = 0.0f;   // velocity
//    float ax = 0.0f, ay = 0.0f;   // accumulated acceleration this frame
//
//    // snapshot for reset
//    float initX = 0.0f, initY = 0.0f;
//    float initVx = 0.0f, initVy = 0.0f;
//
//    bool isStatic = false;         // immovable (infinite mass)
//    bool selected = false;
//};
//
//// ── Force arrow (not an ECS entity – stored separately) ──────────────────────
//struct SimForce
//{
//    ECS::Entity target = 0;        // entity the force is applied to
//    float       fx = 0.0f, fy = 0.0f;   // force vector (pixels/s^2 * mass)
//    float       anchorX = 0.0f, anchorY = 0.0f; // world-space attachment point
//    bool        selected = false;
//};
//
//// ── Interaction mode ──────────────────────────────────────────────────────────
//enum class PhysSimMode
//{
//    Idle,
//    PlacingCircle,
//    PlacingRect,
//    PlacingForce,   // dragging: anchorSet=true after first click on an object
//};
//
//class PhysSim : public State
//{
//public:
//    using State::State;
//
//    void Init()   override;
//    void Update(float dt) override;
//    void Render(float dt) override;
//
//private:
//    // ── Simulation state ──────────────────────────────────────────────────
//    bool              _running = false;
//    PhysSimMode       _mode = PhysSimMode::Idle;
//    bool              _showForces = true;
//
//    // bodies (parallel to ECS entities)
//    std::vector<ECS::Entity> _bodies;
//    std::vector<SimForce>    _forces;
//
//    // selection
//    ECS::Entity _selectedBody = 0;
//    int         _selectedForce = -1;   // index into _forces, -1 = none
//
//    // force-drag state
//    bool  _forceAnchored = false;
//    float _forceAnchorX = 0.0f, _forceAnchorY = 0.0f;
//    ECS::Entity _forceTarget = 0;
//    float _mouseX = 0.0f, _mouseY = 0.0f;
//
//    // previous mouse-button state (for edge detection without actions)
//    bool _prevLMB = false;
//    bool _prevDel = false;
//    bool _prevSpace = false;
//    bool _prevS = false;
//    bool _prevR = false;
//    bool _prevF = false;
//    bool _prevShift = false;
//
//    // ── Helpers ───────────────────────────────────────────────────────────
//    void StepPhysics(float dt);
//    void ResolveCollisions();
//    void SnapshotInitial();
//    void ResetToInitial();
//
//    ECS::Entity SpawnBody(SimShapeType shape, float x, float y);
//    void DeleteSelectedBody();
//    void DeleteSelectedForce();
//
//    // hit-testing
//    ECS::Entity HitTestBody(float x, float y) const;
//    int         HitTestForce(float x, float y) const;  // returns index or -1
//
//    // rendering helpers
//    void DrawBody(const SimBody& b, float x, float y) const;
//    void DrawForceArrow(float x0, float y0, float x1, float y1,
//        bool selected, bool preview) const;
//    void DrawCircle(float cx, float cy, float r) const;
//
//    static constexpr float GRAVITY = 400.0f;  // px / s^2
//    static constexpr float FORCE_SCALE = 0.003f;  // arrow-pixel to force unit
//    static constexpr float HIT_MARGIN = 8.0f;
//};