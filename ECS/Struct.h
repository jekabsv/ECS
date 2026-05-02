#pragma once

#include <array>
#include <string_view>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>


#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

struct SDL_Texture;
struct SDL_FRect;
struct SDL_Renderer;
struct Vertex;

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}
    constexpr explicit Vec2(float s) : x(s), y(s) {}

    constexpr Vec2 operator+(const Vec2& r) const { return { x + r.x, y + r.y }; }
    constexpr Vec2 operator-(const Vec2& r) const { return { x - r.x, y - r.y }; }
    constexpr Vec2 operator*(const Vec2& r) const { return { x * r.x, y * r.y }; }
    constexpr Vec2 operator/(const Vec2& r) const { return { x / r.x, y / r.y }; }

    constexpr Vec2& operator+=(const Vec2& r) { x += r.x; y += r.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& r) { x -= r.x; y -= r.y; return *this; }
    constexpr Vec2& operator*=(const Vec2& r) { x *= r.x; y *= r.y; return *this; }
    constexpr Vec2& operator/=(const Vec2& r) { x /= r.x; y /= r.y; return *this; }

    constexpr Vec2 operator+(float s) const { return { x + s, y + s }; }
    constexpr Vec2 operator-(float s) const { return { x - s, y - s }; }
    constexpr Vec2 operator*(float s) const { return { x * s, y * s }; }
    constexpr Vec2 operator/(float s) const { return { x / s, y / s }; }

    constexpr Vec2& operator+=(float s) { x += s; y += s; return *this; }
    constexpr Vec2& operator-=(float s) { x -= s; y -= s; return *this; }
    constexpr Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    constexpr Vec2& operator/=(float s) { x /= s; y /= s; return *this; }


    constexpr Vec2 operator-() const { return { -x, -y }; }
    constexpr Vec2 operator+() const { return *this; }

    constexpr bool operator==(const Vec2& r) const { return x == r.x && y == r.y; }
    constexpr bool operator!=(const Vec2& r) const { return !(*this == r); }

    float LengthSq()  const { return x * x + y * y; }
    float Length()    const { return std::sqrt(LengthSq()); }
    bool  IsZero()    const { return LengthSq() < 1e-12f; }

    Vec2 Normalized() const
    {
        float len = Length();
        return len < 1e-6f ? Vec2{} : Vec2{ x / len, y / len };
    }

    float Dot(const Vec2& r) const { return x * r.x + y * r.y; }
    float Cross(const Vec2& r) const { return x * r.y - y * r.x; }

    Vec2 Perpendicular()       const { return { -y, x }; }
    Vec2 Abs()                 const { return { std::abs(x), std::abs(y) }; }

    float ToAngle()            const { return std::atan2(y, x); }

    // Clamp both components independently
    Vec2 Clamped(float lo, float hi) const
    {
        return { std::clamp(x, lo, hi), std::clamp(y, lo, hi) };
    }

    // Clamp the magnitude (length) to [0, maxLen]
    Vec2 ClampedMagnitude(float maxLen) const
    {
        float lenSq = LengthSq();
        if (lenSq > maxLen * maxLen)
            return *this * (maxLen / std::sqrt(lenSq));
        return *this;
    }

    // Clamp the magnitude to [minLen, maxLen]
    Vec2 ClampedMagnitudeRange(float minLen, float maxLen) const
    {
        float len = Length();
        if (len < 1e-6f) return {};
        return *this * (std::clamp(len, minLen, maxLen) / len);
    }

    friend std::ostream& operator<<(std::ostream& os, const Vec2& v)
    {
        return os << '(' << v.x << ", " << v.y << ')';
    }
};

struct Vec3
{
    float x, y, z;
};

using Triangle = std::array<Vertex, 3>;

using MeshVertices = std::vector<Vertex>;
using MeshIndices = std::vector<uint32_t>;

struct StringId
{
    uint32_t id = 0;

    StringId() = default;
    constexpr StringId(const char* str) : StringId(std::string_view(str)) {}
    constexpr StringId(std::string_view str)
    {
        uint32_t h = 2166136261u;
        for (char c : str) 
            h = (h ^ (uint8_t)c) * 16777619u;
        id = h;
    }

    bool empty() const { return id == 0 || id == StringId("").id; }

    constexpr bool operator==(const StringId& o) const { return id == o.id; }
    constexpr bool operator!=(const StringId& o) const { return id != o.id; }
};

template<>
struct std::hash<StringId>
{
    size_t operator()(const StringId& s) const noexcept { return s.id; }
};