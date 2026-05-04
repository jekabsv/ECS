#pragma once
#include <cmath>
#include <algorithm>
#include <cassert>
#include <ostream>
#include "Struct.h"


namespace Math
{

    inline constexpr float PI = 3.14159265358979323846f;
    inline constexpr float TAU = 6.28318530717958647692f;
    inline constexpr float HALF_PI = 1.57079632679489661923f;
    inline constexpr float DEG2RAD = PI / 180.0f;
    inline constexpr float RAD2DEG = 180.0f / PI;
    inline constexpr float EPSILON = 1e-6f;

    constexpr Vec2 operator+(float s, const Vec2& v) { return { s + v.x, s + v.y }; }
    constexpr Vec2 operator-(float s, const Vec2& v) { return { s - v.x, s - v.y }; }
    constexpr Vec2 operator*(float s, const Vec2& v) { return { s * v.x, s * v.y }; }
    constexpr Vec2 operator/(float s, const Vec2& v) { return { s / v.x, s / v.y }; }

    inline constexpr Vec2 VEC2_ZERO{ 0.0f, 0.0f };
    inline constexpr Vec2 VEC2_ONE{ 1.0f, 1.0f };
    inline constexpr Vec2 VEC2_RIGHT{ 1.0f, 0.0f };
    inline constexpr Vec2 VEC2_UP{ 0.0f, 1.0f };

    inline float ToRadians(float degrees) { return degrees * DEG2RAD; }
    inline float ToDegrees(float radians) { return radians * RAD2DEG; }

    // Clamp a value between lo and hi
    inline float Clamp(float v, float lo, float hi)
    {
        return std::clamp(v, lo, hi);
    }

    // Clamp to [0,1]
    inline float Saturate(float v)
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    // Linear interpolation
    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

	// Linear interpolation for colors
    inline SDL_FColor LerpColor(SDL_FColor a, SDL_FColor b, float t)
    {
        return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
    }

    // Inverse lerp — what t gives value v between a and b?
    inline float InvLerp(float a, float b, float v)
    {
        assert(std::abs(b - a) > EPSILON && "InvLerp: a == b");
        return (v - a) / (b - a);
    }

    // Remap value from [inLo,inHi] to [outLo,outHi]
    inline float Remap(float v, float inLo, float inHi, float outLo, float outHi)
    {
        return Lerp(outLo, outHi, InvLerp(inLo, inHi, v));
    }

    // Sign: returns -1, 0, or +1
    inline float Sign(float v)
    {
        if (v > 0.0f)  return  1.0f;
        if (v < 0.0f)  return -1.0f;
        return 0.0f;
    }

    // Smooth-step (cubic Hermite)
    inline float SmoothStep(float lo, float hi, float t)
    {
        t = Saturate((t - lo) / (hi - lo));
        return t * t * (3.0f - 2.0f * t);
    }

    // Wrap angle into (-PI, PI]
    inline float WrapAngle(float a)
    {
        while (a > PI)  a -= TAU;
        while (a < -PI)  a += TAU;
        return a;
    }

    // Shortest signed difference from 'current' to 'target', in (-PI, PI]
    inline float AngleDiff(float target, float current)
    {
        return WrapAngle(target - current);
    }

    // Lerp between two angles via the shortest arc
    inline float LerpAngle(float a, float b, float t)
    {
        return WrapAngle(a + AngleDiff(b, a) * t);
    }

    // Move 'current' toward 'target' by at most 'maxDelta' (always takes shortest arc)
    inline float MoveTowardAngle(float current, float target, float maxDelta)
    {
        float diff = AngleDiff(target, current);
        if (std::abs(diff) <= maxDelta) return target;
        return WrapAngle(current + Sign(diff) * maxDelta);
    }

    // Convert a unit direction vector to an angle (atan2 wrapper)
    inline float Vec2ToAngle(float x, float y) { return std::atan2(y, x); }
    inline float Vec2ToAngle(Vec2 v) { return std::atan2(v.y, v.x); }

    // Convert an angle to a unit direction vector
    inline void AngleToVec2(float angle, float& outX, float& outY)
    {
        outX = std::cos(angle);
        outY = std::sin(angle);
    }
    inline Vec2 AngleToVec2(float angle)
    {
        return { std::cos(angle), std::sin(angle) };
    }

    // Squared length — prefer this over Length() when you only need comparisons
    inline float LengthSq(float x, float y) { return x * x + y * y; }
    inline float LengthSq(Vec2 v) { return v.LengthSq(); }

    inline float Length(float x, float y) { return std::sqrt(LengthSq(x, y)); }
    inline float Length(Vec2 v) { return v.Length(); }

    // Returns length; writes unit vector into (outX, outY).
    // If the vector is nearly zero, outputs (0,0) and returns 0.
    inline float Normalize(float x, float y, float& outX, float& outY)
    {
        float len = Length(x, y);
        if (len < EPSILON) { outX = 0.0f; outY = 0.0f; return 0.0f; }
        outX = x / len;
        outY = y / len;
        return len;
    }
    inline Vec2 Normalize(Vec2 v) { return v.Normalized(); }

    inline float DistanceSq(float ax, float ay, float bx, float by)
    {
        float dx = bx - ax, dy = by - ay;
        return dx * dx + dy * dy;
    }
    inline float DistanceSq(Vec2 a, Vec2 b) { return (b - a).LengthSq(); }

    inline float Distance(float ax, float ay, float bx, float by)
    {
        return std::sqrt(DistanceSq(ax, ay, bx, by));
    }
    inline float Distance(Vec2 a, Vec2 b) { return (b - a).Length(); }

    inline float Dot(float ax, float ay, float bx, float by) { return ax * bx + ay * by; }
    inline float Dot(Vec2 a, Vec2 b) { return a.Dot(b); }

    // 2-D "cross product" (scalar z-component of 3-D cross)
    // Positive = b is counter-clockwise from a
    inline float Cross(float ax, float ay, float bx, float by) { return ax * by - ay * bx; }
    inline float Cross(Vec2 a, Vec2 b) { return a.Cross(b); }

    // Clamp vector magnitude to [0, maxLen]
    inline void ClampMagnitude(float x, float y, float maxLen, float& outX, float& outY)
    {
        float lenSq = LengthSq(x, y);
        if (lenSq > maxLen * maxLen)
        {
            float scale = maxLen / std::sqrt(lenSq);
            outX = x * scale; outY = y * scale;
        }
        else { outX = x; outY = y; }
    }
    inline Vec2 ClampMagnitude(Vec2 v, float maxLen) { return v.ClampedMagnitude(maxLen); }

    // Clamp magnitude between [minLen, maxLen] — useful for speed clamping
    inline void ClampMagnitudeRange(float x, float y, float minLen, float maxLen,
        float& outX, float& outY)
    {
        float len = Length(x, y);
        if (len < EPSILON) { outX = 0.0f; outY = 0.0f; return; }
        float clamped = std::clamp(len, minLen, maxLen);
        outX = x * (clamped / len);
        outY = y * (clamped / len);
    }
    inline Vec2 ClampMagnitudeRange(Vec2 v, float minLen, float maxLen)
    {
        return v.ClampedMagnitudeRange(minLen, maxLen);
    }

    // Reflect vector (x,y) about a unit normal (nx,ny)
    inline void Reflect(float x, float y, float nx, float ny, float& outX, float& outY)
    {
        float d = 2.0f * Dot(x, y, nx, ny);
        outX = x - d * nx;
        outY = y - d * ny;
    }
    inline Vec2 Reflect(Vec2 v, Vec2 normal)
    {
        return v - 2.0f * v.Dot(normal) * normal;
    }

    // Linear interpolation between two vectors
    inline void LerpVec2(float ax, float ay, float bx, float by, float t,
        float& outX, float& outY)
    {
        outX = Lerp(ax, bx, t);
        outY = Lerp(ay, by, t);
    }
    inline Vec2 Lerp(Vec2 a, Vec2 b, float t)
    {
        return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
    }

    // Shortest signed delta on one axis for a toroidal world of 'size'
    inline float ToroidalDelta(float delta, float size)
    {
        if (delta > size * 0.5f) delta -= size;
        if (delta < -size * 0.5f) delta += size;
        return delta;
    }

    // Toroidal delta for both axes at once
    inline Vec2 ToroidalDelta(Vec2 delta, float worldW, float worldH)
    {
        return { ToroidalDelta(delta.x, worldW), ToroidalDelta(delta.y, worldH) };
    }

    // Shortest squared distance between two points in a toroidal world
    inline float ToroidalDistanceSq(float ax, float ay,
        float bx, float by,
        float worldW, float worldH)
    {
        float dx = ToroidalDelta(bx - ax, worldW);
        float dy = ToroidalDelta(by - ay, worldH);
        return dx * dx + dy * dy;
    }
    inline float ToroidalDistanceSq(Vec2 a, Vec2 b, float worldW, float worldH)
    {
        return ToroidalDelta(b - a, worldW, worldH).LengthSq();
    }

    inline float ToroidalDistance(float ax, float ay,
        float bx, float by,
        float worldW, float worldH)
    {
        return std::sqrt(ToroidalDistanceSq(ax, ay, bx, by, worldW, worldH));
    }
    inline float ToroidalDistance(Vec2 a, Vec2 b, float worldW, float worldH)
    {
        return std::sqrt(ToroidalDistanceSq(a, b, worldW, worldH));
    }

    // Wrap a position into [origin, origin+size) on one axis
    inline float WrapAxis(float v, float origin, float size)
    {
        v -= origin;
        v = v - size * std::floor(v / size);
        return v + origin;
    }

    // Wrap a 2-D position into the play area [areaX, areaX+areaW) x [0, areaH)
    inline void WrapPosition(float& x, float& y, float areaX, float areaW, float areaH)
    {
        x = WrapAxis(x, areaX, areaW);
        y = WrapAxis(y, 0.0f, areaH);
    }
    inline Vec2 WrapPosition(Vec2 pos, float areaX, float areaW, float areaH)
    {
        return { WrapAxis(pos.x, areaX, areaW), WrapAxis(pos.y, 0.0f, areaH) };
    }

    inline int DrainSpawnBudget(int current, int target, int budgetPerFrame)
    {
        int delta = std::clamp(target - current, -budgetPerFrame, budgetPerFrame);
        return delta;
    }

    // Float in [lo, hi)
    inline float RandFloat(float lo, float hi)
    {
        return lo + (float)(rand() % 10000) / 10000.0f * (hi - lo);
    }

    // Integer in [lo, hi]
    inline int RandInt(int lo, int hi)
    {
        return lo + rand() % (hi - lo + 1);
    }

    // Random angle in [0, TAU)
    inline float RandAngle() { return RandFloat(0.0f, TAU); }

    // Random point inside a rectangle [x, x+w) x [y, y+h)
    inline void RandPointInRect(float x, float y, float w, float h,
        float& outX, float& outY)
    {
        outX = x + RandFloat(0.0f, w);
        outY = y + RandFloat(0.0f, h);
    }
    inline Vec2 RandPointInRect(float x, float y, float w, float h)
    {
        return { x + RandFloat(0.0f, w), y + RandFloat(0.0f, h) };
    }

    // Random unit direction vector
    inline void RandDirection(float& outX, float& outY) { AngleToVec2(RandAngle(), outX, outY); }
    inline Vec2 RandDirection() { return AngleToVec2(RandAngle()); }

    // Random Vec2 with both components in [lo, hi]
    inline Vec2 RandVec2(float lo, float hi)
    {
        return { RandFloat(lo, hi), RandFloat(lo, hi) };
    }

    // Point inside axis-aligned rectangle [rx, rx+rw) x [ry, ry+rh)
    inline bool PointInRect(float px, float py, float rx, float ry, float rw, float rh)
    {
        return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
    }
    inline bool PointInRect(Vec2 p, float rx, float ry, float rw, float rh)
    {
        return PointInRect(p.x, p.y, rx, ry, rw, rh);
    }

    // Point inside circle
    inline bool PointInCircle(float px, float py, float cx, float cy, float radius)
    {
        return DistanceSq(px, py, cx, cy) < radius * radius;
    }
    inline bool PointInCircle(Vec2 p, Vec2 center, float radius)
    {
        return (p - center).LengthSq() < radius * radius;
    }

    // Two axis-aligned rectangles overlap (strictly)
    inline bool RectsOverlap(float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh)
    {
        return ax < bx + bw && ax + aw > bx
            && ay < by + bh && ay + ah > by;
    }

    // Two circles overlap
    inline bool CirclesOverlap(float ax, float ay, float ar,
        float bx, float by, float br)
    {
        float r = ar + br;
        return DistanceSq(ax, ay, bx, by) < r * r;
    }
    inline bool CirclesOverlap(Vec2 a, float ar, Vec2 b, float br)
    {
        float r = ar + br;
        return (b - a).LengthSq() < r * r;
    }

} // namespace Math