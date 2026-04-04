#pragma once
#include "ECS.h"
#include "SharedDataRef.h"

// ─── Components ───────────────────────────────────────────────────────────────

struct RigidBody
{
    float vx = 0.0f, vy = 0.0f;
    float drag = 0.0f;
    bool isStatic = false;
};

struct BoxCollider
{
    float offsetX = 0.0f, offsetY = 0.0f;
    float hw = 0.0f, hh = 0.0f;    // half-extents
    bool isTrigger = false;
};

// Collision results written each frame by the physics system.
// Stored as a raw byte buffer cast to Entity* at read time.
// Max 8 contacts per entity per frame.
static constexpr std::size_t MAX_CONTACTS = 8;
static constexpr std::size_t CONTACT_BUFFER_SIZE = MAX_CONTACTS * sizeof(ECS::Entity);

struct CollisionEvent
{
    alignas(alignof(ECS::Entity)) char buffer[CONTACT_BUFFER_SIZE] = {};
    std::size_t count = 0;

    ECS::Entity* Contacts();
    const ECS::Entity* Contacts() const;
};

static_assert(std::is_trivially_copyable_v<RigidBody>);
static_assert(std::is_trivially_copyable_v<BoxCollider>);
static_assert(std::is_trivially_copyable_v<CollisionEvent>);

// ─── Quadtree ─────────────────────────────────────────────────────────────────

struct QuadNode
{
    float x, y, w, h;              // bounds of this node

    static constexpr int MAX_DEPTH = 6;
    static constexpr int MAX_ENTITIES = 8;

    // Entities whose AABBs fall within this node (leaf storage)
    // Stored inline; overflow causes subdivision.
    ECS::Entity entities[MAX_ENTITIES] = {};
    std::size_t entityCount = 0;

    int children[4] = { -1, -1, -1, -1 };  // indices into QuadTree::nodes, -1 = no child
    bool isLeaf = true;
};

class QuadTree
{
public:
    QuadTree() = default;

    void Init(float x, float y, float w, float h);
    void Clear();
    void Insert(ECS::Entity e, float ax, float ay, float aw, float ah);

    // Returns all candidate entities that share a node with the given AABB.
    // Results written into out[], returns count written (capped at outSize).
    std::size_t Query(float ax, float ay, float aw, float ah,
        ECS::Entity* out, std::size_t outSize) const;

private:
    std::vector<QuadNode> nodes_;

    void Subdivide(int nodeIdx);
    void InsertInto(int nodeIdx, int depth, ECS::Entity e,
        float ax, float ay, float aw, float ah);
    std::size_t QueryInto(int nodeIdx, float ax, float ay, float aw, float ah,
        ECS::Entity* out, std::size_t outSize, std::size_t written) const;

    static bool Overlaps(float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh);
    static bool Contains(float nx, float ny, float nw, float nh,
        float ax, float ay, float aw, float ah);
};

// ─── PhysicsSystem ────────────────────────────────────────────────────────────

class PhysicsSystem
{
public:
    PhysicsSystem() = default;

    // Call once to bind to a world and shared data before enabling any systems.
    void Tie(ECS::World& world, SharedDataRef data);

    // Registers a system that integrates RigidBody velocity into RenderComponent::position.
    // Pass false to unregister (disable).
    void EnableMovement(bool enable = true);

    // Registers a system that builds the quadtree, detects AABB overlaps,
    // and writes CollisionEvent onto both participants each frame.
    void EnableCollisionDetection(bool enable = true);

    // World bounds used by the quadtree. Defaults to SharedData::GAME_WIDTH/HEIGHT.
    // Call before EnableCollisionDetection if you need custom bounds.
    void SetWorldBounds(float x, float y, float w, float h);

    // Read-only access to all collision pairs detected last frame.
    // Each pair is (entityA, entityB); order is not guaranteed.
    const std::vector<std::pair<ECS::Entity, ECS::Entity>>& GetCollisionPairs() const;

private:
    ECS::World* world_ = nullptr;
    SharedDataRef data_;

    float boundsX_ = 0.0f, boundsY_ = 0.0f;
    float boundsW_ = 0.0f, boundsH_ = 0.0f;

    QuadTree quadTree_;

    // Flat list of collision pairs produced last frame.
    std::vector<std::pair<ECS::Entity, ECS::Entity>> collisionPairs_;

    // Frame-boundary flags used by BuildSystem / CollisionSystem.
    // built_   — set on first BuildSystem chunk, cleared on first CollisionSystem chunk.
    // queried_ — set on first CollisionSystem chunk, prevents double-reset.
    bool built_ = false;
    bool queried_ = false;

    // Internal system functions registered with the ECS.
    // Not static — registered as lambdas capturing `this` inside Enable* methods.
    void MovementSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void BuildSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void CollisionSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
};