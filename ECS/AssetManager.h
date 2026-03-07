#pragma once
#include <unordered_map>
#include "Struct.h"
#include <array>
#include <string>

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	int AddMesh(StringId meshName, const Mesh& mesh);
	const Mesh* GetMesh(StringId meshName) const;

	int LoadBMPTexture(StringId TextureName, const std::string& filename, SDL_Renderer* renderer);
	const SDL_Texture* GetTexture(StringId TextureName) const;

	
private:
    std::unordered_map<StringId, Mesh> _meshes;
	std::unordered_map<StringId, SDL_Texture*> _textures;
	//textures, fonts, sounds, etc.
};