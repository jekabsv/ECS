#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <vector>
#include <algorithm>
#include "ECS.H"

// ---------------------------------------------------------------------------
//  Tuning knobs
// ---------------------------------------------------------------------------

static constexpr std::size_t SPATIAL_RADIX_THRESHOLD = 2048;

static constexpr std::size_t SPATIAL_TABLE_SIZE = 1 << 16;
static constexpr std::size_t SPATIAL_TABLE_MASK = SPATIAL_TABLE_SIZE - 1;

// Max number of unique entity IDs (upper 32 bits of Entity handle).
// 1<<17 = 131 072 unique entities.
static constexpr std::size_t SPATIAL_MAX_ENTITY_ID = 1 << 17;

// ---------------------------------------------------------------------------

class SpatialIndex
{
public:
    SpatialIndex()
    {
        m_table.assign(SPATIAL_TABLE_SIZE, EMPTY);
        m_visited.assign(SPATIAL_MAX_ENTITY_ID, 0u);
    }

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

    void Build()
    {
        const std::size_t n = m_entries.size();

        if (n >= SPATIAL_RADIX_THRESHOLD)
        {
            m_scratch.resize(n);
            radixPass(m_entries, m_scratch, 0);
            radixPass(m_scratch, m_entries, 32);
        }
        else
        {
            std::sort(m_entries.begin(), m_entries.end(),
                [](const Entry& a, const Entry& b) { return a.key < b.key; });
        }

        for (std::uint32_t slot : m_usedSlots)
            m_table[slot] = EMPTY;
        m_usedSlots.clear();

        for (std::size_t i = 0; i < n; )
        {
            const std::uint64_t key = m_entries[i].key;
            std::uint32_t       slot = static_cast<std::uint32_t>(key & SPATIAL_TABLE_MASK);

            while (m_table[slot] != EMPTY &&
                m_entries[m_table[slot]].key != key)
            {
                slot = static_cast<std::uint32_t>((slot + 1u) & SPATIAL_TABLE_MASK);
            }

            if (m_table[slot] == EMPTY)
            {
                m_table[slot] = static_cast<std::uint32_t>(i);
                m_usedSlots.push_back(slot);
            }
            while (i < n && m_entries[i].key == key) ++i;
        }

        m_sorted = true;
    }

    std::size_t QueryRectangle(float x, float y, float w, float h,
        std::vector<ECS::Entity>& out) const
    {
        beginQuery();
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);
        const std::size_t before = out.size();
        for (int cy = minCY; cy <= maxCY; ++cy)
            for (int cx = minCX; cx <= maxCX; ++cx)
                collectCell(morton(cx, cy), out);
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
        beginQuery();
        const int cx = worldToCell(x - m_originX);
        const int cy = worldToCell(y - m_originY);
        const std::size_t before = out.size();
        collectCell(morton(cx, cy), out);
        return out.size() - before;
    }

    void Reserve(std::size_t entryCount)
    {
        m_entries.reserve(entryCount);
        m_scratch.reserve(entryCount);
        m_usedSlots.reserve(std::min(entryCount, SPATIAL_TABLE_SIZE));
    }

private:
    struct Entry
    {
        std::uint64_t key;
        ECS::Entity   entity;
    };

    static constexpr std::uint32_t EMPTY = ~std::uint32_t(0);

    // -----------------------------------------------------------------------
    //  Coordinate helpers
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    //  Morton encoding
    // -----------------------------------------------------------------------

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
        return  spreadBits(static_cast<std::uint32_t>(cx))
            | (spreadBits(static_cast<std::uint32_t>(cy)) << 1);
    }

    // -----------------------------------------------------------------------
    //  Radix sort
    // -----------------------------------------------------------------------

    static void radixPass(std::vector<Entry>& src,
        std::vector<Entry>& dst,
        int                 shift)
    {
        constexpr int BUCKETS = 256;
        std::size_t count[BUCKETS] = {};

        for (const auto& e : src)
            ++count[(e.key >> shift) & 0xFF];

        std::size_t offset[BUCKETS];
        offset[0] = 0;
        for (int i = 1; i < BUCKETS; ++i)
            offset[i] = offset[i - 1] + count[i - 1];

        for (const auto& e : src)
            dst[offset[(e.key >> shift) & 0xFF]++] = e;
    }

    // -----------------------------------------------------------------------
    //  Query helpers
    // -----------------------------------------------------------------------

    // Increment generation at the start of each query so each query
    // gets a fresh visited set without clearing the whole array.
    void beginQuery() const
    {
        ++m_generation;
        if (m_generation == 0u)
        {
            std::fill(m_visited.begin(), m_visited.end(), 0u);
            m_generation = 1u;
        }
    }

    void collectCell(std::uint64_t key, std::vector<ECS::Entity>& out) const
    {
        std::uint32_t slot = static_cast<std::uint32_t>(key & SPATIAL_TABLE_MASK);

        while (m_table[slot] != EMPTY)
        {
            const std::size_t idx = m_table[slot];
            if (m_entries[idx].key == key)
            {
                for (std::size_t i = idx;
                    i < m_entries.size() && m_entries[i].key == key;
                    ++i)
                {
                    const ECS::Entity e = m_entries[i].entity;
                    const std::uint32_t id = ECS::EntityId(e);

                    if (id < SPATIAL_MAX_ENTITY_ID)
                    {
                        if (m_visited[id] == m_generation) continue;
                        m_visited[id] = m_generation;
                    }
                    out.push_back(e);
                }
                return;
            }
            slot = static_cast<std::uint32_t>((slot + 1u) & SPATIAL_TABLE_MASK);
        }
    }

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------

    float m_originX = 0.f;
    float m_originY = 0.f;
    float m_cellSize = 1.f;
    float m_invCell = 1.f;
    bool  m_sorted = false;

    std::vector<Entry>         m_entries;
    std::vector<Entry>         m_scratch;

    std::vector<std::uint32_t> m_table;
    std::vector<std::uint32_t> m_usedSlots;

    mutable std::vector<std::uint32_t> m_visited;
    mutable std::uint32_t              m_generation = 1u;
};