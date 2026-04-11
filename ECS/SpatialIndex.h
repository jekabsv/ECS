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

// Entities at or above this count use radix sort in Build(); below uses std::sort.
static constexpr std::size_t SPATIAL_RADIX_THRESHOLD = 2048;

// Hash-table size for the cell→entry-range lookup.
// Must be a power of two. Increase if your scene has many unique occupied cells.
// 1<<16 = 65 536 slots ≈ 256 KB — fits comfortably in L2.
static constexpr std::size_t SPATIAL_TABLE_SIZE = 1 << 16;
static constexpr std::size_t SPATIAL_TABLE_MASK = SPATIAL_TABLE_SIZE - 1;

// ---------------------------------------------------------------------------

class SpatialIndex
{
public:
    SpatialIndex()
    {
        m_table.assign(SPATIAL_TABLE_SIZE, EMPTY);
    }

    // -----------------------------------------------------------------------
    //  Public API  (unchanged from original)
    // -----------------------------------------------------------------------

    void Init(float x, float y, float w, float h, float cellSz = 64.f)
    {
        m_originX  = x;
        m_originY  = y;
        m_cellSize = (cellSz > 0.f ? cellSz : 1.f);
        m_invCell  = 1.f / m_cellSize;
        m_sorted   = false;
    }

    void Clear()
    {
        m_entries.clear();   // keeps heap allocation — intentional
        m_sorted = false;
        // Table is rebuilt in Build(); no need to clear it here.
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
        const std::size_t n = m_entries.size();

        // --- Sort: radix for large sets, std::sort for small ones ----------
        if (n >= SPATIAL_RADIX_THRESHOLD)
        {
            m_scratch.resize(n);
            radixPass(m_entries, m_scratch, 0);   // low  32 bits of key
            radixPass(m_scratch, m_entries, 32);  // high 32 bits of key
        }
        else
        {
            std::sort(m_entries.begin(), m_entries.end(),
                [](const Entry& a, const Entry& b) { return a.key < b.key; });
        }

        // --- Build open-addressing hash table: key → first entry index -----
        //
        // We only store the index of the first Entry for each unique key.
        // Because m_entries is sorted by key, all entries sharing a key are
        // contiguous, so iterating forward from that index covers them all.
        //
        // The table is cleared by writing INVALID_KEY sentinels into only the
        // slots that were previously occupied — O(unique cells) not O(table).

        // Invalidate only previously used slots (avoids full table memset).
        for (std::uint32_t slot : m_usedSlots)
            m_table[slot] = EMPTY;
        m_usedSlots.clear();

        for (std::size_t i = 0; i < n; )
        {
            const std::uint64_t key  = m_entries[i].key;
            std::uint32_t       slot = static_cast<std::uint32_t>(key & SPATIAL_TABLE_MASK);

            // Linear probe: skip slots that belong to a *different* key.
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
            // Advance past all entries with this key.
            while (i < n && m_entries[i].key == key) ++i;
        }

        m_sorted = true;
    }

    std::size_t QueryRectangle(float x, float y, float w, float h,
        std::vector<ECS::Entity>& out) const
    {
        int minCX, minCY, maxCX, maxCY;
        rectToCells(x, y, w, h, minCX, minCY, maxCX, maxCY);
        const std::size_t before = out.size();
        for (int cy = minCY; cy <= maxCY; ++cy)
            for (int cx = minCX; cx <= maxCX; ++cx)
                collectCell(morton(cx, cy), out);
        auto mid = out.begin() + before;
        std::sort(mid, out.end());
        out.erase(std::unique(mid, out.end()), out.end());
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
        const std::size_t before = out.size();
        collectCell(morton(cx, cy), out);
        // single cell — no duplicates possible, no sort needed
        return out.size() - before;
    }

    // Optional: pre-warm the entry buffer capacity.
    // Call once after Init() if you know the approximate entry count upfront.
    void Reserve(std::size_t entryCount)
    {
        m_entries.reserve(entryCount);
        m_scratch.reserve(entryCount);
        m_usedSlots.reserve(std::min(entryCount, SPATIAL_TABLE_SIZE));
    }

private:
    // -----------------------------------------------------------------------
    //  Types
    // -----------------------------------------------------------------------

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
        minCX = worldToCell(x       - m_originX);
        minCY = worldToCell(y       - m_originY);
        maxCX = worldToCell(x + w   - m_originX);
        maxCY = worldToCell(y + h   - m_originY);
    }

    // -----------------------------------------------------------------------
    //  Morton encoding
    // -----------------------------------------------------------------------

    static std::uint64_t spreadBits(std::uint32_t v)
    {
        std::uint64_t x = v;
        x = (x | (x << 16)) & 0x0000'FFFF'0000'FFFFull;
        x = (x | (x <<  8)) & 0x00FF'00FF'00FF'00FFull;
        x = (x | (x <<  4)) & 0x0F0F'0F0F'0F0F'0F0Full;
        x = (x | (x <<  2)) & 0x3333'3333'3333'3333ull;
        x = (x | (x <<  1)) & 0x5555'5555'5555'5555ull;
        return x;
    }

    static std::uint64_t morton(int cx, int cy)
    {
        return  spreadBits(static_cast<std::uint32_t>(cx))
             | (spreadBits(static_cast<std::uint32_t>(cy)) << 1);
    }

    // -----------------------------------------------------------------------
    //  Radix sort  (2-pass, 8-bit buckets, stable, O(n))
    //
    //  Two passes cover 16 bits — sufficient when Morton keys fit in 32 bits
    //  (i.e. world ≤ ~65 535 cells per axis at the chosen cell size).
    //  Add a third/fourth pass (shifts 16, 48) for larger worlds.
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

    // Finds all entries for a given Morton key and appends unique entities
    // to `out` using the generation-stamp visited set (no sort/unique needed).
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
                    out.push_back(m_entries[i].entity);
                }
                return;
            }
            slot = static_cast<std::uint32_t>((slot + 1u) & SPATIAL_TABLE_MASK);
        }
    }
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------

    float m_originX  = 0.f;
    float m_originY  = 0.f;
    float m_cellSize = 1.f;
    float m_invCell  = 1.f;
    bool  m_sorted   = false;

    std::vector<Entry>         m_entries;   // sorted after Build()
    std::vector<Entry>         m_scratch;   // radix sort ping-pong buffer

    // Hash table: slot → index of first Entry in m_entries with that key.
    std::vector<std::uint32_t> m_table;
    std::vector<std::uint32_t> m_usedSlots; // tracks which slots need clearing
};