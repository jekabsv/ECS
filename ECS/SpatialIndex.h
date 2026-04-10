#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <vector>
#include <algorithm>
#include "ECS.H"

class SpatialIndex
{
public:
    SpatialIndex() = default;

    void Init(float x, float y, float w, float h, float cellSz = 64.f)
    {
        m_originX = x;
        m_originY = y;
        m_cellSize = (cellSz > 0.f ? cellSz : 1.f);
        m_invCell = 1.f / m_cellSize;
        m_sorted = false;
    }

    void Clear()
    {
        m_entries.clear();
        m_sorted = false;
    }

    void InsertRectangle(ECS::Entity e, float x, float y, float w, float h)
    {
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);
        for (int cy = minCY; cy <= maxCY; ++cy)
            for (int cx = minCX; cx <= maxCX; ++cx)
                m_entries.push_back({ morton(cx, cy), e });
        m_sorted = false;
    }

    void InsertCircle(ECS::Entity e, float x, float y, float r)
    {
        InsertRectangle(e, x - r, y - r, r * 2.f, r * 2.f);
    }

    // Must be called after all inserts, before any queries.
    void Build()
    {
        std::sort(m_entries.begin(), m_entries.end(),
            [](const Entry& a, const Entry& b) { return a.key < b.key; });
        m_sorted = true;
    }

    std::size_t QueryRectangle(float x, float y, float w, float h,
        std::vector<ECS::Entity>& out) const
    {
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);
        const std::size_t before = out.size();
        for (int cy = minCY; cy <= maxCY; ++cy)
        {
            for (int cx = minCX; cx <= maxCX; ++cx)
            {
                const std::uint64_t key = morton(cx, cy);
                auto lo = std::lower_bound(m_entries.begin(), m_entries.end(), key,
                    [](const Entry& e, std::uint64_t k) { return e.key < k; });
                for (auto it = lo; it != m_entries.end() && it->key == key; ++it)
                    out.push_back(it->entity);
            }
        }
        deduplicate(out, before);
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
        const std::uint64_t key = morton(cx, cy);
        auto lo = std::lower_bound(m_entries.begin(), m_entries.end(), key,
            [](const Entry& e, std::uint64_t k) { return e.key < k; });
        const std::size_t before = out.size();
        for (auto it = lo; it != m_entries.end() && it->key == key; ++it)
            out.push_back(it->entity);
        return out.size() - before;
    }

private:
    struct Entry
    {
        std::uint64_t key;
        ECS::Entity   entity;
    };

    int worldToCell(float localCoord) const
    {
        return static_cast<int>(std::floor(localCoord * m_invCell));
    }

    void rectToCells(float x, float y, float w, float h,
        int& minCX, int& minCY, int& maxCX, int& maxCY) const
    {
        minCX = worldToCell(x - m_originX);
        minCY = worldToCell(y - m_originY);
        maxCX = worldToCell(x + w - m_originX);
        maxCY = worldToCell(y + h - m_originY);
    }

    // Spreads bits of a 32-bit int into even positions for Morton interleaving.
    static std::uint64_t spreadBits(std::uint32_t v)
    {
        std::uint64_t x = v;
        x = (x | (x << 16)) & 0x0000'FFFF'0000'FFFFull;
        x = (x | (x << 8)) & 0x00FF'00FF'00FF'00FFull;
        x = (x | (x << 4)) & 0x0F0F'0F0F'0F0F'0F0Full;
        x = (x | (x << 2)) & 0x3333'3333'3333'3333ull;
        x = (x | (x << 1)) & 0x5555'5555'5555'5555ull;
        return x;
    }

    static std::uint64_t morton(int cx, int cy)
    {
        return (spreadBits(static_cast<std::uint32_t>(cx)))
            | (spreadBits(static_cast<std::uint32_t>(cy)) << 1);
    }

    static void deduplicate(std::vector<ECS::Entity>& out, std::size_t from)
    {
        std::sort(out.begin() + from, out.end());
        out.erase(std::unique(out.begin() + from, out.end()), out.end());
    }

    float m_originX = 0.f;
    float m_originY = 0.f;
    float m_cellSize = 1.f;
    float m_invCell = 1.f;
    bool  m_sorted = false;

    std::vector<Entry> m_entries;
};