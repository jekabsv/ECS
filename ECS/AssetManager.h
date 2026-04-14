#pragma once
#include <unordered_map>
#include <string>
#include "Struct.h"
#include <SDL3_ttf/SDL_ttf.h>

struct SDL_Renderer;
struct SDL_Texture;
enum ShaderStage;
struct ShaderAsset;


class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	int AddMesh(StringId meshName, const Mesh& mesh);
	const Mesh* GetMesh(StringId meshName) const;
	int MoveMeshOrigin(StringId meshName, float dx, float dy);


	int LoadBMPTexture(StringId TextureName, const std::string& filename, SDL_Renderer* renderer);
	const SDL_Texture* GetTexture(StringId TextureName) const;


	int LoadFont(StringId FontName, const std::string& filename);
	TTF_Font* GetFont(StringId TextureName) const;


	int LoadShader(StringId shaderName, const std::string& filename,
               SDL_GPUDevice* device, ShaderStage stage,
               uint32_t numSamplers        = 0,
               uint32_t numStorageTextures = 0,
               uint32_t numStorageBuffers  = 0,
               uint32_t numUniformBuffers  = 0);
	const ShaderAsset* GetShader(StringId shaderName) const;


	
private:

	Mesh* GetEditableMesh(StringId meshName);
    std::unordered_map<StringId, Mesh> _meshes;
	std::unordered_map<StringId, SDL_Texture*> _textures;
	std::unordered_map<StringId, TTF_Font*> _fonts;
	std::unordered_map<StringId, ShaderAsset> _shaders;


	//textures, fonts, sounds, etc.
};