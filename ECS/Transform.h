#pragma once
#include "Struct.h"

struct TransformComponent
{
	TransformComponent(Vec3 Position = {0.0f, 0.0f, 0.0f}, Vec2 Scale = {1.0f, 1.0f}, float Rotation = 0.0f) :
		position(Position), scale(Scale), rotation(Rotation) {};
	Vec3 position = { 0.0f, 0.0f, 0.0f };
	Vec2 scale = { 1.0f, 1.0f };
	float rotation = 0.0f;
};