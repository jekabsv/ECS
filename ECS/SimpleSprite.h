#pragma once
#include <SDL3/SDL_rect.h>
#include "Struct.h"
#include "GPUAssets.h"

struct SimpleSprite
{
	SimpleSprite(SDL_FRect sRect, MaterialInstance _material, bool _render = false)
		: TextureSRect(sRect), material(_material), render(_render) { };

	SimpleSprite(SDL_FRect sRect, StringId _material, bool _render = false)
		: TextureSRect(sRect), render(_render) {
		material.materialName = _material;

		material.fragBufferSize = 0;
		material.vertBufferSize = 0;
		material.textureCount = 0;
	};

	SimpleSprite(SDL_FRect sRect, StringId _material, bool _render, StringId InitialTextureId)
		: TextureSRect(sRect), render(_render) {
		material.materialName = _material;

		material.fragBufferSize = 0;
		material.vertBufferSize = 0;
		material.textureCount = 1;
		
		material.AddTexture(InitialTextureId);
	};

	SimpleSprite() = default;

	SDL_FRect TextureSRect = { 0, 0, 0, 0 };
	MaterialInstance material;
	bool render = false;
};