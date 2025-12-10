#pragma once



struct MeshComponent
{
	MeshComponent(int _meshId, int _TextureId) 
		: meshId(_meshId), TextureId(_TextureId) {};
	int meshId = -1;
	int TextureId = -1;
};