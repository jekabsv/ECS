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


class PhysicsSystem
{
public:
    PhysicsSystem() = default;
    void Tie(ECS::World& world);
    void EnableMovement(bool enable = true);
    void EnableCollisionDetection(bool enable = true);
    void EnableSpatialIndexBuild(bool enable);
    //void SetWorldBounds(float x, float y, float w, float h, SharedDataRef data);
    const std::vector<std::pair<ECS::Entity, ECS::Entity>>& GetCollisionPairs() const;
    const std::vector<ECS::Entity>& GetContacts(ECS::Entity e) const;
    ECS::World* GetWorld() const { return world_; }
private:
    ECS::World* world_ = nullptr;
    float boundsX_ = 0.0f, boundsY_ = 0.0f;
    float boundsW_ = 0.0f, boundsH_ = 0.0f;
    std::vector<std::pair<ECS::Entity, ECS::Entity>> collisionPairs_;
    std::unordered_map<ECS::Entity, std::vector<ECS::Entity>> contactMap_;
    static const std::vector<ECS::Entity> emptyContacts_;
    bool _built = false;
    bool _queried = false;
    void MovementSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void BuildSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
    void CollisionSystem(ECS::ArchetypeContext ctx, float dt, SharedDataRef data);
};