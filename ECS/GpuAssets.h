#pragma once
#include <SDL3/SDL_gpu.h>
#include <string>
#include "Struct.h"

enum class ShaderStage
{
    Vertex,
    Fragment,
    Geometry,
    Compute
};

struct ShaderAsset
{
    ShaderAsset(SDL_GPUShader* _handle) : handle(_handle) {};
    ShaderAsset() = default;

    SDL_GPUShader* handle = nullptr;
    ShaderStage stage = ShaderStage::Vertex;
    std::string format = {};

    size_t UniformSize = 0;

    uint32_t numSamplers = 0;
    uint32_t numStorageTextures = 0;
    uint32_t numStorageBuffers = 0;
    uint32_t numUniformBuffers = 0;
};

struct TextureEntry
{
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t lastUsed = 0;

    void Release(SDL_GPUDevice* device) {
        if (texture) SDL_ReleaseGPUTexture(device, texture);
        if (sampler) SDL_ReleaseGPUSampler(device, sampler);
    }
};

struct MeshEntry
{
    MeshIndices MeshIndices;
    MeshVertices MeshVertices;

    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUBuffer* vertexBuffer = nullptr;
    Uint32 vertexCount = 0;
    Uint32 indexCount = 0;
    uint64_t lastUsed = 0;
    bool isUploaded = false;
    size_t size = 0;

    void Release(SDL_GPUDevice* device) {
        if (vertexBuffer) SDL_ReleaseGPUBuffer(device, vertexBuffer);
        if (indexBuffer)  SDL_ReleaseGPUBuffer(device, indexBuffer);
    }
};

struct TextEntry
{
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t lastUsed = 0;
    void Release(SDL_GPUDevice* device) {
        if (texture) SDL_ReleaseGPUTexture(device, texture);
        if (sampler) SDL_ReleaseGPUSampler(device, sampler);
    }
};
