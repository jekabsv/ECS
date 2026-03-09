#pragma once
#include <cstddef>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <string_view>
#include <cassert>
#include <cstring>
#include "Struct.h"

struct SharedData;
typedef std::shared_ptr<SharedData> SharedDataRef;

namespace ECS
{
    using Entity = uint64_t;
    using ComponentId = uint32_t;
    using ComponentMask = uint64_t;

    constexpr std::size_t CHUNK_SIZE = 16 * 1024;

    constexpr uint32_t EntityId(Entity e) { return (uint32_t)(e >> 32); }
    constexpr uint32_t EntityGeneration(Entity e) { return (uint32_t)(e & 0xFFFFFFFF); }
    constexpr Entity   MakeEntity(uint32_t id, uint32_t gen) { return ((uint64_t)id << 32) | gen; }

    inline ComponentId AllocComponentId() noexcept
    {
        static ComponentId counter = 0;
        return counter++;
    }

    template<typename T>
    ComponentId GetComponentId() noexcept
    {
        static const ComponentId id = AllocComponentId();
        return id;
    }

    template<typename T>
    ComponentMask ComponentBit() noexcept
    {
        return ComponentMask(1) << GetComponentId<T>();
    }

    template<typename... Ts>
    ComponentMask MakeMask() noexcept
    {
        return (ComponentBit<Ts>() | ... | ComponentMask(0));
    }

    inline ComponentMask MakeMaskFromId(ComponentId id) noexcept
    {
        return ComponentMask(1) << id;
    }


    struct ColumnDesc
    {
        ComponentId id;
        std::size_t size;
        std::size_t offset;
    };
    struct ComponentInfo
    {
        ComponentId id;
        std::size_t size;
    };

    class Chunk
    {
    public:
        alignas(64) std::byte buffer[CHUNK_SIZE];
        const std::vector<ColumnDesc>* cols = nullptr;
        std::size_t capacity = 0;
        std::size_t count = 0;

        Entity* Entities();
        Entity& EntityAt(std::size_t row);
        void SetEntity(std::size_t row, Entity e);

        void* ColumnPtr(const ColumnDesc& col);
        void* RowPtr(const ColumnDesc& col, std::size_t row);
        const void* RowPtr(const ColumnDesc& col, std::size_t row) const;

        void CopyRow(std::size_t src_row, Chunk& dst, std::size_t dst_row, const std::vector<ColumnDesc>& descs);
        void MoveLastRowTo(std::size_t dst_row, Chunk& dst, const std::vector<ColumnDesc>& descs);

        template<typename T>
        T* GetArray(ComponentId id)
        {
            for (auto& col : *cols)
                if (col.id == id)
                    return reinterpret_cast<T*>(buffer + col.offset);
            return nullptr;
        }

        template<typename T>
        T& Get(std::size_t row)
        {
            return *GetArray<T>(GetComponentId<T>());
        }

        bool Full() const noexcept;
        bool Empty() const noexcept;
    };
    class Archetype
    {
    public:
        ComponentMask mask;
        std::vector<ColumnDesc> descs;
        std::size_t chunk_capacity = 0;
        std::vector<Chunk> chunks;

        std::unordered_map<ComponentId, Archetype*> add_edge;
        std::unordered_map<ComponentId, Archetype*> remove_edge;

        Archetype(ComponentMask mask, const std::vector<ComponentInfo>& components);

        const ColumnDesc* GetDesc(ComponentId id) const;
        bool HasComponent(ComponentId id) const;
        bool HasComponents(ComponentMask m) const;

        void* GetPtr(std::size_t chunk_idx, std::size_t row, ComponentId id);
        Entity& GetEntity(std::size_t chunk_idx, std::size_t row);
        void Write(std::size_t chunk_idx, std::size_t row, ComponentId id, const void* src);

        template<typename T>
        T& Get(std::size_t chunk_idx, std::size_t row)
        {
            return *static_cast<T*>(GetPtr(chunk_idx, row, GetComponentId<T>()));
        }

        template<typename T>
        void Set(std::size_t chunk_idx, std::size_t row, const T& val)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            Write(chunk_idx, row, GetComponentId<T>(), &val);
        }

        Chunk& GetOrAddChunk();
        std::size_t ChunkCount() const noexcept;
        std::size_t EntityCount() const noexcept;
        std::pair<std::size_t, std::size_t> LastSlot() const;

        std::pair<std::size_t, std::size_t> AllocateSlot();
        Entity SwapRemove(std::size_t chunk_idx, std::size_t row);
        std::pair<std::size_t, std::size_t> MigrateEntityTo(std::size_t chunk_idx, std::size_t row, Archetype& dst);

        void ForEachChunk(const std::function<void(Chunk&, std::size_t)>& fn);
        void ForEachEntity(const std::function<void(Entity, std::size_t, std::size_t)>& fn);
    };


    enum class SystemGroup { Update, Render };

    struct EntityRecord
    {
        Archetype* arch = nullptr;
        std::size_t chunk_idx = 0;
        std::size_t row = 0;
        uint32_t generation = 0;
    };
    struct ComponentContext
    {
        Archetype* arch = nullptr;
        std::size_t chunk_idx = 0;
        std::size_t row = 0;

        template<typename T>
        T& Get() { return arch->Get<T>(chunk_idx, row); }

        template<typename T>
        T* TryGet()
        {
            if (!arch->HasComponent(GetComponentId<T>())) return nullptr;
            return &arch->Get<T>(chunk_idx, row);
        }

        template<typename T>
        bool Has() { return arch->HasComponent(GetComponentId<T>()); }
    };
    using SystemFn = std::function<void(Entity, ComponentContext&, float, SharedDataRef)>;
    struct SystemEntry
    {
        StringId name;
        ComponentMask mask;
        SystemFn fn;
        bool enabled = true;
    };


    class World
    {
        std::vector<EntityRecord> records_;
        std::vector<uint32_t> free_;
        uint32_t next_ = 0;

        std::unordered_map<ComponentMask, std::unique_ptr<Archetype>> archetypes_;

        std::vector<SystemEntry> update_systems_;
        std::vector<SystemEntry> render_systems_;

        Archetype& GetOrCreateArchetype(ComponentMask mask, const std::vector<ComponentInfo>& info);
        Archetype& GetOrCreateEdge(Archetype& arch, ComponentId id, std::size_t size, bool adding);
        void GrowRecords(uint32_t id);
        void RunSystems(std::vector<SystemEntry>& systems, float dt);

        SharedDataRef _data;

    public:



        World();

        Entity Create();
        void Destroy(Entity e);
        bool Alive(Entity e) const noexcept;

        void Tie(SharedDataRef data) {
            _data = data;
        }

        template<typename T>
        void Add(Entity e, T value = T{});

        template<typename T1, typename T2, typename... Ts>
        void Add(Entity e, T1 v1, T2 v2, Ts... values)
        {
            Add<T1>(e, v1);
            Add<T2>(e, v2);
            (Add<Ts>(e, values), ...);
        }

        template<typename T>
        void Remove(Entity e);

        template<typename T1, typename T2, typename... Ts>
        void Remove(Entity e)
        {
            Remove<T1>(e);
            Remove<T2>(e);
            (Remove<Ts>(e), ...);
        }

        template<typename T>
        T& Get(Entity e);

        template<typename T>
        T* TryGet(Entity e) noexcept;

        template<typename T>
        bool Has(Entity e) const noexcept;

        template<typename... Ts>
        void RegisterSystem(StringId name, SystemFn fn, SystemGroup group = SystemGroup::Update);

        void EnableSystem(StringId name);
        void DisableSystem(StringId name);

        void Run(SystemGroup group, float dt);
    };

    template<typename T>
    void World::Add(Entity e, T value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        assert(Alive(e));

        uint32_t eid = EntityId(e);
        auto& rec = records_[eid];
        ComponentId id = GetComponentId<T>();

        if (rec.arch->HasComponent(id))
        {
            rec.arch->Set<T>(rec.chunk_idx, rec.row, value);
            return;
        }

        Archetype& dst = GetOrCreateEdge(*rec.arch, id, sizeof(T), true);
        auto [dst_ci, dst_row] = rec.arch->MigrateEntityTo(rec.chunk_idx, rec.row, dst);
        dst.GetEntity(dst_ci, dst_row) = e;
        dst.Set<T>(dst_ci, dst_row, value);

        Entity moved = rec.arch->SwapRemove(rec.chunk_idx, rec.row);
        if (moved != e)
        {
            uint32_t moved_id = EntityId(moved);
            records_[moved_id].arch = rec.arch;
            records_[moved_id].chunk_idx = rec.chunk_idx;
            records_[moved_id].row = rec.row;
        }

        records_[eid] = { &dst, dst_ci, dst_row, records_[eid].generation };
    }

    template<typename T>
    void World::Remove(Entity e)
    {
        assert(Alive(e));
        uint32_t eid = EntityId(e);
        auto& rec = records_[eid];
        ComponentId id = GetComponentId<T>();
        if (!rec.arch->HasComponent(id)) return;

        Archetype& dst = GetOrCreateEdge(*rec.arch, id, 0, false);
        auto [dst_ci, dst_row] = rec.arch->MigrateEntityTo(rec.chunk_idx, rec.row, dst);
        dst.GetEntity(dst_ci, dst_row) = e;

        Entity moved = rec.arch->SwapRemove(rec.chunk_idx, rec.row);
        if (moved != e)
        {
            uint32_t moved_id = EntityId(moved);
            records_[moved_id].arch = rec.arch;
            records_[moved_id].chunk_idx = rec.chunk_idx;
            records_[moved_id].row = rec.row;
        }

        records_[eid] = { &dst, dst_ci, dst_row, records_[eid].generation };
    }

    template<typename T>
    T& World::Get(Entity e)
    {
        assert(Alive(e));
        uint32_t eid = EntityId(e);
        return records_[eid].arch->Get<T>(records_[eid].chunk_idx, records_[eid].row);
    }

    template<typename T>
    T* World::TryGet(Entity e) noexcept
    {
        if (!Alive(e)) return nullptr;
        uint32_t eid = EntityId(e);
        auto& rec = records_[eid];
        if (!rec.arch->HasComponent(GetComponentId<T>())) return nullptr;
        return &rec.arch->Get<T>(rec.chunk_idx, rec.row);
    }

    template<typename T>
    bool World::Has(Entity e) const noexcept
    {
        if (!Alive(e)) return false;
        uint32_t eid = EntityId(e);
        return records_[eid].arch->HasComponent(GetComponentId<T>());
    }

    template<typename... Ts>
    void World::RegisterSystem(StringId name, SystemFn fn, SystemGroup group)
    {
        if (group == SystemGroup::Update)
            update_systems_.push_back({ name, MakeMask<Ts...>(), std::move(fn) });
        else
            render_systems_.push_back({ name, MakeMask<Ts...>(), std::move(fn) });
    }
}