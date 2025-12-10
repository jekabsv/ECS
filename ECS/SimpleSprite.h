#pragma once
#include <SDL3/SDL.h>

struct SimpleSprite
{
	SimpleSprite(SDL_FRect Sprite, SDL_FRect TextureMask, int _TextureId) : rect(Sprite), TextureRect(TextureMask), TextureId(_TextureId) {};
	SimpleSprite() = default;
	SDL_FRect rect = { 0, 0, 0, 0 };
	SDL_FRect TextureRect = { 0, 0, 0, 0 };
	int TextureId = -1;
};