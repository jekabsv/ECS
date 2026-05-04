#pragma once
#include "SharedDataRef.h"
#include "State.h"

// ============================================================================
// Components
// ============================================================================
struct NBPos { float x, y; };
struct NBVel { float vx, vy; };
struct NBAccel { float ax, ay; };
struct NBMass { float mass; };
struct NBColor { SDL_FColor color; };

// ============================================================================
// Barnes-Hut solver — owned by NBody, pointer published to blackboard
// ============================================================================
class BarnesHut
{
public:
    struct Params
    {
        float theta = 0.6f;
        float G = 500.f;
        float softening = 8.f;
    };

    Params params;

    // Clear all state, ready to accept InsertSlice calls for a new frame.
    void BeginFrame();

    // Append one contiguous block of bodies (called once per archetype chunk).
    void InsertSlice(const NBPos* positions, const NBMass* masses, int count);

    // After ALL InsertSlice calls: build the quadtree.
    void BuildTree(float rootCx, float rootCy, float rootHalf);

    // After BuildTree: solve all forces into _ax/_ay.
    void ComputeForces();

    // Returns true if BuildTree+ComputeForces have been called this frame.
    bool Solved() const { return _solved; }

    int   Count() const { return _count; }
    float GetAx(int i) const { return _ax[i]; }
    float GetAy(int i) const { return _ay[i]; }

private:
    struct QuadNode
    {
        float cx, cy, half;
        float totalMass = 0.f, comX = 0.f, comY = 0.f;
        int   bodyIndex = -1;
        int   children[4] = { -1,-1,-1,-1 };

        bool IsLeaf()  const { return children[0] == -1; }
        bool IsEmpty() const { return bodyIndex == -1 && IsLeaf(); }
    };

    std::vector<float>    _x, _y, _mass;
    std::vector<float>    _ax, _ay;
    std::vector<QuadNode> _tree;
    int  _count = 0;
    bool _solved = false;

    int  AllocNode(float cx, float cy, float half);
    void InsertBody(int node, int bodyIdx, int depth = 0);
    int  Quadrant(int node, float bx, float by) const;
    void AccumulateForce(int node, int bodyIdx);

    static inline float InvSqrt(float x) { return 1.f / std::sqrt(x); }
};

// ============================================================================
// NBody state
// ============================================================================
class NBody : public State
{
public:
    struct PlayArea { float x, y, w, h; };

    using State::State;
    void Init()           override;
    void Update(float dt) override;
    void Render(float dt) override;

private:
    void ResetSimulation();
    void SpawnGalaxy(float cx, float cy, float radius, int n,
        float totalMass, float baseSpin, SDL_FColor col);

    BarnesHut _bh;

    PlayArea _area{};
    float    _rootHalf = 0.f;
    float    _dtScale = 1.0f;
    bool     _paused = false;

    static constexpr int PANEL_WIDTH = 240;
    static constexpr int DEFAULT_COUNT = 3000;
    static constexpr int MIN_COUNT = 100;
    static constexpr int MAX_COUNT = 5000;
    int _targetCount = DEFAULT_COUNT;

    UI::NodeHandle back = UI::NULL_HANDLE;
    UI::NodeHandle countSlider = UI::NULL_HANDLE;
    UI::NodeHandle countInput = UI::NULL_HANDLE;
    UI::NodeHandle thetaSlider = UI::NULL_HANDLE;
    UI::NodeHandle thetaLabel = UI::NULL_HANDLE;
    UI::NodeHandle gSlider = UI::NULL_HANDLE;
    UI::NodeHandle gLabel = UI::NULL_HANDLE;
    UI::NodeHandle softSlider = UI::NULL_HANDLE;
    UI::NodeHandle softLabel = UI::NULL_HANDLE;
    UI::NodeHandle dtSlider = UI::NULL_HANDLE;
    UI::NodeHandle dtLabel = UI::NULL_HANDLE;
    UI::NodeHandle resetButton = UI::NULL_HANDLE;
    UI::NodeHandle pauseButton = UI::NULL_HANDLE;
    UI::NodeHandle bodyCountLabel = UI::NULL_HANDLE;
    bool _focusedCount = false;
};