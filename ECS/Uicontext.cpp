#include "UIContext.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

namespace UI
{

    const std::string Context::emptyString_;

    // =============================================================================
    // Lifecycle
    // =============================================================================

    void Context::Init(Renderer* renderer, float canvasW, float canvasH, SDL_Window* window, AssetManager* assets)
    {
        _assets = assets;
        _renderer = renderer;
        window_ = window;
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
            roots_.push_back(handle);
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

    NodeHandle Context::AddImage(StringId texture, SDL_FRect sourceRect, NodeHandle parent, std::string_view id)
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
            roots_.erase(std::remove(roots_.begin(), roots_.end(), handle), roots_.end());
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

    void Context::SetImage(NodeHandle handle, StringId texture, SDL_FRect sourceRect)
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
        currentInput_ = input;
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
                    bool result = SDL_StopTextInput(window_);
                    //printf("SDL_StartTextInput: %d\n", result);
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
                        SDL_StartTextInput(window_);
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
                        SDL_StopTextInput(window_);
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

            StringId font = node.style.fontName.value_or("");
            if (font.empty()) font = theme_.button.fontName;
            if (font.empty()) font = theme_.GetString("font-default");

            if (!node.widget.text.empty())
            {
                const Edges& pad = node.style.padding.value_or(
                    node.type == WidgetType::Button ? theme_.button.padding : Edges::All(0.0f)
                );

                Vec3 measured = MeasureText(font, node.widget.text);
                float lineHeight = measured.y - measured.z;
                if (w < 0.0f) w = measured.x + pad.left + pad.right;
                if (h < 0.0f) h = lineHeight + pad.top + pad.bottom;
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
            StringId font = node.style.fontName.value_or("");
            if (font.empty()) font = theme_.button.fontName;
            if (font.empty()) font = StringId(theme_.GetString("font-default"));

            const Edges& pad = node.style.padding.value_or(theme_.inputField.padding);

            auto *x = _assets->TryGetGPUFont(font);
            
            float lineHeight = x->lineHeight;

            if (w < 0.0f) w = 200.0f;
            if (h < 0.0f) h = lineHeight + pad.top + pad.bottom;
            break;
        }
        case WidgetType::Image:
        {
            if (!node.widget.texture.empty() && w < 0.0f && h < 0.0f)
            {

				auto *x = _assets->TryGetTexture(node.widget.texture);


                float tw = 0, th = 0;
				tw = (float)x->width;
				th = (float)x->height;

                if (node.widget.textureRect.w > 0) 
                    tw = node.widget.textureRect.w;
                if (node.widget.textureRect.h > 0) 
                    th = node.widget.textureRect.h;
                w = tw;
                h = th;
            }
            if (w < 0.0f)
                w = 0.0f;
            if (h < 0.0f)
                h = 0.0f;
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
                AlignItems resolved = node.flex.alignItems;
                if (selfAlign == AlignSelf::FlexStart)   resolved = AlignItems::FlexStart;
                else if (selfAlign == AlignSelf::FlexEnd) resolved = AlignItems::FlexEnd;
                else if (selfAlign == AlignSelf::Center)  resolved = AlignItems::Center;
                else if (selfAlign == AlignSelf::Stretch) resolved = AlignItems::Stretch;

                if (resolved == AlignItems::Stretch)
                {
                    if (isRow) child.computedRect.h = crossAvail
                        - child.flex.margin.top - child.flex.margin.bottom;
                    else       child.computedRect.w = crossAvail
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
                AlignItems resolved = node.flex.alignItems;
                if (selfAlign == AlignSelf::FlexStart)   resolved = AlignItems::FlexStart;
                else if (selfAlign == AlignSelf::FlexEnd) resolved = AlignItems::FlexEnd;
                else if (selfAlign == AlignSelf::Center)  resolved = AlignItems::Center;
                else if (selfAlign == AlignSelf::Stretch) resolved = AlignItems::Stretch;

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

                if (isRow) { child.computedRect.x = baseX + finalMain;  child.computedRect.y = baseY + finalCross; }
                else { child.computedRect.x = baseX + finalCross; child.computedRect.y = baseY + finalMain; }

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
    // Render pass
    // =============================================================================

    void Context::DrawRect(SDL_FRect rect, Color color, float z)
    {

        _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"), { rect.x + rect.w/2, rect.y + rect.h/2, z }, {rect.w, rect.h}, 0.f, 
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
    }

    void Context::DrawRectBorder(SDL_FRect rect, Color color, float width)
    {
        if (width <= 0.0f) 
            return;

        SDL_FRect top = { rect.x, rect.y, rect.w, width };
        SDL_FRect bottom = { rect.x, rect.y + rect.h - width, rect.w, width };
        SDL_FRect left = { rect.x, rect.y + width, width, rect.h - width * 2.0f };
        SDL_FRect right = { rect.x + rect.w - width, rect.y + width, width, rect.h - width * 2.0f };


        _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"), { top.x + top.w/2.f, top.y + top.h/2.f, 0.1f }, { top.w, top.h }, 0.f, 
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
        _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"), { bottom.x + bottom.w/2.f, bottom.y + bottom.h/2.f, 0.1f }, { bottom.w, bottom.h }, 0.f,
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
        _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"), { left.x + left.w/2.f, left.y + left.h/2.f, 0.1f }, { left.w, left.h }, 0.f, 
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
        _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"), { right.x + right.w/2.f, right.y + right.h/2.f, 0.1f }, { right.w, right.h }, 0.f,
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
    }

    void Context::DrawRoundedRect(SDL_FRect rect, Color color, float radius)
    {

        if (radius <= 0.0f) { DrawRect(rect, color); return; }
        radius = std::min(radius, std::min(rect.w, rect.h) * 0.5f);

        float diameter = radius * 2.0f;

        // Center horizontal band
        SDL_FRect hBand = { rect.x + radius, rect.y, rect.w - diameter, rect.h };
        // Left and right vertical bands (excluding corners)
        SDL_FRect lBand = { rect.x,                  rect.y + radius, radius, rect.h - diameter };
        SDL_FRect rBand = { rect.x + rect.w - radius, rect.y + radius, radius, rect.h - diameter };

        auto submitQuad = [&](const SDL_FRect& r)
            {
                _renderer->SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat"),
                    { r.x + r.w / 2.f, r.y + r.h / 2.f, 0.f },
                    { r.w, r.h }, 0.f,
                    { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
            };

        auto submitCircle = [&](float cx, float cy)
            {
                _renderer->SubmitMesh(MeshInstance("unit_circle32"), MaterialInstance("mat"),
                    { cx, cy, 0.1f },
                    { diameter, diameter }, 0.f,
                    { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
            };

        submitQuad(hBand);
        submitQuad(lBand);
        submitQuad(rBand);

        // Four corner circles
        submitCircle(rect.x + radius, rect.y + radius);           // top-left
        submitCircle(rect.x + rect.w - radius, rect.y + radius);           // top-right
        submitCircle(rect.x + radius, rect.y + rect.h - radius);  // bottom-left
        submitCircle(rect.x + rect.w - radius, rect.y + rect.h - radius);  // bottom-right
    }


    void Context::DrawText(const std::string& text, SDL_FRect rect, StringId font, Color color, TextAlign align)
    {
        if (text.empty())
            return;


        auto* gpuFont = _assets->TryGetGPUFont(font);
        if (!gpuFont)
            return;



        auto size = MeasureText(font, text);
        float lineHeight = size.y - size.z;
        float scale = rect.h / lineHeight;

        if (size.x * scale > rect.w)
            scale = rect.w / size.x;

        float finalW = size.x * scale;
        float x;

        switch (align)
        {
        case TextAlign::Center: x = rect.x + (rect.w - finalW) * 0.5f; break;
        case TextAlign::Right:  x = rect.x + rect.w - finalW; break;
        default:                x = rect.x; break;
        }

        float y = rect.y + (rect.h + (size.y + size.z/2.f) * scale) * 0.5f;

        _renderer->SubmitText(text, font, MaterialInstance("text_mat"),
            { x, y, 0.2f }, { scale, scale }, 0.f,
            { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f });
    }

    Vec3 Context::MeasureText(StringId fontId, const std::string& text)
    {
        float width = 0;
        float maxHeight = 0;
        auto* x = _assets->TryGetGPUFont(fontId);

        if (!x) 
        {
			std::cout << "Warning: font '" << fontId.id << "' not found for measuring text.\n";
            return { 0.f, 0.f, 0.f };
        }

        for (char c : text)
        {
            const GlyphInfo& g = x->glyphs[c];
            width += g.advance;
            float glyphHeight = g.bearingY;
            maxHeight = std::max(maxHeight, glyphHeight);
        }

        return { width, (float)x->ascent , (float)x->descent};
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
        if (state == InteractionState::Pressed)  bg = node.style.backgroundPress.value_or(bs.backgroundPress);
        else if (state == InteractionState::Hovered)  bg = node.style.backgroundHover.value_or(bs.backgroundHover);

        if (!node.enabled) { bg.r = (uint8_t)(bg.r * 0.5f); bg.g = (uint8_t)(bg.g * 0.5f); bg.b = (uint8_t)(bg.b * 0.5f); bg.a = (uint8_t)(bg.a * 0.7f); }

        float radius = node.style.borderRadius.value_or(bs.borderRadius);
        radius *= 2;
        DrawRoundedRect(node.computedRect, bg, radius);

        float borderW = node.style.borderWidth.value_or(bs.borderWidth);
        Color borderC = node.style.border.value_or(bs.border);
        //DrawRectBorder(node.computedRect, borderC, borderW);

        Color fg = node.style.foreground.value_or(bs.foreground);
        if (!node.enabled) fg.a = (uint8_t)(fg.a * 0.5f);

        TextAlign ta = node.style.textAlign.value_or(bs.textAlign);

        StringId font = node.style.fontName.value_or("");
        if (font.empty()) font = theme_.button.fontName;
        if (font.empty()) font = StringId(theme_.GetString("font-default"));

        const Edges& pad = node.style.padding.value_or(bs.padding);
        SDL_FRect textRect = {
            node.computedRect.x + pad.left,   node.computedRect.y + pad.top,
            node.computedRect.w - pad.left - pad.right, node.computedRect.h - pad.top - pad.bottom
        };

        DrawText(node.widget.text, textRect, font, fg, ta);

        /*printf("Button computedRect: x=%.1f y=%.1f w=%.1f h=%.1f\n",
            node.computedRect.x, node.computedRect.y,
            node.computedRect.w, node.computedRect.h);
        printf("textRect: x=%.1f y=%.1f w=%.1f h=%.1f\n",
            textRect.x, textRect.y, textRect.w, textRect.h);*/
    }

    void Context::RenderLabel(const Node& node)
    {
        const LabelStyle& ls = theme_.label;
        Color fg = node.style.foreground.value_or(ls.foreground);
        TextAlign ta = node.style.textAlign.value_or(ls.textAlign);

        StringId font = node.style.fontName.value_or("");
        if (font.empty()) 
            font = theme_.button.fontName;
        if (font.empty()) 
            font = StringId(theme_.GetString("font-default"));

        DrawText(node.widget.text, node.computedRect, font, fg, ta);
    }

    void Context::RenderSlider(const Node& node)
    {
        const SliderStyle& ss = theme_.slider;
        const SDL_FRect& r = node.computedRect;

        float thumbR = ss.thumbRadius;
        float trackH = ss.trackHeight;
        float trackY = r.y + (r.h - trackH) * 0.5f;

        // Track background
        SDL_FRect trackBg = { r.x + thumbR, trackY, r.w - thumbR * 2.0f, trackH };
        DrawRoundedRect(trackBg, ss.trackBackground, ss.borderRadius);

        float range = node.widget.sliderMax - node.widget.sliderMin;
        float t = (range > 0.0f) ? (node.widget.sliderValue - node.widget.sliderMin) / range : 0.0f;
        float fillW = t * trackBg.w;
        SDL_FRect trackFill = { trackBg.x, trackY, fillW, trackH };
        DrawRoundedRect(trackFill, ss.trackFill, ss.borderRadius);

        // Thumb
        float thumbX = trackBg.x + fillW;
        float thumbY = r.y + r.h * 0.5f;
        Color thumbColor = (node.widget.interactionState == InteractionState::Hovered ||
            node.widget.interactionState == InteractionState::Pressed)
            ? ss.thumbHover : ss.thumb;


        _renderer->SubmitMesh(MeshInstance("unit_circle32"), MaterialInstance("mat"),
            { thumbX, thumbY, 0.2f },
            { thumbR * 2.0f, thumbR * 2.0f }, 0.f,
			{ thumbColor.r / 255.f, thumbColor.g / 255.f, thumbColor.b / 255.f, thumbColor.a / 255.f });
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
            node.computedRect.x + pad.left,   node.computedRect.y + pad.top,
            node.computedRect.w - pad.left - pad.right, node.computedRect.h - pad.top - pad.bottom
        };

        StringId font = node.style.fontName.value_or("");
        if (font.empty()) font = theme_.button.fontName;
        if (font.empty()) font = StringId(theme_.GetString("font-default"));



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
                float cursorX = 0.0f;
                if (!val.empty() && node.widget.cursorPos > 0)
                {
                    std::string before = val.substr(0, node.widget.cursorPos);
                    Vec3 measured = MeasureText(font, before);

                    // Match the same scale DrawText uses
                    Vec3 fullMeasured = MeasureText(font, val);
                    float lineHeight = fullMeasured.y - fullMeasured.z;
                    float scale = textRect.h / lineHeight;  
                    if (fullMeasured.x * scale > textRect.w)
                        scale = textRect.w / fullMeasured.x;

                    cursorX = measured.x * scale;
                }

                SDL_FRect cursor = {
                    textRect.x + cursorX,
                    textRect.y,
                    2.0f,
                    textRect.h
                };
                DrawRect(cursor, ifs.foreground, 0.15f);
            }
        }
    }

    void Context::RenderImage(const Node& node)
    {
        if (node.widget.texture.empty()) return;

        float cx = node.computedRect.x + node.computedRect.w * 0.5f;
        float cy = node.computedRect.y + node.computedRect.h * 0.5f;

        float scaleX = node.computedRect.w / node.widget.textureRect.w;
        float scaleY = node.computedRect.h / node.widget.textureRect.h;

        _renderer->SubmitSprite(
            MaterialInstance("sprite_mat", node.widget.texture),
            node.widget.textureRect,
            { cx, cy, 0.1f },
            { scaleX, scaleY }, 0.f,
            { 1.f, 1.f, 1.f, 1.f });
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