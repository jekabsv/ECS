#pragma once
#include "Struct.h"
#include <vector>
#include <array>

struct RenderComponent
{
     RenderComponent(bool _render = false, Vec2 _pos = {0.0f, 0.0f}, Vec2 _scale = {1.0f, 1.0f})
         : render(_render), position(_pos), scale(_scale) { };
    RenderComponent() = default;
    ~RenderComponent() = default;
    bool render;
    Vec2 position;
    Vec2 scale;  
};