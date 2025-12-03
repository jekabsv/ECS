#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <cstring>
#include <cassert>

namespace ecs
{
    using Entity = std::uint32_t;
    constexpr Entity INVALID_ENTITY = static_cast<Entity>(-1);

    constexpr size_t MAX_COMPONENTS = 64;
    using TypeId = std::uint64_t;
    inline TypeId next_type_id() noexcept
    {
        static TypeId counter = 0;
        return counter++;
    }

    template<typename T>
    inline TypeId getTypeId() noexcept
    {
        static const TypeId id = next_type_id();
        return id;
    }

    struct ComponentBuffer
    {
        std::size_t stride = 0;
        std::vector<std::uint8_t> data;

        void reserve(std::size_t count)
        {
            data.reserve(count * stride);
        }

        void push_back_raw(const void* src)
        {
            std::size_t old = data.size();
            data.resize(old + stride);
            std::memcpy(data.data() + old, src, stride);
        }

        void swap_erase(std::size_t index)
        {
            std::size_t last = size() - 1;
            if (index != last)
                std::memcpy(data.data() + index * stride, data.data() + last * stride, stride);
            data.resize(last * stride);
        }

        void* at(std::size_t index)
        {
            return data.data() + index * stride;
        }

        const void* at(std::size_t index) const
        {
            return data.data() + index * stride;
        }

        std::size_t size() const
        {
            return stride == 0 ? 0 : data.size() / stride;
        }
    };

    struct Archetype
    {
        std::uint64_t signature = 0;
        std::vector<Entity> entities;
        std::unordered_map<TypeId, ComponentBuffer> buffers;

        Archetype(std::uint64_t sig = 0) : signature(sig) {}

        std::size_t size() const { return entities.size(); }

        template<typename T>
        void ensure_buffer()
        {
            auto& buf = buffers[getTypeId<T>()];
            if (buf.stride == 0) buf.stride = sizeof(T);
        }

        template<typename T>
        void push_component(const T& comp)
        {
            ensure_buffer<T>();
            buffers[getTypeId<T>()].push_back_raw(&comp);
        }

        template<typename T>
        T& get_component(std::size_t index)
        {
            auto& buf = buffers[getTypeId<T>()];
            return *static_cast<T*>(buf.at(index));
        }

        void swap_remove_entity(std::size_t index)
        {
            std::size_t last = entities.size() - 1;
            if (index != last)
            {
                entities[index] = entities[last];
                for (auto& kv : buffers)
                {
                    auto& buf = kv.second;
                    std::memcpy(buf.at(index), buf.at(last), buf.stride);
                    buf.data.resize((buf.size() - 1) * buf.stride);
                }
            }
            else
            {
                for (auto& kv : buffers)
                {
                    auto& buf = kv.second;
                    buf.data.resize((buf.size() - 1) * buf.stride);
                }
            }
            entities.resize(last);
        }
    };

    struct EntityLocation
    {
        Archetype* archetype = nullptr;
        std::size_t index = 0;
    };

    class EntityManager
    {
        std::vector<Entity> freeList;
        Entity nextId = 0;
    public:
        Entity create()
        {
            if (!freeList.empty())
            {
                Entity e = freeList.back();
                freeList.pop_back();
                return e;
            }
            return nextId++;
        }

        void destroy(Entity e) { freeList.push_back(e); }
    };

    class ECS
    {
        EntityManager em;
        std::unordered_map<std::uint64_t, std::unique_ptr<Archetype>> archetypes;
        std::vector<EntityLocation> locations;

        Archetype* get_or_create_archetype(std::uint64_t signature)
        {
            auto it = archetypes.find(signature);
            if (it != archetypes.end())
            {
                it->second->signature = signature;
                return it->second.get();
            }
            auto ptr = std::make_unique<Archetype>(signature);
            Archetype* p = ptr.get();
            archetypes.emplace(signature, std::move(ptr));
            return p;
        }

        void ensure_locations_size(Entity e)
        {
            if (e >= locations.size()) locations.resize(e + 1);
        }

        template<typename T>
        inline std::uint64_t bit_for() const
        {
            TypeId tid = getTypeId<T>();
            assert(tid < MAX_COMPONENTS && "Exceeded MAX_COMPONENTS");
            return (std::uint64_t)1 << tid;
        }

    public:
        template<typename... Components>
        inline std::uint64_t CreateMask() const
        {
            std::uint64_t mask = 0;
            ((mask |= (std::uint64_t(1) << getTypeId<Components>())), ...);
            return mask;
        }

        Entity create_entity()
        {
            Entity e = em.create();
            ensure_locations_size(e);
            Archetype* a = get_or_create_archetype(0);
            locations[e] = { a, a->entities.size() };
            a->entities.push_back(e);
            return e;
        }

        void destroy_entity(Entity e)
        {
            assert(e < locations.size() && "Invalid entity");
            auto loc = locations[e];
            if (loc.archetype == nullptr) 
                return;
            Archetype* a = loc.archetype;
            std::size_t idx = loc.index;
            Entity moved = a->entities.back();
            a->swap_remove_entity(idx);
            if (moved != e) 
                locations[moved].index = idx;
            locations[e] = { nullptr, 0 };
            em.destroy(e);
        }

        template<typename T>
        void add_component(Entity e, const T& comp)
        {
            assert(e < locations.size() && "Invalid entity");
            auto oldLoc = locations[e];
            Archetype* oldAr = oldLoc.archetype;
            std::uint64_t oldSig = oldAr ? oldAr->signature : 0;
            std::uint64_t bit = bit_for<T>();

            if (oldSig & bit)
            {
                std::size_t idx = oldLoc.index;
                oldAr->get_component<T>(idx) = comp;
                return;
            }

            std::uint64_t newSig = oldSig | bit;
            Archetype* newAr = get_or_create_archetype(newSig);

            std::size_t newIndex = newAr->entities.size();
            newAr->entities.push_back(e);

            if (oldAr)
            {
                for (auto& kv : oldAr->buffers)
                {
                    TypeId tid = kv.first;
                    const ComponentBuffer& oldBuf = kv.second;
                    auto& newBuf = newAr->buffers[tid];
                    if (newBuf.stride == 0) newBuf.stride = oldBuf.stride;
                    newBuf.data.resize(newBuf.data.size() + newBuf.stride);
                    std::memcpy(newBuf.data.data() + (newBuf.size() - 1) * newBuf.stride,
                        oldBuf.at(oldLoc.index), newBuf.stride);
                }

                newAr->ensure_buffer<T>();
                newAr->push_component<T>(comp);

                Entity moved = oldAr->entities.back();
                oldAr->swap_remove_entity(oldLoc.index);
                if (moved != e) 
                    locations[moved].index = oldLoc.index;
            }
            else
            {
                newAr->ensure_buffer<T>();
                newAr->push_component<T>(comp);
            }

            locations[e].archetype = newAr;
            locations[e].index = newIndex;
        }

        template<typename T>
        void remove_component(Entity e)
        {
            assert(e < locations.size() && "Invalid entity");
            auto oldLoc = locations[e];
            Archetype* oldAr = oldLoc.archetype;
            std::uint64_t oldSig = oldAr ? oldAr->signature : 0;
            std::uint64_t bit = bit_for<T>();
            if (!(oldSig & bit)) 
                return;

            std::uint64_t newSig = oldSig & ~bit;
            Archetype* newAr = get_or_create_archetype(newSig);

            std::size_t newIndex = newAr->entities.size();
            newAr->entities.push_back(e);

            for (auto& kv : oldAr->buffers)
            {
                TypeId tid = kv.first;
                if (tid == getTypeId<T>()) 
                    continue;
                const ComponentBuffer& oldBuf = kv.second;
                auto& newBuf = newAr->buffers[tid];
                if (newBuf.stride == 0) 
                    newBuf.stride = oldBuf.stride;
                newBuf.data.resize(newBuf.data.size() + newBuf.stride);
                std::memcpy(newBuf.data.data() + (newBuf.size() - 1) * newBuf.stride,
                    oldBuf.at(oldLoc.index), newBuf.stride);
            }

            Entity moved = oldAr->entities.back();
            oldAr->swap_remove_entity(oldLoc.index);
            if (moved != e) 
                locations[moved].index = oldLoc.index;

            locations[e].archetype = newAr;
            locations[e].index = newIndex;
        }

        template<typename T>
        T& get(Entity e)
        {
            auto loc = locations[e];
            assert(loc.archetype && "Entity has no archetype");
            return loc.archetype->get_component<T>(loc.index);
        }

        void for_each(std::uint64_t hasMask, const std::function<void(Entity, std::function<void* (TypeId)>)>& fn)
        {
            for (auto& kv : archetypes)
            {
                Archetype* a = kv.second.get();
                if ((a->signature & hasMask) == hasMask)
                {
                    for (std::size_t i = 0; i < a->entities.size();i++)
                    {
                        Entity e = a->entities[i];
                        auto accessor = [a, i](TypeId tid) -> void*
                        {
                            auto it = a->buffers.find(tid);
                            if (it == a->buffers.end()) return nullptr;
                            return it->second.at(i);
                        };
                        fn(e, accessor);
                    }
                }
            }
        }
    };
}