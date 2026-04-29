#include "depthState.h"
#include "MeshComponent.h"
#include "Transform.h"

struct Controllable {};

void depthState::Init()
{
    auto player = ecs.Create();
    auto object = ecs.Create();

    ecs.Add<TransformComponent>(player, TransformComponent({ 50, 50, 0.49f }));
    ecs.Add<MeshComponent>(player, MeshComponent("tri", "mat_transp", true));
    ecs.Add<Controllable>(player, Controllable());

    ecs.Add<TransformComponent>(object, TransformComponent({ 50, 50, 0.48f }));
    ecs.Add<MeshComponent>(object, MeshComponent("tri", "mat_transp", true));


    ecs.RegisterSystem<TransformComponent, Controllable>("move",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            for (auto &x : transforms)
            {
                auto d = _data->inputs.GetActionAxis("move");
                x.position.x += d[0] * dt * 1000;
                x.position.y += d[1] * dt * 1000;
            }
        },
        ECS::SystemGroup::Update);


    ecs.RegisterSystem<MeshComponent, TransformComponent>("renderMesh",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto meshes = ctx.Slice<MeshComponent>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (int i = 0; i < meshes.size(); i++)
            {
                auto& meshC = meshes[i];
                auto& transform = transforms[i];

                if (!meshC.render)
                    continue;


                _data->renderer.SubmitMesh(meshC.MeshName, meshC.Material, transform.position, transform.scale, transform.rotation, {1.f, 1.f, 1.f, 1.f});
                //_data->renderer.DrawMesh(meshC.MeshName, meshC.Material, { transform.position.x, transform.position.y }, transform.scale);

                const MeshBase* mesh = _data->assets.GetMesh(meshC.MeshName.meshName);
                if (!mesh)
                    continue;
            }
        },
        ECS::SystemGroup::Render);
}

void depthState::Update(float dt)
{
}
