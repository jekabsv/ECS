#pragma once
#include "ECS.h"

class State
{
public:
	State(SharedDataRef data) : _data(data) {};
	virtual ~State() = default;
	virtual void Init() {};
	virtual void HandleInput() {};
	virtual void Update(float dt) {};
	virtual void Render(float dt) {};


	virtual void Pause() {};
	virtual void Resume() {};

	ECS::World ecs;
	SharedDataRef _data;
};
