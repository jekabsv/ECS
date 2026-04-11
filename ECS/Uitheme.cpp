#include "UITheme.h"

namespace UI
{

    void Theme::SetToken(std::string_view name, TokenValue value)
    {
        tokens_[std::string(name)] = std::move(value);
    }

    bool Theme::HasToken(std::string_view name) const
    {
        return tokens_.count(std::string(name)) > 0;
    }

    Color Theme::GetColor(std::string_view name, Color fallback) const
    {
        auto it = tokens_.find(std::string(name));
        if (it == tokens_.end()) return fallback;
        if (auto* v = std::get_if<Color>(&it->second)) return *v;
        return fallback;
    }

    float Theme::GetFloat(std::string_view name, float fallback) const
    {
        auto it = tokens_.find(std::string(name));
        if (it == tokens_.end()) return fallback;
        if (auto* v = std::get_if<float>(&it->second)) return *v;
        return fallback;
    }

    std::string Theme::GetString(std::string_view name, std::string fallback) const
    {
        auto it = tokens_.find(std::string(name));
        if (it == tokens_.end()) return fallback;
        if (auto* v = std::get_if<std::string>(&it->second)) return *v;
        return fallback;
    }

    void Theme::LoadDarkDefaults()
    {
        // -- Color tokens ---------------------------------------------------------
        SetToken("color-background", Color::RGBA(18, 18, 18));
        SetToken("color-surface", Color::RGBA(30, 30, 30));
        SetToken("color-surface-raised", Color::RGBA(42, 42, 42));
        SetToken("color-primary", Color::RGBA(100, 160, 255));
        SetToken("color-primary-hover", Color::RGBA(130, 180, 255));
        SetToken("color-primary-press", Color::RGBA(70, 130, 220));
        SetToken("color-on-primary", Color::RGBA(255, 255, 255));
        SetToken("color-text", Color::RGBA(230, 230, 230));
        SetToken("color-text-muted", Color::RGBA(140, 140, 140));
        SetToken("color-border", Color::RGBA(60, 60, 60));
        SetToken("color-danger", Color::RGBA(220, 70, 70));
        SetToken("color-success", Color::RGBA(70, 200, 100));

        // -- Spacing tokens -------------------------------------------------------
        SetToken("spacing-xs", 4.0f);
        SetToken("spacing-sm", 8.0f);
        SetToken("spacing-md", 16.0f);
        SetToken("spacing-lg", 24.0f);
        SetToken("spacing-xl", 32.0f);

        // -- Typography tokens ----------------------------------------------------
        SetToken("font-size-sm", 12.0f);
        SetToken("font-size-body", 14.0f);
        SetToken("font-size-md", 16.0f);
        SetToken("font-size-lg", 20.0f);
        SetToken("font-size-xl", 28.0f);
        SetToken("font-default", std::string(""));   // empty = first registered font

        // -- Radius tokens --------------------------------------------------------
        SetToken("radius-sm", 2.0f);
        SetToken("radius-md", 4.0f);
        SetToken("radius-lg", 8.0f);
        SetToken("radius-full", 9999.0f);

        // -- Per-widget defaults (wired to tokens) --------------------------------

        button.background = GetColor("color-surface-raised");
        button.backgroundHover = Color::RGBA(55, 55, 55);
        button.backgroundPress = Color::RGBA(25, 25, 25);
        button.foreground = GetColor("color-text");
        button.border = GetColor("color-border");
        button.borderWidth = 1.0f;
        button.borderRadius = GetFloat("radius-md");
        button.padding = Edges::Axes(GetFloat("spacing-sm"), GetFloat("spacing-md"));
        button.fontSize = GetFloat("font-size-body");

        label.foreground = GetColor("color-text");
        label.fontSize = GetFloat("font-size-body");
        label.textAlign = TextAlign::Left;

        slider.trackBackground = GetColor("color-surface");
        slider.trackFill = GetColor("color-primary");
        slider.thumb = GetColor("color-text");
        slider.thumbHover = GetColor("color-primary-hover");
        slider.trackHeight = 6.0f;
        slider.thumbRadius = 10.0f;
        slider.borderRadius = 3.0f;

        inputField.background = GetColor("color-surface");
        inputField.backgroundFocus = GetColor("color-background");
        inputField.foreground = GetColor("color-text");
        inputField.placeholder = GetColor("color-text-muted");
        inputField.border = GetColor("color-border");
        inputField.borderFocus = GetColor("color-primary");
        inputField.borderWidth = 1.0f;
        inputField.borderRadius = GetFloat("radius-md");
        inputField.padding = Edges::Axes(GetFloat("spacing-sm") - 2.0f, GetFloat("spacing-sm"));
        inputField.fontSize = GetFloat("font-size-body");
    }

} // namespace UI