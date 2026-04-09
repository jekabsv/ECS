#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "ECS.H"

class SpatialIndex
{
public:

    SpatialIndex() = default;

    void Init(float x, float y, float w, float h, float cellSz = 64.f)
    {
        m_originX = x;
        m_originY = y;
        m_worldW = w;
        m_worldH = h;
        m_cellSize = (cellSz > 0.f ? cellSz : 1.f);
        m_invCell = 1.f / m_cellSize;

        const std::size_t cols = static_cast<std::size_t>(std::ceil(w * m_invCell)) + 1;
        const std::size_t rows = static_cast<std::size_t>(std::ceil(h * m_invCell)) + 1;
        m_cells.reserve(cols * rows);

        m_cells.clear();
    }
    void Clear()
    {
        m_cells.clear();
    }

    void InsertRectangle(ECS::Entity e, float x, float y, float w, float h)
    {
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);

        for (int cy = minCY; cy <= maxCY; ++cy)
            for (int cx = minCX; cx <= maxCX; ++cx)
                m_cells[cellKey(cx, cy)].push_back(e);
    }

    void InsertCircle(ECS::Entity e, float x, float y, float r)
    {
        InsertRectangle(e, x - r, y - r, r * 2.f, r * 2.f);
    }

    std::size_t QueryRectangle(float x, float y, float w, float h,
        std::vector<ECS::Entity>& out) const
    {
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);

        const std::size_t before = out.size();
        std::unordered_set<ECS::Entity> seen;

        for (int cy = minCY; cy <= maxCY; ++cy)
        {
            for (int cx = minCX; cx <= maxCX; ++cx)
            {
                auto it = m_cells.find(cellKey(cx, cy));
                if (it == m_cells.end()) continue;

                for (ECS::Entity e : it->second)
                    if (seen.insert(e).second)
                        out.push_back(e);
            }
        }

        return out.size() - before;
    }

    std::size_t QueryCircle(float x, float y, float r,
        std::vector<ECS::Entity>& out) const
    {
        return QueryRectangle(x - r, y - r, r * 2.f, r * 2.f, out);
    }

    std::size_t QueryPoint(float x, float y,
        std::vector<ECS::Entity>& out) const
    {
        const int cx = worldToCell(x - m_originX);
        const int cy = worldToCell(y - m_originY);

        auto it = m_cells.find(cellKey(cx, cy));
        if (it == m_cells.end()) return 0;

        const std::size_t before = out.size();
        std::unordered_set<ECS::Entity> seen;

        for (ECS::Entity e : it->second)
            if (seen.insert(e).second)
                out.push_back(e);

        return out.size() - before;
    }

private:
    int worldToCell(float localCoord) const
    {
        return static_cast<int>(std::floor(localCoord * m_invCell));
    }


    static std::uint64_t cellKey(int cx, int cy)
    {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32)
            | static_cast<std::uint64_t>(static_cast<std::uint32_t>(cy));
    }

    void rectToCells(float x, float y, float w, float h,
        int& minCX, int& minCY,
        int& maxCX, int& maxCY) const
    {
        minCX = worldToCell(x - m_originX);
        minCY = worldToCell(y - m_originY);
        maxCX = worldToCell(x + w - m_originX);
        maxCY = worldToCell(y + h - m_originY);
    }


    float m_originX = 0.f;
    float m_originY = 0.f;
    float m_worldW = 0.f;
    float m_worldH = 0.f;
    float m_cellSize = 1.f;
    float m_invCell = 1.f;

    std::unordered_map<std::uint64_t, std::vector<ECS::Entity>> m_cells;
};