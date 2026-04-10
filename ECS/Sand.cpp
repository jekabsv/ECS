#include "Sand.h"
#include "Transform.h"
#include "MeshComponent.h"

void Sand::Init()
{
	_data->physics.EnableCollisionDetection();
	_data->physics.EnableMovement();

	const int N = 1000;
	for (int i = 0; i < N; i++)
	{
		ECS::Entity e = ecs.Create();

		TransformComponent tr;

		tr.position.x = (float)(rand() % (int)_data->GAME_WIDTH);
		tr.position.y = (float)(rand() % (int)_data->GAME_HEIGHT);


		ecs.Add<BoxCollider>(e, BoxCollider(0.5f, 0.5f));
		ecs.Add<RigidBody>(e, RigidBody(0, 0));
		ecs.Add<TransformComponent>(e, tr);
		ecs.Add<MeshComponent>(e, MeshComponent("square", "", true));
	}

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

                const Mesh* mesh = _data->assets.GetMesh(meshC.MeshName);
                if (!mesh)
                    continue;

                const SDL_Texture* texture = _data->assets.GetTexture(meshC.TextureName);

                std::vector<SDL_Vertex> transformed = *mesh;
                for (auto& v : transformed)
                {
                    v.position.x = v.position.x * transform.scale.x + transform.position.x;
                    v.position.y = v.position.y * transform.scale.y + transform.position.y;
                }

                SDL_RenderGeometry(
                    _data->SDLrenderer,
                    const_cast<SDL_Texture*>(texture),
                    transformed.data(),
                    (int)transformed.size(),
                    nullptr,
                    0
                );
            }
        },
        ECS::SystemGroup::Render);
    ecs.RegisterSystem<RigidBody>("gravity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto rb = ctx.Slice<RigidBody>();
            for (auto &r : rb)
            {
                r.vy += 9.81f * dt;
            }
        },
        ECS::SystemGroup::Render);

    ecs.RegisterSystem<BoxCollider, TransformComponent, RigidBody>("onCollision",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            auto bodies = ctx.Slice<RigidBody>();
            auto entities = ctx.Slice<ECS::Entity>();

            for (std::size_t i = 0; i < entities.size(); i++)
            {
                auto& ev = data->physics.GetContacts(entities[i]);
                if (ev.empty()) continue;

                auto& tr = transforms[i];
                auto& rb = bodies[i];

                for (auto& other : ev)
                {
                    TransformComponent* otherTr = data->physics.GetWorld()->TryGet<TransformComponent>(other);
                    RigidBody* otherRb = data->physics.GetWorld()->TryGet<RigidBody>(other);

                    if (otherTr)
                    {
                        // 1. Simple Ground/Platform collision logic
                        // If we are falling and hit something below us
                        if (tr.position.y < otherTr->position.y)
                        {
                            // Stop downward velocity
                            if (rb.vy > 0) rb.vy = 0;

                            // Positional correction: Snap to the top of the other object
                            // Assuming 0.5f is half-height of your sand grain
                            tr.position.y = otherTr->position.y - 1.0f;
                        }

                        // 2. Horizontal "sliding" (optional for sand feel)
                        // If sand hits exactly on top, give it a tiny random nudge left or right
                        if (std::abs(tr.position.x - otherTr->position.x) < 0.1f)
                        {
                            tr.position.x += (rand() % 2 == 0 ? 0.1f : -0.1f);
                        }
                    }
                }
            }
        },
        ECS::SystemGroup::Update);


    ecs.RegisterSystem<TransformComponent, RigidBody>("boundsCheck",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            auto bodies = ctx.Slice<RigidBody>();

            for (int i = 0; i < transforms.size(); i++)
            {
                auto& tr = transforms[i];
                auto& rb = bodies[i];

                // Check floor
                if (tr.position.y >= (float)_data->GAME_HEIGHT - 1.0f)
                {
                    tr.position.y = (float)_data->GAME_HEIGHT - 1.0f;
                    rb.vy = 0; // Stop falling
                }

                // Check walls
                if (tr.position.x < 0) tr.position.x = 0;
                if (tr.position.x > (float)_data->GAME_WIDTH) tr.position.x = (float)_data->GAME_WIDTH;
            }
        },
        ECS::SystemGroup::Update);

}


void Sand::Update(float dt)
{

}