#pragma once
#include <cstddef>
#include "Struct.h"
#include "GpuAssets.h"

inline void hash_combine(std::size_t& seed, std::size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


struct Material
{
    StringId vertShaderId;
    StringId fragShaderId;
    bool blendMode = false;

    std::vector<uint8_t> uniformVertBufferData;
    std::vector<uint8_t> uniformFragBufferData;

    std::vector<StringId> textureBindings;

    void setData(ShaderStage stage, size_t offset, const void* data, size_t size) {
        std::vector<uint8_t>& targetBuffer = (stage == ShaderStage::Vertex)
            ? uniformVertBufferData
            : uniformFragBufferData;

        if (targetBuffer.size() < offset + size) {
            targetBuffer.resize(offset + size);
        }

        memcpy(targetBuffer.data() + offset, data, size);
    }

    size_t getPipelineHash() const {
        size_t h = 0;
        hash_combine(h, std::hash<uint64_t>{}(vertShaderId.id));
        hash_combine(h, std::hash<uint64_t>{}(fragShaderId.id));
        hash_combine(h, std::hash<bool>{}(blendMode));
        return h;
    }
};


