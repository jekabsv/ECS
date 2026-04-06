#pragma once

#include <unordered_map>
#include "Struct.h"


struct AnimationClip
{
    StringId spritesheet;
    int frameWidth = 0;
    int frameHeight = 0;
    int startX = 0;
    int stopX = 0;
    int row = 0;
    float frameDuration = 0.1f;
};


struct AnimationPlayer
{
    SDL_FRect currentRect = { 0, 0, 0, 0 };
    StringId currentSpritesheet = "";

    StringId currentClip = "";
    float elapsed = 0.0f;
    int currentFrameX = 0;

    bool loop = true;
    bool playing = false;
};

class AnimationSystem
{
public:
    AnimationSystem() = default;

    void AddClip(StringId name, const AnimationClip& clip);
    const AnimationClip* GetClip(StringId name) const;
    int RemoveClip(StringId name);

    void Play(AnimationPlayer& player, StringId clipName, bool loop = true);
    void Stop(AnimationPlayer& player);

    void Update(AnimationPlayer& player, float dt);

private:
    std::unordered_map<StringId, AnimationClip> _clips;
};