#pragma once
#include "Struct.h"
#include <vector>
#include <array>

struct RenderComponent
{
     RenderComponent(bool _render)
         : render(_render){ };
    RenderComponent() = default;
    ~RenderComponent() = default;

    bool render = false;
    Vec2 Position = { 0.0f, 0.0f };
    Vec2 scale = { 1.0f, 1.0f };  
};