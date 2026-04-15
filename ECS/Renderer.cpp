#include "Renderer.h"
#include "AssetManager.h"
#include <cstring>
#include <SDL3_ttf/SDL_ttf.h>


bool TextCacheKey::operator==(const TextCacheKey& o) const
{
    return fontId == o.fontId
        && text == o.text
        && color.r == o.color.r
        && color.g == o.color.g
        && color.b == o.color.b
        && color.a == o.color.a;
}

size_t TextCacheKeyHash::operator()(const TextCacheKey& k) const
{
    size_t h = std::hash<uint32_t>{}(k.fontId.id);
    h ^= std::hash<std::string>{}(k.text) + 0x9e3779b9 + (h << 6) + (h >> 2);
    uint32_t colorPacked = (k.color.r << 24) | (k.color.g << 16) | (k.color.b << 8) | k.color.a;
    h ^= std::hash<uint32_t>{}(colorPacked)+0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

//By default SDL_Vertex
int Renderer::Init(SDL_GPUDevice* gpuDevice, SDL_Window* sdlWindow, AssetManager* assetManager)
{
    device = gpuDevice;
    window = sdlWindow;
    assets = assetManager;

    colorDesc.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    vbd.slot = 0;
    vbd.pitch = sizeof(SDL_Vertex);
    vbd.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbd.instance_step_rate = 0;



    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[0].offset = offsetof(SDL_Vertex, position);

    attrs[1].location = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attrs[1].offset = offsetof(SDL_Vertex, color);

    attrs[2].location = 2;
    attrs[2].buffer_slot = 0;
    attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[2].offset = offsetof(SDL_Vertex, tex_coord);
    attrCount = 3;


    primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    blendEnabled = true;
    pipelineDirty = true;

    return 0;
}
int Renderer::Shutdown()
{
    for (auto& [key, pipeline] : pipelineCache)
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    pipelineCache.clear();

    if (spritePipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, spritePipeline);
        spritePipeline = nullptr;
    }

    for (auto& [key, entry] : textCache)
        entry.Release(device);
    textCache.clear();

    if (tBuf)
    {
        SDL_ReleaseGPUTransferBuffer(device, tBuf);
        tBuf = nullptr;
        transferBufferSize = 0;
    }

    return 0;
}


int Renderer::BeginFrame()
{
    currentCmd = SDL_AcquireGPUCommandBuffer(device);
    if (!currentCmd)
        return -1;
    return 0;
}

int Renderer::StartRenderPass()
{
    if (!currentCmd)
        return -1;

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!SDL_AcquireGPUSwapchainTexture(currentCmd, window, &swapchainTexture, nullptr, nullptr))
        return -1;

    if (!swapchainTexture)
        return -1;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    colorTarget.cycle = false;

    currentPass = SDL_BeginGPURenderPass(currentCmd, &colorTarget, 1, nullptr);
    if (!currentPass)
        return -1;

    return 0;
}

int Renderer::EndRenderPass()
{
    if (!currentPass)
        return -1;
    SDL_EndGPURenderPass(currentPass);
    currentPass = nullptr;
    pipeline = nullptr;
    return 0;
}

int Renderer::Present()
{
    if (!currentCmd)
        return -1;
    SDL_SubmitGPUCommandBuffer(currentCmd);
    currentCmd = nullptr;
    return 0;
}


int Renderer::BindShader(StringId shaderId)
{
    const ShaderAsset* shader = assets->GetShader(shaderId);
    if (!shader || !shader->handle)
        return -1;

    if (shader->stage == ShaderStage::Vertex)
        activeVertShaderId = shaderId;
	else if (shader->stage == ShaderStage::Fragment)
        activeFragShaderId = shaderId;

    pipelineDirty = true;
    return 0;
}

int Renderer::SetVertexAttributes(SDL_GPUVertexAttribute* inAttrs, size_t count)
{
    if (count > 8) 
        count = 8;
    memcpy(attrs, inAttrs, sizeof(SDL_GPUVertexAttribute) * count);
    attrCount = count;
    pipelineDirty = true;
    return 0;
}

int Renderer::SetVertexBufferDescription(SDL_GPUVertexBufferDescription inVbd)
{
    vbd = inVbd;
    pipelineDirty = true;
    return 0;
}

int Renderer::SetBlendEnabled(bool enabled)
{
    blendEnabled = enabled;
    pipelineDirty = true;
    return 0;
}

int Renderer::SetPrimitiveType(SDL_GPUPrimitiveType type)
{
    primitiveType = type;
    pipelineDirty = true;
    return 0;
}


int Renderer::PushVertexUniform(uint32_t slot, const void* data, uint32_t size)
{
    if (!currentCmd) 
        return -1;
    SDL_PushGPUVertexUniformData(currentCmd, slot, data, size);
    return 0;
}

int Renderer::PushFragmentUniform(uint32_t slot, const void* data, uint32_t size)
{
    if (!currentCmd) 
        return -1;
    SDL_PushGPUFragmentUniformData(currentCmd, slot, data, size);
    return 0;
}


SDL_GPUGraphicsPipeline* Renderer::GetOrCreatePipeline(StringId pipelineId)
{
    auto it = pipelineCache.find(pipelineId);
    if (it != pipelineCache.end())
        return it->second;

    const ShaderAsset* vert = assets->GetShader(activeVertShaderId);
    const ShaderAsset* frag = assets->GetShader(activeFragShaderId);
    if (!vert || !frag)
        return nullptr;

    SDL_GPUColorTargetBlendState blendState = {};
    if (blendEnabled)
    {
        blendState.enable_blend = true;
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    }

    SDL_GPUColorTargetDescription colorTarget = colorDesc;
    colorTarget.blend_state = blendState;

    SDL_GPUVertexInputState vertexInput = {};
    vertexInput.vertex_buffer_descriptions = &vbd;
    vertexInput.num_vertex_buffers = 1;
    vertexInput.vertex_attributes = attrs;
    vertexInput.num_vertex_attributes = (uint32_t)attrCount;

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
    targetInfo.color_target_descriptions = &colorTarget;
    targetInfo.num_color_targets = 1;

    SDL_GPUGraphicsPipelineCreateInfo ci = {};
    ci.vertex_shader = vert->handle;
    ci.fragment_shader = frag->handle;
    ci.vertex_input_state = vertexInput;
    ci.primitive_type = primitiveType;
    ci.target_info = targetInfo;
    //Depth later


    SDL_GPUGraphicsPipeline* p = SDL_CreateGPUGraphicsPipeline(device, &ci);
    if (!p)
        SDL_Log("Pipeline creation failed: %s", SDL_GetError());
    if (p)
        pipelineCache[pipelineId] = p;

    return p;
}

int Renderer::ApplyPipeline()
{
    if (!pipelineDirty && pipeline)
        return 0;

    uint32_t h = activeVertShaderId.id;
    h ^= activeFragShaderId.id + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= (uint32_t)primitiveType + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= (uint32_t)blendEnabled + 0x9e3779b9 + (h << 6) + (h >> 2);
    StringId key;
    key.id = h;

    pipeline = GetOrCreatePipeline(key);
    if (!pipeline)
        return -1;

    SDL_BindGPUGraphicsPipeline(currentPass, pipeline);
    pipelineDirty = false;
    return 0;
}


int Renderer::ReserveTransferBuffer(size_t size)
{
    if (size <= transferBufferSize)
        return 0;

    if (tBuf)
    {
        SDL_ReleaseGPUTransferBuffer(device, tBuf);
        tBuf = nullptr;
    }

    SDL_GPUTransferBufferCreateInfo ci = {};
    ci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    ci.size = (Uint32)size;

    tBuf = SDL_CreateGPUTransferBuffer(device, &ci);
    if (!tBuf)
        return -1;

    transferBufferSize = size;
    return 0;
}


int Renderer::UploadMesh(MeshEntry* entry)
{
    if (!entry || entry->MeshVertices.empty())
        return -1;

    size_t vertSize = entry->MeshVertices.size() * sizeof(SDL_Vertex);
    size_t idxSize = entry->MeshIndices.size() * sizeof(uint16_t);
    size_t totalSize = vertSize + idxSize;

    if (ReserveTransferBuffer(totalSize) != 0)
        return -1;

    // Write CPU data into transfer buffer
    void* mapped = SDL_MapGPUTransferBuffer(device, tBuf, false);
    if (!mapped) return -1;
    memcpy(mapped, entry->MeshVertices.data(), vertSize);
    if (idxSize)
        memcpy((uint8_t*)mapped + vertSize, entry->MeshIndices.data(), idxSize);
    SDL_UnmapGPUTransferBuffer(device, tBuf);


    SDL_GPUBufferCreateInfo vbci = {};
    vbci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbci.size = (Uint32)vertSize;
    entry->vertexBuffer = SDL_CreateGPUBuffer(device, &vbci);
    if (!entry->vertexBuffer) return -1;

    if (idxSize)
    {
        SDL_GPUBufferCreateInfo ibci = {};
        ibci.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibci.size = (Uint32)idxSize;
        entry->indexBuffer = SDL_CreateGPUBuffer(device, &ibci);
        if (!entry->indexBuffer) return -1;
    }


    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    if (!uploadCmd) return -1;

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

    SDL_GPUTransferBufferLocation vertSrc = {};
    vertSrc.transfer_buffer = tBuf;
    vertSrc.offset = 0;
    SDL_GPUBufferRegion vertDst = {};
    vertDst.buffer = entry->vertexBuffer;
    vertDst.offset = 0;
    vertDst.size = (Uint32)vertSize;
    SDL_UploadToGPUBuffer(copyPass, &vertSrc, &vertDst, false);

    if (idxSize)
    {
        SDL_GPUTransferBufferLocation idxSrc = {};
        idxSrc.transfer_buffer = tBuf;
        idxSrc.offset = (Uint32)vertSize;
        SDL_GPUBufferRegion idxDst = {};
        idxDst.buffer = entry->indexBuffer;
        idxDst.offset = 0;
        idxDst.size = (Uint32)idxSize;
        SDL_UploadToGPUBuffer(copyPass, &idxSrc, &idxDst, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmd);

    entry->vertexCount = (Uint32)entry->MeshVertices.size();
    entry->indexCount = (Uint32)entry->MeshIndices.size();
    entry->isUploaded = true;

    return 0;
}

int Renderer::UploadTexture(StringId textureId)
{
    SDL_Surface* surface = assets->GetSurface(textureId);
    if (!surface) return -1;

    TextureEntry* texEntry = assets->GetTextureEntry(textureId);
    if (!texEntry) return -1;


    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!converted) return -1;

    size_t dataSize = converted->w * converted->h * 4;
    if (ReserveTransferBuffer(dataSize) != 0)
    {
        SDL_DestroySurface(converted);
        return -1;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, tBuf, false);
    if (!mapped) { SDL_DestroySurface(converted); return -1; }
    memcpy(mapped, converted->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, tBuf);

    SDL_GPUTextureCreateInfo tci = {};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = (Uint32)converted->w;
    tci.height = (Uint32)converted->h;
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    texEntry->texture = SDL_CreateGPUTexture(device, &tci);

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    texEntry->sampler = SDL_CreateGPUSampler(device, &sci);

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tBuf;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = texEntry->texture;
    dst.w = (Uint32)converted->w;
    dst.h = (Uint32)converted->h;
    dst.d = 1;

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmd);

    SDL_DestroySurface(converted);
    return 0;
}

int Renderer::UploadTextSurface(TextEntry& entry, SDL_Surface* surface)
{
    if (!surface) return -1;

    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!converted) return -1;

    size_t dataSize = converted->w * converted->h * 4;
    if (ReserveTransferBuffer(dataSize) != 0)
    {
        SDL_DestroySurface(converted);
        return -1;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, tBuf, false);
    if (!mapped) { SDL_DestroySurface(converted); return -1; }
    memcpy(mapped, converted->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, tBuf);

    SDL_GPUTextureCreateInfo tci = {};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = (Uint32)converted->w;
    tci.height = (Uint32)converted->h;
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    entry.texture = SDL_CreateGPUTexture(device, &tci);
    entry.width = (uint32_t)converted->w;
    entry.height = (uint32_t)converted->h;

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    entry.sampler = SDL_CreateGPUSampler(device, &sci);

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tBuf;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = entry.texture;
    dst.w = (Uint32)converted->w;
    dst.h = (Uint32)converted->h;
    dst.d = 1;

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmd);

    SDL_DestroySurface(converted);
    return 0;
}


int Renderer::MeshDraw(StringId meshId)
{
    if (!currentPass) 
        return -1;
    if (ApplyPipeline() != 0)
        return -1;

    MeshEntry* entry = assets->GetMesh(meshId);
    if (!entry) 
        return -1;

    if (!entry->isUploaded)
        if (UploadMesh(entry) != 0)
            return -1;

    entry->lastUsed = SDL_GetTicks();

    SDL_GPUBufferBinding vbBinding = {};
    vbBinding.buffer = entry->vertexBuffer;
    vbBinding.offset = 0;
    SDL_BindGPUVertexBuffers(currentPass, 0, &vbBinding, 1);

    if (entry->indexBuffer && entry->indexCount > 0)
    {
        SDL_GPUBufferBinding ibBinding = {};
        ibBinding.buffer = entry->indexBuffer;
        ibBinding.offset = 0;
        SDL_BindGPUIndexBuffer(currentPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(currentPass, entry->indexCount, 1, 0, 0, 0);
    }
    else
    {
        SDL_DrawGPUPrimitives(currentPass, entry->vertexCount, 1, 0, 0);
    }

    return 0;
}

int Renderer::SpriteDraw(StringId textureId)
{
    if (!currentPass || !spritePipeline)
        return -1;

    TextureEntry* texEntry = assets->GetTextureEntry(textureId);
    if (!texEntry) return -1;


    if (!texEntry->texture)
    {
        if (UploadTexture(textureId) != 0)
            return -1;
    }

    SDL_BindGPUGraphicsPipeline(currentPass, spritePipeline);
    pipeline = spritePipeline;
    pipelineDirty = true;

    SDL_GPUTextureSamplerBinding binding = {};
    binding.texture = texEntry->texture;
    binding.sampler = texEntry->sampler;
    SDL_BindGPUFragmentSamplers(currentPass, 0, &binding, 1);


    SDL_DrawGPUPrimitives(currentPass, 6, 1, 0, 0);

    return 0;
}

int Renderer::DrawText(StringId fontId, const std::string& text, float x, float y, SDL_Color color)
{
    if (!currentPass || !spritePipeline)
        return -1;

    TextCacheKey key{ fontId, text, color };
    uint64_t now = SDL_GetTicks();

    auto it = textCache.find(key);
    if (it == textCache.end())
    {
        TTF_Font* font = assets->GetFont(fontId);
        if (!font) return -1;

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) return -1;

        TextEntry entry = {};
        if (UploadTextSurface(entry, surface) != 0)
        {
            SDL_DestroySurface(surface);
            return -1;
        }
        SDL_DestroySurface(surface);

        entry.lastUsed = now;
        textCache[key] = entry;
        it = textCache.find(key);
    }
    else
    {
        it->second.lastUsed = now;
    }

    TextEntry& entry = it->second;

    SDL_BindGPUGraphicsPipeline(currentPass, spritePipeline);
    pipeline = spritePipeline;
    pipelineDirty = true;

    SDL_GPUTextureSamplerBinding binding = {};
    binding.texture = entry.texture;
    binding.sampler = entry.sampler;
    SDL_BindGPUFragmentSamplers(currentPass, 0, &binding, 1);

    struct TextPushData { float x, y, w, h; } push{ x, y, (float)entry.width, (float)entry.height };
    SDL_PushGPUVertexUniformData(currentCmd, 0, &push, sizeof(push));

    SDL_DrawGPUPrimitives(currentPass, 6, 1, 0, 0);

    return 0;
}


int Renderer::EvictTextCache(uint64_t maxAge)
{
    uint64_t now = SDL_GetTicks();
    for (auto it = textCache.begin(); it != textCache.end(); )
    {
        if (now - it->second.lastUsed > maxAge)
        {
            it->second.Release(device);
            it = textCache.erase(it);
        }
        else
        {
            it++;
        }
    }
    return 0;
}