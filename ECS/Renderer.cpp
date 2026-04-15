#include "Renderer.h"
#include <iostream>

int Renderer::Init(SDL_GPUDevice* gpuDevice, SDL_Window* sdlWindow, AssetManager* assets, uint32_t screenWidth, uint32_t screenHeight)
{
	_device = gpuDevice;
	_window = sdlWindow;
	_assets = assets;
	_screenHeight = screenHeight;
	_screenWidth = screenWidth;

    SDL_GPUTextureCreateInfo depthInfo = {};
    depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthInfo.width = _screenWidth;
    depthInfo.height = _screenHeight;
    depthInfo.layer_count_or_depth = 1;
    depthInfo.num_levels = 1;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    _depthStencilTexture = SDL_CreateGPUTexture(_device, &depthInfo);

    transferBufferSize = 64 * 1024;
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = (uint32_t)transferBufferSize;
    tBuf = SDL_CreateGPUTransferBuffer(_device, &tbi);

	CreateUnitQuad();

    UpdateDefaultProjection();

    return 0;
}

int Renderer::Shutdown()
{
    SDL_ReleaseGPUTexture(_device, _depthStencilTexture);
    SDL_ReleaseGPUTransferBuffer(_device, tBuf);
    SDL_ReleaseGPUBuffer(_device, _unitQuadMesh.vertexBuffer);
    SDL_ReleaseGPUBuffer(_device, _unitQuadMesh.indexBuffer);
    SDL_ReleaseWindowFromGPUDevice(_device, _window);

    return 0;
}

int Renderer::SetWindowSize(uint32_t width, uint32_t height)
{
    _screenHeight = height;
    _screenWidth = width;

    if (_depthStencilTexture)
    {
        SDL_ReleaseGPUTexture(_device, _depthStencilTexture);
        _depthStencilTexture = nullptr;
    }

    SDL_GPUTextureCreateInfo depthInfo = {};
    depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthInfo.width = _screenWidth;
    depthInfo.height = _screenHeight;
    depthInfo.layer_count_or_depth = 1;
    depthInfo.num_levels = 1;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    _depthStencilTexture = SDL_CreateGPUTexture(_device, &depthInfo);

    UpdateDefaultProjection();

    return 0;
}

int Renderer::StartRenderPass()
{
    currentCmd = SDL_AcquireGPUCommandBuffer(_device);
    if (!currentCmd)
        return -1;

    if (!SDL_AcquireGPUSwapchainTexture(currentCmd, _window, &currentSwapchainTexture, nullptr, nullptr)) {
        SDL_SubmitGPUCommandBuffer(currentCmd);
        return -1;
    }

    if (currentSwapchainTexture == nullptr)
        return -1;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = currentSwapchainTexture;
    colorTarget.clear_color = { 0.1f, 0.1f, 0.1f, 1.0f };
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = _depthStencilTexture;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    currentPass = SDL_BeginGPURenderPass(currentCmd, &colorTarget, 1, &depthTarget);

    SDL_PushGPUVertexUniformData(currentCmd, 0, &_viewProjMatrix, sizeof(Matrix4));

    return 0;
}

int Renderer::EndRenderPass()
{
    if (!currentPass)
        return -1;

    SDL_EndGPURenderPass(currentPass);
    currentPass = nullptr;

    return 0;
}

int Renderer::Present()
{
    if (!currentCmd)
        return -1;

    SDL_SubmitGPUCommandBuffer(currentCmd);

    currentCmd = nullptr;
    currentSwapchainTexture = nullptr;

    return 0;
}

int Renderer::DrawMesh(MeshInstance mesh, MaterialInstance material, Vec2 Position, Vec2 Scale, float Rotation)
{
    MeshBase* meshBase = _assets->GetMesh(mesh.meshName);
    MaterialBase* matBase = _assets->GetMaterial(material.materialName);


    if (!meshBase || !matBase)
        return -1;

    if (!meshBase->isLoaded) {
        UploadMesh(meshBase);
    }

    SDL_GPUGraphicsPipeline* pipeline = GetOrCreatePipeline(matBase);

    if (!pipeline)
        return -1;


    SDL_BindGPUGraphicsPipeline(currentPass, pipeline);

    ObjectData objData;

    float cosR = cosf(Rotation);
    float sinR = sinf(Rotation);

    objData.modelMatrix = { 0 };
    objData.modelMatrix.m[0] = cosR * Scale.x;
    objData.modelMatrix.m[1] = sinR * Scale.x;
    objData.modelMatrix.m[4] = -sinR * Scale.y;
    objData.modelMatrix.m[5] = cosR * Scale.y;
    objData.modelMatrix.m[10] = 1.0f;
    objData.modelMatrix.m[12] = Position.x;
    objData.modelMatrix.m[13] = Position.y;
    objData.modelMatrix.m[15] = 1.0f;

    objData.colorTint[0] = 1.0f; objData.colorTint[1] = 1.0f;
    objData.colorTint[2] = 1.0f; objData.colorTint[3] = 1.0f;

    SDL_PushGPUVertexUniformData(currentCmd, 1, &objData, sizeof(ObjectData));

    if (!material.uniformVertBufferData.empty()) {
        SDL_PushGPUVertexUniformData(currentCmd, 2,
            material.uniformVertBufferData.data(),
            (uint32_t)material.uniformVertBufferData.size());
    }

    if (!material.uniformFragBufferData.empty()) {
        SDL_PushGPUFragmentUniformData(currentCmd, 0,
            material.uniformFragBufferData.data(),
            (uint32_t)material.uniformFragBufferData.size());
    }


    BindMaterialTextures(material);

    SDL_GPUBufferBinding vertexBinding = { meshBase->vertexBuffer, 0 };
    SDL_BindGPUVertexBuffers(currentPass, 0, &vertexBinding, 1);

    SDL_GPUBufferBinding indexBinding = { meshBase->indexBuffer, 0 };
    SDL_BindGPUIndexBuffer(currentPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    uint32_t numIndices = (uint32_t)(meshBase->size / sizeof(uint32_t));
    SDL_DrawGPUIndexedPrimitives(currentPass, numIndices, 1, 0, 0, 0);


    if (!meshBase->meshVertices.empty()) {
        auto& v = meshBase->meshVertices[0];
        SDL_Log("Drawing %d indices. First Vertex: x=%f, y=%f", numIndices, v.position.x, v.position.y);
    }


    return 0;
}

int Renderer::SpriteDraw(MaterialInstance material, Vec2 Position, Vec2 Scale, float Rotation)
{
    MeshInstance quadMesh{ _unitQuadMesh.name };
	return DrawMesh(quadMesh, material, Position, Scale, Rotation);
    return 0;
}

int Renderer::SetProjection(const Matrix4& projection)
{
	_viewProjMatrix = projection;
    return 0;
}

TextureBase Renderer::CreateTexture(SDL_Surface* surface)
{
    TextureBase result = {};
    if (!surface) 
        return result;

    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!converted) return result;

    result.width = converted->w;
    result.height = converted->h;

    SDL_GPUTextureCreateInfo tci = {};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.width = result.width;
    tci.height = result.height;
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    
    result.texture = SDL_CreateGPUTexture(_device, &tci);


    uint32_t pixelDataSize = converted->h * converted->pitch;
    ReserveTransferBuffer(pixelDataSize);


    void* mapPtr = SDL_MapGPUTransferBuffer(_device, tBuf, false);
    memcpy(mapPtr, converted->pixels, pixelDataSize);
    SDL_UnmapGPUTransferBuffer(_device, tBuf);


    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tBuf;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = result.texture;
    dst.w = result.width;
    dst.h = result.height;
    dst.d = 1;

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);


    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    result.sampler = SDL_CreateGPUSampler(_device, &sci);


    SDL_DestroySurface(converted);
    return result;

}

int Renderer::ReserveTransferBuffer(size_t size)
{
    if (transferBufferSize >= size) {
        return 0;
    }

    if (tBuf != nullptr) {
        SDL_WaitForGPUIdle(_device);
        SDL_ReleaseGPUTransferBuffer(_device, tBuf);
    }


    size_t newSize = transferBufferSize == 0 ? 64 * 1024 : transferBufferSize;
    while (newSize < size) {
        newSize *= 2;
    }

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = (uint32_t)newSize;

    tBuf = SDL_CreateGPUTransferBuffer(_device, &tbi);
    if (!tBuf) {
        SDL_Log("Failed to reallocate Transfer Buffer: %s", SDL_GetError());
        transferBufferSize = 0;
        return -1;
    }

    transferBufferSize = newSize;
    return 0;
}

SDL_GPUGraphicsPipeline* Renderer::GetOrCreatePipeline(MaterialBase* base)
{
    if (base->pipeline != nullptr)
        return base->pipeline;

    const Shader* vertShader = _assets->GetShader(base->vertexShader);
    const Shader* fragShader = _assets->GetShader(base->fragmentShader);

    if (!vertShader || !fragShader)
    {
        SDL_Log("Failed to create pipeline: Missing shaders.");
        return nullptr;
    }

    SDL_GPUColorTargetDescription colorTarget = {};
    colorTarget.format = base->colorTargetFormat;

    if (base->blendMode != BlendMode::None)
    {
        colorTarget.blend_state.enable_blend = true;

        if (base->blendMode == BlendMode::Alpha)
        {
            colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
            colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        }
        else if (base->blendMode == BlendMode::Additive)
        {
            colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        }
        //other blend modes
    }


    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;

    if (base->hasDepthTarget)
    {
        pipelineInfo.target_info.has_depth_stencil_target = true;
        pipelineInfo.target_info.depth_stencil_format = base->depthStencilFormat;
    }

    pipelineInfo.depth_stencil_state.enable_depth_test = base->depthTestEnabled;
    pipelineInfo.depth_stencil_state.enable_depth_write = base->depthWriteEnabled;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    pipelineInfo.rasterizer_state.cull_mode = base->cullMode;
    pipelineInfo.rasterizer_state.fill_mode = base->fillMode;
    pipelineInfo.primitive_type = base->primitiveType;

    pipelineInfo.vertex_shader = vertShader->gpuShader;
    pipelineInfo.fragment_shader = fragShader->gpuShader;

    pipelineInfo.vertex_input_state.num_vertex_attributes = (uint32_t)base->attributes.size();
    pipelineInfo.vertex_input_state.vertex_attributes = base->attributes.data();




    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;
    vertexBufferDesc.pitch = sizeof(SDL_Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;

    base->pipeline = SDL_CreateGPUGraphicsPipeline(_device, &pipelineInfo);

    if (!base->pipeline) {
        SDL_Log("PIPELINE ERROR: %s", SDL_GetError());
    }
    else {
        SDL_Log("PIPELINE SUCCESS: Created pipeline for material.");
    }

    return base->pipeline;
}

void Renderer::BindMaterialTextures(MaterialInstance material)
{
    if (material.textures.empty())
        return;

    for (uint32_t i = 0; i < (uint32_t)material.textures.size();i++) {
        TextureBase* texBase = _assets->GetTexture(material.textures[i]);

        if (texBase && texBase->texture && texBase->sampler)
        {
            SDL_GPUTextureSamplerBinding binding = {};
            binding.texture = texBase->texture;
            binding.sampler = texBase->sampler;

            SDL_BindGPUFragmentSamplers(currentPass, i, &binding, 1);
        }
        else {
            // "Missing Texture"
        }
    }
}

void Renderer::CreateUnitQuad()
{
    MeshVertices vertices = {
        { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f },
        {  0.5f,  0.5f, 0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, 0.0f, 0.0f, 0.0f }
    };
    MeshIndices indices = { 0, 1, 2,   2, 3, 0 };

    _unitQuadMesh.name = StringId("unit_quad");
    _unitQuadMesh.meshVertices = vertices;
	_unitQuadMesh.meshIndices = indices;
}

void Renderer::UpdateDefaultProjection()
{
    float left = 0.0f;
    float right = (float)_screenWidth;
    float top = 0.0f;
    float bottom = (float)_screenHeight;
    float near = -1.0f;
    float far = 1.0f;
	_viewProjMatrix = Matrix4::Orthographic(left, right, top, bottom, near, far);
}

void Renderer::UploadMesh(MeshBase* mesh) {
    if (!mesh || mesh->isLoaded)
        return;

    size_t vertSize = mesh->meshVertices.size() * sizeof(SDL_Vertex);
    size_t indexSize = mesh->meshIndices.size() * sizeof(uint32_t);
    size_t totalSize = vertSize + indexSize;

    ReserveTransferBuffer(totalSize);

    SDL_GPUBufferCreateInfo vertBufInfo = {};
    vertBufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertBufInfo.size = (uint32_t)vertSize;
    mesh->vertexBuffer = SDL_CreateGPUBuffer(_device, &vertBufInfo);

    SDL_GPUBufferCreateInfo indexBufInfo = {};
    indexBufInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufInfo.size = (uint32_t)indexSize;
    mesh->indexBuffer = SDL_CreateGPUBuffer(_device, &indexBufInfo);

    void* mapPtr = SDL_MapGPUTransferBuffer(_device, tBuf, false);
    memcpy(mapPtr, mesh->meshVertices.data(), vertSize);
    memcpy((uint8_t*)mapPtr + vertSize, mesh->meshIndices.data(), indexSize);
    SDL_UnmapGPUTransferBuffer(_device, tBuf);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation vertSrc = { tBuf, 0 };
    SDL_GPUBufferRegion vertDst = { mesh->vertexBuffer, 0, (uint32_t)vertSize };
    SDL_UploadToGPUBuffer(copyPass, &vertSrc, &vertDst, false);

    SDL_GPUTransferBufferLocation indexSrc = { tBuf, (uint32_t)vertSize };
    SDL_GPUBufferRegion indexDst = { mesh->indexBuffer, 0, (uint32_t)indexSize };
    SDL_UploadToGPUBuffer(copyPass, &indexSrc, &indexDst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_WaitForGPUIdle(_device);

    /*std::cout << "Uploaded Mesh: " << mesh->name.id
        << " | Verts: " << mesh->meshVertices.size()
		<< " | Indices: " << mesh->meshIndices.size() << std::endl;*/

    mesh->size = indexSize;
    mesh->isLoaded = true;

}