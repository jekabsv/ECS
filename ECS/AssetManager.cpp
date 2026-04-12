#include "AssetManager.h"

#include <iostream>
#include <SDL3/SDL.h>
#include "logger.h"




int AssetManager::AddMesh(StringId meshName, const Mesh& mesh)
{
    _meshes[meshName] = mesh;
    LOG_DEBUG(GlobalLogger(), "AssetManager", "Adding mesh: " + std::to_string(meshName.id));
    return 0;
}
const Mesh* AssetManager::GetMesh(StringId meshName) const
{
    auto it = _meshes.find(meshName);
    if (it == _meshes.end())
    {
        LOG_WARN(GlobalLogger(), "AssetManager", "Mesh not found");
        return nullptr;
    }
    return &it->second;
}

int AssetManager::MoveMeshOrigin(StringId meshName, float dx, float dy)
{
    Mesh *mesh = GetEditableMesh(meshName);
    if (!mesh)
        return -1;
    for (auto &m : *mesh)
    {
        m.position.x -= dx;
        m.position.y -= dy;
    }
    return 0;
}

int AssetManager::LoadBMPTexture(StringId TextureName, const std::string& filename, SDL_Renderer* renderer)
{
    SDL_Surface* surface = SDL_LoadBMP(filename.c_str());
    if (!surface) {
        LOG_ERROR(GlobalLogger(), "AssetManager", std::string("Failed to load BMP: ") + SDL_GetError());
        return -1;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        LOG_ERROR(GlobalLogger(), "AssetManager", std::string("Failed to create texture: ") + SDL_GetError());
        return -1;
    }
    _textures[TextureName] = texture;
    return 0;
}

const SDL_Texture* AssetManager::GetTexture(StringId TextureName) const
{
    auto it = _textures.find(TextureName);
    if (it == _textures.end())
    {
        //LOG_WARN(GlobalLogger(), "AssetManager", "Texture not found");
        return nullptr;
    }
    return it->second;
}

int AssetManager::LoadFont(StringId FontName, const std::string& filename)
{
    TTF_Font* font = TTF_OpenFont(filename.c_str(), 24);
    if (!font)
    {
		LOG_WARN(GlobalLogger(), "AssetManager", std::string("Failed to load font: ") + SDL_GetError());
        return -1;
    }
    _fonts[FontName] = font;
    return 0;
}

TTF_Font* AssetManager::GetFont(StringId FontName) const
{
    auto it = _fonts.find(FontName);
    if (it == _fonts.end())
    {
		LOG_WARN(GlobalLogger(), "AssetManager", "Font not found");
        return nullptr;
    }
    return it->second;
}

Mesh* AssetManager::GetEditableMesh(StringId meshName)
{
    auto it = _meshes.find(meshName);
    if (it == _meshes.end())
    {
        LOG_WARN(GlobalLogger(), "AssetManager", "Mesh not found");
        return nullptr;
    }
    return &it->second;
}
