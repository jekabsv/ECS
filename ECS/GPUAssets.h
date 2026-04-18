#pragma once
#include "Struct.h"
#include <SDL3/SDL_gpu.h>
#include <string>


enum ShaderStage
{
	VERTEX,
	FRAGMENT,
	GEOMETRY,
	COMPUTE
};

enum class BlendMode 
{
	None,
	Alpha,
	Additive, 
	Multiply 
};

struct Matrix4
{
	float m[16];
	static Matrix4 Orthographic(float left, float right, float top, float bottom, float near, float far)
	{
		Matrix4 result{};
		result.m[0] = 2.0f / (right - left);

		// VULKAN/SDL_GPU FIX: 
		// We want 'top' to map to -1 and 'bottom' to map to 1.
		// Standard OpenGL is 2.0 / (top - bottom), we use the inverse:
		result.m[5] = 2.0f / (top - bottom);

		result.m[10] = -1.0f / (far - near); // Vulkan depth is 0 to 1, not -1 to 1

		result.m[12] = -(right + left) / (right - left);

		// VULKAN/SDL_GPU FIX:
		result.m[13] = -(top + bottom) / (top - bottom);

		result.m[14] = -near / (far - near);
		result.m[15] = 1.0f;
		return result;
	}
};

struct EngineData {
	Matrix4 projection;
	float time;
	float padding[3];
};

struct ObjectData {
	Matrix4 modelMatrix;
	float colorTint[4];
};


struct MeshBase
{
	StringId name;

	MeshVertices meshVertices;
	MeshIndices meshIndices;

	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;

	uint32_t vertexStride = 0;

	bool isLoaded = false;

	size_t size = 0;

	uint64_t lastUsed = 0;
};

struct Shader
{
	Shader() = default;
	Shader(SDL_GPUShader* shader) : gpuShader(shader) {}

	ShaderStage stage = VERTEX;
	SDL_GPUShader* gpuShader;

	std::string format = {};

	size_t UniformSize = 0;

	uint32_t numSamplers = 0;
	uint32_t numStorageTextures = 0;
	uint32_t numStorageBuffers = 0;
	uint32_t numUniformBuffers = 0;

	const char* entryPoint= "main";
};


struct TextureBase
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


struct MaterialBase
{
	MaterialBase() = default;
	MaterialBase(StringId vertShader, StringId fragShader) : vertexShader(vertShader), fragmentShader(fragShader) {
	}

	StringId vertexShader = "";
	StringId fragmentShader = "";

	SDL_GPUGraphicsPipeline* pipeline = nullptr;

	SDL_GPUTextureFormat colorTargetFormat = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

	bool hasDepthTarget = true;
	SDL_GPUTextureFormat depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;



	BlendMode blendMode = BlendMode::Alpha;

	std::vector<SDL_GPUVertexAttribute> attributes = {};
	SDL_GPUPrimitiveType primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_NONE;
	SDL_GPUFillMode fillMode = SDL_GPU_FILLMODE_FILL;

	bool depthTestEnabled = false;
	bool depthWriteEnabled = false;

	static void SetSDL_VertexAttr(MaterialBase& mat)
	{
		mat.attributes = {
		{ 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,  offsetof(SDL_Vertex, position)  },
		{ 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,  offsetof(SDL_Vertex, color)     },
		{ 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,  offsetof(SDL_Vertex, tex_coord) },
		};
	}
	static void ApplySpriteDefaults(MaterialBase& mat)
	{
		mat.cullMode = SDL_GPU_CULLMODE_NONE;
		mat.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		mat.fillMode = SDL_GPU_FILLMODE_FILL;
		mat.colorTargetFormat = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
		SDL_GPUTextureFormat depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

	}
	static void MakeSpriteTransparent(MaterialBase& mat)
	{
		ApplySpriteDefaults(mat);
		mat.blendMode = BlendMode::Alpha;
		mat.depthTestEnabled = false;
		mat.depthWriteEnabled = false;
	}
	static void MakeOpaque(MaterialBase& mat)
	{
		ApplySpriteDefaults(mat);
		mat.blendMode = BlendMode::None;
		mat.depthTestEnabled = false;
		mat.depthWriteEnabled = false;
	}
	static void MakeAdditive(MaterialBase& mat)
	{
		ApplySpriteDefaults(mat);
		mat.blendMode = BlendMode::Additive;
		mat.depthTestEnabled = false;
		mat.depthWriteEnabled = false;
	}
	static void MakeOverlay(MaterialBase& mat)
	{
		ApplySpriteDefaults(mat);
		mat.blendMode = BlendMode::Alpha;
		mat.depthTestEnabled = false;
		mat.depthWriteEnabled = false;
		mat.hasDepthTarget = false;
	}
};




struct MeshInstance 
{
	MeshInstance() = default;
	MeshInstance(StringId meshName) : meshName(meshName) {}

	StringId meshName;
	MeshBase* meshBase = nullptr;
};

struct MaterialInstance
{ 
	MaterialInstance() = default;
	MaterialInstance(StringId materialName) : materialName(materialName) {}

	StringId materialName = "";
	MaterialBase* materialBase = nullptr;

	uint8_t uniformVertBufferData[128] = { 0 };
	uint32_t vertBufferSize = 0;

	uint8_t uniformFragBufferData[128] = { 0 };
	uint32_t fragBufferSize = 0;

	StringId textures[8] = { "" };
	TextureBase *texturebases[8] = { nullptr };
	uint32_t textureCount = 0;

	void ClearUniforms() {
		vertBufferSize = 0;
		fragBufferSize = 0;
	}

	void AddTexture(StringId texture) {
		if (textureCount < 8) {
			textures[textureCount++] = texture;
		}
	}

	template<typename T>
	bool AddFragData(const T& data) {
		uint32_t size = sizeof(T);
		if (fragBufferSize + size > 128) {
			// Buffer Overflow
			return false;
		}

		memcpy(&uniformFragBufferData[fragBufferSize], &data, size);
		fragBufferSize += size;
		return true;
	}

	template<typename T>
	bool AddVertData(const T& data) {
		uint32_t size = sizeof(T);
		if (vertBufferSize + size > 128) {
			// Buffer Overflow
			return false;
		}

		memcpy(&uniformVertBufferData[vertBufferSize], &data, size);
		vertBufferSize += size;
		return true;
	}
};