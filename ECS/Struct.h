#pragma once

#include <array>
#include <string_view>
#include <vector>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

struct SDL_Vertex;
struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;

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

struct StringId
{
    uint32_t id = 0;

    StringId() = default;
    constexpr StringId(const char* str) : StringId(std::string_view(str)) {}
    constexpr StringId(std::string_view str)
    {
        uint32_t h = 2166136261u;
        for (char c : str) 
            h = (h ^ (uint8_t)c) * 16777619u;
        id = h;
    }

    constexpr bool operator==(const StringId& o) const { return id == o.id; }
    constexpr bool operator!=(const StringId& o) const { return id != o.id; }
};

template<>
struct std::hash<StringId>
{
    size_t operator()(const StringId& s) const noexcept { return s.id; }
};


class Renderer
{
public:
    SDL_Renderer* SDLrenderer = nullptr;
    Renderer() = default;
    Renderer(SDL_Renderer* _SDLrenderer) : SDLrenderer(_SDLrenderer) {};
    int Render(const SDL_Texture* texture, const SDL_FRect* srect, const SDL_FRect* dsrect) const;
};