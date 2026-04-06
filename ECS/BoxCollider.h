#pragma once


struct BoxCollider
{
    BoxCollider(float halfWidth = 0.0f, float HalfHeight = 0.0f, bool _isTrigger = false, float _offsetX = 0.0f, float _offsetY = 0.0f)
        : offsetX(_offsetX), offsetY(_offsetY), hw(halfWidth), hh(HalfHeight), isTrigger(_isTrigger) {
    };
    float offsetX = 0.0f, offsetY = 0.0f;
    float hw = 0.0f, hh = 0.0f; // half size
    bool isTrigger = false;
};
