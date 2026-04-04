#include "PhysicsSystem.h"
#include "RenderComponent.h"
#include "logger.h"

// ─── CollisionEvent ───────────────────────────────────────────────────────────

ECS::Entity* CollisionEvent::Contacts()
{
    return reinterpret_cast<ECS::Entity*>(buffer);
}
const ECS::Entity* CollisionEvent::Contacts() const
{
    return reinterpret_cast<const ECS::Entity*>(buffer);
}

// ─── QuadTree ─────────────────────────────────────────────────────────────────

bool QuadTree::Overlaps(float ax, float ay, float aw, float ah,
    float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx &&
        ay < by + bh && ay + ah > by;
}

bool QuadTree::Contains(float nx, float ny, float nw, float nh,
    float ax, float ay, float aw, float ah)
{
    return ax >= nx && ay >= ny &&
        ax + aw <= nx + nw && ay + ah <= ny + nh;
}

void QuadTree::Init(float x, float y, float w, float h)
{
    nodes_.clear();
    QuadNode root;
    root.x = x; root.y = y; root.w = w; root.h = h;
    nodes_.push_back(root);
}

void QuadTree::Clear()
{
    if (nodes_.empty()) return;
    QuadNode root;
    root.x = nodes_[0].x; root.y = nodes_[0].y;
    root.w = nodes_[0].w; root.h = nodes_[0].h;
    nodes_.clear();
    nodes_.push_back(root);
}

void QuadTree::Subdivide(int nodeIdx)
{
    // Cache bounds before push_back — vector may reallocate.
    float x = nodes_[nodeIdx].x;
    float y = nodes_[nodeIdx].y;
    float hw = nodes_[nodeIdx].w * 0.5f;
    float hh = nodes_[nodeIdx].h * 0.5f;

    int base = (int)nodes_.size();

    QuadNode nw, ne, sw, se;
    nw.x = x;      nw.y = y;      nw.w = hw; nw.h = hh;
    ne.x = x + hw; ne.y = y;      ne.w = hw; ne.h = hh;
    sw.x = x;      sw.y = y + hh; sw.w = hw; sw.h = hh;
    se.x = x + hw; se.y = y + hh; se.w = hw; se.h = hh;

    nodes_.push_back(nw);
    nodes_.push_back(ne);
    nodes_.push_back(sw);
    nodes_.push_back(se);

    nodes_[nodeIdx].children[0] = base;
    nodes_[nodeIdx].children[1] = base + 1;
    nodes_[nodeIdx].children[2] = base + 2;
    nodes_[nodeIdx].children[3] = base + 3;
    nodes_[nodeIdx].isLeaf = false;
}

void QuadTree::InsertInto(int nodeIdx, int depth, ECS::Entity e,
    float ax, float ay, float aw, float ah)
{
    if (!nodes_[nodeIdx].isLeaf)
    {
        for (int i = 0; i < 4; i++)
        {
            int ci = nodes_[nodeIdx].children[i];
            if (Contains(nodes_[ci].x, nodes_[ci].y,
                nodes_[ci].w, nodes_[ci].h,
                ax, ay, aw, ah))
            {
                InsertInto(ci, depth + 1, e, ax, ay, aw, ah);
                return;
            }
        }
        // Straddles multiple children — store at this internal node.
        if (nodes_[nodeIdx].entityCount < QuadNode::MAX_ENTITIES)
            nodes_[nodeIdx].entities[nodes_[nodeIdx].entityCount++] = e;
        return;
    }

    if (nodes_[nodeIdx].entityCount < QuadNode::MAX_ENTITIES || depth >= QuadNode::MAX_DEPTH)
    {
        if (nodes_[nodeIdx].entityCount < QuadNode::MAX_ENTITIES)
            nodes_[nodeIdx].entities[nodes_[nodeIdx].entityCount++] = e;
        return;
    }

    // Subdivide and redistribute existing entities then insert the new one.
    ECS::Entity existing[QuadNode::MAX_ENTITIES];
    std::size_t existingCount = nodes_[nodeIdx].entityCount;
    for (std::size_t i = 0; i < existingCount; i++)
        existing[i] = nodes_[nodeIdx].entities[i];
    nodes_[nodeIdx].entityCount = 0;

    Subdivide(nodeIdx);

    for (std::size_t i = 0; i < existingCount; i++)
        nodes_[nodeIdx].entities[nodes_[nodeIdx].entityCount++] = existing[i];

    InsertInto(nodeIdx, depth, e, ax, ay, aw, ah);
}

void QuadTree::Insert(ECS::Entity e, float ax, float ay, float aw, float ah)
{
    InsertInto(0, 0, e, ax, ay, aw, ah);
}

std::size_t QuadTree::QueryInto(int nodeIdx,
    float ax, float ay, float aw, float ah,
    ECS::Entity* out, std::size_t outSize,
    std::size_t written) const
{
    const QuadNode& node = nodes_[nodeIdx];

    for (std::size_t i = 0; i < node.entityCount && written < outSize; i++)
        out[written++] = node.entities[i];

    if (node.isLeaf) return written;

    for (int i = 0; i < 4; i++)
    {
        int ci = node.children[i];
        const QuadNode& child = nodes_[ci];
        if (Overlaps(ax, ay, aw, ah, child.x, child.y, child.w, child.h))
            written = QueryInto(ci, ax, ay, aw, ah, out, outSize, written);
    }

    return written;
}

std::size_t QuadTree::Query(float ax, float ay, float aw, float ah,
    ECS::Entity* out, std::size_t outSize) const
{
    if (nodes_.empty()) return 0;
    return QueryInto(0, ax, ay, aw, ah, out, outSize, 0);
}

// ─── PhysicsSystem ────────────────────────────────────────────────────────────

void PhysicsSystem::Tie(ECS::World& world, SharedDataRef data)
{
    world_ = &world;
    data_ = data;
    boundsX_ = 0.0f;
    boundsY_ = 0.0f;
    boundsW_ = (float)data->GAME_WIDTH;
    boundsH_ = (float)data->GAME_HEIGHT;
    quadTree_.Init(boundsX_, boundsY_, boundsW_, boundsH_);
}

void PhysicsSystem::SetWorldBounds(float x, float y, float w, float h)
{
    boundsX_ = x; boundsY_ = y;
    boundsW_ = w; boundsH_ = h;
    quadTree_.Init(x, y, w, h);
}

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

    world_->RegisterSystem<RigidBody, RenderComponent>("physicsMove",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            MovementSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Update);
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

    // Registered first — clears and builds the quadtree each frame.
    world_->RegisterSystem<RigidBody, BoxCollider, RenderComponent>("physicsBuild",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            BuildSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Update);

    // Registered after — queries the built tree and writes collision results.
    world_->RegisterSystem<RigidBody, BoxCollider, RenderComponent>("physicsCollide",
        [this](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            CollisionSystem(ctx, dt, data);
        },
        ECS::SystemGroup::Update);
}

// ─── Movement system ──────────────────────────────────────────────────────────

void PhysicsSystem::MovementSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{
    auto rbs = ctx.Slice<RigidBody>();
    auto rcs = ctx.Slice<RenderComponent>();

    for (std::size_t i = 0; i < rbs.size(); i++)
    {
        auto& rb = rbs[i];
        if (rb.isStatic) continue;

        float dragFactor = 1.0f - rb.drag * dt;
        if (dragFactor < 0.0f) dragFactor = 0.0f;
        rb.vx *= dragFactor;
        rb.vy *= dragFactor;

        rcs[i].position.x += rb.vx * dt;
        rcs[i].position.y += rb.vy * dt;
    }
}

// ─── Build system ─────────────────────────────────────────────────────────────
// Called once per matching chunk. The first chunk clears the tree and
// collision state; subsequent chunks just insert into the already-cleared tree.

void PhysicsSystem::BuildSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{
    if (!built_)
    {
        quadTree_.Clear();
        collisionPairs_.clear();
        built_ = true;
        queried_ = false;
    }

    auto entities = ctx.Slice<ECS::Entity>();
    auto rcs = ctx.Slice<RenderComponent>();
    auto bcs = ctx.Slice<BoxCollider>();

    for (std::size_t i = 0; i < entities.size(); i++)
    {
        CollisionEvent* ev = world_->TryGet<CollisionEvent>(entities[i]);
        if (ev) ev->count = 0;

        auto& rc = rcs[i];
        auto& bc = bcs[i];
        float ax = rc.position.x + bc.offsetX - bc.hw;
        float ay = rc.position.y + bc.offsetY - bc.hh;
        float aw = bc.hw * 2.0f;
        float ah = bc.hh * 2.0f;
        quadTree_.Insert(entities[i], ax, ay, aw, ah);
    }
}

// ─── Collision system ─────────────────────────────────────────────────────────
// Called once per matching chunk after all BuildSystem chunks have run.
// Queries the fully-built quadtree and writes results.

void PhysicsSystem::CollisionSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
{
    if (!queried_)
    {
        built_ = false; // Ready for next frame's BuildSystem.
        queried_ = true;
    }

    auto entities = ctx.Slice<ECS::Entity>();
    auto rcs = ctx.Slice<RenderComponent>();
    auto bcs = ctx.Slice<BoxCollider>();

    constexpr std::size_t QUERY_BUF_SIZE = 64;
    ECS::Entity candidates[QUERY_BUF_SIZE];

    for (std::size_t i = 0; i < entities.size(); i++)
    {
        auto& rc = rcs[i];
        auto& bc = bcs[i];

        float ax = rc.position.x + bc.offsetX - bc.hw;
        float ay = rc.position.y + bc.offsetY - bc.hh;
        float aw = bc.hw * 2.0f;
        float ah = bc.hh * 2.0f;

        std::size_t found = quadTree_.Query(ax, ay, aw, ah, candidates, QUERY_BUF_SIZE);

        for (std::size_t j = 0; j < found; j++)
        {
            ECS::Entity other = candidates[j];
            if (other == entities[i]) continue;
            if (entities[i] > other) continue; // Avoid duplicate pairs.

            BoxCollider* otherBc = world_->TryGet<BoxCollider>(other);
            RenderComponent* otherRc = world_->TryGet<RenderComponent>(other);
            if (!otherBc || !otherRc) continue;

            float bx = otherRc->position.x + otherBc->offsetX - otherBc->hw;
            float by = otherRc->position.y + otherBc->offsetY - otherBc->hh;
            float bw = otherBc->hw * 2.0f;
            float bh = otherBc->hh * 2.0f;

            if (!(ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by))
                continue;

            collisionPairs_.push_back({ entities[i], other });

            CollisionEvent* evA = world_->TryGet<CollisionEvent>(entities[i]);
            if (evA && evA->count < MAX_CONTACTS)
                evA->Contacts()[evA->count++] = other;

            CollisionEvent* evB = world_->TryGet<CollisionEvent>(other);
            if (evB && evB->count < MAX_CONTACTS)
                evB->Contacts()[evB->count++] = entities[i];
        }
    }
}