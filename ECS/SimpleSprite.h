#pragma once
#include <SDL3/SDL_rect.h>
#include "Struct.h"

struct SimpleSprite
{
	SimpleSprite(SDL_FRect _rect, SDL_FRect TextureMask, StringId _TextureName, bool _render = false)
		: rect(_rect), TextureRect(TextureMask), TextureName(_TextureName), render(_render) { };
	SimpleSprite() = default;


	StringId MaterialName = "";

	SDL_FRect rect = { 0, 0, 0, 0 };
	SDL_FRect TextureRect = { 0, 0, 0, 0 };
	StringId TextureName = "";
	bool render = false;
};