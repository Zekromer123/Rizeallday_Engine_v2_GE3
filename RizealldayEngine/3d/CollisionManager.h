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
/// Broadphase: a uniform grid (sorted cell list) built from each collider's world AABB, plus an
/// attribute/mask filter, so only nearby pairs that want to collide reach the narrowphase.
/// Small sets (or SetBroadphaseEnabled(false)) fall back to checking every pair.
/// Handles any combination of Sphere, AABB, and OBB colliders.
class CollisionManager {
public:
    struct Stats {
        uint32_t colliders = 0;      // registered colliders in the last CheckAllCollisions()
        uint32_t narrowphaseTests = 0; // pairs that passed broadphase + layer filter
        uint32_t collisions = 0;     // pairs that actually intersected
        bool usedBroadphase = false;
    };

    /// Singleton access
    static CollisionManager* GetInstance();

    /// The grid broadphase is used once at least kBroadphaseMinColliders are registered
    static constexpr size_t kBroadphaseMinColliders = 32;
    void SetBroadphaseEnabled(bool enabled) { broadphaseEnabled_ = enabled; }
    bool IsBroadphaseEnabled() const { return broadphaseEnabled_; }
    const Stats& GetLastStats() const { return lastStats_; }

    /// Register a collidable for this frame's check.
    void RegisterCollider(ICollidable* collidable);

    /// Remove a collidable immediately if registered
    void RemoveCollider(ICollidable* collidable);

    /// Perform all registered collision checks this frame and clear the list.
    void CheckAllCollisions();

    /// Clear all registered colliders without checking
    void Clear();

private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    /// Check two specific colliders (layer filter + narrowphase + callbacks)
    void CheckCollisionPair(ICollidable* a, ICollidable* b);

    struct ProxyInfo {
        Vector3 min{};
        Vector3 max{};
        uint32_t attribute = 0;
        uint32_t mask = 0;
        int cellMin[3] = { 0, 0, 0 };
        int cellMax[3] = { 0, 0, 0 };
        bool active = false;
        bool large = false; // spans too many cells; tested against everything instead
    };
    struct CellEntry {
        uint64_t key = 0;
        uint32_t proxy = 0;
    };

    void CheckAllPairsBruteForce();
    void CheckAllPairsBroadphase();
    void TestProxyPair(uint32_t a, uint32_t b);

    std::vector<ICollidable*> colliders_;
    std::vector<ProxyInfo> proxies_;   // reused between frames
    std::vector<CellEntry> cellEntries_;
    std::vector<uint32_t> largeProxies_;
    bool broadphaseEnabled_ = true;
    Stats lastStats_{};
};
