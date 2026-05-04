#include "AnimationSystem.h"
#include "logger.h"
#include <cassert>

int AnimationSystem::AddClip(StringId name, const AnimationClip& clip)
{
    _clips[name] = clip;
    LOG_DEBUG(GlobalLogger(), "AnimationSystem", "Added clip: " + std::to_string(name.id));
    return 0;
}

const AnimationClip* AnimationSystem::TryGetClip(StringId name) const
{
    auto it = _clips.find(name);
    return it != _clips.end() ? &it->second : nullptr;
}

const AnimationClip& AnimationSystem::GetClip(StringId name) const
{
    auto it = _clips.find(name);
    assert(it != _clips.end() && "AnimationSystem::GetClip — clip not found");
    return it->second;
}

int AnimationSystem::RemoveClip(StringId name)
{
    auto it = _clips.find(name);
    if (it == _clips.end())
    {
        LOG_WARN(GlobalLogger(), "AnimationSystem", "RemoveClip: clip not found");
        return -1;
    }
    _clips.erase(it);
    return 0;
}

int AnimationSystem::Play(AnimationPlayer& player, StringId clipName, bool loop)
{
    const AnimationClip* clip = TryGetClip(clipName);
    if (!clip)
    {
        LOG_WARN(GlobalLogger(), "AnimationSystem", "Play: clip not found, player unchanged");
        return -1;
    }

    player.currentClip = clipName;
    player.currentFrame = 0;
    player.elapsed = 0.0f;
    player.loop = loop;
    player.playing = true;
    player.currentSpritesheet = clip->spritesheet;
    player.currentRect = {
        (float)clip->startX,
        (float)(clip->startRow * clip->frameHeight),
        (float)clip->frameWidth,
        (float)clip->frameHeight
    };

    LOG_DEBUG(GlobalLogger(), "AnimationSystem",
        "Playing clip: " + std::to_string(clipName.id) +
        ", loop: " + (loop ? "true" : "false"));
    return 0;
}

int AnimationSystem::Stop(AnimationPlayer& player)
{
    player.playing = false;
    player.elapsed = 0.0f;
    LOG_DEBUG(GlobalLogger(), "AnimationSystem", "Stopped animation player");
    return 0;
}

void AnimationSystem::Update(AnimationPlayer& player, float dt)
{
    if (!player.playing) return;
    const AnimationClip* clip = TryGetClip(player.currentClip);
    if (!clip) { player.playing = false; return; }

    player.elapsed += dt;
    if (player.elapsed < clip->frameDuration) return;
    player.elapsed -= clip->frameDuration;

    int next = player.currentFrame + 1;
    if (next >= clip->totalFrames) {
        if (player.loop) next = 0;
        else { player.playing = false; return; }
    }

    player.currentFrame = next;

    // 2D -> pixel rect
    int col = (next % clip->columns);
    int row = clip->startRow + (next / clip->columns);
    player.currentRect = {
        (float)(clip->startX + col * clip->frameWidth),
        (float)(row * clip->frameHeight),
        (float)clip->frameWidth,
        (float)clip->frameHeight
    };
    player.currentSpritesheet = clip->spritesheet;
}