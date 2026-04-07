#pragma once
#include "ECS.h"
#include "RigidBody.h"
#include "BoxCollider.h"
#include <unordered_map>
#include <vector>

struct SharedData;
typedef std::shared_ptr<SharedData> SharedDataRef;

static constexpr std::size_t MAX_CONTACTS = 8;

static_assert(std::is_trivially_copyable_v<RigidBody>);
static_assert(std::is_trivially_copyable_v<BoxCollider>);

struct QuadNode
{
    float x, y, w, h;
    static constexpr int MAX_DEPTH = 6;
    static constexpr int MAX_ENTITIES = 8;
    ECS::Entity entities[MAX_ENTITIES] = {};
    std::size_t entityCount = 0;
    int children[4] = { -1, -1, -1, -1 };
    bool isLeaf = true;
};

class QuadTree
{
public:
    QuadTree() = default;
    void Init(float x, float y, float w, float h);
    void Clear();
    void Insert(ECS::Entity e, float ax, float ay, float aw, float ah);
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

class PhysicsSystem
{
public:
    PhysicsSystem() = default;
    void Tie(ECS::World& world, SharedDataRef data);
    void EnableMovement(bool enable = true);
    void EnableCollisionDetection(bool enable = true);
    void SetWorldBounds(float x, float y, float w, float h);
    const std::vector<std::pair<ECS::Entity, ECS::Entity>>& GetCollisionPairs() const;
    const std::vector<ECS::Entity>& GetContacts(ECS::Entity e) const;
    ECS::World* GetWorld() const { return world_; }
private:
    ECS::World* world_ = nullptr;
    float boundsX_ = 0.0f, boundsY_ = 0.0f;
    float boundsW_ = 0.0f, boundsH_ = 0.0f;
    QuadTree quadTree_;
    std::vector<std::pair<ECS::Entity, ECS::Entity>> collisionPairs_;
    std::unordered_map<ECS::Entity, std::vector<ECS::Entity>> contactMap_;
    static const std::vector<ECS::Entity> emptyContacts_;
    bool _built = false;
    bool _queried = false;
    void MovementSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void BuildSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void CollisionSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
};