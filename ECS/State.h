#pragma once
#include "ECS.h"

class State
{
public:
	virtual ~State() = default;
	virtual void Init() {};
	virtual void HandleInput() {};
	virtual void Update() {};
	virtual void Render() {};


	virtual void Pause() {};
	virtual void Resume() {};

	ecs::ECS ecs;
};
