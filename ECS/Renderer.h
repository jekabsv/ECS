#pragma once
#include "GPUAssets.h"
#include "AssetManager.h"


class Renderer
{
public:

	int Init(SDL_GPUDevice* gpuDevice, SDL_Window* sdlWindow, AssetManager* assets, uint32_t screenWidth, uint32_t screenHeight);
	int Shutdown();

	int SetWindowSize(uint32_t width, uint32_t height);

	int StartRenderPass();
	int EndRenderPass();
	int Present();

	int SubmitMesh(MeshInstance mesh, MaterialInstance& material,
		Vec3 Position = { 0.0f, 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f,
		SDL_FColor colorTint = { 1.0f, 1.0f, 1.0f, 1.0f });

	int SubmitMesh(MeshInstance mesh, MaterialInstance&& material,
		Vec3 Position = { 0.0f, 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f,
		SDL_FColor colorTint = { 1.0f, 1.0f, 1.0f, 1.0f });

	int SubmitSprite(MaterialInstance& material, SDL_FRect sRect, 
		Vec3 Position = { 0.f, 0.f, 0.f }, Vec2 Scale = { 1.f, 1.f }, float Rotation = 0.f,
		SDL_FColor colorTint = { 1.0f, 1.0f, 1.0f, 1.0f });

	int DrawMesh(MeshInstance& mesh, MaterialInstance& material,
		Vec2 Position = { 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f, 
		SDL_FColor colorTint = {1.0f, 1.0f, 1.0f, 1.0f});

	int DrawMesh(StringId mesh, MaterialInstance& material,
		Vec2 Position = { 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f,
		SDL_FColor colorTint = { 1.0f, 1.0f, 1.0f, 1.0f });


	int SpriteDraw(MaterialInstance& material, SDL_FRect sRect,
		Vec2 Position = { 0.0f, 0.0f }, Vec2 Scale = { 1.0f, 1.0f }, float Rotation = 0.0f,
		SDL_FColor colorTint = { 1.0f, 1.0f, 1.0f, 1.0f });

	int SetProjection(const Matrix4& projection);

	TextureBase CreateTexture(SDL_Surface* surface);

private:


	//ModelMatrix(64) + colorTint(16) + uniformVert(128) + uniformFrag(128) = 336 bytes

	std::vector<DrawCall> drawCalls;
	std::vector<RenderBatch> batches;

	size_t drawCallSize;
	size_t batchesSize;

	SDL_GPUBuffer* _instanceBuffer = nullptr;
	SDL_GPUTransferBuffer* _instanceTransferBuffer = nullptr;
	size_t _instanceBufferCapacity = 0;

	int ReserveInstanceBuffer(size_t requiredBytes);
	void BuildBatches();
	void Flush();

	uint64_t BatchKey(const DrawCall& dc) const;


	uint32_t _screenWidth, _screenHeight;

	AssetManager* _assets = nullptr;
	SDL_GPUDevice* _device = nullptr;
	SDL_Window* _window = nullptr;

	SDL_GPUCommandBuffer* currentCmd = nullptr;
	SDL_GPURenderPass* currentPass = nullptr;
	SDL_GPUTexture* currentSwapchainTexture = nullptr;

	SDL_GPUTransferBuffer* tBuf = nullptr;
	size_t transferBufferSize = 0;

	SDL_GPUTexture* _depthStencilTexture = nullptr;

	EngineData _engineData;

	MeshBase _unitQuadMesh;

	SDL_GPUGraphicsPipeline* GetOrCreatePipeline(MaterialBase* base);

	StringId currentPipeline = "";
	StringId currentMesh = "";

	void BindMaterialTextures(MaterialInstance material);

	int ReserveTransferBuffer(size_t size);

	void CreateUnitQuad();

	void UpdateDefaultProjection();

	void UploadMesh(MeshBase* mesh);

	template<typename T>
	void PushConstants(const T& data, uint32_t offsetVert = 0, uint32_t offsetFrag = 0);

	ObjectData BuildObjectData(Vec3 Position, Vec2 Scale, float Rotation, SDL_FColor colorTint);
};

template<typename T>
inline void Renderer::PushConstants(const T& data, uint32_t offsetVert, uint32_t offsetFrag)
{
	if (offsetVert + sizeof(T) <= 256)
		SDL_PushGPUUniformBuffer(currentCmd, currentPass, SDL_GPU_SHADERSTAGE_VERTEX, &data, sizeof(T), offsetVert);
	if (offsetFrag + sizeof(T) <= 256)
		SDL_PushGPUUniformBuffer(currentCmd, currentPass, SDL_GPU_SHADERSTAGE_FRAGMENT, &data, sizeof(T), offsetFrag);
}