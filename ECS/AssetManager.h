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

	int AddMesh(StringId meshName, const Mesh& mesh);
	const Mesh* GetMesh(StringId meshName) const;
	int MoveMeshOrigin(StringId meshName, float dx, float dy);


	int LoadBMPTexture(StringId TextureName, const std::string& filename, SDL_Renderer* renderer);
	const SDL_Texture* GetTexture(StringId TextureName) const;


	int LoadFont(StringId FontName, const std::string& filename);
	TTF_Font* GetFont(StringId TextureName) const;

	
private:

	Mesh* GetEditableMesh(StringId meshName);
    std::unordered_map<StringId, Mesh> _meshes;
	std::unordered_map<StringId, SDL_Texture*> _textures;
	std::unordered_map<StringId, TTF_Font*> _fonts;

	//textures, fonts, sounds, etc.
};