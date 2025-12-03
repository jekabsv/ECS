#pragma once
#include <unordered_map>
#include "Struct.h"
#include <array>

struct ArrayHash {
    std::size_t operator()(const std::array<char, 16>& a) const 
{
        std::size_t h = 0;
        for (char c : a)
            h = h * 31 + static_cast<unsigned char>(c);
        return h;
    }
};

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	void AddMesh(const char* name, const Mesh& mesh);
    Mesh GetMesh(const char* name);
    Mesh GetMesh(std::array<char, 16> name);

private:
    std::unordered_map<std::array<char, 16>, Mesh, ArrayHash> _meshes;
	//textures, fonts, sounds, etc.
};