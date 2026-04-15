    #include "Boids.h"
    #include "MeshComponent.h"
    #include "Transform.h"
    #include "RigidBody.h"
    #include "BoxCollider.h"
    #include <cstdlib>
    #include <cmath>

    static float randf(float lo, float hi)
    {
        return lo + (float)(rand() % 10000) / 10000.0f * (hi - lo);
    }

    static float angleDiff(float target, float current)
    {
        float d = target - current;
        while (d > 3.14159265f) d -= 6.28318530f;
        while (d < -3.14159265f) d += 6.28318530f;
        return d;
    }

    void Boids::Init()
    {
        _data->physics.EnableCollisionDetection(false);
        _data->physics.EnableSpatialIndexBuild(true);

        const float W = (float)_data->GAME_WIDTH;
        const float H = (float)_data->GAME_HEIGHT;

        MeshVertices boidMesh = {
            { {  8.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -5.0f, -4.0f }, { 0.5f, 0.6f, 1.0f, 1.0f } },
            { { -5.0f,  4.0f }, { 0.5f, 0.6f, 1.0f, 1.0f } },
        };

        /*for(auto &x : boidMesh)
        {
		    x.position.x /= 2.0f;
		    x.position.y /= 2.0f;
	    }*/

        //_data->assets.AddMesh("boid", boidMesh);


        for (int i = 0; i < COUNT; i++)
        {
            ECS::Entity e = ecs.Create();

            float angle = randf(0.0f, 6.2831f);
            float speed = randf(MIN_SPEED, MAX_SPEED);

            TransformComponent tr;
            tr.position.x = randf(0.0f, W);
            tr.position.y = randf(0.0f, H);
            tr.scale = { 1.0f, 1.0f };

            RigidBody rb;
            rb.vx = std::cos(angle) * speed;
            rb.vy = std::sin(angle) * speed;
            rb.isStatic = false;
            rb.drag = 0.0f;

            tr.rotation = angle;

            BoxCollider bc(PERCEPTION, PERCEPTION, true);

            ecs.Add<TransformComponent>(e, tr);
            ecs.Add<RigidBody>(e, rb);
            ecs.Add<BoxCollider>(e, bc);
            ecs.Add<MeshComponent>(e, MeshComponent("boid", "", true));
        }


        ecs.RegisterSystem<TransformComponent, RigidBody>(
            "boidFlock",
            [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
            {
                const float W = (float)data->GAME_WIDTH;
                const float H = (float)data->GAME_HEIGHT;

                auto trs = ctx.Slice<TransformComponent>();
                auto rbs = ctx.Slice<RigidBody>();
                auto entities = ctx.Slice<ECS::Entity>();

                std::vector<ECS::Entity> found;
                found.reserve(64);

                for (std::size_t i = 0; i < entities.size(); i++)
                {
                    auto& tr = trs[i];
                    auto& rb = rbs[i];

                    found.clear();
                    data->spatialIndex.QueryRectangle(
                        tr.position.x - PERCEPTION,
                        tr.position.y - PERCEPTION,
                        PERCEPTION * 2.0f,
                        PERCEPTION * 2.0f,
                        found);

                    float sepX = 0, sepY = 0;
                    float alignSin = 0, alignCos = 0;
                    float cohX = 0, cohY = 0;
                    int   neighbors = 0;
                    int   separators = 0;

                    for (ECS::Entity other : found)
                    {
					    if (neighbors > 16) 
                            break;
                        if (other == entities[i]) 
                            continue;

                        TransformComponent* otherTr = data->physics.GetWorld()->TryGet<TransformComponent>(other);
                        RigidBody* otherRb = data->physics.GetWorld()->TryGet<RigidBody>(other);
                        if (!otherTr || !otherRb) 
                            continue;

                        float dx = otherTr->position.x - tr.position.x;
                        float dy = otherTr->position.y - tr.position.y;
                        if (dx > W * 0.5f) dx -= W;
                        if (dx < -W * 0.5f) dx += W;
                        if (dy > H * 0.5f) dy -= H;
                        if (dy < -H * 0.5f) dy += H;

                        float distSq = dx * dx + dy * dy;
                        if (distSq > PERCEPTION * PERCEPTION) continue;

                        cohX += dx;
                        cohY += dy;

                        alignSin += std::sin(otherTr->rotation);
                        alignCos += std::cos(otherTr->rotation);

                        neighbors++;

                        if (distSq < SEPARATION_DIST * SEPARATION_DIST && distSq > 0.0001f)
                        {
                            float dist = std::sqrt(distSq);
                            sepX -= dx / dist;
                            sepY -= dy / dist;
                            separators++;
                        }
                    }

                    if (neighbors == 0)
                    {
                        tr.position.x += rb.vx * dt;
                        tr.position.y += rb.vy * dt;
                        if (tr.position.x < 0) tr.position.x += W;
                        if (tr.position.x >= W) tr.position.x -= W;
                        if (tr.position.y < 0) tr.position.y += H;
                        if (tr.position.y >= H) tr.position.y -= H;
                        continue;
                    }

                    float blendSin = 0, blendCos = 0;

                    // Alignment
                    {
                        float a = std::atan2(alignSin, alignCos);
                        blendSin += std::sin(a) * W_ALIGNMENT;
                        blendCos += std::cos(a) * W_ALIGNMENT;
                    }

                    // Cohesion
                    {
                        float a = std::atan2(cohY / neighbors, cohX / neighbors);
                        blendSin += std::sin(a) * W_COHESION;
                        blendCos += std::cos(a) * W_COHESION;
                    }

                    // Separation
                    if (separators > 0)
                    {
                        float a = std::atan2(sepY, sepX);
                        blendSin += std::sin(a) * W_SEPARATION;
                        blendCos += std::cos(a) * W_SEPARATION;
                    }

                    float desiredAngle = std::atan2(blendSin, blendCos);

                    // Rotate current angle toward desired by at most MAX_TURN_RATE * dt
                    float diff = angleDiff(desiredAngle, tr.rotation);
                    float maxTurn = MAX_TURN_RATE * dt;
                    if (diff > maxTurn) diff = maxTurn;
                    else if (diff < -maxTurn) diff = -maxTurn;

                    tr.rotation += diff;

                    // Normalise angle
                    while (tr.rotation > 3.14159265f)  tr.rotation -= 6.28318530f;
                    while (tr.rotation < -3.14159265f) tr.rotation += 6.28318530f;

                    // Rebuild velocity from new angle, preserving speed
                    float speed = std::sqrt(rb.vx * rb.vx + rb.vy * rb.vy);
                    if (speed < MIN_SPEED) speed = MIN_SPEED;
                    if (speed > MAX_SPEED) speed = MAX_SPEED;
                    rb.vx = std::cos(tr.rotation) * speed;
                    rb.vy = std::sin(tr.rotation) * speed;

                    // Move
                    tr.position.x += rb.vx * dt;
                    tr.position.y += rb.vy * dt;

                    if (tr.position.x < 0) tr.position.x += W;
                    if (tr.position.x >= W) tr.position.x -= W;
                    if (tr.position.y < 0) tr.position.y += H;
                    if (tr.position.y >= H) tr.position.y -= H;
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

                    const MeshBase* mesh = data->assets.GetMesh(meshes[i].MeshName);
                    if (!mesh) continue;

                    float cosA = std::cos(trs[i].rotation);
                    float sinA = std::sin(trs[i].rotation);

                    std::vector<SDL_Vertex> verts = mesh->meshVertices;
                    for (auto& v : verts)
                    {
                        float rx = v.position.x * cosA - v.position.y * sinA;
                        float ry = v.position.x * sinA + v.position.y * cosA;
                        v.position.x = rx + trs[i].position.x;
                        v.position.y = ry + trs[i].position.y;
                    }

                    SDL_RenderGeometry(data->SDLrenderer, nullptr,
                        verts.data(), (int)verts.size(), nullptr, 0);
                }
            },
            ECS::SystemGroup::Render);


        ui.GetTheme().LoadDarkDefaults();

        ui.RegisterFont("main", _data->assets.GetFont("main"));


        UI::NodeHandle root = ui.AddContainer();
        ui.SetSize(root, UI::SizeValue::Px(500), UI::SizeValue::Auto());
        ui.SetFlexDirection(root, UI::FlexDirection::Column);
        ui.SetJustify(root, UI::JustifyContent::FlexStart);
        ui.SetAlignItems(root, UI::AlignItems::Stretch);
        ui.SetGap(root, 12.0f);
        ui.SetPadding(root, UI::Edges::All(20.0f));

        back = ui.AddButton("Back to menu", root);


    }

    void Boids::Update(float dt)
    {
        if (ui.IsClicked(back))
            _data->state.RemoveState();

        if (_data->inputs.GetActionState("next") == InputSystem::Pressed)
		    _data->state.RemoveState();
    }