#pragma once
#include <unordered_map>
#include <string>
#include "Struct.h"
#include <SDL3_ttf/SDL_ttf.h>
#include "GpuAssets.h"
#include "Materials.h"

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



	int AddMesh(StringId meshName, const MeshVertices& vertices, const MeshIndices& indices);
	MeshEntry* GetMesh(StringId);


	int LoadTexture(StringId TextureName, TextureEntry texture);
	TextureEntry* GetTextureEntry(StringId);


	int LoadMaterial(StringId materialName, const std::string filepath) { return 0; };
	Material* GetMaterial(StringId materialName);


	int loadText(StringId textId, TextEntry text);
	TextEntry* GetTextEntry(StringId textId);

	int AddGraphicsPipeline(StringId pipelineId, SDL_GPUGraphicsPipeline *pipeline);
	SDL_GPUGraphicsPipeline* GetGraphicsPipeline(size_t pipelineId);
	
private:

	std::unordered_map<StringId, ShaderAsset> _shaders;//shaders loaded when material is loaded

	std::unordered_map<StringId, SDL_Surface*> _surfaces;//surfaces loaded by user
	std::unordered_map<StringId, TextureEntry> _textures;//textures populated by renderer when draw call uses the texture

	std::unordered_map<StringId, MeshEntry> _meshes;
	std::unordered_map<StringId, Material> _materials;

	std::unordered_map<StringId, TTF_Font*> _fonts;//fonts loaded by user
	std::unordered_map<StringId, TextEntry> _textCache;//text populated by renderer when draw call uses a font

	std::unordered_map<size_t, SDL_GPUGraphicsPipeline*> _GraphicsPipelines; //populated by renderer



	//textures, fonts, sounds, etc.
};