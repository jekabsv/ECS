#pragma once
#include "UITypes.h"
#include "UITheme.h"
#include "UINode.h"
#include "struct.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <SDL3/SDL_video.h>

class Renderer;
struct TTF_Font;

namespace UI
{
    struct InputState
    {
        float    mouseX = 0.0f;
        float    mouseY = 0.0f;
        bool     mouseDown = false;
        bool     mousePressed = false;
        bool     mouseReleased = false;
        std::string textInput;
        bool keyBackspace = false;
        bool keyDelete = false;
        bool keyLeft = false;
        bool keyRight = false;
        bool keyHome = false;
        bool keyEnd = false;
        bool keyEnter = false;
        bool keyTab = false;
    };

    class Context
    {
    public:
        Context() = default;
        ~Context() = default;

        // -- Lifecycle --------------------------------------------------------
        void Init(Renderer* renderer, float canvasW, float canvasH, SDL_Window* window);
        void Resize(float canvasW, float canvasH);
        void Update(const InputState& input, float dt);

        // -- Tree building ----------------------------------------------------
        NodeHandle AddContainer(NodeHandle parent = NULL_HANDLE, std::string_view id = "");
        NodeHandle AddLabel(std::string_view text, NodeHandle parent = NULL_HANDLE, std::string_view id = "");
        NodeHandle AddButton(std::string_view label, NodeHandle parent = NULL_HANDLE, std::string_view id = "");
        NodeHandle AddSlider(float initialValue, float min, float max, NodeHandle parent = NULL_HANDLE, std::string_view id = "");
        NodeHandle AddInputField(std::string_view placeholder = "", NodeHandle parent = NULL_HANDLE, std::string_view id = "");
        NodeHandle AddImage(StringId textureId, SDL_FRect sourceRect = { 0,0,0,0 }, NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        void Remove(NodeHandle handle);
        void ClearChildren(NodeHandle handle);

        // -- Node lookup ------------------------------------------------------
        NodeHandle Find(std::string_view id) const;
        bool       Exists(NodeHandle handle)   const;

        // -- Polling ----------------------------------------------------------
        InteractionState   Poll(NodeHandle handle) const;
        bool               IsClicked(NodeHandle handle) const;
        bool               SliderChanged(NodeHandle handle) const;
        float              GetSliderValue(NodeHandle handle) const;
        bool               InputChanged(NodeHandle handle) const;
        const std::string& GetInputValue(NodeHandle handle) const;

        // -- Mutation ---------------------------------------------------------
        void SetText(NodeHandle handle, std::string_view text);
        void SetVisible(NodeHandle handle, bool visible);
        void SetEnabled(NodeHandle handle, bool enabled);
        void SetSliderValue(NodeHandle handle, float value);
        void SetInputValue(NodeHandle handle, std::string_view value);
        void SetImage(NodeHandle handle, StringId textureId, SDL_FRect sourceRect = { 0,0,0,0 });

        void SetSize(NodeHandle handle, SizeValue width, SizeValue height);
        void SetFlexDirection(NodeHandle handle, FlexDirection dir);
        void SetJustify(NodeHandle handle, JustifyContent justify);
        void SetAlignItems(NodeHandle handle, AlignItems align);
        void SetFlexWrap(NodeHandle handle, FlexWrap wrap);
        void SetGap(NodeHandle handle, float gap, float columnGap = -1.0f);
        void SetPadding(NodeHandle handle, Edges padding);
        void SetMargin(NodeHandle handle, Edges margin);
        void SetFlexGrow(NodeHandle handle, float grow);
        void SetFlexShrink(NodeHandle handle, float shrink);
        void SetFlexBasis(NodeHandle handle, SizeValue basis);
        void SetStyleOverride(NodeHandle handle, const StyleOverride& style);

        // -- Font -------------------------------------------------------------
        void RegisterFont(std::string_view fontName, TTF_Font* font, StringId gpuFontId);

        // -- Theme ------------------------------------------------------------
        Theme& GetTheme() { return theme_; }
        const Theme& GetTheme() const { return theme_; }

        // -- Per-frame passes -------------------------------------------------
        void ProcessInput(const InputState& input, float dt);
        void LayoutPass();
        void RenderPass();

    private:

        SDL_Window* window_ = nullptr;

        Node& GetNode(NodeHandle handle);
        const Node& GetNode(NodeHandle handle) const;
        NodeHandle  AllocNode(WidgetType type, NodeHandle parent, std::string_view id);
        void        RemoveRecursive(NodeHandle handle);

        struct FlexLine
        {
            std::vector<NodeHandle> items;
            float mainSize = 0.0f;
            float crossSize = 0.0f;
        };

        void  LayoutNode(Node& node, float availableW, float availableH);
        float ResolveSize(SizeValue sv, float available) const;
        void  MeasureNode(Node& node, float availableW, float availableH);

        void RenderNode(const Node& node, float z);
        void RenderContainer(const Node& node, float z);
        void RenderButton(const Node& node, float z);
        void RenderLabel(const Node& node, float z);
        void RenderSlider(const Node& node, float z);
        void RenderInput(const Node& node, float z);
        void RenderImage(const Node& node, float z);

        void DrawRect(SDL_FRect rect, Color color, float z);
        void DrawRectBorder(SDL_FRect rect, Color color, float width, float z);
        void DrawRoundedRect(SDL_FRect rect, Color color, float radius, float z);
        void DrawText(const std::string& text, SDL_FRect rect, StringId fontId, Color color, TextAlign align, float z);

        TTF_Font* ResolveFont(const Node& node) const;
        TTF_Font* ResolveFont_ById(StringId id)     const;
        StringId  ResolveFontId(const Node& node) const;

        Renderer* renderer_ = nullptr;
        float     canvasW_ = 0.0f;
        float     canvasH_ = 0.0f;

        Theme theme_;

        std::unordered_map<NodeHandle, Node>        nodes_;
        std::vector<NodeHandle>                     freeHandles_;
        NodeHandle                                  nextHandle_ = 1;
        std::vector<NodeHandle>                     roots_;
        std::unordered_map<std::string, NodeHandle> idMap_;

        std::unordered_map<std::string, TTF_Font*> fonts_;
        std::unordered_map<std::string, StringId>  fontIds_;

        InputState currentInput_;
        NodeHandle focusedNode_ = NULL_HANDLE;

        static const std::string emptyString_;
    };

} // namespace UI