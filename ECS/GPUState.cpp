#include "GPUState.h"
#include "SharedDataRef.h";

void GPUState::Init()
{
    _data->assets.LoadShader("basic_vert", "../shaders/basic_vert.spv",
        _data->device, ShaderStage::Vertex);

    _data->assets.LoadShader("basic_frag", "../shaders/basic_frag.spv",
        _data->device, ShaderStage::Fragment);

    verts[0].position.x = 0.0f;
    verts[0].position.y = 0.5f;

    verts[1].position.x = -0.5f;
    verts[1].position.y = -0.5f;

    verts[2].position.x = 0.5f;
    verts[2].position.y = -0.5f;

    verts[0].color = { 1, 0, 0, 1 };
    verts[1].color = { 0, 1, 0, 1 };
    verts[2].color = { 0, 0, 1, 1 };

}

void GPUState::Render(float dt)
{
    /*SDL_GPUTexture* backbuffer = SDL_AcquireGPUSwapchainTexture(
        _data->device,
        _data->window,
        nullptr, nullptr
    );

    if (!backbuffer)
        return;

    // Describe how the render pass uses the texture
    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = backbuffer;
    colorTarget.clear_color = { 0.1f, 0.1f, 0.1f, 1.0f }; // dark gray
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(
        _data->renderer.CommandBuffer(),   // must be valid
        &colorTarget,
        1,      // number of color targets
        nullptr // no depth target
    );
    _data->shaders.BindPipeline("basic", pass);

    // Create/upload vertex buffer (simple version)
    SDL_GPUBufferCreateInfo bufInfo{};
    bufInfo.size = sizeof(verts);
    bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;

    SDL_GPUBuffer* vbo = SDL_CreateGPUBuffer(_data->device, &bufInfo);

    // Upload
    SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(
        _data->device,
        &(SDL_GPUTransferBufferCreateInfo{
            sizeof(verts),
            SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD
            })
    );

    void* mapped = SDL_MapGPUTransferBuffer(_data->device, staging, true);
    memcpy(mapped, verts, sizeof(verts));
    SDL_UnmapGPUTransferBuffer(_data->device, staging);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_data->device);

    SDL_UploadToGPUBuffer(cmd,
        &(SDL_GPUBufferRegion{ vbo, 0, sizeof(verts) }),
        &(SDL_GPUTransferBufferRegion{ staging, 0, sizeof(verts) }),
        false
    );

    SDL_SubmitGPUCommandBuffer(cmd);

    // Bind + draw
    SDL_BindGPUVertexBuffers(pass, 0, &vbo, nullptr, 1);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

    SDL_EndGPURenderPass(pass);
    _data->shaders.UnbindPipeline();*/
}
