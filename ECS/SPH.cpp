#include "SPH.h"
#include "MeshComponent.h"
#include "Transform.h"
#include "RigidBody.h"
#include "BoxCollider.h"
#include <SDL3/SDL_mouse.h>
#include <format>
#include <cmath>
#include <algorithm>
#include "EngineMath.h"

using namespace Math;

static const StringId BB_SPH_AREA = StringId("sph_area");
static const StringId BB_SPH_H = StringId("sph_h");
static const StringId BB_SPH_MAXSPEED = StringId("sph_maxspeed");
static const StringId BB_SPH_REST_RHO = StringId("sph_rest_rho");
static const StringId COLOR_MODE = StringId("sph_color_mode");

static const StringId BB_OBJ_FX = StringId("obj_fx");
static const StringId BB_OBJ_FY = StringId("obj_fy");
static const StringId BB_OBJ_TORQUE = StringId("obj_torque");
static const StringId BB_OBJ_PX = StringId("obj_px");
static const StringId BB_OBJ_PY = StringId("obj_py");
static const StringId BB_OBJ_SUBMERGED = StringId("obj_submerged");

struct GhostParticle { float localX = 0, localY = 0; };
struct GhostSPH { float density = 0, pressure = 0; };
struct SPHParticle { float density = 0, pressure = 0, fx = 0, fy = 0; };

namespace SPHKernel 
{
    inline float Poly6(float rSq, float hSq)
    {
        if (rSq >= hSq) return 0.f;
        float d = hSq - rSq;
        return (4.f / (3.14159265f * hSq * hSq * hSq * hSq)) * d * d * d;
    }
    inline float SpikyGrad(float r, float h) 
    {
        if (r <= 0.f || r >= h) return 0.f;
        float d = h - r;
        return (45.f / (3.14159265f * h * h * h * h * h * h)) * d * d;
    }
    inline float ViscLap(float r, float h) 
    {
        if (r >= h) return 0.f;
        return (45.f / (3.14159265f * h * h * h * h * h * h)) * (h - r);
    }
}

static float ComputeRestDensity(float H, float spacing) 
{
    const float hSq = H * H; float rho = 0;
    for (float dy = -H; dy <= H; dy += spacing)
        for (float dx = -H; dx <= H; dx += spacing)
            rho += SPHKernel::Poly6(dx * dx + dy * dy, hSq);
    return rho;
}

static std::vector<std::pair<float, float>> BuildRectOffsets(float hw, float hh, float sp)
{
    std::vector<std::pair<float, float>> pts;
    // BuildRectOffsets
    auto edge = [&](float x0, float y0, float x1, float y1)
        {
            Vec2 d = { x1 - x0, y1 - y0 };
            int n = std::max(1, (int)(d.Length() / sp));
            for (int i = 0; i <= n; i++)
            {
                float t = (float)i / n;
                pts.push_back({ x0 + d.x * t, y0 + d.y * t });
            }
        };
    edge(-hw, -hh, hw, -hh); 
    edge(hw, -hh, hw, hh);
    edge(hw, hh, -hw, hh); 
    edge(-hw, hh, -hw, -hh);
    for (float y = -hh + sp; y < hh; y += sp)
        for (float x = -hw + sp; x < hw; x += sp)
            pts.push_back({ x, y });
    return pts;
}


static uint32_t maxId = 0;

void SPH::SpawnParticle(const PlayArea& area)
{
    ECS::Entity e = ecs.Create();
    maxId = std::max(maxId, ECS::EntityId(e));
    Vec2 pos = RandPointInRect(area.x + area.w * 0.35f, area.y + area.h * 0.10f, area.w * 0.30f, area.h * 0.50f);
    TransformComponent tr; tr.position = { pos.x, pos.y }; tr.scale = { 8, 8 }; tr.rotation = 0;
    RigidBody rb; rb.vx = rb.vy = 0; rb.isStatic = false; rb.drag = 0;
    ecs.Add<TransformComponent>(e, tr);
    ecs.Add<RigidBody>(e, rb);
    ecs.Add<BoxCollider>(e, BoxCollider(1/8.f, 1/8.f / 2, true));
    ecs.Add<MeshComponent>(e, MeshComponent("unit_circle32", "mat", true));
    ecs.Add<SPHParticle>(e, SPHParticle{});
    _particles.push_back(e); _currentCount++;
}

void SPH::DespawnLastParticle() 
{
    if (_particles.empty()) 
        return;
    ecs.Destroy(_particles.back()); _particles.pop_back(); _currentCount--;
}

void SPH::SpawnRigidObject(float cx, float cy)
{
    if (_rigidObj.active) DestroyRigidObject();
    _rigidObj = RigidObject{};
    _rigidObj.px = cx; _rigidObj.py = cy;
    _rigidObj.held = true; _rigidObj.active = true;
    float W = _rigidObj.hw * 2, H2 = _rigidObj.hh * 2;
    _rigidObj.inertia = (float)_objMass * (W * W + H2 * H2) / 12.f;

    auto offsets = BuildRectOffsets(_rigidObj.hw, _rigidObj.hh, GHOST_SPACING);
    _rigidObj.totalGhosts = (int)offsets.size();

    for (auto& [lx, ly] : offsets) 
    {
        ECS::Entity e = ecs.Create();
        TransformComponent tr; tr.position = { cx + lx, cy + ly };
        tr.scale = { 1, 1 };; tr.rotation = 0;
        GhostParticle gp{ lx, ly };
        GhostSPH gs{};
        ecs.Add<TransformComponent>(e, tr);
        ecs.Add<GhostParticle>(e, gp);
        ecs.Add<GhostSPH>(e, gs);
        ecs.Add<BoxCollider>(e, BoxCollider(H * 0.5f, H * 0.5f, true));
        _rigidObj.ghosts.push_back(e);
    }
}

void SPH::DestroyRigidObject() 
{
    for (ECS::Entity e : _rigidObj.ghosts) ecs.Destroy(e);
    _rigidObj.ghosts.clear(); _rigidObj.active = false;
}

void SPH::SyncGhostPositions() 
{
    if (!_rigidObj.active) 
        return;

    Vec2 rot = { std::cos(_rigidObj.angle), std::sin(_rigidObj.angle) };
    for (ECS::Entity e : _rigidObj.ghosts)
    {
        auto* tr = ecs.TryGet<TransformComponent>(e);
        auto* gp = ecs.TryGet<GhostParticle>(e);
        if (!tr || !gp) continue;
        Vec2 local = { gp->localX, gp->localY };
        tr->position.x = _rigidObj.px + rot.x * local.x - rot.y * local.y;
        tr->position.y = _rigidObj.py + rot.y * local.x + rot.x * local.y;
    }
}

void SPH::Init()
{
    _data->physics.EnableCollisionDetection(false);
    _data->physics.EnableSpatialIndexBuild(true);
    _data->physics.EnableMovement(false);

    const PlayArea area{ (float)PANEL_WIDTH, 0, (float)_data->GAME_WIDTH - PANEL_WIDTH, (float)_data->GAME_HEIGHT };
    const float restDensity = ComputeRestDensity(H, H * 0.5f);

    _data->session.Set<PlayArea>(BB_SPH_AREA, area);
    _data->session.Set<float>(BB_SPH_H, H);
    _data->session.Set<float>(BB_SPH_MAXSPEED, MAX_SPEED);
    _data->session.Set<float>(BB_SPH_REST_RHO, restDensity);
    _data->session.Set<int>(COLOR_MODE, 0);
    _data->session.Set<float>(BB_OBJ_FX, 0.f);
    _data->session.Set<float>(BB_OBJ_FY, 0.f);
    _data->session.Set<float>(BB_OBJ_TORQUE, 0.f);
    _data->session.Set<float>(BB_OBJ_PX, 0.f);
    _data->session.Set<float>(BB_OBJ_PY, 0.f);
    _data->session.Set<int>(BB_OBJ_SUBMERGED, 0);

    _data->spatialIndex.Init(area.x, area.y, area.w, area.h, H);


    _particles.reserve(MAX_COUNT);
    for (int i = 0; i < DEFAULT_COUNT; i++) 
        SpawnParticle(area);

    ecs.RegisterSystem<GhostParticle, GhostSPH>(
        "ghostDensityFill",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const float restRho = data->session.Get<float>(BB_SPH_REST_RHO);
            auto gs = ctx.Slice<GhostSPH>();
            for (std::size_t i = 0; i < gs.size(); i++) {
                gs[i].density = restRho * 1.8f;
                float ratio = 1.8f, r2 = ratio * ratio, r7 = r2 * r2 * r2 * ratio;
                float p = GAS_CONSTANT * (r7 - 1.f);
                gs[i].pressure = p > 0 ? p : 0;
            }
        },
        ECS::SystemGroup::PreUpdate);

    ecs.RegisterSystem<TransformComponent, RigidBody, SPHParticle>(
        "sphDensity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const float H = data->session.Get<float>(BB_SPH_H);
            const float hSq = H * H;
            const float restRho = data->session.Get<float>(BB_SPH_REST_RHO);
            auto trs = ctx.Slice<TransformComponent>();
            auto sph = ctx.Slice<SPHParticle>();
            auto ents = ctx.Slice<ECS::Entity>();
            std::vector<ECS::Entity> found; found.reserve(64);

            int totalNeighbours = 0;
            int zeroNeighbourCount = 0;
            bool printed = false;


            for (std::size_t i = 0; i < ents.size(); i++) {
                found.clear();
                data->spatialIndex.QueryRectangle(
                    trs[i].position.x - H, trs[i].position.y - H, H * 2, H * 2, found);


                float rho = 0;
                for (ECS::Entity o : found) {
                    auto* oTr = data->physics.GetWorld()->TryGet<TransformComponent>(o);
                    if (!oTr) continue;
                    Vec2 d = { oTr->position.x - trs[i].position.x,
                                oTr->position.y - trs[i].position.y };
                    rho += SPHKernel::Poly6(d.LengthSq(), hSq);
                }
                sph[i].density = rho;
                float ratio = (restRho > 1e-6f) ? rho / restRho : 1.f;
                float r2 = ratio * ratio, r4 = r2 * r2, r7 = r4 * r2 * ratio;
                float p = GAS_CONSTANT * (r7 - 1.f);
                sph[i].pressure = p > 0 ? p : 0;
            }
        },
        ECS::SystemGroup::PreUpdate);

  
    ecs.RegisterSystem<TransformComponent, RigidBody, SPHParticle>(
        "sphForces",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const float H = data->session.Get<float>(BB_SPH_H);
            auto trs = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            auto sph = ctx.Slice<SPHParticle>();
            auto ents = ctx.Slice<ECS::Entity>();

            const float objPx = data->session.Get<float>(BB_OBJ_PX);
            const float objPy = data->session.Get<float>(BB_OBJ_PY);
            float& netFx = data->session.Get<float>(BB_OBJ_FX);
            float& netFy = data->session.Get<float>(BB_OBJ_FY);
            float& netTorque = data->session.Get<float>(BB_OBJ_TORQUE);
            int& submerged = data->session.Get<int>(BB_OBJ_SUBMERGED);

            std::vector<ECS::Entity> found; found.reserve(128);

            for (std::size_t i = 0; i < ents.size(); i++) {
                auto& tr = trs[i]; auto& rb = rbs[i]; auto& sp = sph[i];
                sp.fx = sp.fy = 0;
                if (sp.density < 1e-6f) continue;

                found.clear();
                data->spatialIndex.QueryRectangle(
                    tr.position.x - H, tr.position.y - H, H * 2, H * 2, found);

                float fpx = 0, fpy = 0, fvx = 0, fvy = 0, pressAcc = 0;
                static constexpr float PRESSURE_CAP = 1500000.f;

                for (ECS::Entity o : found) {
                    if (o == ents[i]) continue;
                    if (pressAcc > PRESSURE_CAP) break;
                    auto* oTr = data->physics.GetWorld()->TryGet<TransformComponent>(o);
                    if (!oTr) 
                        continue;

                    Vec2 d = { oTr->position.x - tr.position.x,
                            oTr->position.y - tr.position.y };
                    float r = d.Length();
                    if (r < 1e-4f || r >= H) continue;
                    Vec2 n = d * (1.f / r);
                    float grad = SPHKernel::SpikyGrad(r, H);

                    auto* oSph = data->physics.GetWorld()->TryGet<SPHParticle>(o);
                    auto* oRb = data->physics.GetWorld()->TryGet<RigidBody>(o);
                    auto* oGs = data->physics.GetWorld()->TryGet<GhostSPH>(o);

                    float oPress = 0, oDens = 1; bool isGhost = false;
                    if (oSph && oSph->density > 1e-6f) {
                        oPress = oSph->pressure; oDens = oSph->density;
                    }
                    else if (oGs && oGs->density > 1e-6f) {
                        oPress = oGs->pressure; oDens = oGs->density; isGhost = true;
                    }
                    else continue;

                    float pContrib = (sp.pressure + oPress) / (2.f * oDens) * grad;
                    pressAcc += pContrib;
                    fpx -= pContrib * n.x;  fpy -= pContrib * n.y;

                    if (oRb && oSph) 
                    {
                        float vt = VISCOSITY / oDens * SPHKernel::ViscLap(r, H);
                        fvx += vt * (oRb->vx - rb.vx);  fvy += vt * (oRb->vy - rb.vy);
                    }

                    if (isGhost) 
                    {
                        float rfx = pContrib * n.x, rfy = pContrib * n.y;
                        netFx += rfx; netFy += rfy;
                        Vec2 arm = { oTr->position.x - objPx, oTr->position.y - objPy };
                        netTorque += arm.Cross({ rfx, rfy });
                        submerged++;
                    }
                }
                sp.fx = fpx + fvx; sp.fy = fpy + fvy;
            }
        },
        ECS::SystemGroup::PreUpdate);


    ecs.RegisterSystem<TransformComponent, RigidBody, SPHParticle>(
        "sphIntegrate",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const auto  area = data->session.Get<PlayArea>(BB_SPH_AREA);
            const float maxSpd = data->session.Get<float>(BB_SPH_MAXSPEED);
            auto trs = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            auto sph = ctx.Slice<SPHParticle>();
            for (std::size_t i = 0; i < trs.size(); i++) {
                auto& tr = trs[i];
                auto& rb = rbs[i]; 
                auto& sp = sph[i];

                Vec2 vel = { rb.vx, rb.vy };
                if (sp.density < 1e-6f)
                    vel.y += GRAVITY * dt;
                else
                    vel += (Vec2{ sp.fx, sp.fy } *(dt / sp.density)) + Vec2{ 0, GRAVITY * dt };

                vel = vel.ClampedMagnitude(maxSpd);
                rb.vx = vel.x;  rb.vy = vel.y;
                tr.position.x += rb.vx * dt;
                tr.position.y += rb.vy * dt;

                const float minX = area.x + 4, maxX = area.x + area.w - 4, minY = area.y + 4, maxY = area.y + area.h - 4;
                if (tr.position.x < minX) { tr.position.x = minX; if (rb.vx < 0) rb.vx = -rb.vx * DAMPING; }
                if (tr.position.x > maxX) { tr.position.x = maxX; if (rb.vx > 0) rb.vx = -rb.vx * DAMPING; }
                if (tr.position.y < minY) { tr.position.y = minY; if (rb.vy < 0) rb.vy = -rb.vy * DAMPING; }
                if (tr.position.y > maxY) { tr.position.y = maxY; if (rb.vy > 0) rb.vy = -rb.vy * DAMPING; rb.vx *= 0.85f; }
            }
        },
        ECS::SystemGroup::Update);


    ecs.RegisterSystem<TransformComponent, MeshComponent, RigidBody, SPHParticle>(
        "sphRender",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto trs = ctx.Slice<TransformComponent>();
            auto meshes = ctx.Slice<MeshComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            auto sph = ctx.Slice<SPHParticle>();
            const float restRho = data->session.Get<float>(BB_SPH_REST_RHO);
            for (std::size_t i = 0; i < trs.size(); i++) {
                if (!meshes[i].render) continue;
                int MODE = data->session.Get<int>(COLOR_MODE);
                SDL_FColor c = { 1, 1, 1, 1 };
                if (MODE == 1) {
                    float t = std::clamp(std::log1p(std::max(sph[i].pressure, 1000.f) - 1000.f) / std::log1p(99000.f), 0.f, 1.f);
                    c = t < 0.5f ? LerpColor({ 0.05f,0.05f,0.20f,1 }, { 1.f,0.20f,0.f,1 }, t * 2.f)
                        : LerpColor({ 1.f,0.20f,0.f,1 }, { 1.f,1.f,0.80f,1 }, (t - 0.5f) * 2.f);
                }
                else if (MODE == 2) {
                    float d = std::clamp((sph[i].density / restRho - 0.5f) / 0.75f, 0.f, 1.f);
                    c = d < 0.33f ? LerpColor({ 0.20f,0.85f,0.80f,1 }, { 0.10f,0.55f,0.85f,1 }, d * 3.f)
                        : d < 0.66f ? LerpColor({ 0.10f,0.55f,0.85f,1 }, { 0.10f,0.20f,0.65f,1 }, (d - 0.33f) * 3.f)
                        : LerpColor({ 0.10f,0.20f,0.65f,1 }, { 0.08f,0.10f,0.35f,1 }, (d - 0.66f) * 3.f);
                }
                else if (MODE == 3) {
                    float s = std::clamp(std::sqrt(rbs[i].vx * rbs[i].vx + rbs[i].vy * rbs[i].vy) / 250.f, 0.f, 1.f);
                    c = s < 0.33f ? LerpColor({ 0.45f,0.f,0.90f,1 }, { 0.f,0.70f,0.95f,1 }, s * 3.f)
                        : s < 0.66f ? LerpColor({ 0.f,0.70f,0.95f,1 }, { 0.20f,0.95f,0.30f,1 }, (s - 0.33f) * 3.f)
                        : LerpColor({ 0.20f,0.95f,0.30f,1 }, { 1.f,0.98f,0.f,1 }, (s - 0.66f) * 3.f);
                }
                data->renderer.SubmitMesh(
                    meshes[i].MeshName, meshes[i].Material,
                    trs[i].position, trs[i].scale, trs[i].rotation, c);
            }
        },
        ECS::SystemGroup::Render);

    ui.GetTheme().LoadDarkDefaults();
    ui.GetTheme().SetToken("font-default", StringId("tnr"));
    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(PANEL_WIDTH), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::FlexStart);
    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 6.f);
    ui.SetPadding(root, UI::Edges::All(20.f));

    back = ui.AddButton("Back to menu", root);
    ui.SetSize(back, UI::SizeValue::Auto(), UI::SizeValue::Px(60));

    ui.SetSize(ui.AddLabel("Particle count:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    input = ui.AddInputField(std::format("{}", DEFAULT_COUNT), root);
    ui.SetSize(input, UI::SizeValue::Auto(), UI::SizeValue::Px(60));
    slider = ui.AddSlider((float)DEFAULT_COUNT, (float)MIN_COUNT, (float)MAX_COUNT, root);

    ui.SetSize(ui.AddLabel("Kernel radius (H):", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    radiusInput = ui.AddInputField(std::format("{:.0f}", H), root);
    ui.SetSize(radiusInput, UI::SizeValue::Auto(), UI::SizeValue::Px(60));

    ui.SetSize(ui.AddLabel("Object mass:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    massInput = ui.AddInputField(std::format("{}", _objMass), root);
    ui.SetSize(massInput, UI::SizeValue::Auto(), UI::SizeValue::Px(60));

    ui.SetSize(ui.AddLabel("Interaction mode:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    modeButton = ui.AddButton("Mode: Attract", root);
    ui.SetSize(modeButton, UI::SizeValue::Auto(), UI::SizeValue::Px(60));

    removeObjButton = ui.AddButton("Remove Object", root);
    ui.SetSize(removeObjButton, UI::SizeValue::Auto(), UI::SizeValue::Px(60));

    ui.SetSize(ui.AddLabel("Color mode:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    colorModeButton = ui.AddButton("None", root);
    ui.SetSize(colorModeButton, UI::SizeValue::Auto(), UI::SizeValue::Px(60));
}



void SPH::Update(float dt)
{
    bool curFocused = (ui.Poll(input) == UI::InteractionState::Focused);
    const bool countJustUnfocused = (!curFocused && focused);
    if (countJustUnfocused) {
        try {
            float v = Clamp(std::stof(ui.GetInputValue(input)), (float)MIN_COUNT, (float)MAX_COUNT);
            ui.SetSliderValue(slider, v);
        }
        catch (...) {}
    }
    focused = curFocused;
    if (!focused) ui.SetInputValue(input, std::format("{:.0f}", ui.GetSliderValue(slider)));


    bool curFR = (ui.Poll(radiusInput) == UI::InteractionState::Focused);
    const bool radiusJustUnfocused = (!curFR && focusedRadius);
    if (radiusJustUnfocused) {
        try {
            float v = std::stof(ui.GetInputValue(radiusInput));
            if (v > 1.f) {
                H = v;
                _data->session.Set<float>(BB_SPH_H, H);
                _data->session.Set<float>(BB_SPH_REST_RHO, ComputeRestDensity(H, H * 0.5f));
                const auto a = _data->session.Get<PlayArea>(BB_SPH_AREA);
                _data->spatialIndex.Init(a.x, a.y, a.w, a.h, H);
            }
        }
        catch (...) {}
    }
    focusedRadius = curFR;


    bool curFM = (ui.Poll(massInput) == UI::InteractionState::Focused);
    const bool massJustUnfocused = (!curFM && focusedMass);
    if (massJustUnfocused) {
        try {
            int v = std::max(1, (int)std::stof(ui.GetInputValue(massInput)));
            _objMass = v;
            ui.SetInputValue(massInput, std::format("{}", _objMass));
            if (_rigidObj.active) {
                float W = _rigidObj.hw * 2, H2 = _rigidObj.hh * 2;
                _rigidObj.inertia = (float)_objMass * (W * W + H2 * H2) / 12.f;
            }
        }
        catch (...) {}
    }
    focusedMass = curFM;

    const bool anyInputJustUnfocused = countJustUnfocused || radiusJustUnfocused || massJustUnfocused;


    if (ui.IsClicked(modeButton)) {
        if (_interactionMode == InteractionMode::Attract) {
            _interactionMode = InteractionMode::Repel;
            ui.SetText(modeButton, "Mode: Repel");
        }
        else if (_interactionMode == InteractionMode::Repel) {
            _interactionMode = InteractionMode::Object;
            ui.SetText(modeButton, "Mode: Object");
        }
        else {
            _interactionMode = InteractionMode::Attract;
            ui.SetText(modeButton, "Mode: Attract");
        }
    }


    if (ui.IsClicked(removeObjButton)) {
        if (_rigidObj.active) DestroyRigidObject();
    }


    if (ui.IsClicked(colorModeButton)) {
        int m = (_data->session.Get<int>(COLOR_MODE) + 1) % 4;
        _data->session.Set<int>(COLOR_MODE, m);
        const char* lbl[] = { "None", "Pressure", "Density", "Speed" };
        ui.SetText(colorModeButton, lbl[m]);
    }


    bool held = _data->inputs.IsHeld("click");
    bool justPressed = held && !_prevClickHeld && !anyInputJustUnfocused;
    bool justReleased = !held && _prevClickHeld;
    _prevClickHeld = held;
    Vec2 mouse = _data->inputs.GetMousePosition();



    if (_interactionMode == InteractionMode::Object)
    {
        const bool clickInPlayArea = (mouse.x > (float)PANEL_WIDTH);

        if (justPressed && clickInPlayArea) 
            SpawnRigidObject(mouse.x, mouse.y);

        if (held && _rigidObj.active && _rigidObj.held) {
            _rigidObj.px = mouse.x;
            _rigidObj.py = mouse.y;
            _rigidObj.vx = _rigidObj.vy = _rigidObj.omega = 0;

            _data->session.Set<float>(BB_OBJ_PX, _rigidObj.px);
            _data->session.Set<float>(BB_OBJ_PY, _rigidObj.py);
            SyncGhostPositions();
        }

        if (justReleased && _rigidObj.active) 
            _rigidObj.held = false;
    }
    else
    {
        if (held) 
        {
            const float IR = H * 20.f, IS = 800.f, rSq = IR * IR;
            const float sign = (_interactionMode == InteractionMode::Attract) ? 1.f : -1.f;
            for (ECS::Entity e : _particles) {
                auto* tr = ecs.TryGet<TransformComponent>(e);
                auto* rb = ecs.TryGet<RigidBody>(e);
                if (!tr || !rb) 
                    continue;

                Vec2 toMouse = { mouse.x - tr->position.x, mouse.y - tr->position.y };
                float dSq = toMouse.LengthSq();
                if (dSq < 1.f || dSq > rSq) 
                    continue;
                float d = std::sqrt(dSq);
                Vec2 impulse = toMouse * (sign * IS * (1.f - d / IR) * dt / d);
                rb->vx += impulse.x;
                rb->vy += impulse.y;

            }
        }
    }


    if (_rigidObj.active && !_rigidObj.held)
    {
        _data->session.Set<float>(BB_OBJ_PX, _rigidObj.px);
        _data->session.Set<float>(BB_OBJ_PY, _rigidObj.py);

        float rfx = _data->session.Get<float>(BB_OBJ_FX);
        float rfy = _data->session.Get<float>(BB_OBJ_FY);
        float rtorque = _data->session.Get<float>(BB_OBJ_TORQUE);
        int   submerged = _data->session.Get<int>(BB_OBJ_SUBMERGED);

        _data->session.Set<float>(BB_OBJ_FX, 0.f);
        _data->session.Set<float>(BB_OBJ_FY, 0.f);
        _data->session.Set<float>(BB_OBJ_TORQUE, 0.f);
        _data->session.Set<int>(BB_OBJ_SUBMERGED, 0);

        const float subFraction = _rigidObj.totalGhosts > 0
            ? std::clamp((float)submerged / (float)_rigidObj.totalGhosts, 0.f, 1.f)
            : 0.f;

        const float objectVolume = (_rigidObj.hw * 2.f) * (_rigidObj.hh * 2.f);
        const float gravityFy = (float)_objMass * GRAVITY;
        const float buoyancyFy = -(FLUID_DENSITY * objectVolume * GRAVITY) * subFraction;

        const float dragFx = -_rigidObj.vx * OBJ_LINEAR_DAMP * subFraction;
        const float dragFy = -_rigidObj.vy * OBJ_LINEAR_DAMP * subFraction;

        const float totalFx = rfx + dragFx;
        const float totalFy = rfy + gravityFy + buoyancyFy + dragFy;

        rtorque -= _rigidObj.omega * OBJ_ANGULAR_DAMP;

        const float mass = (float)_objMass;
        _rigidObj.vx += (totalFx / mass) * dt;
        _rigidObj.vy += (totalFy / mass) * dt;
        _rigidObj.omega += (rtorque / _rigidObj.inertia) * dt;

        float spd = std::sqrt(_rigidObj.vx * _rigidObj.vx + _rigidObj.vy * _rigidObj.vy);
        if (spd > MAX_SPEED * 0.5f) { float inv = MAX_SPEED * 0.5f / spd; _rigidObj.vx *= inv; _rigidObj.vy *= inv; }
        _rigidObj.omega = std::clamp(_rigidObj.omega, -OBJ_MAX_OMEGA, OBJ_MAX_OMEGA);

        _rigidObj.px += _rigidObj.vx * dt;
        _rigidObj.py += _rigidObj.vy * dt;
        _rigidObj.angle += _rigidObj.omega * dt;

        const auto area = _data->session.Get<PlayArea>(BB_SPH_AREA);
        float margin = std::max(_rigidObj.hw, _rigidObj.hh) + 4.f;
        auto clampAxis = [&](float& pos, float& vel, float mn, float mx) {
            if (pos < mn) { pos = mn; if (vel < 0) vel = -vel * DAMPING; }
            if (pos > mx) { pos = mx; if (vel > 0) vel = -vel * DAMPING; }
            };
        clampAxis(_rigidObj.px, _rigidObj.vx, area.x + margin, area.x + area.w - margin);
        clampAxis(_rigidObj.py, _rigidObj.vy, area.y + margin, area.y + area.h - margin);
        if (_rigidObj.py >= area.y + area.h - margin) _rigidObj.vx *= 0.85f;

        SyncGhostPositions();
    }


    int delta = DrainSpawnBudget(_currentCount, (int)ui.GetSliderValue(slider), SPAWN_BUDGET);
    const auto area = _data->session.Get<PlayArea>(BB_SPH_AREA);
    for (int i = 0; i < delta; i++) SpawnParticle(area);
    for (int i = 0; i < -delta; i++) DespawnLastParticle();

    if (ui.IsClicked(back) || _data->inputs.GetActionState("next") == InputSystem::Pressed) {
        if (_rigidObj.active) DestroyRigidObject();
        _data->state.RemoveState();
    }
}



void SPH::Render(float dt)
{
    _data->renderer.ReserveDrawCalls(_currentCount);
    if (!_rigidObj.active) 
        return;

    SDL_FColor body = _rigidObj.held
        ? SDL_FColor{ 0.55f, 0.82f, 1.00f, 0.92f }
    : SDL_FColor{ 0.78f, 0.78f, 0.82f, 0.90f };
    _data->renderer.SubmitMesh(
        MeshInstance("unit_quad"), MaterialInstance("mat"),
        Vec3{ _rigidObj.px, _rigidObj.py, 0 },
        Vec2{ _rigidObj.hw * 2, _rigidObj.hh * 2 },
        _rigidObj.angle, body);

    static constexpr float BW = 2.f;
    SDL_FColor border{ 1, 1, 1, 1 };
    float cosA = std::cos(_rigidObj.angle), sinA = std::sin(_rigidObj.angle);
    float W = _rigidObj.hw * 2, H2 = _rigidObj.hh * 2;
    
    Vec2 rot = { cosA, sinA };
    auto strip = [&](float lx, float ly, float sw, float sh)
        {
            Vec2 c = { lx + sw * .5f, ly + sh * .5f };
            Vec3 pos = {
                _rigidObj.px + rot.x * c.x - rot.y * c.y,
                _rigidObj.py + rot.y * c.x + rot.x * c.y, 0
            };
            _data->renderer.SubmitMesh(
                MeshInstance("unit_quad"), MaterialInstance("mat"),
                pos, Vec2{ sw, sh }, _rigidObj.angle, border);
        };

    float ox = -_rigidObj.hw, oy = -_rigidObj.hh;
    strip(ox, oy, W, BW);
    strip(ox, oy + H2 - BW, W, BW);
    strip(ox, oy + BW, BW, H2 - BW * 2);
    strip(ox + W - BW, oy + BW, BW, H2 - BW * 2);
}