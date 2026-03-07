#pragma once
#include <SDL3/SDL.h>
#include "Struct.h"

struct SimpleSprite
{
	SimpleSprite(SDL_FRect Sprite, SDL_FRect TextureMask, StringId _TextureName) : rect(Sprite), TextureRect(TextureMask), TextureName(_TextureName) {};
	SimpleSprite() = default;
	SDL_FRect rect = { 0, 0, 0, 0 };
	SDL_FRect TextureRect = { 0, 0, 0, 0 };
	StringId TextureName = "";
};