#pragma once
#include "3d/Collider.h"
#include <vector>
#include <cstdint>

/// Base interface for any game object that participates in collision detection.
/// Supports SphereCollider, AABBCollider, and OBBCollider (rotated box).
class ICollidable {
public:
    virtual ~ICollidable() = default;

    /// Return the active collider shape (Sphere, AABB, or OBB)
    virtual ColliderShape GetColliderShape() const { return ColliderShape::Sphere; }

    /// Return SphereCollider (if GetColliderShape() == ColliderShape::Sphere)
    virtual const SphereCollider& GetSphereCollider() const {
        static SphereCollider dummy;
        return dummy;
    }

    /// Return AABBCollider (if GetColliderShape() == ColliderShape::AABB)
    virtual const AABBCollider& GetAABBCollider() const {
        static AABBCollider dummy;
        return dummy;
    }

    /// Return OBBCollider (if GetColliderShape() == ColliderShape::OBB)
    virtual const OBBCollider& GetOBBCollider() const {
        static OBBCollider dummy;
        return dummy;
    }

    /// Called when collision is detected with another ICollidable
    virtual void OnCollision(ICollidable* other) {}

    /// Parameterless callback overload
    virtual void OnCollision() {}
};

/// Central collision detection manager.
/// Checks registered objects each frame using round-robin pairwise checks.
/// Handles any combination of Sphere, AABB, and OBB colliders.
class CollisionManager {
public:
    /// Singleton access
    static CollisionManager* GetInstance();

    /// Register a collidable for this frame's check.
    void RegisterCollider(ICollidable* collidable);

    /// Perform all registered collision checks this frame and clear the list.
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
