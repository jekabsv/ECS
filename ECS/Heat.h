#pragma once
#include "SharedDataRef.h"
#include "State.h"

class Heat : public State
{
public:
    using State::State;
    void Init()           override;
    void Update(float dt) override;
    void Render(float dt) override;

    // -------------------------------------------------------------------------
    // Materials  (order must match MAT_PROPS table in .cpp)
    // -------------------------------------------------------------------------
    enum class Material : uint8_t { Air = 0, Copper, Wood, Insulator };

    // -------------------------------------------------------------------------
    // Material conductivity multipliers  (relative to base diffusivity)
    //   Copper   — excellent conductor, spreads heat fast in all directions
    //   Wood     — poor conductor, takes a long time to heat / cool
    //   Insulator— barely conducts, holds temperature but transfers almost nothing
    // -------------------------------------------------------------------------
    static constexpr float COND_AIR = 1.0f;
    static constexpr float COND_COPPER = 10.0f;
    static constexpr float COND_WOOD = 0.18f;
    static constexpr float COND_INSULATOR = 0.04f;

    // -------------------------------------------------------------------------
    // Per-material ambient cooling rate  (fraction of temperature lost per second)
    //   Copper cools quickly because it conducts heat away rapidly.
    //   Wood / insulator retain heat much longer.
    // -------------------------------------------------------------------------
    static constexpr float COOL_AIR = 0.015f;
    static constexpr float COOL_COPPER = 0.022f;
    static constexpr float COOL_WOOD = 0.004f;
    static constexpr float COOL_INSULATOR = 0.002f;

    // -------------------------------------------------------------------------
    // Grid resolution
    // -------------------------------------------------------------------------
    static constexpr int   GRID_COLS = 160;
    static constexpr int   GRID_ROWS = 100;

    // -------------------------------------------------------------------------
    // Diffusion
    // -------------------------------------------------------------------------
    static constexpr float DEFAULT_DIFFUSIVITY = 0.35f;
    static constexpr float MIN_DIFFUSIVITY = 0.01f;
    static constexpr float MAX_DIFFUSIVITY = 1.5f;

    // -------------------------------------------------------------------------
    // Brush
    // -------------------------------------------------------------------------
    static constexpr int   DEFAULT_BRUSH_RADIUS = 4;
    static constexpr int   MIN_BRUSH_RADIUS = 1;
    static constexpr int   MAX_BRUSH_RADIUS = 20;
    static constexpr float BRUSH_STRENGTH = 0.90f;

    // -------------------------------------------------------------------------
    // Pinned cells
    // -------------------------------------------------------------------------
    static constexpr float SOURCE_VALUE = 1.0f;
    static constexpr float SINK_VALUE = 0.0f;

    // -------------------------------------------------------------------------
    // Wind / advection
    // -------------------------------------------------------------------------
    static constexpr float DEFAULT_WIND_X = 0.0f;
    static constexpr float DEFAULT_WIND_Y = 0.0f;
    static constexpr float MAX_WIND_SPEED = 40.0f;

    // -------------------------------------------------------------------------
    // Simulation stability
    // -------------------------------------------------------------------------
    static constexpr float MAX_SIM_DT = 0.012f;

    // -------------------------------------------------------------------------
    // Panel
    // -------------------------------------------------------------------------
    static constexpr int   PANEL_WIDTH = 240;

private:
    struct PlayArea { float x, y, w, h; };

    void ResetGrid();
    void StampBrush(float wx, float wy);

    // =========================================================================
    // Runtime
    // =========================================================================
    PlayArea _area{};
    int   _cols = GRID_COLS;
    int   _rows = GRID_ROWS;
    float _cellW = 1.f;
    float _cellH = 1.f;

    std::vector<float>    _curr;    // temperature  [0, 1]
    std::vector<float>    _next;
    std::vector<Material> _mat;     // per-cell material
    std::vector<int8_t>   _pinned;  // 0=free, +1=source, -1=sink

    float _diffusivity = DEFAULT_DIFFUSIVITY;
    int   _brushRadius = DEFAULT_BRUSH_RADIUS;

    float _windX = DEFAULT_WIND_X;
    float _windY = DEFAULT_WIND_Y;

    enum class BrushMode
    {
        Hot,        // paint transient heat into any cell
        Sink,       // persistent cold plate (pinned to 0)
        Source,     // persistent heat source (pinned to 1)
        Copper,     // paint copper material
        Wood,       // paint wood material
        Insulator,  // paint insulator material
        Erase,      // restore cell to Air, unpin
    };
    BrushMode _brushMode = BrushMode::Hot;

    bool _prevClickHeld = false;

    // =========================================================================
    // UI handles
    // =========================================================================
    UI::NodeHandle back = UI::NULL_HANDLE;
    UI::NodeHandle diffSlider = UI::NULL_HANDLE;
    UI::NodeHandle diffInput = UI::NULL_HANDLE;
    UI::NodeHandle brushSlider = UI::NULL_HANDLE;
    UI::NodeHandle brushInput = UI::NULL_HANDLE;
    UI::NodeHandle modeButton = UI::NULL_HANDLE;
    UI::NodeHandle resetButton = UI::NULL_HANDLE;
    UI::NodeHandle windXSlider = UI::NULL_HANDLE;
    UI::NodeHandle windYSlider = UI::NULL_HANDLE;
    UI::NodeHandle windLabel = UI::NULL_HANDLE;

    bool _focusedDiff = false;
    bool _focusedBrush = false;
};