#pragma once
#include "SharedDataRef.h"
#include "State.h"

class SPH : public State
{
public:
    using State::State;
    void Init()           override;
    void Update(float dt) override;
    void Render(float dt) override;

private:
    struct PlayArea { float x, y, w, h; };
    struct RigidObject
    {
        float px = 0.f, py = 0.f;
        float vx = 0.f, vy = 0.f;
        float angle = 0.f;
        float omega = 0.f;
        float hw = 50.f;
        float hh = 22.f;
        bool  held = false;
        bool  active = false;
        float inertia = 1.f;
        int   totalGhosts = 0;
        std::vector<ECS::Entity> ghosts;
    };

    void SpawnParticle(const PlayArea& area);
    void DespawnLastParticle();
    void SpawnRigidObject(float cx, float cy);
    void DestroyRigidObject();
    void SyncGhostPositions();

    float H = 32.0f;

    static constexpr float GAS_CONSTANT = 4000.0f;
    static constexpr float VISCOSITY = 40.0f;
    static constexpr float GRAVITY = 300.0f;
    static constexpr float MAX_SPEED = 600.0f;
    static constexpr float DAMPING = 0.4f;
    static constexpr int DEFAULT_COUNT = 2000;
    static constexpr int MIN_COUNT = 50;
    static constexpr int MAX_COUNT = 5000;
    static constexpr int SPAWN_BUDGET = 5;
    static constexpr int PANEL_WIDTH = 220;
    static constexpr float GHOST_SPACING = 7.f;


    int   _objMass = 10;
    static constexpr float FLUID_DENSITY = 0.012f;
    static constexpr float OBJ_LINEAR_DAMP = 0.8f;
    static constexpr float OBJ_ANGULAR_DAMP = 5.0f;
    static constexpr float OBJ_MAX_OMEGA = 10.5f;

    std::vector<ECS::Entity> _particles;
    int _currentCount = 0;

    RigidObject _rigidObj;
    bool _prevClickHeld = false;

    // UI handles
    UI::NodeHandle back = UI::NULL_HANDLE;
    UI::NodeHandle slider = UI::NULL_HANDLE;
    UI::NodeHandle input = UI::NULL_HANDLE;
    UI::NodeHandle radiusInput = UI::NULL_HANDLE;
    UI::NodeHandle modeButton = UI::NULL_HANDLE;
    UI::NodeHandle colorModeButton = UI::NULL_HANDLE;
    UI::NodeHandle removeObjButton = UI::NULL_HANDLE;
    UI::NodeHandle massInput = UI::NULL_HANDLE;

    bool focused = false;
    bool focusedRadius = false;
    bool focusedMass = false;

    enum class InteractionMode { Attract, Repel, Object };
    InteractionMode _interactionMode = InteractionMode::Attract;
};