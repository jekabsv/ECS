#pragma once
#include <unordered_map>
#include <string>
#include "Struct.h"
#include <SDL3_ttf/SDL_ttf.h>

struct SDL_Renderer;
struct SDL_Texture;


class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;


	int LoadBMPSurface(StringId TextureName, const std::string& filename);
	SDL_Surface* GetSurface(StringId);

	int LoadFont(StringId FontName, const std::string& filename);
	TTF_Font* GetFont(StringId TextureName) const;


	int LoadShader(StringId shaderName, const std::string& filename,
               SDL_GPUDevice* device, ShaderStage stage,
               uint32_t numSamplers        = 0,
               uint32_t numStorageTextures = 0,
               uint32_t numStorageBuffers  = 0,
               uint32_t numUniformBuffers  = 0);
	const ShaderAsset* GetShader(StringId shaderName) const;



	int LoadMesh(StringId meshName, const MeshVertices& vertices, const MeshIndices& indices);
	MeshEntry* GetMesh(StringId);


	int LoadTexture(StringId TextureName, TextureEntry texture);
	TextureEntry* GetTextureEntry(StringId);




	
private:

	std::unordered_map<StringId, SDL_Surface*> _surfaces;
	std::unordered_map<StringId, TextureEntry> _textures;

	std::unordered_map<StringId, TTF_Font*> _fonts;

	std::unordered_map<StringId, ShaderAsset> _shaders;

	std::unordered_map<StringId, MeshEntry> _meshes;



	//textures, fonts, sounds, etc.
};