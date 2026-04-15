#pragma once
#include "BoxCollider.h"
#include "Transform.h"

namespace Utility
{

    static std::string GetBackendExtension(SDL_GPUDevice* device)
    {
        if (!device)
            return "";
        const char* driver = SDL_GetGPUDeviceDriver(device);
        if (!driver)
            return "";
        std::string d(driver);
        if (d == "vulkan")      return "spv";
        else if (d == "direct3d12")  return "dxil";
        else if (d == "metal")       return "msl";
        return "";
    }

    static std::string ExtractExtension(const std::string& filename)
    {
        auto pos = filename.rfind('.');
        if (pos == std::string::npos)
            return "";
        return filename.substr(pos + 1);
    }

    static std::vector<uint8_t> ReadFileBinary(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f)
            return {};
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> buf(size);
        f.read(reinterpret_cast<char*>(buf.data()), size);
        return buf;
    }

    //true if rectangle x1|y1|w1|h1 overlaps rectangle x2|y2|w2|h2
    inline bool Overlap(float x1, float y1, float w1, float h1,
        float x2, float y2, float w2, float h2)
    {
        if (x1 + w1 <= x2 || x2 + w2 <= x1)
            return false;

        if (y1 + h1 <= y2 || y2 + h2 <= y1)
            return false;
        return true;
    }

    //true if colliders overlap
    inline bool Overlap(const TransformComponent &t1, BoxCollider &col1, const TransformComponent &t2, const BoxCollider &col2)
    {
        return Overlap(t1.position.x + col1.offsetX * t1.scale.x - col1.hw * t1.scale.x,
            t1.position.y + col1.offsetY * t1.scale.y - col1.hh * t1.scale.y,
            col1.hw * 2.0f * t1.scale.x, col1.hh * 2.0f * t1.scale.y,
            t2.position.x + col2.offsetX * t2.scale.x - col2.hw * t2.scale.x,
            t2.position.y + col2.offsetY * t2.scale.y - col2.hh * t2.scale.y,
            col2.hw * 2.0f * t2.scale.x, col2.hh * 2.0f * t2.scale.y);
    }

    //true if rectangle x|y|w|h contains point px|py
    inline bool Contains(float px, float py, float x, float y, float w, float h) {
        return (px >= x &&
            px <= x + w &&
            py >= y &&
            py <= y + h);
    }

    //true if Collider contains point px|py
    inline bool Contains(float px, float py, const TransformComponent& t1, BoxCollider& col1) {
        return Contains(px, py, t1.position.x + col1.offsetX * t1.scale.x - col1.hw * t1.scale.x,
            t1.position.y + col1.offsetY * t1.scale.y - col1.hh * t1.scale.y,
            col1.hw * 2.0f * t1.scale.x, col1.hh * 2.0f * t1.scale.y);
    }

    inline bool Contains(float px, float py, TransformComponent *t1, BoxCollider *col1) {
        return Contains(px, py, t1->position.x + col1->offsetX * t1->scale.x - col1->hw * t1->scale.x,
            t1->position.y + col1->offsetY * t1->scale.y - col1->hh * t1->scale.y,
            col1->hw * 2.0f * t1->scale.x, col1->hh * 2.0f * t1->scale.y);
    }
}