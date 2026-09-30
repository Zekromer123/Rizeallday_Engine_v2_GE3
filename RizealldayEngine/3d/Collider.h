#pragma once
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

/// =========================================================================
/// Rizeallday Engine v2 - 3D Collision System
/// Ported & adapted from MT3 Coursework (https://github.com/Zekromer123/MT3.git)
/// Supports: Sphere, AABB, OBB (Box), Plane, Triangle, Segment, and Ray
/// =========================================================================

/// Collision shape type
enum class ColliderShape {
    Sphere,
    AABB,
    OBB,
};

/// Bitmask attributes for collision filtering
enum CollisionAttribute : uint32_t {
    Player       = 1 << 0,
    Enemy        = 1 << 1,
    PlayerBullet = 1 << 2,
    EnemyBullet  = 1 << 3,
    Ground       = 1 << 4,
    Obstacle     = 1 << 5,
    All          = 0xFFFFFFFF,
};

using CollisionMask = uint32_t;

// Forward declarations
struct SphereCollider;
struct AABBCollider;
struct OBBCollider;
struct PlaneCollider;
struct TriangleCollider;

/// Line segment with start point (origin) and displacement vector (diff)
struct Segment {
    Vector3 origin    = { 0.0f, 0.0f, 0.0f };
    Vector3 different = { 0.0f, 0.0f, 1.0f }; // displacement vector (end = origin + different)

    Vector3 End() const { return origin + different; }
};

/// Ray with start point (origin) and directional vector (diff)
struct Ray {
    Vector3 origin    = { 0.0f, 0.0f, 0.0f };
    Vector3 different = { 0.0f, 0.0f, 1.0f };
};

/// Infinite line through origin along direction diff
struct Line {
    Vector3 origin    = { 0.0f, 0.0f, 0.0f };
    Vector3 different = { 0.0f, 0.0f, 1.0f };
};

/// Plane definition: normal * p = distance
struct PlaneCollider {
    Vector3  normal    = { 0.0f, 1.0f, 0.0f };
    float    distance  = 0.0f;
    uint32_t attribute = 0;
    uint32_t mask      = 0;
    bool     enabled   = true;

    bool Intersects(const SphereCollider& sphere) const;
    bool Intersects(const Segment& segment) const;
};

/// Triangle definition: 3 points in 3D space
struct TriangleCollider {
    Vector3  vertices[3] = { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } };
    uint32_t attribute   = 0;
    uint32_t mask        = 0;
    bool     enabled     = true;

    Vector3 Normal() const;
    bool Intersects(const Segment& segment) const;
};

/// Sphere Collider (Center + Radius)
struct SphereCollider {
    Vector3  center    = { 0.0f, 0.0f, 0.0f };
    float    radius    = 1.0f;
    uint32_t attribute = 0;
    uint32_t mask      = 0;
    bool     enabled   = true;

    bool Intersects(const SphereCollider& other) const;
    bool Intersects(const AABBCollider& aabb) const;
    bool Intersects(const OBBCollider& obb) const;
    bool Intersects(const PlaneCollider& plane) const;
};

/// Axis-Aligned Bounding Box (AABB) Collider
struct AABBCollider {
    Vector3  min       = { -0.5f, -0.5f, -0.5f };
    Vector3  max       = {  0.5f,  0.5f,  0.5f };
    uint32_t attribute = 0;
    uint32_t mask      = 0;
    bool     enabled   = true;

    Vector3 Center() const { return (min + max) * 0.5f; }
    Vector3 HalfExtents() const { return (max - min) * 0.5f; }
    Vector3 Size() const { return max - min; }

    void SetCenterSize(const Vector3& center, const Vector3& halfExtents) {
        min = center - halfExtents;
        max = center + halfExtents;
    }

    void EnsureValid() {
        if (min.x > max.x) std::swap(min.x, max.x);
        if (min.y > max.y) std::swap(min.y, max.y);
        if (min.z > max.z) std::swap(min.z, max.z);
    }

    bool Intersects(const AABBCollider& other) const;
    bool Intersects(const SphereCollider& sphere) const;
    bool Intersects(const OBBCollider& obb) const;
    bool Intersects(const Segment& segment) const;
};

/// Oriented Bounding Box (OBB / Rotated Box) Collider
/// Directly ported from MT3_02_08_EX ~ MT3_02_10_EX
struct OBBCollider {
    Vector3  center           = { 0.0f, 0.0f, 0.0f };
    Vector3  orientations[3]  = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }; // Unit axes X, Y, Z
    Vector3  size             = { 0.5f, 0.5f, 0.5f }; // Half-extents along orientations
    uint32_t attribute        = 0;
    uint32_t mask             = 0;
    bool     enabled          = true;

    /// Update orientation axes from Euler angles (radians)
    void SetRotation(const Vector3& rotate);

    /// Update orientation axes directly from a rotation matrix
    void SetRotationMatrix(const Matrix4x4& rot);

    /// Compute world transform matrix for this OBB
    Matrix4x4 GetWorldMatrix() const;

    /// Checks if this OBB overlaps another OBB using Separating Axis Theorem (15 axes)
    bool Intersects(const OBBCollider& other) const;

    /// Checks if this OBB overlaps a sphere
    bool Intersects(const SphereCollider& sphere) const;

    /// Checks if this OBB overlaps an AABB
    bool Intersects(const AABBCollider& aabb) const;

    /// Checks if this OBB intersects a segment
    bool Intersects(const Segment& segment) const;
};

/// =========================================================================
/// Math & Intersection Free Functions (Ported from MT3)
/// =========================================================================

/// Vector projection of v1 onto v2
inline Vector3 Project(const Vector3& v1, const Vector3& v2) {
    float magSq = Dot(v2, v2);
    if (magSq < 1e-6f) return { 0.0f, 0.0f, 0.0f };
    float t = Dot(v1, v2) / magSq;
    return v2 * t;
}

/// Reflect vector against surface normal
inline Vector3 Reflect(const Vector3& input, const Vector3& normal) {
    float dot = Dot(input, normal);
    return input - (normal * (2.0f * dot));
}

/// Find closest point on segment to arbitrary point
Vector3 ClosestPoint(const Vector3& point, const Segment& segment);

/// Find closest point on AABB to arbitrary point
Vector3 ClosestPoint(const Vector3& point, const AABBCollider& aabb);

/// Find closest point on OBB to arbitrary point
Vector3 ClosestPoint(const Vector3& point, const OBBCollider& obb);

/// 1. Sphere vs Sphere (Squared distance, no sqrt)
bool CheckSphereVsSphere(const SphereCollider& a, const SphereCollider& b);

/// 2. Sphere vs Plane
bool CheckSphereVsPlane(const SphereCollider& sphere, const PlaneCollider& plane);

/// 3. Sphere vs AABB
bool CheckSphereVsAABB(const SphereCollider& sphere, const AABBCollider& aabb);

/// 4. Sphere vs OBB
bool CheckSphereVsOBB(const SphereCollider& sphere, const OBBCollider& obb);

/// 5. AABB vs AABB
bool CheckAABBvsAABB(const AABBCollider& a, const AABBCollider& b);

/// 6. AABB vs Segment (Slab method)
bool CheckAABBvsSegment(const AABBCollider& aabb, const Segment& segment);

/// 7. Plane vs Segment
bool CheckPlaneVsSegment(const PlaneCollider& plane, const Segment& segment);

/// 8. Triangle vs Segment (Plane intersection + Point in Triangle test)
bool CheckTriangleVsSegment(const TriangleCollider& triangle, const Segment& segment);

/// 9. OBB vs Segment
bool CheckOBBvsSegment(const OBBCollider& obb, const Segment& segment);

/// 10. OBB vs OBB (Separating Axis Theorem / SAT with 15 test axes)
bool CheckOBBvsOBB(const OBBCollider& obb1, const OBBCollider& obb2);

/// 11. OBB vs AABB (Treats AABB as an aligned OBB)
bool CheckOBBvsAABB(const OBBCollider& obb, const AABBCollider& aabb);
