#pragma once
#include <string>
#include <unordered_map>
#include <SDL3/SDL_gpu.h>
#include "Struct.h"
#include "logger.h"

enum ShaderStage
{
    Vertex,
    Fragment
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

struct GPUPipeline
{
    SDL_GPUGraphicsPipeline* handle = nullptr;
    StringId vertName;
    StringId fragName;
};

class ShaderManager
{
public:
    ShaderManager() = default;
    ~ShaderManager();

    void Init(SDL_GPUDevice* device);

    static std::string GetBackendExtension(SDL_GPUDevice* device);

    int CreatePipeline(StringId pipelineName, StringId vertName, StringId fragName,
        const ShaderAsset* vert, const ShaderAsset* frag);

    const GPUPipeline* GetPipeline(StringId pipelineName) const;
    int DestroyPipeline(StringId pipelineName);

    int BindPipeline(StringId pipelineName, SDL_GPURenderPass* pass);
    void UnbindPipeline();
    const GPUPipeline* GetBoundPipeline() const;



private:
    SDL_GPUDevice* _device = nullptr;
    std::unordered_map<StringId, GPUPipeline> _pipelines;

    StringId _boundPipeline;
    bool     _hasBound = false;

    static SDL_GPUVertexInputState MakeVertexInputState();
};