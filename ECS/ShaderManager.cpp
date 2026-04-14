#include "ShaderManager.h"
#include "logger.h"
#include <SDL3/SDL_gpu.h>
#include <fstream>
#include <vector>
#include <filesystem>

ShaderManager::~ShaderManager()
{
    if (!_device)
        return;
    for (auto& [id, pipeline] : _pipelines)
        if (pipeline.handle)
            SDL_ReleaseGPUGraphicsPipeline(_device, pipeline.handle);
}

void ShaderManager::Init(SDL_GPUDevice* device)
{
    _device = device;
}

std::string ShaderManager::GetBackendExtension(SDL_GPUDevice* device)
{
    if (!device)
        return "";
    const char* driver = SDL_GetGPUDeviceDriver(device);
    if (!driver)
        return "";
    std::string d(driver);
    if (d == "vulkan")      return "spv";
    else if (d == "direct3d12")  return "dxil";
    else if (d == "metal")       return "msl";
    return "";
}

SDL_GPUVertexInputState ShaderManager::MakeVertexInputState()
{
    // SDL_Vertex: position (float2) + color (float4) packed
    static SDL_GPUVertexAttribute attrs[2] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(SDL_Vertex, position) },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(SDL_Vertex, color)    }
    };
    static SDL_GPUVertexBufferDescription bufDesc = {
        0,                                  // slot
        sizeof(SDL_Vertex),                 // pitch
        SDL_GPU_VERTEXINPUTRATE_VERTEX,     // input rate
        0                                   // instance step rate
    };

    SDL_GPUVertexInputState state{};
    state.vertex_attributes = attrs;
    state.num_vertex_attributes = 2;
    state.vertex_buffer_descriptions = &bufDesc;
    state.num_vertex_buffers = 1;
    return state;
}

int ShaderManager::CreatePipeline(StringId pipelineName,
    StringId vertName, StringId fragName,
    const ShaderAsset* vert,
    const ShaderAsset* frag)
{
    if (!_device)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "CreatePipeline called before Init()");
        return -1;
    }
    if (!vert || !vert->handle)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "CreatePipeline: invalid vertex shader");
        return -1;
    }
    if (!frag || !frag->handle)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "CreatePipeline: invalid fragment shader");
        return -1;
    }
    if (vert->stage != ShaderStage::Vertex)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager",
            "CreatePipeline: shader passed as vertex is not a vertex stage shader");
        return -1;
    }
    if (frag->stage != ShaderStage::Fragment)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager",
            "CreatePipeline: shader passed as fragment is not a fragment stage shader");
        return -1;
    }

    auto vertexInputState = MakeVertexInputState();

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vert->handle;
    info.fragment_shader = frag->handle;
    info.vertex_input_state = vertexInputState;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.target_info.color_target_descriptions = &colorTarget;
    info.target_info.num_color_targets = 1;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

    SDL_GPUGraphicsPipeline* handle = SDL_CreateGPUGraphicsPipeline(_device, &info);
    if (!handle)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager",
            "SDL_CreateGPUGraphicsPipeline failed");
        return -1;
    }

    GPUPipeline pipeline{};
    pipeline.handle = handle;
    pipeline.vertName = vertName;
    pipeline.fragName = fragName;

    _pipelines[pipelineName] = pipeline;
    return 0;
}

const GPUPipeline* ShaderManager::GetPipeline(StringId pipelineName) const
{
    auto it = _pipelines.find(pipelineName);
    if (it == _pipelines.end())
        return nullptr;
    return &it->second;
}

int ShaderManager::DestroyPipeline(StringId pipelineName)
{
    auto it = _pipelines.find(pipelineName);
    if (it == _pipelines.end())
        return -1;
    if (it->second.handle)
        SDL_ReleaseGPUGraphicsPipeline(_device, it->second.handle);
    _pipelines.erase(it);
    return 0;
}

int ShaderManager::BindPipeline(StringId pipelineName, SDL_GPURenderPass* pass)
{
    if (!pass)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "BindPipeline: null render pass");
        return -1;
    }

    if (_hasBound && _boundPipeline == pipelineName)
        return 0;

    auto it = _pipelines.find(pipelineName);
    if (it == _pipelines.end())
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "BindPipeline: pipeline not found");
        return -1;
    }
    if (!it->second.handle)
    {
        LOG_WARN(GlobalLogger(), "ShaderManager", "BindPipeline: pipeline has null handle");
        return -1;
    }

    SDL_BindGPUGraphicsPipeline(pass, it->second.handle);
    _boundPipeline = pipelineName;
    _hasBound = true;
    return 0;
}

void ShaderManager::UnbindPipeline()
{
    _boundPipeline = StringId{};
    _hasBound = false;
}

const GPUPipeline* ShaderManager::GetBoundPipeline() const
{
    if (!_hasBound)
        return nullptr;
    auto it = _pipelines.find(_boundPipeline);
    if (it == _pipelines.end())
        return nullptr;
    return &it->second;
}