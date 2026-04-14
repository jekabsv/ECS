#include "AssetManager.h"

#include <iostream>
#include <SDL3/SDL.h>
#include "logger.h"
#include "ShaderManager.h"
#include "Utility.h"



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

int AssetManager::LoadShader(StringId shaderName, const std::string& filename,
    SDL_GPUDevice* device, ShaderStage stage,
    uint32_t numSamplers,
    uint32_t numStorageTextures,
    uint32_t numStorageBuffers,
    uint32_t numUniformBuffers)
{
    std::string ext = Utility::ExtractExtension(filename);
    std::string expected = ShaderManager::GetBackendExtension(device);

    if (!expected.empty() && ext != expected)
        LOG_WARN(GlobalLogger(), "AssetManager",
            "Shader '" + filename + "' has extension '." + ext +
            "' but current backend expects '." + expected + "'");

    auto blob = Utility::ReadFileBinary(filename);
    if (blob.empty())
    {
        LOG_WARN(GlobalLogger(), "AssetManager",
            "Shader file not found or empty: " + filename);
        return -1;
    }

    SDL_GPUShaderStage gpuStage = (stage == ShaderStage::Vertex)
        ? SDL_GPU_SHADERSTAGE_VERTEX
        : SDL_GPU_SHADERSTAGE_FRAGMENT;

    SDL_GPUShaderCreateInfo info{};
    info.code = blob.data();
    info.code_size = blob.size();
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV; // resolved per-backend below
    info.stage = gpuStage;
    info.num_samplers = numSamplers;
    info.num_storage_textures = numStorageTextures;
    info.num_storage_buffers = numStorageBuffers;
    info.num_uniform_buffers = numUniformBuffers;

    // Map extension to SDL format flag
    if (ext == "spv")  info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (ext == "dxil") info.format = SDL_GPU_SHADERFORMAT_DXIL;
    else if (ext == "msl")  info.format = SDL_GPU_SHADERFORMAT_MSL;
    else
        LOG_WARN(GlobalLogger(), "AssetManager",
            "Unknown shader extension '." + ext + "', defaulting to SPIRV");

    SDL_GPUShader* handle = SDL_CreateGPUShader(device, &info);
    if (!handle)
    {
        LOG_WARN(GlobalLogger(), "AssetManager",
            "SDL_CreateGPUShader failed for: " + filename);
        return -1;
    }

    ShaderAsset asset(handle);
    asset.stage = stage;
    asset.format = ext;
    asset.numSamplers = numSamplers;
    asset.numStorageTextures = numStorageTextures;
    asset.numStorageBuffers = numStorageBuffers;
    asset.numUniformBuffers = numUniformBuffers;

    _shaders[shaderName] = asset;
    return 0;
}

const ShaderAsset* AssetManager::GetShader(StringId shaderName) const
{
    auto it = _shaders.find(shaderName);
    if (it == _shaders.end())
        return nullptr;
    return &it->second;
}