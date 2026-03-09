#pragma once
#include <any>
#include <unordered_map>
#include "Struct.h"
#include "logger.h"

class Blackboard
{
public:
    Blackboard() = default;

    template<typename T>
    void Set(StringId key, T value)
    {
        _store[key] = std::move(value);
    }

    template<typename T>
    T Get(StringId key) const
    {
        auto it = _store.find(key);
        if (it == _store.end())
        {
            LOG_WARN(GlobalLogger(), "Blackboard", "Get: key not found");
            return T{};
        }
        const T* ptr = std::any_cast<T>(&it->second);
        if (!ptr)
        {
            LOG_WARN(GlobalLogger(), "Blackboard", "Get: type mismatch for key");
            return T{};
        }
        return *ptr;
    }

    template<typename T>
    T* TryGet(StringId key)
    {
        auto it = _store.find(key);
        if (it == _store.end())
            return nullptr;
        return std::any_cast<T>(&it->second);
    }

    template<typename T>
    const T* TryGet(StringId key) const
    {
        auto it = _store.find(key);
        if (it == _store.end())
            return nullptr;
        return std::any_cast<T>(&it->second);
    }

    bool Has(StringId key) const
    {
        return _store.find(key) != _store.end();
    }

    void Remove(StringId key)
    {
        auto it = _store.find(key);
        if (it == _store.end())
        {
            LOG_WARN(GlobalLogger(), "Blackboard", "Remove: key not found");
            return;
        }
        _store.erase(it);
    }

    void Clear()
    {
        _store.clear();
    }

private:
    std::unordered_map<StringId, std::any> _store;
};