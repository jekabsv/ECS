#include "Boids.h"
#include "MeshComponent.h"
#include "Transform.h"
#include "RigidBody.h"
#include "BoxCollider.h"
#include <format>

using namespace Math;

static const StringId BB_AREA = StringId("boids_area");
static const StringId BB_PERCEPTION = StringId("boids_perception");


void Boids::SpawnBoid(const PlayArea& area)
{
    ECS::Entity e = ecs.Create();

    float angle = RandAngle();
    Vec2  vel = AngleToVec2(angle) * RandFloat(MIN_SPEED, MAX_SPEED);
    Vec2  pos = RandPointInRect(area.x, 0.0f, area.w, area.h);

    TransformComponent tr;
    tr.position = { pos.x, pos.y };
    tr.position = { area.x + area.w / 2.f, area.h / 2.f};
    tr.scale = { 1.0f, 1.0f };
    tr.rotation = angle;

    RigidBody rb;
    rb.vx = vel.x;  rb.vy = vel.y;
    rb.isStatic = false;
    rb.drag = 0.0f;

    ecs.Add<TransformComponent>(e, tr);
    ecs.Add<RigidBody>(e, rb);
    ecs.Add<BoxCollider>(e, BoxCollider(1, 1, true));
    ecs.Add<MeshComponent>(e, MeshComponent("boid", "mat", true));

    _boidEntities.push_back(e);
    _currentCount++;
}

void Boids::DespawnLastBoid()
{
    if (_boidEntities.empty()) 
        return;
    ecs.Destroy(_boidEntities.back());
    _boidEntities.pop_back();
    _currentCount--;
}

void Boids::Init()
{
    _data->physics.EnableCollisionDetection(false);
    _data->physics.EnableSpatialIndexBuild(true);

    const PlayArea area{
        .x = (float)inputContainerWidth,
        .w = (float)_data->GAME_WIDTH - inputContainerWidth,
        .h = (float)_data->GAME_HEIGHT
    };
    _data->session.Set<PlayArea>(BB_AREA, area);
    _data->session.Set<float>(BB_PERCEPTION, PERCEPTION);


    _data->assets.AddMesh("boid",
        MeshVertices{
            {  8.0f,  0.0f,  1.0f, 1.0f, 1.0f, 1.0f },
            { -5.0f, -4.0f,  0.5f, 0.6f, 1.0f, 1.0f },
            { -5.0f,  4.0f,  0.5f, 0.6f, 1.0f, 1.0f },
        },
        { 0, 1, 2 });

    _boidEntities.reserve(MAX_COUNT);
    for (int i = 0; i < DEFAULT_COUNT; i++)
        SpawnBoid(area);

    ecs.RegisterSystem<TransformComponent, RigidBody>(
        "boidFlock",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            const auto area = data->session.Get<PlayArea>(BB_AREA);
            auto trs = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            auto entities = ctx.Slice<ECS::Entity>();

            const float PERCEPTION = data->session.Get<float>(BB_PERCEPTION);

            std::vector<ECS::Entity> found;
            found.reserve(200);

            for (std::size_t i = 0; i < entities.size(); i++)
            {
                auto& tr = trs[i];
                auto& rb = rbs[i];

                found.clear();
                data->spatialIndex.QueryRectangle(
                    tr.position.x - PERCEPTION, tr.position.y - PERCEPTION,
                    PERCEPTION * 2.0f, PERCEPTION * 2.0f,
                    found);

                Vec2 sep = {};
                Vec2 align = {};
                Vec2 coh = {};
                int n = 0;
                int ns = 0;

                for (ECS::Entity other : found)
                {
                    if (other == entities[i] || n > 32) 
                        continue;

                    auto* oTr = data->physics.GetWorld()->TryGet<TransformComponent>(other);
                    if (!oTr) continue;

                    Vec2  delta = ToroidalDelta({ oTr->position.x - tr.position.x,
                                                   oTr->position.y - tr.position.y },
                        area.w, area.h);
                    float distSq = delta.LengthSq();
                    if (distSq > PERCEPTION * PERCEPTION) continue;

                    coh += delta;
                    align += AngleToVec2(oTr->rotation);
                    n++;

                    if (distSq < SEPARATION_DIST * SEPARATION_DIST && distSq > EPSILON)
                    {
                        sep -= delta * (1.0f / std::sqrt(distSq));
                        ns++;
                    }
                }

                if (n == 0)
                {
                    tr.position.x += rb.vx * dt;
                    tr.position.y += rb.vy * dt;
                    WrapPosition(tr.position.x, tr.position.y, area.x, area.w, area.h);
                    continue;
                }
            
                Vec2 blend = align.Normalized() * W_ALIGNMENT
                    + (coh / (float)n).Normalized() * W_COHESION
                    + (ns > 0 ? sep.Normalized() * W_SEPARATION : Vec2{});

                float desired = blend.ToAngle();

                tr.rotation = MoveTowardAngle(tr.rotation, desired, MAX_TURN_RATE * dt);

                float speed = ClampMagnitudeRange({ rb.vx, rb.vy }, MIN_SPEED, MAX_SPEED).Length();
                Vec2  vel = AngleToVec2(tr.rotation) * speed;
                rb.vx = vel.x;
                rb.vy = vel.y;

                tr.position.x += rb.vx * dt;
                tr.position.y += rb.vy * dt;
                WrapPosition(tr.position.x, tr.position.y, area.x, area.w, area.h);
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<TransformComponent, MeshComponent>(
        "RenderMesh",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto trs = ctx.Slice<TransformComponent>();
            auto meshes = ctx.Slice<MeshComponent>();

            for (std::size_t i = 0; i < trs.size(); i++)
            {
                if (!meshes[i].render) continue;
                data->renderer.SubmitMesh(
                    meshes[i].MeshName, meshes[i].Material,
                    trs[i].position, trs[i].scale, trs[i].rotation,
                    { 1.f, 1.f, 1.f, 1.f });
            }
        },
        ECS::SystemGroup::Render);

    ui.GetTheme().LoadDarkDefaults();
    ui.GetTheme().SetToken("font-default", StringId("tnr"));

    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(inputContainerWidth), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::FlexStart);
    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 6.0f);
    ui.SetPadding(root, UI::Edges::All(20.0f));

    back = ui.AddButton("Back to menu", root);
	ui.SetSize(back, UI::SizeValue::Auto(), UI::SizeValue::Px(60));
	ui.SetSize(ui.AddLabel("Number of boids:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    input = ui.AddInputField(std::format("{}", DEFAULT_COUNT), root);
	ui.SetSize(input, UI::SizeValue::Auto(), UI::SizeValue::Px(60));
    slider = ui.AddSlider((float)DEFAULT_COUNT, (float)MIN_COUNT, (float)MAX_COUNT, root);
	ui.SetSize(ui.AddLabel("Perception radius:", root), UI::SizeValue::Auto(), UI::SizeValue::Px(20));
    perceptionInput = ui.AddInputField(std::format("{:.0f}", PERCEPTION), root);
	ui.SetSize(perceptionInput, UI::SizeValue::Auto(), UI::SizeValue::Px(60));
}


void Boids::Update(float dt)
{
    bool currentlyFocused = (ui.Poll(input) == UI::InteractionState::Focused);
	bool currentlyFocusedPerception = (ui.Poll(perceptionInput) == UI::InteractionState::Focused);

    if (!currentlyFocused && focused)
    {
        try
        {
            float v = Clamp(std::stof(ui.GetInputValue(input)),
                (float)MIN_COUNT, (float)MAX_COUNT);
            ui.SetSliderValue(slider, v);
        }
        catch (...) {}
    }

    if(!currentlyFocusedPerception && focusedPerception)
    {
        try
        {
            PERCEPTION = std::stof(ui.GetInputValue(perceptionInput));
            _data->session.Set<float>(BB_PERCEPTION, PERCEPTION);
        }
        catch (...) {}
	}

    focused = currentlyFocused;
    if (!focused)
        ui.SetInputValue(input, std::format("{:.0f}", ui.GetSliderValue(slider)));

	focusedPerception = currentlyFocusedPerception;

    const auto area = _data->session.Get<PlayArea>(BB_AREA);
    int delta = DrainSpawnBudget(_currentCount, (int)ui.GetSliderValue(slider), SPAWN_BUDGET);

    for (int i = 0; i < delta; i++) 
        SpawnBoid(area);
    for (int i = 0; i < -delta; i++) 
        DespawnLastBoid();

    if (ui.IsClicked(back) || _data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.RemoveState();
}