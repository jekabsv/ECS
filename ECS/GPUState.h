#pragma once
#include "State.h"


class GPUState : public State

{
public:
	using State::State;
	void Init() override;
	void Update(float dt) override;
	void Render(float dt) override;


private:
	SDL_Vertex verts[3];

};