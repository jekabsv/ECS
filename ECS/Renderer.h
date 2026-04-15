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



class Renderer
{
public:

    //Material stores ShaderIds
	//when a material is used renderer gets material hash, checks if identical pipeline exists in cache,
    //if not creates pipeline with the shaders and stores it in cache with the material hash as key
    //Material 

    int Init(SDL_GPUDevice* gpuDevice, SDL_Window* sdlWindow, AssetManager* assets);

    int StartRenderPass();

    int EndRenderPass();
    int Present();
    

    int MeshDraw(StringId meshId, StringId materialId, 
        Vec2 Position = { 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f);


    int SpriteDraw(StringId materialId,
        Vec2 Position = { 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f);

    //int DrawText(StringId fontId, const std::string& text, float x, float y, SDL_Color color); //Later

private:

    int PushVertexUniform(uint32_t slot, const void* data, uint32_t size);
    int PushFragmentUniform(uint32_t slot, const void* data, uint32_t size);

    int ApplyPipeline();

    int UploadMesh(MeshEntry* entry);
    int UploadTexture(StringId textureId);
    //int UploadTextSurface(TextEntry& entry, SDL_Surface* surface); //Later
    
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
    StringId activeSpriteVertShaderId;
    StringId activeSpriteFragShaderId;

    SDL_GPUVertexBufferDescription vbd = {};
    SDL_GPUVertexAttribute attrs[8] = {};
    size_t attrCount = 0;
    SDL_GPUPrimitiveType primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUColorTargetDescription colorDesc = {};


};