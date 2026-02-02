#pragma once
#include <SDL3\SDL.h>
#include <array>


struct Vec2
{
    float x, y;
};

struct Vec3
{
    float x, y, z;
};

using Triangle = std::array<SDL_Vertex, 3>;

using Mesh = std::vector<SDL_Vertex>;
