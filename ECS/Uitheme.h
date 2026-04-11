#pragma once
#include "UITypes.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <variant>

namespace UI
{
    // -------------------------------------------------------------------------
    // A token value is one of: Color, float (size/spacing), string (font name)
    // -------------------------------------------------------------------------
    using TokenValue = std::variant<Color, float, std::string>;

    // -------------------------------------------------------------------------
    // Widget-type style overrides
    // Per-widget-type default styling on top of the base theme tokens.
    // The style system resolves: widget override → theme token → hard default.
    // -------------------------------------------------------------------------
    struct ButtonStyle
    {
        Color       background = Color::RGBA(60, 60, 60);
        Color       backgroundHover = Color::RGBA(80, 80, 80);
        Color       backgroundPress = Color::RGBA(40, 40, 40);
        Color       foreground = Color::RGBA(255, 255, 255);
        Color       border = Color::Transparent();
        float       borderWidth = 0.0f;
        float       borderRadius = 4.0f;
        Edges       padding = Edges::Axes(8.0f, 16.0f);
        std::string fontName = "";      // empty = theme default
        float       fontSize = 0.0f;    // 0 = theme default
        TextAlign   textAlign = TextAlign::Center;
    };

    struct LabelStyle
    {
        Color       foreground = Color::RGBA(255, 255, 255);
        std::string fontName = "";
        float       fontSize = 0.0f;
        TextAlign   textAlign = TextAlign::Left;
    };

    struct SliderStyle
    {
        Color trackBackground = Color::RGBA(40, 40, 40);
        Color trackFill = Color::RGBA(100, 160, 255);
        Color thumb = Color::RGBA(255, 255, 255);
        Color thumbHover = Color::RGBA(200, 220, 255);
        float trackHeight = 6.0f;
        float thumbRadius = 10.0f;
        float borderRadius = 3.0f;
    };

    struct InputFieldStyle
    {
        Color       background = Color::RGBA(30, 30, 30);
        Color       backgroundFocus = Color::RGBA(20, 20, 20);
        Color       foreground = Color::RGBA(255, 255, 255);
        Color       placeholder = Color::RGBA(120, 120, 120);
        Color       border = Color::RGBA(80, 80, 80);
        Color       borderFocus = Color::RGBA(100, 160, 255);
        float       borderWidth = 1.0f;
        float       borderRadius = 4.0f;
        Edges       padding = Edges::Axes(6.0f, 10.0f);
        std::string fontName = "";
        float       fontSize = 0.0f;
    };

    // -------------------------------------------------------------------------
    // Theme — the central token store + per-widget-type style defaults
    // -------------------------------------------------------------------------
    class Theme
    {
    public:
        // -- Token store ------------------------------------------------------

        void   SetToken(std::string_view name, TokenValue value);
        bool   HasToken(std::string_view name) const;

        // Returns token or a fallback if not found
        Color       GetColor(std::string_view name, Color       fallback = {}) const;
        float       GetFloat(std::string_view name, float       fallback = 0.f) const;
        std::string GetString(std::string_view name, std::string fallback = "") const;

        // -- Per-widget style defaults ----------------------------------------
        ButtonStyle     button;
        LabelStyle      label;
        SliderStyle     slider;
        InputFieldStyle inputField;

        // -- Convenience: load a built-in default dark theme ------------------
        void LoadDarkDefaults();

    private:
        std::unordered_map<std::string, TokenValue> tokens_;
    };

} // namespace UI