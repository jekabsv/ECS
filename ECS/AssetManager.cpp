#include "AssetManager.h"
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_surface.h>

int AssetManager::AddMesh(StringId meshName, const Mesh& mesh)
{
    _meshes[meshName] = mesh;
    return 1;
}
const Mesh* AssetManager::GetMesh(StringId meshName) const
{
    auto it = _meshes.find(meshName);
    if (it == _meshes.end())
        return nullptr;
    return &it->second;
}

int AssetManager::LoadBMPTexture(StringId TextureName, const std::string& filename, SDL_Renderer* renderer)
{
    SDL_Surface* surface = SDL_LoadBMP(filename.c_str());
    if (!surface) {
        SDL_Log("Failed to load BMP: %s", SDL_GetError());
        return 0;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return 0;
    }
    _textures[TextureName] = texture;
    return 1;
}

const SDL_Texture* AssetManager::GetTexture(StringId TextureName) const
{
    auto it = _textures.find(TextureName);
    if (it == _textures.end())
        return nullptr;
    return it->second;
}

