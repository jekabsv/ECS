#pragma once

struct RigidBody
{
    RigidBody(float _vx = 0.0f, float _vy = 0.0f, bool _isStatic = false, float _drag = 0.0f)
        : vx(_vx), vy(_vy), isStatic(_isStatic), drag(_drag) {};
    float vx = 0.0f, vy = 0.0f;
    float drag = 0.0f;
    bool isStatic = false;
};