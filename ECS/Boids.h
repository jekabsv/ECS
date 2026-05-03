#pragma once
#include "SharedDataRef.h"
#include "State.h"
#include "EngineMath.h"

class Boids : public State
{
public:
    using State::State;
    void Init()         override;
    void Update(float dt) override;
	void Render(float dt) override;

private:
    // Play area passed to spawn and into the blackboard
    struct PlayArea { float x, w, h; };

    void SpawnBoid(const PlayArea& area);
    void DespawnLastBoid();

    // Flocking constants
    float PERCEPTION = 40.0f;
    static constexpr float SEPARATION_DIST = 20.0f;
    static constexpr float MIN_SPEED = 80.0f;
    static constexpr float MAX_SPEED = 150.0f;
    static constexpr float MAX_TURN_RATE = 2.0f;
    static constexpr float W_SEPARATION = 3.8f;
    static constexpr float W_ALIGNMENT = 1.0f;
    static constexpr float W_COHESION = 0.9f;

    // Count management
    static constexpr int DEFAULT_COUNT = 1000;
    static constexpr int MIN_COUNT = 100;
    static constexpr int MAX_COUNT = 20000;
    static constexpr int SPAWN_BUDGET = 200;
    static constexpr int inputContainerWidth = 200;

    std::vector<ECS::Entity> _boidEntities;
    int _currentCount = 0;

    // UI handles
    UI::NodeHandle back = UI::NULL_HANDLE;
    UI::NodeHandle slider = UI::NULL_HANDLE;
    UI::NodeHandle input = UI::NULL_HANDLE;
    UI::NodeHandle perceptionInput = UI::NULL_HANDLE;
    UI::NodeHandle ScaleInput = UI::NULL_HANDLE;
    bool focused = false;
	bool focusedPerception = false;
};