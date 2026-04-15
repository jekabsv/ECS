#pragma once

#include <array>
#include <string_view>
#include <vector>
#include <string>

#include <SDL3/SDL.h>
#include "Struct.h"

struct SDL_Vertex;
struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;

struct Vec2
{
    float x, y;
};

struct Vec3
{
    float x, y, z;
};

using Triangle = std::array<SDL_Vertex, 3>;

using MeshVertices = std::vector<SDL_Vertex>;
using MeshIndices = std::vector<uint16_t>;

struct StringId
{
    uint32_t id = 0;

    StringId() = default;
    constexpr StringId(const char* str) : StringId(std::string_view(str)) {}
    constexpr StringId(std::string_view str)
    {
        uint32_t h = 2166136261u;
        for (char c : str) 
            h = (h ^ (uint8_t)c) * 16777619u;
        id = h;
    }

    constexpr bool operator==(const StringId& o) const { return id == o.id; }
    constexpr bool operator!=(const StringId& o) const { return id != o.id; }
};

template<>
struct std::hash<StringId>
{
    size_t operator()(const StringId& s) const noexcept { return s.id; }
};



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





inline void hash_combine(std::size_t& seed, std::size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


struct Material
{
	StringId name;

	StringId vertShaderId;
	StringId fragShaderId;

    bool blendMode = false;


    std::vector<uint8_t> uniformBufferData;

    std::unordered_map<StringId, TextureEntry> textures;

    void setData(size_t offset, const void* data, size_t size) {
        if (uniformBufferData.size() < offset + size) {
            uniformBufferData.resize(offset + size);
        }
        memcpy(uniformBufferData.data() + offset, data, size);
    }

    size_t getPipelineHash() const {
        size_t h = 0;

        hash_combine(h, std::hash<uint64_t>{}(vertShaderId.id));
        hash_combine(h, std::hash<uint64_t>{}(fragShaderId.id));
        hash_combine(h, std::hash<bool>{}(blendMode));
        return h;
    }
};