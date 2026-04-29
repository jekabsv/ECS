#pragma once
#include "UITypes.h"
#include "UITheme.h"
#include "struct.h"
#include <string>
#include <vector>
#include <functional>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>
#include <optional>

struct TTF_Font;

namespace UI
{
    struct StyleOverride
    {
        std::optional<Color>       background;
        std::optional<Color>       backgroundHover;
        std::optional<Color>       backgroundPress;
        std::optional<Color>       foreground;
        std::optional<Color>       border;
        std::optional<float>       borderWidth;
        std::optional<float>       borderRadius;
        std::optional<Edges>       padding;
        std::optional<Edges>       margin;
        std::optional<std::string> fontName;
        std::optional<float>       fontSize;
        std::optional<TextAlign>   textAlign;
        std::optional<Color>       placeholder;
    };

    struct WidgetData
    {
        // Label / Button
        std::string text;

        // Interaction
        InteractionState interactionState = InteractionState::None;

        // Slider
        float sliderValue = 0.0f;
        float sliderMin = 0.0f;
        float sliderMax = 1.0f;
        bool  sliderChanged = false;

        // InputField
        std::string inputValue;
        std::string placeholder;
        bool        inputChanged = false;
        bool        focused = false;
        uint32_t    cursorPos = 0;

        // Image
        StringId  textureId = {};
        SDL_FRect textureRect = { 0,0,0,0 };
    };

    struct FlexProps
    {
        FlexDirection  direction = FlexDirection::Row;
        FlexWrap       wrap = FlexWrap::NoWrap;
        JustifyContent justifyContent = JustifyContent::FlexStart;
        AlignItems     alignItems = AlignItems::Stretch;
        AlignContent   alignContent = AlignContent::Stretch;
        float          gap = 0.0f;
        float          columnGap = 0.0f;

        AlignSelf  alignSelf = AlignSelf::Auto;
        float      flexGrow = 0.0f;
        float      flexShrink = 1.0f;
        SizeValue  flexBasis = SizeValue::Auto();

        SizeValue width = SizeValue::Auto();
        SizeValue height = SizeValue::Auto();
        SizeValue minWidth = SizeValue::Px(0.0f);
        SizeValue minHeight = SizeValue::Px(0.0f);
        SizeValue maxWidth = SizeValue::Auto();
        SizeValue maxHeight = SizeValue::Auto();

        Edges padding = Edges::All(0.0f);
        Edges margin = Edges::All(0.0f);

        Overflow overflow = Overflow::Visible;
    };

    struct Node
    {
        NodeHandle  handle = NULL_HANDLE;
        WidgetType  type = WidgetType::Container;
        std::string id;

        NodeHandle              parent = NULL_HANDLE;
        std::vector<NodeHandle> children;

        FlexProps     flex;
        StyleOverride style;
        WidgetData    widget;

        bool visible = true;
        bool enabled = true;

        SDL_FRect computedRect = { 0, 0, 0, 0 };

        TTF_Font* resolvedFont = nullptr;
    };

} // namespace UI