#include "PhysicsSystem.h"
#include "logger.h"
#include "SharedDataRef.h"
#include "Transform.h"
#include <chrono>


const std::vector<ECS::Entity> PhysicsSystem::emptyContacts_;

const std::vector<ECS::Entity>& PhysicsSystem::GetContacts(ECS::Entity e) const
{
    auto it = contactMap_.find(e);
    if (it == contactMap_.end()) return emptyContacts_;
    return it->second;
}

void PhysicsSystem::Tie(ECS::World& world)
{
    world_ = &world;
    world_->RegisterSystem<BoxCollider, TransformComponent>("physicsBuild",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            BuildSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Initialise);

    world_->RegisterSystem<BoxCollider, TransformComponent>("physicsCollide",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            CollisionSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Update);

    world_->RegisterSystem<RigidBody, TransformComponent>("physicsMove",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            MovementSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Update);

    world.DisableSystem("physicsBuild");
    world.DisableSystem("physicsCollide");
    world.DisableSystem("physicsMove");

}

//void PhysicsSystem::SetWorldBounds(float x, float y, float w, float h, SharedDataRef data)
//{
//    boundsX_ = x; boundsY_ = y;
//    boundsW_ = w; boundsH_ = h;
//    data->spatialIndex.Init(x, y, w, h);
//}

const std::vector<std::pair<ECS::Entity, ECS::Entity>>& PhysicsSystem::GetCollisionPairs() const
{
    return collisionPairs_;
}

void PhysicsSystem::EnableMovement(bool enable)
{
    if (!world_)
    {
        LOG_WARN(GlobalLogger(), "PhysicsSystem", "EnableMovement called before Tie()");
        return;
    }

    if (!enable) { world_->DisableSystem("physicsMove"); return; }

    world_->EnableSystem("physicsMove");
}

void PhysicsSystem::EnableCollisionDetection(bool enable)
{
    if (!world_)
    {
        LOG_WARN(GlobalLogger(), "PhysicsSystem", "EnableCollisionDetection called before Tie()");
        return;
    }

    if (!enable)
    {
        world_->DisableSystem("physicsBuild");
        world_->DisableSystem("physicsCollide");
        return;
    }

    world_->EnableSystem("physicsBuild");
    world_->EnableSystem("physicsCollide");
    
}

void PhysicsSystem::EnableSpatialIndexBuild(bool enable)
{
    if (!world_)
    {
        LOG_WARN(GlobalLogger(), "PhysicsSystem", "EnableCollisionDetection called before Tie()");
        return;
    }

    if (!enable)
    {
        world_->DisableSystem("physicsBuild");
        return;
    }

    world_->EnableSystem("physicsBuild");

}


void PhysicsSystem::MovementSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{
    auto rbs = ctx.Slice<RigidBody>();
    auto rcs = ctx.Slice<TransformComponent>();

    for (std::size_t i = 0; i < rbs.size(); i++)
    {
        auto& rb = rbs[i];
        if (rb.isStatic) 
            continue;

        float dragFactor = 1.0f - rb.drag * dt;
        if (dragFactor < 0.0f) 
            dragFactor = 0.0f;
        rb.vx *= dragFactor;
        rb.vy *= dragFactor;

        rcs[i].position.x += rb.vx * dt;
        rcs[i].position.y += rb.vy * dt;
    }
}

void PhysicsSystem::BuildSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{

    auto t0 = std::chrono::high_resolution_clock::now();

    data->spatialIndex.Clear();
    collisionPairs_.clear();


    auto entities = ctx.Slice<ECS::Entity>();
    auto rcs = ctx.Slice<TransformComponent>();
    auto bcs = ctx.Slice<BoxCollider>();

    for (std::size_t i = 0; i < entities.size(); i++)
    {
        auto it = contactMap_.find(entities[i]);
        if (it != contactMap_.end()) 
            it->second.clear();

        auto& rc = rcs[i];
        auto& bc = bcs[i];
        float ax = rc.position.x + bc.offsetX * rc.scale.x - bc.hw * rc.scale.x;
        float ay = rc.position.y + bc.offsetY * rc.scale.y - bc.hh * rc.scale.y;
        float aw = bc.hw * 2.0f * rc.scale.x;
        float ah = bc.hh * 2.0f * rc.scale.y;

        data->spatialIndex.InsertRectangle(entities[i], ax, ay, aw, ah);
    }

    data->spatialIndex.Build();

    auto t1 = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
    //if (ms > 1.0f) printf("BuildSystem: %.2f ms\n", ms);
}

void PhysicsSystem::CollisionSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{
    auto t0 = std::chrono::high_resolution_clock::now();


    if (!_queried)
    {
        _built = false;
        _queried = true;
    }

    auto entities = ctx.Slice<ECS::Entity>();
    auto rcs = ctx.Slice<TransformComponent>();
    auto bcs = ctx.Slice<BoxCollider>();

    std::vector<ECS::Entity> found;
    found.reserve(64);

    for (std::size_t i = 0; i < entities.size(); i++)
    {
        auto& rc = rcs[i];
        auto& bc = bcs[i];

        float ax = rc.position.x + (bc.offsetX * rc.scale.x) - (bc.hw * rc.scale.x);
        float ay = rc.position.y + (bc.offsetY * rc.scale.y) - (bc.hh * rc.scale.y);
        float aw = bc.hw * 2.0f * rc.scale.x;
        float ah = bc.hh * 2.0f * rc.scale.y;
        
        
        found.clear();
        data->spatialIndex.QueryRectangle(ax, ay, aw, ah, found);

        for (auto x : found)
        {
            ECS::Entity other = x;
            if (other == entities[i]) 
                continue;
            if (entities[i] > other)
                continue;

            BoxCollider* otherBc = world_->TryGet<BoxCollider>(other);
            TransformComponent* otherRc = world_->TryGet<TransformComponent>(other);
            if (!otherBc || !otherRc) continue;

            float bx = otherRc->position.x + (otherBc->offsetX * otherRc->scale.x) - (otherBc->hw * otherRc->scale.x);
            float by = otherRc->position.y + (otherBc->offsetY * otherRc->scale.y) - (otherBc->hh * otherRc->scale.y);
            float bw = otherBc->hw * 2.0f * otherRc->scale.x;
            float bh = otherBc->hh * 2.0f * otherRc->scale.y;

            if (!(ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by))
                continue;

            collisionPairs_.push_back({ entities[i], other });

            auto& contactsA = contactMap_[entities[i]];
            if (contactsA.size() < MAX_CONTACTS)
                contactsA.push_back(other);

            auto& contactsB = contactMap_[other];
            if (contactsB.size() < MAX_CONTACTS)
                contactsB.push_back(entities[i]);
        }
    }


    auto t1 = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
    //if (ms > 1.0f) printf("CollisionSystem: %.2f ms\n", ms);
}


