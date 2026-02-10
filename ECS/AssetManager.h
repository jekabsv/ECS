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

	void AddMesh(int id, const Mesh& mesh);
    Mesh& GetMesh(int id);

	void LoadBMPTexture(int id, std::string filename, SDL_Renderer* renderer);
	SDL_Texture* GetTexture(int id);

	
private:
    std::unordered_map<int, Mesh> _meshes;
	std::unordered_map<int, SDL_Texture*> _textures;
	//textures, fonts, sounds, etc.
};