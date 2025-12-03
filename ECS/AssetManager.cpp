#include "AssetManager.h"
#include <iostream>


void AssetManager::AddMesh(const char* name, const Mesh& mesh) 
{
    std::array<char, 16> key{};
    strncpy_s(key.data(), key.size(), name, _TRUNCATE);
    _meshes[key] = mesh;
}

Mesh AssetManager::GetMesh(const char* name)
{
    std::array<char, 16> key{};
    strncpy_s(key.data(), key.size(), name, _TRUNCATE);
    return _meshes[key];
}

Mesh AssetManager::GetMesh(std::array<char, 16> name)
{
    return _meshes[name];
    
}
