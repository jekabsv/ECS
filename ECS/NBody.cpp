#include "NBody.h"
#include <format>
#include <cmath>
#include <algorithm>
#include <random>

static constexpr StringId BB_NBODY_BH("nbody_bh");
static constexpr StringId BB_NBODY_AREA("nbody_area");
static constexpr StringId BB_NBODY_DTSCALE("nbody_dt");
static constexpr StringId BB_NBODY_CURSOR("nbody_cursor");  // read index for integrate

// ============================================================================
// BarnesHut
// ============================================================================

void BarnesHut::BeginFrame()
{
    _x.clear();
    _y.clear();
    _mass.clear();
    _ax.clear();
    _ay.clear();
    _tree.clear();
    _count = 0;
    _solved = false;
}

void BarnesHut::InsertSlice(const NBPos* positions, const NBMass* masses, int count)
{
    _x.reserve(_x.size() + count);
    _y.reserve(_y.size() + count);
    _mass.reserve(_mass.size() + count);
    for (int i = 0; i < count; i++)
    {
        _x.push_back(positions[i].x);
        _y.push_back(positions[i].y);
        _mass.push_back(masses[i].mass);
    }
    _count += count;
}

int BarnesHut::AllocNode(float cx, float cy, float half)
{
    QuadNode n;
    n.cx = cx; n.cy = cy; n.half = half;
    n.totalMass = 0.f; n.comX = 0.f; n.comY = 0.f;
    n.bodyIndex = -1;
    n.children[0] = n.children[1] = n.children[2] = n.children[3] = -1;
    _tree.push_back(n);
    return (int)_tree.size() - 1;
}

int BarnesHut::Quadrant(int node, float bx, float by) const
{
    return ((bx >= _tree[node].cx) ? 1 : 0) | ((by >= _tree[node].cy) ? 2 : 0);
}

void BarnesHut::InsertBody(int node, int bodyIdx, int depth)
{
    if (depth > 64)
    {
        _tree[node].totalMass += _mass[bodyIdx];
        _tree[node].comX += _mass[bodyIdx] * _x[bodyIdx];
        _tree[node].comY += _mass[bodyIdx] * _y[bodyIdx];
        return;
    }

    float bx = _x[bodyIdx];
    float by = _y[bodyIdx];
    float bm = _mass[bodyIdx];

    _tree[node].totalMass += bm;
    _tree[node].comX += bm * bx;
    _tree[node].comY += bm * by;

    if (_tree[node].IsEmpty())
    {
        _tree[node].bodyIndex = bodyIdx;
        return;
    }

    if (_tree[node].IsLeaf() && _tree[node].bodyIndex >= 0)
    {
        int existing = _tree[node].bodyIndex;
        _tree[node].bodyIndex = -2;

        float h2 = _tree[node].half * 0.5f;
        float ncx = _tree[node].cx;
        float ncy = _tree[node].cy;

        for (int q = 0; q < 4; q++)
        {
            float childCx = ncx + ((q & 1) ? h2 : -h2);
            float childCy = ncy + ((q & 2) ? h2 : -h2);
            _tree[node].children[q] = AllocNode(childCx, childCy, h2);
        }

        int q0 = Quadrant(node, _x[existing], _y[existing]);
        InsertBody(_tree[node].children[q0], existing, depth + 1);
    }

    int q = Quadrant(node, bx, by);
    InsertBody(_tree[node].children[q], bodyIdx, depth + 1);
}

void BarnesHut::BuildTree(float rootCx, float rootCy, float rootHalf)
{
    // Size _ax/_ay now that _count is final
    _ax.assign(_count, 0.f);
    _ay.assign(_count, 0.f);

    _tree.reserve(_count * 8);
    AllocNode(rootCx, rootCy, rootHalf);

    for (int i = 0; i < _count; i++)
        InsertBody(0, i);

    for (auto& nd : _tree)
        if (nd.totalMass > 0.f)
        {
            nd.comX /= nd.totalMass;
            nd.comY /= nd.totalMass;
        }
}

void BarnesHut::AccumulateForce(int node, int bodyIdx)
{
    const QuadNode& nd = _tree[node];
    if (nd.totalMass <= 0.f) return;

    float dx = nd.comX - _x[bodyIdx];
    float dy = nd.comY - _y[bodyIdx];
    float distSq = dx * dx + dy * dy + params.softening * params.softening;
    float s = nd.half * 2.f;

    if (nd.IsLeaf() || (s * s < params.theta * params.theta * distSq))
    {
        if (nd.IsLeaf() && nd.bodyIndex == bodyIdx) return;
        float inv = InvSqrt(distSq);
        float inv3 = inv * inv * inv;
        float f = params.G * nd.totalMass * inv3;
        _ax[bodyIdx] += f * dx;
        _ay[bodyIdx] += f * dy;
        return;
    }

    for (int q = 0; q < 4; q++)
        if (nd.children[q] >= 0)
            AccumulateForce(nd.children[q], bodyIdx);
}

void BarnesHut::ComputeForces()
{
    std::fill(_ax.begin(), _ax.end(), 0.f);
    std::fill(_ay.begin(), _ay.end(), 0.f);
    for (int i = 0; i < _count; i++)
        AccumulateForce(0, i);
    _solved = true;
}

// ============================================================================
// Init
// ============================================================================
void NBody::Init()
{
    _data->physics.EnableCollisionDetection(false);
    _data->physics.EnableSpatialIndexBuild(false);
    _data->physics.EnableMovement(false);

    _area = {
        (float)PANEL_WIDTH, 0.f,
        (float)(_data->GAME_WIDTH - PANEL_WIDTH),
        (float)_data->GAME_HEIGHT
    };
    _rootHalf = std::max(_area.w, _area.h) * 0.5f + 1.f;

    _data->session.Set<BarnesHut*>(BB_NBODY_BH, &_bh);
    _data->session.Set<PlayArea>(BB_NBODY_AREA, _area);
    _data->session.Set<float>(BB_NBODY_DTSCALE, _dtScale);
    _data->session.Set<int>(BB_NBODY_CURSOR, 0);

    _data->assets.AddMesh("nbody_dot",
        MeshVertices{
            { -1.f, -1.f, 1.f,1.f,1.f,1.f },
            {  1.f, -1.f, 1.f,1.f,1.f,1.f },
            {  1.f,  1.f, 1.f,1.f,1.f,1.f },
            { -1.f,  1.f, 1.f,1.f,1.f,1.f },
        },
        { 0u,1u,2u,0u,2u,3u });

    // -------------------------------------------------------------------------
    // System ordering summary:
    //
    //   Actual engine order per tick (from Engine::Update):
    //     ecs.Run(PreUpdate)  — nbody_snapshot fills BH
    //     ecs.Run(Update)     — nbody_solve builds tree + computes forces
    //     State::Update()     — NBody::Update() runs (UI, param sync, etc.)
    //     ecs.Run(PostUpdate) — nbody_integrate reads forces + integrates
    //
    //   BeginFrame() is called at the START of nbody_snapshot, but only when
    //   the previous frame's solve has already completed (_solved == true).
    //   This means:
    //     - Frame 0: _solved=false → no reset, snapshot inserts, solve runs
    //     - Frame 1+: _solved=true → reset at top of snapshot, then insert fresh
    //
    //   This guarantees integrate always sees a valid, solved BH.
    // -------------------------------------------------------------------------

    // PreUpdate: reset BH if last frame was solved, then fill from ECS chunks
    ecs.RegisterSystem<NBPos, NBMass>(
        "nbody_snapshot",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            BarnesHut* bh = data->session.Get<BarnesHut*>(BB_NBODY_BH);

            // Reset at the start of a new frame, but only after solve completed.
            // This fires on the first chunk each frame (Solved flips to false
            // after BeginFrame, so subsequent chunks skip the reset).
            if (bh->Solved())
            {
                bh->BeginFrame();
                data->session.Set<int>(BB_NBODY_CURSOR, 0);
            }

            auto positions = ctx.Slice<NBPos>();
            auto masses = ctx.Slice<NBMass>();
            bh->InsertSlice(positions.data(), masses.data(), (int)positions.size());
        },
        ECS::SystemGroup::PreUpdate)
        .Read<NBPos>()
        .Read<NBMass>();

    // Update: build tree + solve forces — runs once per frame (guard with Solved())
    ecs.RegisterSystem<NBPos, NBMass>(
        "nbody_solve",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            BarnesHut* bh = data->session.Get<BarnesHut*>(BB_NBODY_BH);
            if (bh->Solved()) return; // already done this frame

            const auto& area = data->session.Get<NBody::PlayArea>(BB_NBODY_AREA);
            float rootCx = area.x + area.w * 0.5f;
            float rootCy = area.y + area.h * 0.5f;
            float rootHalf = std::max(area.w, area.h) * 0.5f + 1.f;

            bh->BuildTree(rootCx, rootCy, rootHalf);
            bh->ComputeForces();
        },
        ECS::SystemGroup::Update)
        .Read<NBPos>()
        .Read<NBMass>();

    // PostUpdate: read solved forces + integrate per chunk
    ecs.RegisterSystem<NBPos, NBVel, NBAccel>(
        "nbody_integrate",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const BarnesHut* bh = data->session.Get<BarnesHut*>(BB_NBODY_BH);
            const float      dtScale = data->session.Get<float>(BB_NBODY_DTSCALE);
            int& readIdx = data->session.Get<int>(BB_NBODY_CURSOR);

            // Guard: if BH wasn't solved (e.g. paused / first frame), skip
            if (!bh->Solved()) return;

            auto  pos = ctx.Slice<NBPos>();
            auto  vel = ctx.Slice<NBVel>();
            auto  accel = ctx.Slice<NBAccel>();
            int   count = (int)pos.size();
            float simDt = dt * dtScale;

            for (int i = 0; i < count; i++, readIdx++)
            {
                accel[i].ax = bh->GetAx(readIdx);
                accel[i].ay = bh->GetAy(readIdx);
                vel[i].vx += accel[i].ax * simDt;
                vel[i].vy += accel[i].ay * simDt;
                pos[i].x += vel[i].vx * simDt;
                pos[i].y += vel[i].vy * simDt;
            }
        },
        ECS::SystemGroup::PostUpdate)
        .Write<NBPos>()
        .Write<NBVel>()
        .Write<NBAccel>();

    // Render: cull to play area, submit visible bodies
    ecs.RegisterSystem<NBPos, NBColor>(
        "nbody_render",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            const auto& area = data->session.Get<NBody::PlayArea>(BB_NBODY_AREA);
            auto positions = ctx.Slice<NBPos>();
            auto colors = ctx.Slice<NBColor>();
            float xMin = area.x, xMax = area.x + area.w;
            float yMin = area.y, yMax = area.y + area.h;

            for (std::size_t i = 0; i < positions.size(); i++)
            {
                if (positions[i].x < xMin || positions[i].x > xMax) continue;
                if (positions[i].y < yMin || positions[i].y > yMax) continue;
                data->renderer.SubmitMesh(
                    MeshInstance("nbody_dot"),
                    MaterialInstance("mat"),
                    { positions[i].x, positions[i].y, 0.2f },
                    { 1.5f, 1.5f },
                    0.f,
                    colors[i].color);
            }
        },
        ECS::SystemGroup::Render)
        .Read<NBPos>()
        .Read<NBColor>();

    ResetSimulation();

    // ---- UI -----------------------------------------------------------------
    ui.GetTheme().LoadDarkDefaults();
    ui.GetTheme().SetToken("font-default", StringId("tnr"));

    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(PANEL_WIDTH), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::FlexStart);
    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 5.f);
    ui.SetPadding(root, UI::Edges::All(14.f));

    back = ui.AddButton("Back to menu", root);
    ui.SetSize(back, UI::SizeValue::Auto(), UI::SizeValue::Px(44));

    bodyCountLabel = ui.AddLabel(std::format("Bodies: {}", _targetCount), root);
    ui.SetSize(bodyCountLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(22));

    ui.SetSize(ui.AddLabel("Body count:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    countInput = ui.AddInputField(std::format("{}", DEFAULT_COUNT), root);
    ui.SetSize(countInput, UI::SizeValue::Auto(), UI::SizeValue::Px(44));
    countSlider = ui.AddSlider((float)DEFAULT_COUNT, (float)MIN_COUNT, (float)MAX_COUNT, root);

    thetaLabel = ui.AddLabel(std::format("Theta: {:.2f}", _bh.params.theta), root);
    ui.SetSize(thetaLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    thetaSlider = ui.AddSlider(_bh.params.theta, 0.1f, 1.5f, root);

    gLabel = ui.AddLabel(std::format("G: {:.0f}", _bh.params.G), root);
    ui.SetSize(gLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    gSlider = ui.AddSlider(_bh.params.G, 50.f, 5000.f, root);

    softLabel = ui.AddLabel(std::format("Softening: {:.1f}", _bh.params.softening), root);
    ui.SetSize(softLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    softSlider = ui.AddSlider(_bh.params.softening, 1.f, 50.f, root);

    dtLabel = ui.AddLabel(std::format("Time scale: {:.2f}", _dtScale), root);
    ui.SetSize(dtLabel, UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    dtSlider = ui.AddSlider(_dtScale, 0.1f, 4.0f, root);

    resetButton = ui.AddButton("Reset", root);
    ui.SetSize(resetButton, UI::SizeValue::Auto(), UI::SizeValue::Px(44));

    pauseButton = ui.AddButton("Pause", root);
    ui.SetSize(pauseButton, UI::SizeValue::Auto(), UI::SizeValue::Px(44));
}

// ============================================================================
// Spawn
// ============================================================================
void NBody::SpawnGalaxy(float cx, float cy, float radius, int n,
    float totalMass, float baseSpin, SDL_FColor col)
{
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> angleDist(0.f, 6.2831853f);
    std::uniform_real_distribution<float> radiusDist(0.f, 1.f);
    std::uniform_real_distribution<float> colorJitter(-0.08f, 0.08f);

    float massPerBody = totalMass / (float)n;

    for (int i = 0; i < n; i++)
    {
        float angle = angleDist(rng);
        float r = radius * std::sqrt(radiusDist(rng));
        float bx = cx + r * std::cos(angle);
        float by = cy + r * std::sin(angle);

        float enclosed = totalMass * (r / radius) * (r / radius);
        float speed = (r > 1.f) ? std::sqrt(_bh.params.G * enclosed / r) : 0.f;

        ECS::Entity e = ecs.Create();
        ecs.Add<NBPos>(e, { bx, by });
        ecs.Add<NBVel>(e, { -std::sin(angle) * speed * baseSpin,
                                std::cos(angle) * speed * baseSpin });
        ecs.Add<NBAccel>(e, { 0.f, 0.f });
        ecs.Add<NBMass>(e, { massPerBody });
        ecs.Add<NBColor>(e, { {
            std::clamp(col.r + colorJitter(rng), 0.f, 1.f),
            std::clamp(col.g + colorJitter(rng), 0.f, 1.f),
            std::clamp(col.b + colorJitter(rng), 0.f, 1.f),
            col.a
        } });
    }
}

void NBody::ResetSimulation()
{
    std::vector<ECS::Entity> toDestroy;
    for (auto& ctx : ecs.View<NBPos>())
    {
        auto entities = ctx.Slice<ECS::Entity>();
        for (auto e : entities)
            toDestroy.push_back(e);
    }
    for (auto e : toDestroy)
        ecs.Destroy(e);

    float cx = _area.x + _area.w * 0.5f;
    float cy = _area.h * 0.5f;
    int   half = _targetCount / 2;
    int   rest = _targetCount - half;

    SpawnGalaxy(cx - _area.w * 0.2f, cy, _area.h * 0.22f, half,
        half * 12.f, 1.f, { 0.55f, 0.75f, 1.0f, 1.f });
    SpawnGalaxy(cx + _area.w * 0.2f, cy, _area.h * 0.18f, rest,
        rest * 10.f, -1.f, { 1.0f, 0.65f, 0.25f, 1.f });

    // Clear BH state and cursor so snapshot starts clean next PreUpdate
    _bh.BeginFrame();
    _data->session.Set<int>(BB_NBODY_CURSOR, 0);
}

// ============================================================================
// Update
//
// Actual engine order per tick (Engine::Update):
//   ecs.Run(PreUpdate)  — nbody_snapshot  (resets BH if solved, then fills it)
//   ecs.Run(Update)     — nbody_solve     (builds tree + computes forces)
//   State::Update()     — this function   (UI, param sync — BH already solved)
//   ecs.Run(PostUpdate) — nbody_integrate (reads forces + integrates positions)
//
// BeginFrame() is NO LONGER called here. It is called at the top of
// nbody_snapshot when _solved == true, ensuring it always fires at the
// start of a new PreUpdate pass, never after solve but before integrate.
// ============================================================================
void NBody::Update(float dt)
{
    // ---- UI -----------------------------------------------------------------
    bool curFocused = (ui.Poll(countInput) == UI::InteractionState::Focused);
    bool justUnfocused = (!curFocused && _focusedCount);
    _focusedCount = curFocused;

    if (justUnfocused)
    {
        try
        {
            int v = std::clamp((int)std::stof(ui.GetInputValue(countInput)), MIN_COUNT, MAX_COUNT);
            _targetCount = v;
            ui.SetSliderValue(countSlider, (float)v);
            ui.SetInputValue(countInput, std::format("{}", v));
            ResetSimulation();
        }
        catch (...) {}
    }

    if (!curFocused)
    {
        int sliderVal = (int)ui.GetSliderValue(countSlider);
        if (sliderVal != _targetCount)
        {
            _targetCount = sliderVal;
            ResetSimulation();
        }
        ui.SetInputValue(countInput, std::format("{}", _targetCount));
    }

    _bh.params.theta = ui.GetSliderValue(thetaSlider);
    _bh.params.G = ui.GetSliderValue(gSlider);
    _bh.params.softening = ui.GetSliderValue(softSlider);
    _dtScale = ui.GetSliderValue(dtSlider);

    _data->session.Set<float>(BB_NBODY_DTSCALE, _dtScale);

    ui.SetText(thetaLabel, std::format("Theta: {:.2f}", _bh.params.theta));
    ui.SetText(gLabel, std::format("G: {:.0f}", _bh.params.G));
    ui.SetText(softLabel, std::format("Softening: {:.1f}", _bh.params.softening));
    ui.SetText(dtLabel, std::format("Time scale: {:.2f}", _dtScale));
    ui.SetText(bodyCountLabel, std::format("Bodies: {}", _targetCount));

    if (ui.IsClicked(resetButton)) ResetSimulation();

    if (ui.IsClicked(pauseButton))
    {
        _paused = !_paused;
        ui.SetText(pauseButton, _paused ? "Resume" : "Pause");
        if (_paused)
        {
            ecs.DisableSystem("nbody_snapshot");
            ecs.DisableSystem("nbody_solve");
            ecs.DisableSystem("nbody_integrate");
        }
        else
        {
            ecs.EnableSystem("nbody_snapshot");
            ecs.EnableSystem("nbody_solve");
            ecs.EnableSystem("nbody_integrate");
            // Clear stale state so snapshot starts fresh on resume
            _bh.BeginFrame();
            _data->session.Set<int>(BB_NBODY_CURSOR, 0);
        }
    }

    if (ui.IsClicked(back) || _data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.RemoveState();
}

// ============================================================================
// Render
// ============================================================================
void NBody::Render(float dt)
{
    _data->renderer.ReserveDrawCalls(_targetCount);
}