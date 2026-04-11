#pragma once
#include <cstdint>
#include <string>

namespace UI
{
    // -------------------------------------------------------------------------
    // Handle to a node in the UI tree. Stable across frames.
    // -------------------------------------------------------------------------
    using NodeHandle = uint32_t;
    static constexpr NodeHandle NULL_HANDLE = 0;

    // -------------------------------------------------------------------------
    // Widget types
    // -------------------------------------------------------------------------
    enum class WidgetType
    {
        Container,      // Pure layout box, no visual widget
        Label,          // Static text
        Button,         // Clickable box with label
        Slider,         // Draggable value in [min, max]
        InputField,     // Single-line text input
        Image,          // SDL_Texture display
    };

    // -------------------------------------------------------------------------
    // Widget interaction state (polled by user)
    // -------------------------------------------------------------------------
    enum class InteractionState : uint8_t
    {
        None,
        Hovered,
        Pressed,    // Held this frame
        Released,   // Released this frame (i.e. "clicked")
        Focused,    // For InputField
    };

    // -------------------------------------------------------------------------
    // Flex layout enums  (mirrors CSS flexbox)
    // -------------------------------------------------------------------------
    enum class FlexDirection { Row, RowReverse, Column, ColumnReverse };
    enum class FlexWrap { NoWrap, Wrap, WrapReverse };
    enum class JustifyContent { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly };
    enum class AlignItems { FlexStart, FlexEnd, Center, Stretch, Baseline };
    enum class AlignContent { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, Stretch };
    enum class AlignSelf { Auto, FlexStart, FlexEnd, Center, Stretch, Baseline };

    // -------------------------------------------------------------------------
    // Size units
    // -------------------------------------------------------------------------
    enum class SizeUnit { Px, Percent, Auto };

    struct SizeValue
    {
        float    value = 0.0f;
        SizeUnit unit = SizeUnit::Auto;

        static SizeValue Px(float v) { return { v, SizeUnit::Px }; }
        static SizeValue Percent(float v) { return { v, SizeUnit::Percent }; }
        static SizeValue Auto() { return { 0, SizeUnit::Auto }; }
    };

    // -------------------------------------------------------------------------
    // Four-sided value (padding / margin / border)
    // -------------------------------------------------------------------------
    struct Edges
    {
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        float left = 0.0f;

        static Edges All(float v) { return { v, v, v, v }; }
        static Edges Axes(float vertical, float horizontal) { return { vertical, horizontal, vertical, horizontal }; }
        static Edges TRBL(float t, float r, float b, float l) { return { t, r, b, l }; }
    };

    // -------------------------------------------------------------------------
    // Color (RGBA, 0-255)
    // -------------------------------------------------------------------------
    struct Color
    {
        uint8_t r = 255, g = 255, b = 255, a = 255;

        static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { return { r, g, b, a }; }
        static Color Hex(uint32_t hex) // 0xRRGGBBAA
        {
            return {
                (uint8_t)((hex >> 24) & 0xFF),
                (uint8_t)((hex >> 16) & 0xFF),
                (uint8_t)((hex >> 8) & 0xFF),
                (uint8_t)((hex) & 0xFF)
            };
        }
        static Color Transparent() { return { 0, 0, 0, 0 }; }
    };

    // -------------------------------------------------------------------------
    // Text alignment
    // -------------------------------------------------------------------------
    enum class TextAlign { Left, Center, Right };

    // -------------------------------------------------------------------------
    // Overflow behaviour (for future scroll support)
    // -------------------------------------------------------------------------
    enum class Overflow { Visible, Hidden, Scroll };

} // namespace UI