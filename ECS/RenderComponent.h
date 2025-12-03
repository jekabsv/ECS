#pragma once
#include "Struct.h"
#include <vector>
#include <array>

class RenderComponent
{
public:
     RenderComponent(bool _render, const char* _mesh)
         : render(_render){
         strncpy_s(mesh.data(), mesh.size(), _mesh, _TRUNCATE);
     };
    RenderComponent() = default;
    ~RenderComponent() = default;

    bool render = false;
    std::array<char, 16> mesh;
    Vec2 Position = { 0.0f, 0.0f };
    Vec2 scale = { 1.0f, 1.0f };
};