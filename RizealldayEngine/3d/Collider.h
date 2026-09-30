#pragma once
#include "math/Vector3.h"

/// Collision shape type
enum class ColliderShape {
    Sphere,
    AABB,
};

/// Base collider component
/// Attach to any game object to enable collision detection.
/// Usage:
///   class Player {
///   public:
///     SphereCollider collider;
///   };
///   player.collider.center = player.worldTransform.translation;
///   player.collider.radius = 1.0f;
///   player.collider.attribute = CollisionAttribute::Player;
///   player.collider.mask = CollisionMask::EnemyBullet;

/// Bitmask attributes for collision filtering
enum CollisionAttribute : uint32_t {
    Player      = 1 << 0,
    Enemy       = 1 << 1,
    PlayerBullet = 1 << 2,
    EnemyBullet  = 1 << 3,
    Ground      = 1 << 4,
    All         = 0xFFFFFFFF,
};

/// Collision masks (what you want to DETECT)
using CollisionMask = uint32_t;

/// Sphere Collider
struct SphereCollider {
    Vector3  center  = { 0.0f, 0.0f, 0.0f };
    float    radius  = 1.0f;
    uint32_t attribute = 0; // What AM I?
    uint32_t mask      = 0; // What do I COLLIDE WITH?
    bool     enabled   = true;

    /// Checks if this sphere overlaps another sphere
    bool Intersects(const SphereCollider& other) const;
};

/// Axis-Aligned Bounding Box Collider
struct AABBCollider {
    Vector3  min      = { -0.5f, -0.5f, -0.5f };
    Vector3  max      = {  0.5f,  0.5f,  0.5f };
    uint32_t attribute = 0;
    uint32_t mask      = 0;
    bool     enabled   = true;

    /// Helper: get center
    Vector3 Center() const { return { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f }; }
    
    /// Set center + half-extents
    void SetCenterSize(const Vector3& center, const Vector3& halfExtents) {
        min = { center.x - halfExtents.x, center.y - halfExtents.y, center.z - halfExtents.z };
        max = { center.x + halfExtents.x, center.y + halfExtents.y, center.z + halfExtents.z };
    }

    /// Checks if this AABB overlaps another AABB
    bool Intersects(const AABBCollider& other) const;

    /// Checks if a sphere overlaps this AABB
    bool Intersects(const SphereCollider& sphere) const;
};

/// --- Free collision detection functions ---

/// Sphere vs Sphere (squared distance, no sqrt)
inline bool CheckSphereVsSphere(const SphereCollider& a, const SphereCollider& b) {
    if (!a.enabled || !b.enabled) return false;
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float dz = b.center.z - a.center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSum = a.radius + b.radius;
    return distSq <= radiusSum * radiusSum;
}

/// AABB vs AABB
inline bool CheckAABBvsAABB(const AABBCollider& a, const AABBCollider& b) {
    if (!a.enabled || !b.enabled) return false;
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

/// Sphere vs AABB
inline bool CheckSphereVsAABB(const SphereCollider& sphere, const AABBCollider& aabb) {
    if (!sphere.enabled || !aabb.enabled) return false;
    // Closest point on AABB to sphere center
    float cx = (sphere.center.x < aabb.min.x) ? aabb.min.x : (sphere.center.x > aabb.max.x) ? aabb.max.x : sphere.center.x;
    float cy = (sphere.center.y < aabb.min.y) ? aabb.min.y : (sphere.center.y > aabb.max.y) ? aabb.max.y : sphere.center.y;
    float cz = (sphere.center.z < aabb.min.z) ? aabb.min.z : (sphere.center.z > aabb.max.z) ? aabb.max.z : sphere.center.z;
    float dx = cx - sphere.center.x;
    float dy = cy - sphere.center.y;
    float dz = cz - sphere.center.z;
    return (dx * dx + dy * dy + dz * dz) <= (sphere.radius * sphere.radius);
}
