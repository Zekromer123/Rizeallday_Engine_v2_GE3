#pragma once
#include "3d/Collider.h"
#include <functional>
#include <vector>
#include <cstdint>

/// A registered collidable object entry in the CollisionManager.
/// Each ICollidable provides a SphereCollider and an OnCollision callback.
class ICollidable {
public:
    virtual ~ICollidable() = default;

    /// Return the sphere collider (updated every frame by the owner)
    virtual const SphereCollider& GetSphereCollider() const = 0;

    /// Called when a collision is detected with another ICollidable
    virtual void OnCollision(ICollidable* other) {}

    /// Called when a collision is detected (simple, no other info)
    virtual void OnCollision() {}
};

/// Central collision detection manager.
/// 
/// Usage:
///   1. Register every collidable object with RegisterCollider().
///   2. Call CheckAllCollisions() once per frame (after Update, before Draw).
///   3. CollisionManager calls OnCollision() on both objects when a hit is detected.
///
/// Pattern: Based on the KamataEngine / GE3 curriculum Collision System
///  (conversation f44d8d3a-49b8-4e90-aa49-300a74de5732)
class CollisionManager {
public:
    /// Singleton access
    static CollisionManager* GetInstance();

    /// Register a collidable for this frame's check.
    /// Call once per frame (before CheckAllCollisions) for each active object.
    void RegisterCollider(ICollidable* collidable);

    /// Perform all registered collision checks this frame.
    /// Then clears the registration list (you must re-register every frame).
    void CheckAllCollisions();

    /// Clear all registered colliders without checking
    void Clear();

private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    /// Check two specific colliders
    void CheckCollisionPair(ICollidable* a, ICollidable* b);

    std::vector<ICollidable*> colliders_;
};
