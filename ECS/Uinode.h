#pragma once
#include "UITypes.h"
#include "UITheme.h"
#include <string>
#include <vector>
#include <functional>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>
#include <optional>

struct SDL_Texture;
struct TTF_Font;

namespace UI
{
    // -------------------------------------------------------------------------
    // Per-node style overrides.
    // Any field left as std::nullopt falls back to the widget-type style
    // which in turn falls back to theme tokens.
    // -------------------------------------------------------------------------
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
        std::optional<Color>       placeholder;    // InputField only
    };

    // -------------------------------------------------------------------------
    // Widget-specific data, stored as a union-like struct.
    // Only the fields relevant to the node's WidgetType are used.
    // -------------------------------------------------------------------------
    struct WidgetData
    {
        // Label / Button
        std::string text;

        // Button / InputField / Slider interaction results (written each frame)
        InteractionState interactionState = InteractionState::None;

        // Slider
        float   sliderValue = 0.0f;
        float   sliderMin = 0.0f;
        float   sliderMax = 1.0f;
        bool    sliderChanged = false;  // true for exactly one frame after change

        // InputField
        std::string inputValue;
        std::string placeholder;
        bool        inputChanged = false;
        bool        focused = false;
        uint32_t    cursorPos = 0;

        // Image
        SDL_Texture* texture = nullptr;    // not owned
        SDL_FRect    textureRect = { 0,0,0,0 }; // source rect, zeroed = full texture
    };

    // -------------------------------------------------------------------------
    // Flex properties of a node
    // -------------------------------------------------------------------------
    struct FlexProps
    {
        // Container properties (only meaningful when node has children)
        FlexDirection  direction = FlexDirection::Row;
        FlexWrap       wrap = FlexWrap::NoWrap;
        JustifyContent justifyContent = JustifyContent::FlexStart;
        AlignItems     alignItems = AlignItems::Stretch;
        AlignContent   alignContent = AlignContent::Stretch;
        float          gap = 0.0f;   // main-axis gap between items
        float          columnGap = 0.0f;   // cross-axis gap (when wrapping)

        // Item properties (how this node behaves inside its parent)
        AlignSelf  alignSelf = AlignSelf::Auto;
        float      flexGrow = 0.0f;
        float      flexShrink = 1.0f;
        SizeValue  flexBasis = SizeValue::Auto();

        // Sizing
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

    // -------------------------------------------------------------------------
    // A single node in the retained UI tree
    // -------------------------------------------------------------------------
    struct Node
    {
        // Identity
        NodeHandle  handle = NULL_HANDLE;
        WidgetType  type = WidgetType::Container;
        std::string id;         // optional user-assigned string id for lookup

        // Tree structure
        NodeHandle              parent = NULL_HANDLE;
        std::vector<NodeHandle> children;

        // Layout
        FlexProps flex;

        // Style overrides (null = fall through to theme)
        StyleOverride style;

        // Widget data (text, value, interaction state, etc.)
        WidgetData widget;

        // Visibility / enabled
        bool visible = true;
        bool enabled = true;    // if false, no interaction but still rendered (greyed out)

        // Computed layout rect (written by the layout pass each frame)
        SDL_FRect computedRect = { 0, 0, 0, 0 };

        // Resolved font (set during render pass; ptr not owned)
        TTF_Font* resolvedFont = nullptr;
    };

} // namespace UI