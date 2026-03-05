#pragma once
#include "Struct.h"
#include <vector>
#include <array>

struct RenderComponent
{
     RenderComponent(bool _render)
         : render(_render) { };
     RenderComponent(bool _render, Vec2 pos)
         : render(_render), position(pos) { };
     RenderComponent(bool _render, Vec2 pos, Vec2 _scale)
         : render(_render), position(pos), scale(_scale) { };
    RenderComponent() = default;
    ~RenderComponent() = default;

    bool render = false;
    Vec2 position = { 0.0f, 0.0f };
    Vec2 scale = { 1.0f, 1.0f };  
};