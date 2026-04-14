#pragma once
#include "Struct.h"
#include <unordered_map>
#include <vector>

struct SDL_FRect;

struct Camera
{
    Vec2 position = { 0.0f, 0.0f };
    float zoom = 1.0f;
    float rotation = 0.0f; 
    SDL_FRect viewport = { 0.0f, 0.0f, 1920.0f, 1080.0f };

    bool active = true;

    Vec2 WorldToScreen(Vec2 world) const;
    Vec2 ScreenToWorld(Vec2 screen) const;
};

class CameraSystem
{
public:
    CameraSystem() = default;

    Camera& AddCamera(StringId name);
    Camera* GetCamera(StringId name);
    int             RemoveCamera(StringId name);

    void            SetActive(StringId name);
    void            SetActive(StringId name, bool active);

    Camera* GetPrimary();
    std::vector<Camera*> GetAllActive();

private:
    std::unordered_map<StringId, Camera> _cameras;
    StringId _primary = {};
};