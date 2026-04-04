#pragma once
#include <SDL3/SDL.h>
#include "Struct.h"

struct TransformComponent
{
	TransformComponent(Vec2 Position = {0.0f, 0.0f}, Vec2 Scale = {1.0f, 1.0f}, Vec2 Rotation = {0.0f, 0.0f}) :
		position(Position), scale(Scale), rotation(Rotation) {};
	Vec2 position = { 0.0f, 0.0f };
	Vec2 scale = { 1.0f, 1.0f };
	Vec2 rotation = { 0.0f, 0.0f };
};