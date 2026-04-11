#include "UIContext.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace UI
{

    const std::string Context::emptyString_;

    // =============================================================================
    // Lifecycle
    // =============================================================================

    void Context::Init(SDL_Renderer* renderer, float canvasW, float canvasH)
    {
        renderer_ = renderer;
        canvasW_ = canvasW;
        canvasH_ = canvasH;
        theme_.LoadDarkDefaults();
    }

    void Context::Resize(float canvasW, float canvasH)
    {
        canvasW_ = canvasW;
        canvasH_ = canvasH;
    }

    void Context::Update(const InputState& input, float dt)
    {
        currentInput_ = input;
        ProcessInput(input, dt);
        LayoutPass();
        RenderPass();
    }

    // =============================================================================
    // Tree building
    // =============================================================================

    NodeHandle Context::AllocNode(WidgetType type, NodeHandle parent, std::string_view id)
    {
        NodeHandle handle;
        if (!freeHandles_.empty())
        {
            handle = freeHandles_.back();
            freeHandles_.pop_back();
        }
        else
        {
            handle = nextHandle_++;
        }

        Node node;
        node.handle = handle;
        node.type = type;
        node.parent = parent;
        node.id = std::string(id);
        nodes_[handle] = std::move(node);

        if (!id.empty())
            idMap_[std::string(id)] = handle;

        if (parent == NULL_HANDLE)
        {
            roots_.push_back(handle);
        }
        else
        {
            auto it = nodes_.find(parent);
            if (it != nodes_.end())
                it->second.children.push_back(handle);
        }

        return handle;
    }

    NodeHandle Context::AddContainer(NodeHandle parent, std::string_view id)
    {
        return AllocNode(WidgetType::Container, parent, id);
    }

    NodeHandle Context::AddLabel(std::string_view text, NodeHandle parent, std::string_view id)
    {
        NodeHandle h = AllocNode(WidgetType::Label, parent, id);
        nodes_[h].widget.text = std::string(text);
        return h;
    }

    NodeHandle Context::AddButton(std::string_view label, NodeHandle parent, std::string_view id)
    {
        NodeHandle h = AllocNode(WidgetType::Button, parent, id);
        nodes_[h].widget.text = std::string(label);
        return h;
    }

    NodeHandle Context::AddSlider(float initialValue, float min, float max, NodeHandle parent, std::string_view id)
    {
        NodeHandle h = AllocNode(WidgetType::Slider, parent, id);
        auto& w = nodes_[h].widget;
        w.sliderValue = std::clamp(initialValue, min, max);
        w.sliderMin = min;
        w.sliderMax = max;
        return h;
    }

    NodeHandle Context::AddInputField(std::string_view placeholder, NodeHandle parent, std::string_view id)
    {
        NodeHandle h = AllocNode(WidgetType::InputField, parent, id);
        nodes_[h].widget.placeholder = std::string(placeholder);
        return h;
    }

    NodeHandle Context::AddImage(SDL_Texture* texture, SDL_FRect sourceRect, NodeHandle parent, std::string_view id)
    {
        NodeHandle h = AllocNode(WidgetType::Image, parent, id);
        auto& w = nodes_[h].widget;
        w.texture = texture;
        w.textureRect = sourceRect;
        return h;
    }

    void Context::RemoveRecursive(NodeHandle handle)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;

        // Recurse into children first
        for (NodeHandle child : it->second.children)
            RemoveRecursive(child);

        // Remove from id map
        if (!it->second.id.empty())
            idMap_.erase(it->second.id);

        // Clear focus if needed
        if (focusedNode_ == handle)
            focusedNode_ = NULL_HANDLE;

        nodes_.erase(it);
        freeHandles_.push_back(handle);
    }

    void Context::Remove(NodeHandle handle)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;

        NodeHandle parent = it->second.parent;

        // Detach from parent's children list
        if (parent == NULL_HANDLE)
        {
            roots_.erase(std::remove(roots_.begin(), roots_.end(), handle), roots_.end());
        }
        else
        {
            auto pit = nodes_.find(parent);
            if (pit != nodes_.end())
            {
                auto& ch = pit->second.children;
                ch.erase(std::remove(ch.begin(), ch.end(), handle), ch.end());
            }
        }

        RemoveRecursive(handle);
    }

    void Context::ClearChildren(NodeHandle handle)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;

        std::vector<NodeHandle> children = it->second.children; // copy
        for (NodeHandle child : children)
            RemoveRecursive(child);
        it->second.children.clear();
    }

    // =============================================================================
    // Node lookup
    // =============================================================================

    Node& Context::GetNode(NodeHandle handle)
    {
        auto it = nodes_.find(handle);
        assert(it != nodes_.end() && "UIContext::GetNode — invalid handle");
        return it->second;
    }

    const Node& Context::GetNode(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        assert(it != nodes_.end() && "UIContext::GetNode — invalid handle");
        return it->second;
    }

    NodeHandle Context::Find(std::string_view id) const
    {
        auto it = idMap_.find(std::string(id));
        if (it == idMap_.end()) return NULL_HANDLE;
        return it->second;
    }

    bool Context::Exists(NodeHandle handle) const
    {
        return nodes_.count(handle) > 0;
    }

    // =============================================================================
    // Polling
    // =============================================================================

    InteractionState Context::Poll(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return InteractionState::None;
        return it->second.widget.interactionState;
    }

    bool Context::IsClicked(NodeHandle handle) const
    {
        return Poll(handle) == InteractionState::Released;
    }

    bool Context::SliderChanged(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return false;
        return it->second.widget.sliderChanged;
    }

    float Context::GetSliderValue(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return 0.0f;
        return it->second.widget.sliderValue;
    }

    bool Context::InputChanged(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return false;
        return it->second.widget.inputChanged;
    }

    const std::string& Context::GetInputValue(NodeHandle handle) const
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return emptyString_;
        return it->second.widget.inputValue;
    }

    // =============================================================================
    // Mutation
    // =============================================================================

    void Context::SetText(NodeHandle handle, std::string_view text)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.widget.text = std::string(text);
    }

    void Context::SetVisible(NodeHandle handle, bool visible)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.visible = visible;
    }

    void Context::SetEnabled(NodeHandle handle, bool enabled)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.enabled = enabled;
    }

    void Context::SetSliderValue(NodeHandle handle, float value)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        auto& w = it->second.widget;
        w.sliderValue = std::clamp(value, w.sliderMin, w.sliderMax);
    }

    void Context::SetInputValue(NodeHandle handle, std::string_view value)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        auto& w = it->second.widget;
        w.inputValue = std::string(value);
        w.cursorPos = (uint32_t)w.inputValue.size();
    }

    void Context::SetImage(NodeHandle handle, SDL_Texture* texture, SDL_FRect sourceRect)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.widget.texture = texture;
        it->second.widget.textureRect = sourceRect;
    }

    void Context::SetSize(NodeHandle handle, SizeValue width, SizeValue height)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.width = width;
        it->second.flex.height = height;
    }

    void Context::SetFlexDirection(NodeHandle handle, FlexDirection dir)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.direction = dir;
    }

    void Context::SetJustify(NodeHandle handle, JustifyContent justify)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.justifyContent = justify;
    }

    void Context::SetAlignItems(NodeHandle handle, AlignItems align)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.alignItems = align;
    }

    void Context::SetFlexWrap(NodeHandle handle, FlexWrap wrap)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.wrap = wrap;
    }

    void Context::SetGap(NodeHandle handle, float gap, float columnGap)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.gap = gap;
        it->second.flex.columnGap = (columnGap < 0.0f) ? gap : columnGap;
    }

    void Context::SetPadding(NodeHandle handle, Edges padding)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.padding = padding;
    }

    void Context::SetMargin(NodeHandle handle, Edges margin)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.margin = margin;
    }

    void Context::SetFlexGrow(NodeHandle handle, float grow)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.flexGrow = grow;
    }

    void Context::SetFlexShrink(NodeHandle handle, float shrink)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.flexShrink = shrink;
    }

    void Context::SetFlexBasis(NodeHandle handle, SizeValue basis)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.flex.flexBasis = basis;
    }

    void Context::SetStyleOverride(NodeHandle handle, const StyleOverride& style)
    {
        auto it = nodes_.find(handle);
        if (it == nodes_.end()) return;
        it->second.style = style;
    }

    void Context::RegisterFont(std::string_view fontName, TTF_Font* font)
    {
        fonts_[std::string(fontName)] = font;
    }

    // =============================================================================
    // Input processing
    // =============================================================================

    static bool PointInRect(float px, float py, const SDL_FRect& r)
    {
        return px >= r.x && px < r.x + r.w &&
            py >= r.y && py < r.y + r.h;
    }

    void Context::ProcessInput(const InputState& input, float dt)
    {
        // Reset per-frame flags on all nodes
        for (auto& [h, node] : nodes_)
        {
            node.widget.sliderChanged = false;
            node.widget.inputChanged = false;
            node.widget.interactionState = InteractionState::None;
        }

        // Handle focus click-away
        if (input.mousePressed && focusedNode_ != NULL_HANDLE)
        {
            auto it = nodes_.find(focusedNode_);
            if (it != nodes_.end())
            {
                if (!PointInRect(input.mouseX, input.mouseY, it->second.computedRect))
                {
                    it->second.widget.focused = false;
                    focusedNode_ = NULL_HANDLE;
                    SDL_StopTextInput(nullptr);
                }
            }
        }

        // Process each node
        for (auto& [h, node] : nodes_)
        {
            if (!node.visible || !node.enabled) continue;

            bool hovered = PointInRect(input.mouseX, input.mouseY, node.computedRect);

            switch (node.type)
            {
                // -----------------------------------------------------------------
            case WidgetType::Button:
            {
                if (!hovered)
                {
                    node.widget.interactionState = InteractionState::None;
                    break;
                }
                if (input.mouseDown && !input.mouseReleased)
                    node.widget.interactionState = InteractionState::Pressed;
                else if (input.mouseReleased)
                    node.widget.interactionState = InteractionState::Released;
                else
                    node.widget.interactionState = InteractionState::Hovered;
                break;
            }

            // -----------------------------------------------------------------
            case WidgetType::Slider:
            {
                bool dragging = hovered && input.mouseDown;

                // Allow continuing drag even if mouse left the rect
                // (track by checking if this node was pressed last frame)
                bool wasPressedLastFrame = (node.widget.interactionState == InteractionState::Pressed);
                if (wasPressedLastFrame && input.mouseDown)
                    dragging = true;

                if (dragging)
                {
                    node.widget.interactionState = InteractionState::Pressed;

                    // Map mouse X into slider value
                    const SDL_FRect& r = node.computedRect;
                    const float thumbR = theme_.slider.thumbRadius;
                    float trackLeft = r.x + thumbR;
                    float trackRight = r.x + r.w - thumbR;
                    float trackLen = trackRight - trackLeft;

                    if (trackLen > 0.0f)
                    {
                        float t = std::clamp((input.mouseX - trackLeft) / trackLen, 0.0f, 1.0f);
                        float newVal = node.widget.sliderMin + t * (node.widget.sliderMax - node.widget.sliderMin);
                        if (newVal != node.widget.sliderValue)
                        {
                            node.widget.sliderValue = newVal;
                            node.widget.sliderChanged = true;
                        }
                    }
                }
                else if (hovered)
                {
                    node.widget.interactionState = InteractionState::Hovered;
                }
                break;
            }

            // -----------------------------------------------------------------
            case WidgetType::InputField:
            {
                // Click to focus
                if (hovered && input.mousePressed)
                {
                    if (focusedNode_ != h)
                    {
                        // Unfocus previous
                        if (focusedNode_ != NULL_HANDLE)
                        {
                            auto pit = nodes_.find(focusedNode_);
                            if (pit != nodes_.end())
                                pit->second.widget.focused = false;
                        }
                        focusedNode_ = h;
                        node.widget.focused = true;
                        SDL_StartTextInput(nullptr);
                    }
                    // Place cursor at end (could be improved with click-to-position)
                    node.widget.cursorPos = (uint32_t)node.widget.inputValue.size();
                }

                if (node.widget.focused)
                {
                    node.widget.interactionState = InteractionState::Focused;

                    std::string& val = node.widget.inputValue;
                    uint32_t& cur = node.widget.cursorPos;
                    bool changed = false;

                    // Text input
                    if (!input.textInput.empty())
                    {
                        val.insert(cur, input.textInput);
                        cur += (uint32_t)input.textInput.size();
                        changed = true;
                    }

                    // Backspace
                    if (input.keyBackspace && cur > 0)
                    {
                        val.erase(cur - 1, 1);
                        cur--;
                        changed = true;
                    }

                    // Delete
                    if (input.keyDelete && cur < val.size())
                    {
                        val.erase(cur, 1);
                        changed = true;
                    }

                    // Cursor movement
                    if (input.keyLeft && cur > 0)           cur--;
                    if (input.keyRight && cur < val.size())  cur++;
                    if (input.keyHome)                        cur = 0;
                    if (input.keyEnd)                         cur = (uint32_t)val.size();

                    // Tab — move focus to next input field (basic impl)
                    if (input.keyTab)
                    {
                        node.widget.focused = false;
                        focusedNode_ = NULL_HANDLE;
                        SDL_StopTextInput(nullptr);
                    }

                    node.widget.inputChanged = changed;
                }
                else if (hovered)
                {
                    node.widget.interactionState = InteractionState::Hovered;
                }
                break;
            }

            default:
                break;
            }
        }
    }

    // =============================================================================
    // Layout pass — full flexbox
    // =============================================================================

    float Context::ResolveSize(SizeValue sv, float available) const
    {
        switch (sv.unit)
        {
        case SizeUnit::Px:      return sv.value;
        case SizeUnit::Percent: return available * sv.value / 100.0f;
        case SizeUnit::Auto:    return -1.0f;   // caller must handle
        }
        return -1.0f;
    }

    // Measure intrinsic (content) size of a leaf node before flex layout.
    void Context::MeasureNode(Node& node, float availableW, float availableH)
    {
        // Only leaves need intrinsic measurement
        if (!node.children.empty()) return;

        float w = ResolveSize(node.flex.width, availableW);
        float h = ResolveSize(node.flex.height, availableH);

        switch (node.type)
        {
        case WidgetType::Label:
        case WidgetType::Button:
        {
            TTF_Font* font = ResolveFont(node);
            if (font && !node.widget.text.empty())
            {
                int tw = 0, th = 0;
                TTF_GetStringSize(font, node.widget.text.c_str(), 0, &tw, &th);

                const Edges& pad = node.style.padding.value_or(
                    node.type == WidgetType::Button
                    ? theme_.button.padding
                    : Edges::All(0.0f)
                );

                if (w < 0.0f) w = (float)tw + pad.left + pad.right;
                if (h < 0.0f) h = (float)th + pad.top + pad.bottom;
            }
            else
            {
                if (w < 0.0f) w = 0.0f;
                if (h < 0.0f) h = 0.0f;
            }
            break;
        }

        case WidgetType::Slider:
        {
            if (w < 0.0f) w = 200.0f;   // sensible default width
            if (h < 0.0f) h = theme_.slider.thumbRadius * 2.0f;
            break;
        }

        case WidgetType::InputField:
        {
            TTF_Font* font = ResolveFont(node);
            int th = 0;
            if (font) TTF_GetStringSize(font, "M", 0, nullptr, &th);
            const Edges& pad = node.style.padding.value_or(theme_.inputField.padding);
            if (w < 0.0f) w = 200.0f;
            if (h < 0.0f) h = (float)th + pad.top + pad.bottom;
            break;
        }

        case WidgetType::Image:
        {
            if (node.widget.texture && w < 0.0f && h < 0.0f)
            {
                float tw = 0, th = 0;
                SDL_GetTextureSize(node.widget.texture, &tw, &th);
                if (node.widget.textureRect.w > 0) tw = node.widget.textureRect.w;
                if (node.widget.textureRect.h > 0) th = node.widget.textureRect.h;
                w = tw; h = th;
            }
            if (w < 0.0f) w = 0.0f;
            if (h < 0.0f) h = 0.0f;
            break;
        }

        default:
            if (w < 0.0f) w = 0.0f;
            if (h < 0.0f) h = 0.0f;
            break;
        }

        // Apply min/max
        float minW = ResolveSize(node.flex.minWidth, availableW); if (minW < 0) minW = 0;
        float minH = ResolveSize(node.flex.minHeight, availableH); if (minH < 0) minH = 0;
        float maxW = ResolveSize(node.flex.maxWidth, availableW);
        float maxH = ResolveSize(node.flex.maxHeight, availableH);

        w = std::max(w, minW); if (maxW >= 0) w = std::min(w, maxW);
        h = std::max(h, minH); if (maxH >= 0) h = std::min(h, maxH);

        node.computedRect.w = w;
        node.computedRect.h = h;
    }

    void Context::LayoutNode(Node& node, float availableW, float availableH)
    {
        if (!node.visible) return;

        const Edges& margin = node.flex.margin;

        // Resolve own size
        float selfW = ResolveSize(node.flex.width, availableW);
        float selfH = ResolveSize(node.flex.height, availableH);

        // If auto, we'll determine from content after children are measured
        bool autoW = (selfW < 0.0f);
        bool autoH = (selfH < 0.0f);

        if (!autoW) node.computedRect.w = selfW;
        if (!autoH) node.computedRect.h = selfH;

        // Leaf nodes — just measure
        if (node.children.empty())
        {
            MeasureNode(node, availableW, availableH);
            return;
        }

        // -----------------------------------------------------------------
        // Container: flex layout
        // -----------------------------------------------------------------

        const Edges& pad = node.flex.padding;
        bool isRow = (node.flex.direction == FlexDirection::Row ||
            node.flex.direction == FlexDirection::RowReverse);

        float innerW = (autoW ? availableW : node.computedRect.w) - pad.left - pad.right;
        float innerH = (autoH ? availableH : node.computedRect.h) - pad.top - pad.bottom;
        innerW = std::max(innerW, 0.0f);
        innerH = std::max(innerH, 0.0f);

        // --- Step 1: measure children (base sizes) ---
        for (NodeHandle ch : node.children)
        {
            auto it = nodes_.find(ch);
            if (it == nodes_.end() || !it->second.visible) continue;
            Node& child = it->second;

            float childAvailW = isRow ? innerW : innerW;
            float childAvailH = isRow ? innerH : innerH;

            // Resolve flex-basis first
            float basis = ResolveSize(child.flex.flexBasis, isRow ? innerW : innerH);
            if (basis >= 0.0f)
            {
                if (isRow) child.computedRect.w = basis;
                else       child.computedRect.h = basis;
            }

            LayoutNode(child, childAvailW, childAvailH);
        }

        // --- Step 2: collect visible children ---
        std::vector<NodeHandle> visibleChildren;
        for (NodeHandle ch : node.children)
        {
            auto it = nodes_.find(ch);
            if (it != nodes_.end() && it->second.visible)
                visibleChildren.push_back(ch);
        }

        // --- Step 3: build flex lines (wrapping) ---
        float gap = node.flex.gap;
        float crossGap = (node.flex.columnGap > 0.0f) ? node.flex.columnGap : gap;

        float mainAvail = isRow ? innerW : innerH;
        float crossAvail = isRow ? innerH : innerW;

        std::vector<FlexLine> lines;
        {
            FlexLine current;
            for (NodeHandle ch : visibleChildren)
            {
                Node& child = GetNode(ch);
                float mainSize = isRow ? child.computedRect.w : child.computedRect.h;
                mainSize += (isRow ? child.flex.margin.left + child.flex.margin.right
                    : child.flex.margin.top + child.flex.margin.bottom);

                float usedMain = current.mainSize + (current.items.empty() ? 0.0f : gap);

                if (node.flex.wrap != FlexWrap::NoWrap
                    && !current.items.empty()
                    && usedMain + mainSize > mainAvail)
                {
                    lines.push_back(current);
                    current = FlexLine{};
                }

                current.items.push_back(ch);
                current.mainSize += (current.items.size() == 1 ? 0.0f : gap) + mainSize;

                float crossSize = isRow ? child.computedRect.h : child.computedRect.w;
                crossSize += (isRow ? child.flex.margin.top + child.flex.margin.bottom
                    : child.flex.margin.left + child.flex.margin.right);
                current.crossSize = std::max(current.crossSize, crossSize);
            }
            if (!current.items.empty())
                lines.push_back(current);
        }

        // --- Step 4: flex grow / shrink per line ---
        for (auto& line : lines)
        {
            float freeSpace = mainAvail - line.mainSize;

            if (freeSpace > 0.0f)
            {
                // Grow
                float totalGrow = 0.0f;
                for (NodeHandle ch : line.items)
                    totalGrow += GetNode(ch).flex.flexGrow;
                if (totalGrow > 0.0f)
                {
                    for (NodeHandle ch : line.items)
                    {
                        Node& child = GetNode(ch);
                        float share = freeSpace * (child.flex.flexGrow / totalGrow);
                        if (isRow) child.computedRect.w += share;
                        else       child.computedRect.h += share;
                    }
                    line.mainSize = mainAvail;
                }
            }
            else if (freeSpace < 0.0f)
            {
                // Shrink
                float totalShrink = 0.0f;
                for (NodeHandle ch : line.items)
                    totalShrink += GetNode(ch).flex.flexShrink;
                if (totalShrink > 0.0f)
                {
                    float overflow = -freeSpace;
                    for (NodeHandle ch : line.items)
                    {
                        Node& child = GetNode(ch);
                        float share = overflow * (child.flex.flexShrink / totalShrink);
                        if (isRow) child.computedRect.w = std::max(0.0f, child.computedRect.w - share);
                        else       child.computedRect.h = std::max(0.0f, child.computedRect.h - share);
                    }
                    line.mainSize = mainAvail;
                }
            }
        }

        // --- Step 5: cross-axis sizing (align-items / align-self) ---
        for (auto& line : lines)
        {
            for (NodeHandle ch : line.items)
            {
                Node& child = GetNode(ch);
                AlignItems effective = theme_.button.textAlign == TextAlign::Center
                    ? node.flex.alignItems
                    : node.flex.alignItems; // placeholder

                AlignSelf selfAlign = child.flex.alignSelf;
                AlignItems resolvedAlign = node.flex.alignItems;
                if (selfAlign == AlignSelf::FlexStart)   resolvedAlign = AlignItems::FlexStart;
                else if (selfAlign == AlignSelf::FlexEnd) resolvedAlign = AlignItems::FlexEnd;
                else if (selfAlign == AlignSelf::Center)  resolvedAlign = AlignItems::Center;
                else if (selfAlign == AlignSelf::Stretch) resolvedAlign = AlignItems::Stretch;

                if (resolvedAlign == AlignItems::Stretch)
                {
                    if (isRow) child.computedRect.h = line.crossSize
                        - child.flex.margin.top - child.flex.margin.bottom;
                    else       child.computedRect.w = line.crossSize
                        - child.flex.margin.left - child.flex.margin.right;
                }
            }
        }

        // --- Step 6: position children (justify-content + cross placement) ---

        // Total cross size for align-content
        float totalCrossUsed = 0.0f;
        for (std::size_t li = 0; li < lines.size(); li++)
            totalCrossUsed += lines[li].crossSize + (li > 0 ? crossGap : 0.0f);

        float crossOffset = 0.0f;
        // align-content
        if (node.flex.alignContent == AlignContent::Center)
            crossOffset = (crossAvail - totalCrossUsed) * 0.5f;
        else if (node.flex.alignContent == AlignContent::FlexEnd)
            crossOffset = crossAvail - totalCrossUsed;

        bool reverseMain = (node.flex.direction == FlexDirection::RowReverse ||
            node.flex.direction == FlexDirection::ColumnReverse);
        bool reverseLines = (node.flex.wrap == FlexWrap::WrapReverse);
        if (reverseLines) crossOffset = crossAvail - totalCrossUsed - crossOffset;

        float baseX = node.computedRect.x + pad.left;
        float baseY = node.computedRect.y + pad.top;

        for (auto& line : lines)
        {
            float freeMain = mainAvail - line.mainSize;

            float mainOffset = 0.0f;
            float spaceBetween = 0.0f;
            float spaceAround = 0.0f;
            int   n = (int)line.items.size();

            switch (node.flex.justifyContent)
            {
            case JustifyContent::FlexStart:   mainOffset = 0.0f; break;
            case JustifyContent::FlexEnd:     mainOffset = freeMain; break;
            case JustifyContent::Center:      mainOffset = freeMain * 0.5f; break;
            case JustifyContent::SpaceBetween:
                spaceBetween = (n > 1) ? freeMain / (n - 1) : 0.0f; break;
            case JustifyContent::SpaceAround:
                spaceAround = (n > 0) ? freeMain / n : 0.0f;
                mainOffset = spaceAround * 0.5f; break;
            case JustifyContent::SpaceEvenly:
                spaceAround = (n > 0) ? freeMain / (n + 1) : 0.0f;
                mainOffset = spaceAround; break;
            }

            if (reverseMain) mainOffset = mainAvail - mainOffset;

            for (NodeHandle ch : line.items)
            {
                Node& child = GetNode(ch);

                float childMain = isRow ? child.computedRect.w : child.computedRect.h;
                float childCross = isRow ? child.computedRect.h : child.computedRect.w;
                float mMarginA = isRow ? child.flex.margin.left : child.flex.margin.top;
                float mMarginB = isRow ? child.flex.margin.right : child.flex.margin.bottom;
                float cMarginA = isRow ? child.flex.margin.top : child.flex.margin.left;
                float cMarginB = isRow ? child.flex.margin.bottom : child.flex.margin.right;

                // Cross position
                float crossPos = 0.0f;
                AlignSelf selfAlign = child.flex.alignSelf;
                AlignItems resolvedAlign = node.flex.alignItems;
                if (selfAlign == AlignSelf::FlexStart)   resolvedAlign = AlignItems::FlexStart;
                else if (selfAlign == AlignSelf::FlexEnd) resolvedAlign = AlignItems::FlexEnd;
                else if (selfAlign == AlignSelf::Center)  resolvedAlign = AlignItems::Center;
                else if (selfAlign == AlignSelf::Stretch) resolvedAlign = AlignItems::Stretch;

                switch (resolvedAlign)
                {
                case AlignItems::FlexStart: crossPos = cMarginA; break;
                case AlignItems::FlexEnd:   crossPos = line.crossSize - childCross - cMarginB; break;
                case AlignItems::Center:    crossPos = (line.crossSize - childCross) * 0.5f; break;
                case AlignItems::Stretch:   crossPos = cMarginA; break;
                default:                    crossPos = cMarginA; break;
                }

                if (reverseLines)
                    crossPos = line.crossSize - crossPos - childCross;

                float finalMain = mainOffset + mMarginA;
                float finalCross = crossOffset + crossPos;

                if (isRow)
                {
                    child.computedRect.x = baseX + finalMain;
                    child.computedRect.y = baseY + finalCross;
                }
                else
                {
                    child.computedRect.x = baseX + finalCross;
                    child.computedRect.y = baseY + finalMain;
                }

                if (reverseMain)
                    mainOffset -= childMain + mMarginB + gap + spaceBetween + spaceAround;
                else
                    mainOffset += childMain + mMarginB + gap + spaceBetween + spaceAround;
            }

            crossOffset += line.crossSize + crossGap;
            if (reverseLines) crossOffset = -crossOffset;
        }

        // --- Step 7: resolve auto size of container ---
        if (autoW || autoH)
        {
            float contentW = 0.0f, contentH = 0.0f;
            for (NodeHandle ch : visibleChildren)
            {
                const Node& child = GetNode(ch);
                contentW = std::max(contentW, child.computedRect.x - node.computedRect.x + child.computedRect.w + child.flex.margin.right);
                contentH = std::max(contentH, child.computedRect.y - node.computedRect.y + child.computedRect.h + child.flex.margin.bottom);
            }
            if (autoW) node.computedRect.w = contentW + pad.left + pad.right;
            if (autoH) node.computedRect.h = contentH + pad.top + pad.bottom;
        }

        // Apply min/max to self
        float minW = ResolveSize(node.flex.minWidth, availableW); if (minW < 0) minW = 0;
        float minH = ResolveSize(node.flex.minHeight, availableH); if (minH < 0) minH = 0;
        float maxW = ResolveSize(node.flex.maxWidth, availableW);
        float maxH = ResolveSize(node.flex.maxHeight, availableH);
        node.computedRect.w = std::max(node.computedRect.w, minW);
        node.computedRect.h = std::max(node.computedRect.h, minH);
        if (maxW >= 0) node.computedRect.w = std::min(node.computedRect.w, maxW);
        if (maxH >= 0) node.computedRect.h = std::min(node.computedRect.h, maxH);
    }

    void Context::LayoutPass()
    {
        for (NodeHandle root : roots_)
        {
            auto it = nodes_.find(root);
            if (it == nodes_.end()) continue;
            Node& node = it->second;

            // Root nodes fill canvas if size is Auto
            if (node.flex.width.unit == SizeUnit::Auto)
            {
                node.computedRect.x = 0.0f;
                node.computedRect.w = canvasW_;
            }
            else
            {
                node.computedRect.w = ResolveSize(node.flex.width, canvasW_);
                node.computedRect.x = node.flex.margin.left;
            }

            if (node.flex.height.unit == SizeUnit::Auto)
            {
                node.computedRect.y = 0.0f;
                node.computedRect.h = canvasH_;
            }
            else
            {
                node.computedRect.h = ResolveSize(node.flex.height, canvasH_);
                node.computedRect.y = node.flex.margin.top;
            }

            LayoutNode(node, canvasW_, canvasH_);
        }
    }

    // =============================================================================
    // Font resolution
    // =============================================================================

    TTF_Font* Context::ResolveFont(const Node& node) const
    {
        // 1. Per-node override
        if (node.style.fontName.has_value() && !node.style.fontName->empty())
        {
            auto it = fonts_.find(*node.style.fontName);
            if (it != fonts_.end()) return it->second;
        }

        // 2. Widget-type default
        std::string widgetFont;
        switch (node.type)
        {
        case WidgetType::Button:     widgetFont = theme_.button.fontName;     break;
        case WidgetType::Label:      widgetFont = theme_.label.fontName;      break;
        case WidgetType::InputField: widgetFont = theme_.inputField.fontName; break;
        default: break;
        }
        if (!widgetFont.empty())
        {
            auto it = fonts_.find(widgetFont);
            if (it != fonts_.end()) return it->second;
        }

        // 3. Theme default font token
        std::string themeFont = theme_.GetString("font-default");
        if (!themeFont.empty())
        {
            auto it = fonts_.find(themeFont);
            if (it != fonts_.end()) return it->second;
        }

        // 4. First registered font
        if (!fonts_.empty()) return fonts_.begin()->second;

        return nullptr;
    }

    float Context::ResolveFontSize(const Node& node) const
    {
        if (node.style.fontSize.has_value() && *node.style.fontSize > 0.0f)
            return *node.style.fontSize;
        switch (node.type)
        {
        case WidgetType::Button:     if (theme_.button.fontSize > 0.0f) return theme_.button.fontSize;     break;
        case WidgetType::Label:      if (theme_.label.fontSize > 0.0f) return theme_.label.fontSize;      break;
        case WidgetType::InputField: if (theme_.inputField.fontSize > 0.0f) return theme_.inputField.fontSize; break;
        default: break;
        }
        return theme_.GetFloat("font-size-body", 14.0f);
    }

    // =============================================================================
    // Render pass
    // =============================================================================

    void Context::DrawRect(SDL_FRect rect, Color color)
    {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &rect);
    }

    void Context::DrawRectBorder(SDL_FRect rect, Color color, float width)
    {
        if (width <= 0.0f) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        // Draw 4 filled rects for border sides (simple, no radius)
        SDL_FRect top = { rect.x,               rect.y,                rect.w, width };
        SDL_FRect bottom = { rect.x,               rect.y + rect.h - width, rect.w, width };
        SDL_FRect left = { rect.x,               rect.y + width,        width, rect.h - width * 2.0f };
        SDL_FRect right = { rect.x + rect.w - width, rect.y + width,    width, rect.h - width * 2.0f };
        SDL_RenderFillRect(renderer_, &top);
        SDL_RenderFillRect(renderer_, &bottom);
        SDL_RenderFillRect(renderer_, &left);
        SDL_RenderFillRect(renderer_, &right);
    }

    void Context::DrawRoundedRect(SDL_FRect rect, Color color, float radius)
    {
        // SDL3 has no built-in rounded rect fill, so we approximate with
        // three overlapping rects + filled circles at corners.
        if (radius <= 0.0f) { DrawRect(rect, color); return; }

        radius = std::min(radius, std::min(rect.w, rect.h) * 0.5f);
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);

        // Center horizontal band
        SDL_FRect hBand = { rect.x + radius, rect.y, rect.w - radius * 2.0f, rect.h };
        SDL_RenderFillRect(renderer_, &hBand);

        // Left and right vertical bands
        SDL_FRect lBand = { rect.x, rect.y + radius, radius, rect.h - radius * 2.0f };
        SDL_FRect rBand = { rect.x + rect.w - radius, rect.y + radius, radius, rect.h - radius * 2.0f };
        SDL_RenderFillRect(renderer_, &lBand);
        SDL_RenderFillRect(renderer_, &rBand);

        // Filled quarter-circles at corners using point drawing
        int r = (int)radius;
        for (int dy = -r; dy <= r; dy++)
        {
            for (int dx = -r; dx <= r; dx++)
            {
                if (dx * dx + dy * dy <= r * r)
                {
                    // Top-left
                    SDL_RenderPoint(renderer_,
                        rect.x + radius + dx, rect.y + radius + dy);
                    // Top-right
                    SDL_RenderPoint(renderer_,
                        rect.x + rect.w - radius + dx, rect.y + radius + dy);
                    // Bottom-left
                    SDL_RenderPoint(renderer_,
                        rect.x + radius + dx, rect.y + rect.h - radius + dy);
                    // Bottom-right
                    SDL_RenderPoint(renderer_,
                        rect.x + rect.w - radius + dx, rect.y + rect.h - radius + dy);
                }
            }
        }
    }

    void Context::DrawText(const std::string& text, SDL_FRect rect, TTF_Font* font, Color color, TextAlign align)
    {
        if (!font || text.empty()) return;

        int tw = 0, th = 0;
        TTF_GetStringSize(font, text.c_str(), 0, &tw, &th);

        SDL_Color sdlColor = { color.r, color.g, color.b, color.a };
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, sdlColor);
        if (!surface) return;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_DestroySurface(surface);
        if (!tex) return;

        float x = rect.x;
        switch (align)
        {
        case TextAlign::Center: x = rect.x + (rect.w - tw) * 0.5f; break;
        case TextAlign::Right:  x = rect.x + rect.w - tw;           break;
        default:                x = rect.x;                          break;
        }
        float y = rect.y + (rect.h - th) * 0.5f;

        SDL_FRect dst = { x, y, (float)tw, (float)th };
        SDL_RenderTexture(renderer_, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }

    void Context::RenderContainer(const Node& node)
    {
        // A container may still have a background if style overrides set one
        if (node.style.background.has_value())
        {
            float radius = node.style.borderRadius.value_or(0.0f);
            DrawRoundedRect(node.computedRect, *node.style.background, radius);
        }
    }

    void Context::RenderButton(const Node& node, const InputState& input)
    {
        const ButtonStyle& bs = theme_.button;
        InteractionState state = node.widget.interactionState;

        Color bg = node.style.background.value_or(bs.background);
        if (state == InteractionState::Pressed)
            bg = node.style.backgroundPress.value_or(bs.backgroundPress);
        else if (state == InteractionState::Hovered)
            bg = node.style.backgroundHover.value_or(bs.backgroundHover);

        if (!node.enabled)
        {
            bg.r = (uint8_t)(bg.r * 0.5f);
            bg.g = (uint8_t)(bg.g * 0.5f);
            bg.b = (uint8_t)(bg.b * 0.5f);
            bg.a = (uint8_t)(bg.a * 0.7f);
        }

        float radius = node.style.borderRadius.value_or(bs.borderRadius);
        radius *= 2;
        DrawRoundedRect(node.computedRect, bg, radius);

        float borderW = node.style.borderWidth.value_or(bs.borderWidth);
        Color borderC = node.style.border.value_or(bs.border);
        //DrawRectBorder(node.computedRect, borderC, borderW);

        Color fg = node.style.foreground.value_or(bs.foreground);
        if (!node.enabled) fg.a = (uint8_t)(fg.a * 0.5f);

        TextAlign ta = node.style.textAlign.value_or(bs.textAlign);
        TTF_Font* font = ResolveFont(node);

        const Edges& pad = node.style.padding.value_or(bs.padding);
        SDL_FRect textRect = {
            node.computedRect.x + pad.left,
            node.computedRect.y + pad.top,
            node.computedRect.w - pad.left - pad.right,
            node.computedRect.h - pad.top - pad.bottom
        };

        DrawText(node.widget.text, textRect, font, fg, ta);
    }

    void Context::RenderLabel(const Node& node)
    {
        const LabelStyle& ls = theme_.label;
        Color fg = node.style.foreground.value_or(ls.foreground);
        TextAlign ta = node.style.textAlign.value_or(ls.textAlign);
        TTF_Font* font = ResolveFont(node);
        DrawText(node.widget.text, node.computedRect, font, fg, ta);
    }

    void Context::RenderSlider(const Node& node)
    {
        const SliderStyle& ss = theme_.slider;
        const SDL_FRect& r = node.computedRect;
        InteractionState state = node.widget.interactionState;

        float thumbR = ss.thumbRadius;
        float trackH = ss.trackHeight;
        float trackY = r.y + (r.h - trackH) * 0.5f;

        // Track background
        SDL_FRect trackBg = { r.x + thumbR, trackY, r.w - thumbR * 2.0f, trackH };
        DrawRoundedRect(trackBg, ss.trackBackground, ss.borderRadius);

        // Track fill
        float range = node.widget.sliderMax - node.widget.sliderMin;
        float t = (range > 0.0f) ? (node.widget.sliderValue - node.widget.sliderMin) / range : 0.0f;
        float fillW = t * trackBg.w;
        SDL_FRect trackFill = { trackBg.x, trackY, fillW, trackH };
        DrawRoundedRect(trackFill, ss.trackFill, ss.borderRadius);

        // Thumb
        float thumbX = trackBg.x + fillW;
        float thumbY = r.y + r.h * 0.5f;

        Color thumbColor = (state == InteractionState::Hovered || state == InteractionState::Pressed)
            ? ss.thumbHover : ss.thumb;

        // Draw thumb as a filled circle (via point sampling)
        SDL_SetRenderDrawColor(renderer_, thumbColor.r, thumbColor.g, thumbColor.b, thumbColor.a);
        int ri = (int)thumbR;
        for (int dy = -ri; dy <= ri; dy++)
            for (int dx = -ri; dx <= ri; dx++)
                if (dx * dx + dy * dy <= ri * ri)
                    SDL_RenderPoint(renderer_, thumbX + dx, thumbY + dy);
    }

    void Context::RenderInput(const Node& node)
    {
        const InputFieldStyle& ifs = theme_.inputField;
        bool focused = node.widget.focused;

        Color bg = node.style.background.value_or(
            focused ? ifs.backgroundFocus : ifs.background);
        float radius = node.style.borderRadius.value_or(ifs.borderRadius);
        DrawRoundedRect(node.computedRect, bg, radius);

        float borderW = node.style.borderWidth.value_or(ifs.borderWidth);
        Color borderC = node.style.border.value_or(
            focused ? ifs.borderFocus : ifs.border);
        DrawRectBorder(node.computedRect, borderC, borderW);

        const Edges& pad = node.style.padding.value_or(ifs.padding);
        SDL_FRect textRect = {
            node.computedRect.x + pad.left,
            node.computedRect.y + pad.top,
            node.computedRect.w - pad.left - pad.right,
            node.computedRect.h - pad.top - pad.bottom
        };

        TTF_Font* font = ResolveFont(node);

        const std::string& val = node.widget.inputValue;
        if (val.empty() && !focused)
        {
            Color ph = node.style.placeholder.value_or(ifs.placeholder);
            DrawText(node.widget.placeholder, textRect, font, ph, TextAlign::Left);
        }
        else
        {
            Color fg = node.style.foreground.value_or(ifs.foreground);
            DrawText(val, textRect, font, fg, TextAlign::Left);

            // Cursor
            if (focused)
            {
                int cursorX = 0;
                if (font && !val.empty())
                {
                    std::string before = val.substr(0, node.widget.cursorPos);
                    int w = 0;
                    TTF_GetStringSize(font, before.c_str(), 0, &w, nullptr);
                    cursorX = w;
                }

                SDL_FRect cursor = {
                    textRect.x + cursorX,
                    textRect.y,
                    2.0f,
                    textRect.h
                };
                DrawRect(cursor, ifs.foreground);
            }
        }
    }

    void Context::RenderImage(const Node& node)
    {
        if (!node.widget.texture) return;

        const SDL_FRect& src = node.widget.textureRect;
        const SDL_FRect* srcPtr = (src.w > 0 && src.h > 0) ? &src : nullptr;
        SDL_RenderTexture(renderer_, node.widget.texture, srcPtr, &node.computedRect);
    }

    void Context::RenderNode(const Node& node)
    {
        if (!node.visible) return;

        switch (node.type)
        {
        case WidgetType::Container:  RenderContainer(node); break;
        case WidgetType::Label:      RenderLabel(node);     break;
        case WidgetType::Button:     RenderButton(node, currentInput_); break;
        case WidgetType::Slider:     RenderSlider(node);    break;
        case WidgetType::InputField: RenderInput(node);     break;
        case WidgetType::Image:      RenderImage(node);     break;
        }

        // Recurse into children
        for (NodeHandle ch : node.children)
        {
            auto it = nodes_.find(ch);
            if (it != nodes_.end())
                RenderNode(it->second);
        }
    }

    void Context::RenderPass()
    {
        for (NodeHandle root : roots_)
        {
            auto it = nodes_.find(root);
            if (it != nodes_.end())
                RenderNode(it->second);
        }
    }

} // namespace UI   