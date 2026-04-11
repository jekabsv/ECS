#pragma once
#include "ECS.h"
#include "Uicontext.h"
#include <memory>
struct SharedData;
using SharedDataRef = std::shared_ptr<SharedData>;


class State
{
public:
	State(SharedDataRef data) : _data(data) {};
	virtual ~State() = default;
	virtual void Init() {};
	virtual void HandleInput() {};
	virtual void Update(float dt) {};
	virtual void Render(float dt) {};
	void SetData();


	virtual void Pause() {};
	virtual void Resume() {};

	ECS::World ecs;
	SharedDataRef _data;
	UI::Context ui;
};
