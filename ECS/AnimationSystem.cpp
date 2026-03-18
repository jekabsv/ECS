#include "AnimationSystem.h"
#include "logger.h"

void AnimationSystem::AddClip(StringId name, const AnimationClip& clip)
{
    _clips[name] = clip;
}

const AnimationClip* AnimationSystem::GetClip(StringId name) const
{
    auto it = _clips.find(name);
    if (it == _clips.end())
    {
        LOG_WARN(GlobalLogger(), "AnimationSystem", "GetClip: clip not found");
        return nullptr;
    }
    return &it->second;
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

void AnimationSystem::Play(AnimationPlayer& player, StringId clipName, bool loop)
{
    const AnimationClip* clip = GetClip(clipName);
    if (!clip)
    {
        LOG_WARN(GlobalLogger(), "AnimationSystem", "Play: clip not found, player unchanged");
        return;
    }

    player.currentClip = clipName;
    player.currentFrameX = clip->startX;
    player.elapsed = 0.0f;
    player.loop = loop;
    player.playing = true;

    player.currentSpritesheet = clip->spritesheet;
    player.currentRect = {
        (float)clip->startX,
        (float)(clip->row * clip->frameHeight),
        (float)clip->frameWidth,
        (float)clip->frameHeight
    };
}

void AnimationSystem::Stop(AnimationPlayer& player)
{
    player.playing = false;
    player.elapsed = 0.0f;
}

void AnimationSystem::Update(AnimationPlayer& player, float dt)
{
    if (!player.playing)
        return;

    const AnimationClip* clip = GetClip(player.currentClip);
    if (!clip)
    {
        LOG_WARN(GlobalLogger(), "AnimationSystem", "Update: active clip no longer exists");
        player.playing = false;
        return;
    }

    player.elapsed += dt;

    if (player.elapsed < clip->frameDuration)
        return;

    player.elapsed -= clip->frameDuration;

    int nextFrameX = player.currentFrameX + clip->frameWidth;

    if (nextFrameX > clip->stopX)
    {
        if (player.loop)
            nextFrameX = clip->startX;
        else
        {
            // One-shot finished: freeze on last frame, stop playing
            player.playing = false;
            return;
        }
    }

    player.currentFrameX = nextFrameX;
    player.currentSpritesheet = clip->spritesheet;
    player.currentRect = {
        (float)nextFrameX,
        (float)(clip->row * clip->frameHeight),
        (float)clip->frameWidth,
        (float)clip->frameHeight
    };
}