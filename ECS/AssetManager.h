#pragma once
#include <unordered_map>
#include <string>
#include "Struct.h"
#include <SDL3_ttf/SDL_ttf.h>
#include "GPUAssets.h"

struct SDL_Renderer;
struct SDL_Texture;
enum ShaderStage;
struct Shader;


class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;




	int LoadBMPSurface(StringId TextureName, const std::string& filename);
	SDL_Surface* GetSurface(StringId TextureName);

	int LoadFont(StringId FontName, const std::string& filename);
	TTF_Font* GetFont(StringId TextureName) const;


	int LoadShader(StringId shaderName, const std::string& filename,
               SDL_GPUDevice* device, ShaderStage stage,
               uint32_t numSamplers        = 0,
               uint32_t numStorageTextures = 0,
               uint32_t numStorageBuffers  = 0,
               uint32_t numUniformBuffers  = 0);
	const Shader* GetShader(StringId shaderName) const;

	int LoadMesh(StringId meshName, const std::string& filename) { return 0; };
	int AddMesh(StringId meshName, const MeshVertices& vertices, const MeshIndices& indices);
	MeshBase *GetMesh(StringId meshName);
	

	int AddMaterial(StringId materialName, const MaterialBase& material);
	int LoadMaterial(StringId materialName, const std::string filename) { return 0; };
	MaterialBase* GetMaterial(StringId materialName);

	int AddTexture(StringId textureName, const TextureBase& texture);
	TextureBase* GetTexture(StringId textureName);


	GPUFont* GetGPUFont(StringId fontName);
	int AddGPUFont(StringId fontName, GPUFont font);

private:

	std::unordered_map<StringId, TTF_Font*> _fonts;

	std::unordered_map<StringId, SDL_Surface*> _surfaces;
	std::unordered_map<StringId, MeshBase> _meshBases;
	std::unordered_map<StringId, Shader> _shaders;
	std::unordered_map<StringId, MaterialBase> _materialBases;
	std::unordered_map<StringId, TextureBase> _textureBases;
	std::unordered_map<StringId, GPUFont> _GPUFonts;

	//textures, fonts, sounds, etc.
};