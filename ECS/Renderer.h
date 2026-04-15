#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <string>
#include <memory>

#include "Struct.h"

struct ShaderAsset;
class AssetManager;


struct TextCacheKey
{
    StringId fontId;
    std::string text;
    SDL_Color color;

    bool operator==(const TextCacheKey& o) const;
};

struct TextCacheKeyHash {
    size_t operator()(const TextCacheKey& k) const;
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


class Renderer
{
public:
    int Init(SDL_GPUDevice* gpuDevice, SDL_Window* sdlWindow, AssetManager* assets);
    int Shutdown();

    int BeginFrame();
    int StartRenderPass();
    int EndRenderPass();
    int Present();

    int BindShader(StringId shaderId);
    int SetVertexAttributes(SDL_GPUVertexAttribute* attrs, size_t count);
    int SetVertexBufferDescription(SDL_GPUVertexBufferDescription vbd);
    int SetBlendEnabled(bool enabled);
    int SetPrimitiveType(SDL_GPUPrimitiveType type);

    int PushVertexUniform(uint32_t slot, const void* data, uint32_t size);
    int PushFragmentUniform(uint32_t slot, const void* data, uint32_t size);

    int MeshDraw(StringId meshId);
    int SpriteDraw(StringId textureId);
    int DrawText(StringId fontId, const std::string& text, float x, float y, SDL_Color color);

private:
    SDL_GPUGraphicsPipeline* GetOrCreatePipeline(StringId pipelineId);
    int ApplyPipeline();

    int UploadMesh(MeshEntry* entry);
    int UploadTexture(StringId textureId);
    int UploadTextSurface(TextEntry& entry, SDL_Surface* surface);
    int ReserveTransferBuffer(size_t size);

    int EvictTextCache(uint64_t maxAge);

    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;
    AssetManager* assets = nullptr;

    SDL_GPUCommandBuffer* currentCmd = nullptr;
    SDL_GPURenderPass* currentPass = nullptr;

    SDL_GPUTransferBuffer* tBuf = nullptr;
    size_t transferBufferSize = 0;

    StringId activeVertShaderId;
    StringId activeFragShaderId;

    SDL_GPUVertexBufferDescription vbd = {};
    SDL_GPUVertexAttribute attrs[8] = {};
    size_t attrCount = 0;
    SDL_GPUPrimitiveType primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    bool blendEnabled = false;
    bool pipelineDirty = true;

    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUGraphicsPipeline* spritePipeline = nullptr;

    SDL_GPUColorTargetDescription colorDesc = {};

    std::unordered_map<StringId, SDL_GPUGraphicsPipeline*> pipelineCache;
    std::unordered_map<TextCacheKey, TextEntry, TextCacheKeyHash> textCache;
};