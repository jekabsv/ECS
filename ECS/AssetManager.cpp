#include "AssetManager.h"
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_surface.h>

void AssetManager::AddMesh(int id, const Mesh& mesh) 
{
    _meshes[id] = mesh;
}

Mesh& AssetManager::GetMesh(int id)
{
    return _meshes[id];
}

void AssetManager::LoadBMPTexture(int id, std::string filename, SDL_Renderer* renderer)
{
    SDL_Surface* surface = SDL_LoadBMP(filename.c_str());
    if (!surface) {
        SDL_Log("Failed to load BMP: %s", SDL_GetError());
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return;
    }
    _textures[id] = texture;
    return;
}

SDL_Texture* AssetManager::GetTexture(int id)
{
    if(id == -1)
        return nullptr;
    return _textures[id];
}
