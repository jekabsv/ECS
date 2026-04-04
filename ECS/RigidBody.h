#pragma once

struct RigidBody
{
    float vx = 0.0f, vy = 0.0f;
    float drag = 0.0f;
    bool isStatic = false;
};