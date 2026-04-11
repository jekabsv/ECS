#pragma once
#include "UITypes.h"
#include "UITheme.h"
#include "UINode.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <optional>

struct SDL_Renderer;
struct TTF_Font;
struct SDL_Texture;

namespace UI
{
    // -------------------------------------------------------------------------
    // Input snapshot passed to Update() each frame.
    // Keeps the UI system decoupled from your InputSystem.
    // -------------------------------------------------------------------------
    struct InputState
    {
        float    mouseX = 0.0f;
        float    mouseY = 0.0f;
        bool     mouseDown = false;  // primary button held
        bool     mousePressed = false;  // went down this frame
        bool     mouseReleased = false;  // went up this frame

        // Text input this frame (UTF-8). Filled by SDL_EVENT_TEXT_INPUT.
        std::string textInput;

        // Special keys (for InputField cursor / delete)
        bool keyBackspace = false;
        bool keyDelete = false;
        bool keyLeft = false;
        bool keyRight = false;
        bool keyHome = false;
        bool keyEnd = false;
        bool keyEnter = false;
        bool keyTab = false;
    };

    // -------------------------------------------------------------------------
    // UIContext — owns the retained node tree, runs layout + render each frame.
    //
    // Typical usage:
    //   Init()          — once, in State::Init()
    //   AddButton(...)  — once (or whenever structure changes)
    //   Update(input)   — every frame (processes input, runs layout, renders)
    //   Poll(handle)    — every frame to read interaction state
    //   SetText(...)    — whenever you need to mutate a node
    // -------------------------------------------------------------------------
    class Context
    {
    public:
        Context() = default;
        ~Context() = default;

        // -- Lifecycle --------------------------------------------------------

        /// Call once. Provide renderer + initial canvas size.
        void Init(SDL_Renderer* renderer, float canvasW, float canvasH);

        /// Call when the window is resized.
        void Resize(float canvasW, float canvasH);

        /// Call every frame: processes input, runs layout, renders.
        void Update(const InputState& input, float dt);

        // -- Tree building ----------------------------------------------------
        // All Add* functions return a stable NodeHandle.
        // Parent = NULL_HANDLE means "root level".

        /// Add a pure layout container.
        NodeHandle AddContainer(NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Add a text label.
        NodeHandle AddLabel(std::string_view text, NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Add a button with a text label.
        NodeHandle AddButton(std::string_view label, NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Add a slider. Value is in [min, max].
        NodeHandle AddSlider(float initialValue, float min, float max, NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Add a single-line text input field.
        NodeHandle AddInputField(std::string_view placeholder = "", NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Add an image widget.
        NodeHandle AddImage(SDL_Texture* texture, SDL_FRect sourceRect = { 0,0,0,0 }, NodeHandle parent = NULL_HANDLE, std::string_view id = "");

        /// Remove a node (and all its children) from the tree.
        void Remove(NodeHandle handle);

        /// Remove all children of a container, leaving the container itself.
        void ClearChildren(NodeHandle handle);

        // -- Node lookup ------------------------------------------------------

        /// Find a node by string id (returns NULL_HANDLE if not found).
        NodeHandle Find(std::string_view id) const;

        /// Returns true if the handle is valid and the node exists.
        bool Exists(NodeHandle handle) const;

        // -- Polling ----------------------------------------------------------

        /// Returns the current interaction state of a widget.
        InteractionState Poll(NodeHandle handle) const;

        /// Convenience: true for exactly the frame the button was released.
        bool IsClicked(NodeHandle handle) const;

        /// Returns true if a slider value changed this frame.
        bool SliderChanged(NodeHandle handle) const;

        /// Returns the current slider value.
        float GetSliderValue(NodeHandle handle) const;

        /// Returns true if the input field text changed this frame.
        bool InputChanged(NodeHandle handle) const;

        /// Returns the current text of an input field.
        const std::string& GetInputValue(NodeHandle handle) const;

        // -- Mutation ---------------------------------------------------------

        void SetText(NodeHandle handle, std::string_view text);
        void SetVisible(NodeHandle handle, bool visible);
        void SetEnabled(NodeHandle handle, bool enabled);
        void SetSliderValue(NodeHandle handle, float value);
        void SetInputValue(NodeHandle handle, std::string_view value);
        void SetImage(NodeHandle handle, SDL_Texture* texture, SDL_FRect sourceRect = { 0,0,0,0 });

        // Flex mutations
        void SetSize(NodeHandle handle, SizeValue width, SizeValue height);
        void SetFlexDirection(NodeHandle handle, FlexDirection dir);
        void SetJustify(NodeHandle handle, JustifyContent justify);
        void SetAlignItems(NodeHandle handle, AlignItems align);
        void SetFlexWrap(NodeHandle handle, FlexWrap wrap);
        void SetGap(NodeHandle handle, float gap, float columnGap = -1.0f); // -1 = same as gap
        void SetPadding(NodeHandle handle, Edges padding);
        void SetMargin(NodeHandle handle, Edges margin);
        void SetFlexGrow(NodeHandle handle, float grow);
        void SetFlexShrink(NodeHandle handle, float shrink);
        void SetFlexBasis(NodeHandle handle, SizeValue basis);

        // Style overrides
        void SetStyleOverride(NodeHandle handle, const StyleOverride& style);

        // -- Font -------------------------------------------------------------

        /// Register a font. fontName must match what's in your AssetManager.
        /// The context does not own the font pointer.
        void RegisterFont(std::string_view fontName, TTF_Font* font);

        // -- Theme ------------------------------------------------------------

        Theme& GetTheme() { return theme_; }
        const Theme& GetTheme() const { return theme_; }

    private:
        // -- Internal node management -----------------------------------------

        Node& GetNode(NodeHandle handle);
        const Node& GetNode(NodeHandle handle) const;
        NodeHandle  AllocNode(WidgetType type, NodeHandle parent, std::string_view id);
        void        RemoveRecursive(NodeHandle handle);

        // -- Per-frame passes -------------------------------------------------

        void ProcessInput(const InputState& input, float dt);
        void LayoutPass();
        void RenderPass();

        // -- Layout internals -------------------------------------------------

        struct FlexLine
        {
            std::vector<NodeHandle> items;
            float mainSize = 0.0f;
            float crossSize = 0.0f;
        };

        void  LayoutNode(Node& node, float availableW, float availableH);
        float ResolveSize(SizeValue sv, float available) const;
        void  MeasureNode(Node& node, float availableW, float availableH);

        // -- Render internals -------------------------------------------------

        void RenderNode(const Node& node);
        void RenderButton(const Node& node, const InputState& input);
        void RenderLabel(const Node& node);
        void RenderSlider(const Node& node);
        void RenderInput(const Node& node);
        void RenderImage(const Node& node);
        void RenderContainer(const Node& node);

        void DrawRect(SDL_FRect rect, Color color);
        void DrawRectBorder(SDL_FRect rect, Color color, float width);
        void DrawRoundedRect(SDL_FRect rect, Color color, float radius);
        void DrawText(const std::string& text, SDL_FRect rect, TTF_Font* font, Color color, TextAlign align);

        TTF_Font* ResolveFont(const Node& node) const;
        float     ResolveFontSize(const Node& node) const;

        // -- State ------------------------------------------------------------

        SDL_Renderer* renderer_ = nullptr;
        float         canvasW_ = 0.0f;
        float         canvasH_ = 0.0f;

        Theme theme_;

        // Node storage: handle → Node. Handle 0 is reserved (NULL_HANDLE).
        // Handles are monotonically increasing; slots are reused on Remove().
        std::unordered_map<NodeHandle, Node> nodes_;
        std::vector<NodeHandle>              freeHandles_;
        NodeHandle                           nextHandle_ = 1;

        // Root-level nodes (those with parent == NULL_HANDLE), in order.
        std::vector<NodeHandle> roots_;

        // String-id lookup
        std::unordered_map<std::string, NodeHandle> idMap_;

        // Font registry: name → TTF_Font* (not owned)
        std::unordered_map<std::string, TTF_Font*> fonts_;

        // Input state snapshot (kept for render pass to use for hover etc.)
        InputState currentInput_;

        // Which node currently has keyboard focus (InputField)
        NodeHandle focusedNode_ = NULL_HANDLE;

        // Fallback empty string for GetInputValue on invalid handles
        static const std::string emptyString_;
    };

} // namespace UI