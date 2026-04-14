#include "Camera.h"
#include <SDL3/SDL_rect.h>
#include <cmath>

// ─── Camera ───────────────────────────────────────────────────────────────────

Vec2 Camera::WorldToScreen(Vec2 world) const
{
    // Translate relative to camera position
    float dx = world.x - position.x;
    float dy = world.y - position.y;

    // Apply rotation (around camera center)
    float cosR = std::cos(-rotation);
    float sinR = std::sin(-rotation);
    float rx = dx * cosR - dy * sinR;
    float ry = dx * sinR + dy * cosR;

    // Apply zoom
    rx *= zoom;
    ry *= zoom;

    // Translate to viewport center
    float cx = viewport.x + viewport.w * 0.5f;
    float cy = viewport.y + viewport.h * 0.5f;

    return { cx + rx, cy + ry };
}

Vec2 Camera::ScreenToWorld(Vec2 screen) const
{
    float cx = viewport.x + viewport.w * 0.5f;
    float cy = viewport.y + viewport.h * 0.5f;

    // Remove viewport center offset
    float dx = screen.x - cx;
    float dy = screen.y - cy;

    // Remove zoom
    dx /= zoom;
    dy /= zoom;

    // Remove rotation
    float cosR = std::cos(rotation);
    float sinR = std::sin(rotation);
    float rx = dx * cosR - dy * sinR;
    float ry = dx * sinR + dy * cosR;

    return { position.x + rx, position.y + ry };
}

// ─── CameraSystem ─────────────────────────────────────────────────────────────

Camera& CameraSystem::AddCamera(StringId name)
{
    auto& cam = _cameras[name];
    if (_primary.id == 0)
        _primary = name;
    return cam;
}

Camera* CameraSystem::GetCamera(StringId name)
{
    auto it = _cameras.find(name);
    if (it == _cameras.end())
        return nullptr;
    return &it->second;
}

int CameraSystem::RemoveCamera(StringId name)
{
    auto it = _cameras.find(name);
    if (it == _cameras.end())
        return -1;

    _cameras.erase(it);

    if (_primary == name)
    {
        _primary = {};
        for (auto& [id, cam] : _cameras)
        {
            if (cam.active)
            {
                _primary = id;
                break;
            }
        }
    }

    return 0;
}

void CameraSystem::SetActive(StringId name)
{
    auto* cam = GetCamera(name);
    if (!cam) return;
    cam->active = true;
    _primary = name;
}

void CameraSystem::SetActive(StringId name, bool active)
{
    auto* cam = GetCamera(name);
    if (!cam) return;
    cam->active = active;

    if (!active && _primary == name)
    {
        _primary = {};
        for (auto& [id, c] : _cameras)
        {
            if (c.active)
            {
                _primary = id;
                break;
            }
        }
    }
}

Camera* CameraSystem::GetPrimary()
{
    if (_primary.id == 0)
        return nullptr;
    return GetCamera(_primary);
}

std::vector<Camera*> CameraSystem::GetAllActive()
{
    std::vector<Camera*> result;
    for (auto& [id, cam] : _cameras)
        if (cam.active)
            result.push_back(&cam);
    return result;
}