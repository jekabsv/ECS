#include "Heat.h"
#include "MeshComponent.h"
#include "Transform.h"
#include "RigidBody.h"
#include <format>
#include <cmath>
#include <algorithm>
#include "EngineMath.h"

using namespace Math;

static const StringId BB_HEAT_AREA = StringId("heat_area");

// =============================================================================
// Material properties table — indexed by Heat::Material enum
// =============================================================================
struct MatProps
{
    float conductivity; // multiplier on diffusivity  (air=1, copper>>1, insulator<<1)
    float cooling;      // per-second fractional cooling (solids retain heat longer)
};

static constexpr MatProps MAT_PROPS[] = {
    // Air       — reference medium
    { Heat::COND_AIR,       Heat::COOL_AIR       },
    // Copper    — spreads heat very fast, cools moderately
    { Heat::COND_COPPER,    Heat::COOL_COPPER     },
    // Wood      — poor conductor, retains heat well
    { Heat::COND_WOOD,      Heat::COOL_WOOD       },
    // Insulator — barely conducts, barely cools (foam/brick)
    { Heat::COND_INSULATOR, Heat::COOL_INSULATOR  },
};

// =============================================================================
// Color gradients per material
// =============================================================================
static SDL_FColor GradientSample(const SDL_FColor* stops, const float* ts, int n, float t)
{
    t = std::clamp(t, 0.f, 1.f);
    for (int i = 0; i < n - 1; i++)
    {
        if (t <= ts[i + 1])
        {
            float local = (t - ts[i]) / (ts[i + 1] - ts[i]);
            return LerpColor(stops[i], stops[i + 1], local);
        }
    }
    return stops[n - 1];
}

// Air — classic heatmap: near-black → indigo → teal → amber → dull red
static SDL_FColor AirColor(float t)
{
    static constexpr float ts[] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f };
    static constexpr SDL_FColor cs[] = {
        { 0.04f, 0.02f, 0.06f, 1.f },
        { 0.05f, 0.10f, 0.40f, 1.f },
        { 0.05f, 0.45f, 0.45f, 1.f },
        { 0.65f, 0.28f, 0.04f, 1.f },
        { 0.75f, 0.12f, 0.04f, 1.f },
    };
    return GradientSample(cs, ts, 5, t);
}

// Copper — dark brown → warm orange → bright copper-gold (metallic feel)
static SDL_FColor CopperColor(float t)
{
    static constexpr float ts[] = { 0.f, 0.33f, 0.66f, 1.f };
    static constexpr SDL_FColor cs[] = {
        { 0.10f, 0.05f, 0.02f, 1.f },
        { 0.40f, 0.18f, 0.04f, 1.f },
        { 0.65f, 0.35f, 0.08f, 1.f },
        { 0.80f, 0.52f, 0.15f, 1.f },
    };
    return GradientSample(cs, ts, 4, t);
}

// Wood — very dark grey-brown → warm charred brown → faint orange glow
static SDL_FColor WoodColor(float t)
{
    static constexpr float ts[] = { 0.f, 0.4f, 0.75f, 1.f };
    static constexpr SDL_FColor cs[] = {
        { 0.08f, 0.05f, 0.03f, 1.f },
        { 0.25f, 0.14f, 0.06f, 1.f },
        { 0.45f, 0.22f, 0.07f, 1.f },
        { 0.58f, 0.18f, 0.05f, 1.f },
    };
    return GradientSample(cs, ts, 4, t);
}

// Insulator — dark charcoal → muted warm grey (barely glows even when hot)
static SDL_FColor InsulatorColor(float t)
{
    static constexpr float ts[] = { 0.f, 0.5f, 1.f };
    static constexpr SDL_FColor cs[] = {
        { 0.08f, 0.08f, 0.09f, 1.f },
        { 0.18f, 0.16f, 0.14f, 1.f },
        { 0.28f, 0.22f, 0.16f, 1.f },
    };
    return GradientSample(cs, ts, 3, t);
}

static SDL_FColor MaterialColor(Heat::Material mat, float t)
{
    switch (mat)
    {
    case Heat::Material::Copper:    return CopperColor(t);
    case Heat::Material::Wood:      return WoodColor(t);
    case Heat::Material::Insulator: return InsulatorColor(t);
    default:                        return AirColor(t);
    }
}

// =============================================================================
// Helpers
// =============================================================================
static inline int Idx(int col, int row, int cols) { return row * cols + col; }

static inline float MatConductivity(Heat::Material m)
{
    return MAT_PROPS[(int)m].conductivity;
}
static inline float MatCooling(Heat::Material m)
{
    return MAT_PROPS[(int)m].cooling;
}

// =============================================================================
void Heat::ResetGrid()
{
    std::fill(_curr.begin(), _curr.end(), 0.f);
    std::fill(_next.begin(), _next.end(), 0.f);
    std::fill(_mat.begin(), _mat.end(), Material::Air);
    std::fill(_pinned.begin(), _pinned.end(), 0);
}

// =============================================================================
void Heat::StampBrush(float wx, float wy)
{
    int cx = (int)((wx - _area.x) / _cellW);
    int cy = (int)((wy - _area.y) / _cellH);
    int r = _brushRadius;
    float rSq = (float)(r * r);

    for (int dy = -r; dy <= r; dy++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            float d2 = (float)(dx * dx + dy * dy);
            if (d2 > rSq) continue;

            int col = cx + dx;
            int row = cy + dy;
            if (col < 0 || col >= _cols || row < 0 || row >= _rows) continue;

            int idx = Idx(col, row, _cols);
            float falloff = std::exp(-d2 / (rSq * 0.4f));

            switch (_brushMode)
            {
            case BrushMode::Hot:
                // paint heat into any non-pinned cell regardless of material
                if (_pinned[idx] == 0)
                    _curr[idx] = std::clamp(_curr[idx] + falloff * BRUSH_STRENGTH, 0.f, 1.f);
                break;

            case BrushMode::Sink:
                _pinned[idx] = -1;
                _curr[idx] = SINK_VALUE;
                _mat[idx] = Material::Air; // sinks are not a solid material
                break;

            case BrushMode::Source:
                _pinned[idx] = +1;
                _curr[idx] = SOURCE_VALUE;
                _mat[idx] = Material::Air;
                break;

            case BrushMode::Copper:
                _pinned[idx] = 0;
                _mat[idx] = Material::Copper;
                break;

            case BrushMode::Wood:
                _pinned[idx] = 0;
                _mat[idx] = Material::Wood;
                break;

            case BrushMode::Insulator:
                _pinned[idx] = 0;
                _mat[idx] = Material::Insulator;
                break;

            case BrushMode::Erase:
                _pinned[idx] = 0;
                _mat[idx] = Material::Air;
                break;
            }
        }
    }
}

// =============================================================================
void Heat::Init()
{
    _data->physics.EnableCollisionDetection(false);
    _data->physics.EnableSpatialIndexBuild(false);
    _data->physics.EnableMovement(false);

    _area = { (float)PANEL_WIDTH, 0.f,
              (float)(_data->GAME_WIDTH - PANEL_WIDTH),
              (float)_data->GAME_HEIGHT };

    _cols = GRID_COLS;
    _rows = GRID_ROWS;
    _cellW = _area.w / (float)_cols;
    _cellH = _area.h / (float)_rows;

    int total = _cols * _rows;
    _curr.assign(total, 0.f);
    _next.assign(total, 0.f);
    _mat.assign(total, Material::Air);
    _pinned.assign(total, 0);

    _data->session.Set<PlayArea>(BB_HEAT_AREA, _area);

    // -------------------------------------------------------------------------
    // UI
    // -------------------------------------------------------------------------
    ui.GetTheme().LoadDarkDefaults();
    ui.GetTheme().SetToken("font-default", StringId("tnr"));

    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(PANEL_WIDTH), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::FlexStart);
    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 5.f);
    ui.SetPadding(root, UI::Edges::All(16.f));

    back = ui.AddButton("Back to menu", root);
    ui.SetSize(back, UI::SizeValue::Auto(), UI::SizeValue::Px(52));

    // Diffusivity
    ui.SetSize(ui.AddLabel("Diffusivity:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    diffInput = ui.AddInputField(std::format("{:.2f}", DEFAULT_DIFFUSIVITY), root);
    ui.SetSize(diffInput, UI::SizeValue::Auto(), UI::SizeValue::Px(44));
    diffSlider = ui.AddSlider(DEFAULT_DIFFUSIVITY, MIN_DIFFUSIVITY, MAX_DIFFUSIVITY, root);

    // Brush radius
    ui.SetSize(ui.AddLabel("Brush radius:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    brushInput = ui.AddInputField(std::format("{}", DEFAULT_BRUSH_RADIUS), root);
    ui.SetSize(brushInput, UI::SizeValue::Auto(), UI::SizeValue::Px(44));
    brushSlider = ui.AddSlider((float)DEFAULT_BRUSH_RADIUS, (float)MIN_BRUSH_RADIUS, (float)MAX_BRUSH_RADIUS, root);

    // Brush mode  (Hot → Sink → Source → Copper → Wood → Insulator → Erase)
    modeButton = ui.AddButton("Mode: Hot", root);
    ui.SetSize(modeButton, UI::SizeValue::Auto(), UI::SizeValue::Px(44));

    // Reset
    resetButton = ui.AddButton("Reset", root);
    ui.SetSize(resetButton, UI::SizeValue::Auto(), UI::SizeValue::Px(44));

    // Wind
    windLabel = ui.AddLabel("Wind  X: 0   Y: 0", root);
    ui.SetSize(windLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(20));

    ui.SetSize(ui.AddLabel("Wind X (left / right):", root), UI::SizeValue::Auto(), UI::SizeValue::Px(18));
    windXSlider = ui.AddSlider(DEFAULT_WIND_X, -MAX_WIND_SPEED, MAX_WIND_SPEED, root);

    ui.SetSize(ui.AddLabel("Wind Y (up / down):", root), UI::SizeValue::Auto(), UI::SizeValue::Px(18));
    windYSlider = ui.AddSlider(DEFAULT_WIND_Y, -MAX_WIND_SPEED, MAX_WIND_SPEED, root);
}

// =============================================================================
void Heat::Update(float dt)
{
    // ---- Sync diffusivity ---------------------------------------------------
    bool curFocusedDiff = (ui.Poll(diffInput) == UI::InteractionState::Focused);
    bool curFocusedBrush = (ui.Poll(brushInput) == UI::InteractionState::Focused);

    if (!curFocusedDiff && _focusedDiff)
    {
        try {
            float v = std::clamp(std::stof(ui.GetInputValue(diffInput)),
                MIN_DIFFUSIVITY, MAX_DIFFUSIVITY);
            ui.SetSliderValue(diffSlider, v);
            _diffusivity = v;
        }
        catch (...) {}
    }
    if (!curFocusedBrush && _focusedBrush)
    {
        try {
            int v = std::clamp(std::stoi(ui.GetInputValue(brushInput)),
                MIN_BRUSH_RADIUS, MAX_BRUSH_RADIUS);
            ui.SetSliderValue(brushSlider, (float)v);
            _brushRadius = v;
        }
        catch (...) {}
    }
    _focusedDiff = curFocusedDiff;
    _focusedBrush = curFocusedBrush;

    if (!_focusedDiff)
    {
        _diffusivity = ui.GetSliderValue(diffSlider);
        ui.SetInputValue(diffInput, std::format("{:.2f}", _diffusivity));
    }
    if (!_focusedBrush)
    {
        _brushRadius = std::max(1, (int)ui.GetSliderValue(brushSlider));
        ui.SetInputValue(brushInput, std::format("{}", _brushRadius));
    }

    // ---- Wind sliders -------------------------------------------------------
    _windX = ui.GetSliderValue(windXSlider);
    _windY = ui.GetSliderValue(windYSlider);
    ui.SetText(windLabel, std::format("Wind  X:{:+.0f}  Y:{:+.0f}", _windX, _windY));

    // ---- Mode button  (Hot → Sink → Source → Copper → Wood → Insulator → Erase) ---
    if (ui.IsClicked(modeButton))
    {
        switch (_brushMode)
        {
        case BrushMode::Hot:       _brushMode = BrushMode::Sink;      ui.SetText(modeButton, "Mode: Sink");      break;
        case BrushMode::Sink:      _brushMode = BrushMode::Source;    ui.SetText(modeButton, "Mode: Source");    break;
        case BrushMode::Source:    _brushMode = BrushMode::Copper;    ui.SetText(modeButton, "Mode: Copper");    break;
        case BrushMode::Copper:    _brushMode = BrushMode::Wood;      ui.SetText(modeButton, "Mode: Wood");      break;
        case BrushMode::Wood:      _brushMode = BrushMode::Insulator; ui.SetText(modeButton, "Mode: Insulator"); break;
        case BrushMode::Insulator: _brushMode = BrushMode::Erase;     ui.SetText(modeButton, "Mode: Erase");     break;
        case BrushMode::Erase:     _brushMode = BrushMode::Hot;       ui.SetText(modeButton, "Mode: Hot");       break;
        }
    }

    if (ui.IsClicked(resetButton))
        ResetGrid();

    // ---- Mouse paint --------------------------------------------------------
    bool held = _data->inputs.IsHeld("click");
    Vec2 mouse = _data->inputs.GetMousePosition();

    if (held && mouse.x > _area.x)
        StampBrush(mouse.x, mouse.y);

    _prevClickHeld = held;

    // =========================================================================
    // Simulation — sub-stepped for stability
    // =========================================================================
    float remaining = dt;
    while (remaining > 0.f)
    {
        float step = std::min(remaining, MAX_SIM_DT);
        remaining -= step;

        // ---- 1. Enforce pinned cells ----------------------------------------
        for (int i = 0; i < _cols * _rows; i++)
        {
            if (_pinned[i] > 0) _curr[i] = SOURCE_VALUE;
            else if (_pinned[i] < 0) _curr[i] = SINK_VALUE;
        }

        // ---- 2. Diffusion — ALL non-pinned cells participate ----------------
        // Each cell uses its own material's conductivity scaled by the base
        // diffusivity. Interface conductivity is the arithmetic mean of the two
        // neighbours, so heat flows across material boundaries proportionally.
        for (int row = 0; row < _rows; row++)
        {
            for (int col = 0; col < _cols; col++)
            {
                int idx = Idx(col, row, _cols);
                if (_pinned[idx] != 0) { _next[idx] = _curr[idx]; continue; }

                float c = _curr[idx];
                float kC = MatConductivity(_mat[idx]);

                int idxU = Idx(col, std::max(row - 1, 0), _cols);
                int idxD = Idx(col, std::min(row + 1, _rows - 1), _cols);
                int idxL = Idx(std::max(col - 1, 0), row, _cols);
                int idxR = Idx(std::min(col + 1, _cols - 1), row, _cols);

                // Arithmetic mean at each interface — neither side dominates fully
                float kU = 0.5f * (kC + MatConductivity(_mat[idxU]));
                float kD = 0.5f * (kC + MatConductivity(_mat[idxD]));
                float kL = 0.5f * (kC + MatConductivity(_mat[idxL]));
                float kR = 0.5f * (kC + MatConductivity(_mat[idxR]));

                float laplacian = kU * (_curr[idxU] - c)
                    + kD * (_curr[idxD] - c)
                    + kL * (_curr[idxL] - c)
                    + kR * (_curr[idxR] - c);

                float cooling = MatCooling(_mat[idx]);
                _next[idx] = std::clamp(c + _diffusivity * step * laplacian
                    - cooling * step * c,
                    0.f, 1.f);
            }
        }
        std::swap(_curr, _next);

        // ---- 3. Wind advection (semi-Lagrangian, bilinear) ------------------
        // Only air cells are advected. Solid material cells keep their diffused
        // value — wind moves air heat but not material heat.
        if (std::abs(_windX) > 0.01f || std::abs(_windY) > 0.01f)
        {
            for (int row = 0; row < _rows; row++)
            {
                for (int col = 0; col < _cols; col++)
                {
                    int idx = Idx(col, row, _cols);

                    // Solids and pinned cells are immovable by wind
                    if (_pinned[idx] != 0 || _mat[idx] != Material::Air)
                    {
                        _next[idx] = _curr[idx];
                        continue;
                    }

                    float srcCol = std::clamp((float)col - _windX * step, 0.f, (float)(_cols - 1));
                    float srcRow = std::clamp((float)row - _windY * step, 0.f, (float)(_rows - 1));

                    int c0 = (int)srcCol, c1 = std::min(c0 + 1, _cols - 1);
                    int r0 = (int)srcRow, r1 = std::min(r0 + 1, _rows - 1);
                    float tc = srcCol - (float)c0;
                    float tr = srcRow - (float)r0;

                    // If the source cell is a solid, use the current cell's value
                    // (wind doesn't pull heat out of solids)
                    auto sampleOrSelf = [&](int sc, int sr) {
                        return (_mat[Idx(sc, sr, _cols)] == Material::Air)
                            ? _curr[Idx(sc, sr, _cols)]
                            : _curr[idx];
                        };

                    float v = (1.f - tr) * ((1.f - tc) * sampleOrSelf(c0, r0) + tc * sampleOrSelf(c1, r0))
                        + tr * ((1.f - tc) * sampleOrSelf(c0, r1) + tc * sampleOrSelf(c1, r1));

                    _next[idx] = std::clamp(v, 0.f, 1.f);
                }
            }
            std::swap(_curr, _next);
        }
    }

    // ---- Back ---------------------------------------------------------------
    if (ui.IsClicked(back) || _data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.RemoveState();
}

// =============================================================================
void Heat::Render(float dt)
{
    _data->renderer.ReserveDrawCalls(_cols * _rows);

    for (int row = 0; row < _rows; row++)
    {
        for (int col = 0; col < _cols; col++)
        {
            int idx = Idx(col, row, _cols);
            float t = _curr[idx];
            float wx = _area.x + (col + 0.5f) * _cellW;
            float wy = _area.y + (row + 0.5f) * _cellH;

            SDL_FColor color;
            bool alwaysRender = false;

            if (_pinned[idx] < 0)
            {
                // Sink — always visible, dim steel-blue (cold plate)
                color = { 0.10f, 0.16f, 0.30f, 1.f };
                alwaysRender = true;
            }
            else
            {
                Material m = _mat[idx];
                if (m != Material::Air)
                {
                    // Solid materials always render with their own palette
                    color = MaterialColor(m, t);
                    alwaysRender = true;
                }
                else
                {
                    // Air — skip near-zero so background stays dark
                    if (t < 0.005f) continue;
                    color = AirColor(t);

                    // Source tint: warm gold overlay
                    if (_pinned[idx] > 0)
                        color = LerpColor(color, { 1.f, 0.85f, 0.5f, 1.f }, 0.25f);
                }
            }

            if (!alwaysRender && t < 0.005f) continue;

            _data->renderer.SubmitMesh(
                MeshInstance("unit_quad"),
                MaterialInstance("mat"),
                { wx, wy, 0.f },
                { _cellW, _cellH },
                0.f,
                color);
        }
    }
}