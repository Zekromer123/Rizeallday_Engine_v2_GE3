#include "3d/Collider.h"

// =========================================================================
// OBB Collider Helpers
// =========================================================================

void OBBCollider::SetRotation(const Vector3& rotate) {
    Matrix4x4 rx = Mathematics::MakeRotateXMatrix(rotate.x);
    Matrix4x4 ry = Mathematics::MakeRotateYMatrix(rotate.y);
    Matrix4x4 rz = Mathematics::MakeRotateZMatrix(rotate.z);
    Matrix4x4 rot = Mathematics::Multiply(rx, Mathematics::Multiply(ry, rz));
    SetRotationMatrix(rot);
}

void OBBCollider::SetRotationMatrix(const Matrix4x4& rot) {
    orientations[0] = Normalize(Vector3{ rot.m[0][0], rot.m[0][1], rot.m[0][2] });
    orientations[1] = Normalize(Vector3{ rot.m[1][0], rot.m[1][1], rot.m[1][2] });
    orientations[2] = Normalize(Vector3{ rot.m[2][0], rot.m[2][1], rot.m[2][2] });
}

Matrix4x4 OBBCollider::GetWorldMatrix() const {
    return Matrix4x4{
        orientations[0].x, orientations[0].y, orientations[0].z, 0.0f,
        orientations[1].x, orientations[1].y, orientations[1].z, 0.0f,
        orientations[2].x, orientations[2].y, orientations[2].z, 0.0f,
        center.x,          center.y,          center.z,          1.0f
    };
}

Vector3 TriangleCollider::Normal() const {
    Vector3 v01 = vertices[1] - vertices[0];
    Vector3 v02 = vertices[2] - vertices[0];
    return Normalize(Cross(v01, v02));
}

// =========================================================================
// Closest Point Algorithms (from MT3)
// =========================================================================

Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
    Vector3 originToPoint = point - segment.origin;
    float dot1 = Dot(originToPoint, segment.different);
    float dot2 = Dot(segment.different, segment.different);

    if (dot2 < 1e-6f) return segment.origin;

    float t = std::clamp(dot1 / dot2, 0.0f, 1.0f);
    return segment.origin + segment.different * t;
}

Vector3 ClosestPoint(const Vector3& point, const AABBCollider& aabb) {
    return Vector3{
        std::clamp(point.x, aabb.min.x, aabb.max.x),
        std::clamp(point.y, aabb.min.y, aabb.max.y),
        std::clamp(point.z, aabb.min.z, aabb.max.z)
    };
}

Vector3 ClosestPoint(const Vector3& point, const OBBCollider& obb) {
    Matrix4x4 world = obb.GetWorldMatrix();
    Matrix4x4 invWorld = Mathematics::Inverse(world);
    Vector3 localPoint = Mathematics::Transform(point, invWorld);

    AABBCollider localAABB;
    localAABB.min = { -obb.size.x, -obb.size.y, -obb.size.z };
    localAABB.max = {  obb.size.x,  obb.size.y,  obb.size.z };

    Vector3 localClosest = ClosestPoint(localPoint, localAABB);
    return Mathematics::Transform(localClosest, world);
}

// =========================================================================
// Projection for OBB SAT (Separating Axis Theorem)
// =========================================================================

static void GetOBBProjection(const OBBCollider& obb, const Vector3& axis, float& outMin, float& outMax) {
    Vector3 localVertices[8] = {
        { -obb.size.x, -obb.size.y, -obb.size.z },
        {  obb.size.x, -obb.size.y, -obb.size.z },
        { -obb.size.x,  obb.size.y, -obb.size.z },
        {  obb.size.x,  obb.size.y, -obb.size.z },
        { -obb.size.x, -obb.size.y,  obb.size.z },
        {  obb.size.x, -obb.size.y,  obb.size.z },
        { -obb.size.x,  obb.size.y,  obb.size.z },
        {  obb.size.x,  obb.size.y,  obb.size.z },
    };

    Matrix4x4 world = obb.GetWorldMatrix();
    outMin = (std::numeric_limits<float>::max)();
    outMax = -(std::numeric_limits<float>::max)();

    for (int i = 0; i < 8; ++i) {
        Vector3 worldPos = Mathematics::Transform(localVertices[i], world);
        float proj = Dot(worldPos, axis);
        outMin = (std::min)(outMin, proj);
        outMax = (std::max)(outMax, proj);
    }
}

// =========================================================================
// Intersection Routines (from MT3)
// =========================================================================

// 1. Sphere vs Sphere
bool CheckSphereVsSphere(const SphereCollider& a, const SphereCollider& b) {
    if (!a.enabled || !b.enabled) return false;
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float dz = b.center.z - a.center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSum = a.radius + b.radius;
    return distSq <= radiusSum * radiusSum;
}

// 2. Sphere vs Plane (MT3_02_02 / Sphere.cpp)
bool CheckSphereVsPlane(const SphereCollider& sphere, const PlaneCollider& plane) {
    if (!sphere.enabled || !plane.enabled) return false;
    float distance = Dot(sphere.center, plane.normal) - plane.distance;
    return std::abs(distance) <= sphere.radius;
}

// 3. Sphere vs AABB (MT3_02_06 / Sphere.cpp)
bool CheckSphereVsAABB(const SphereCollider& sphere, const AABBCollider& aabb) {
    if (!sphere.enabled || !aabb.enabled) return false;
    Vector3 closest = ClosestPoint(sphere.center, aabb);
    Vector3 diff = closest - sphere.center;
    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    return distSq <= (sphere.radius * sphere.radius);
}

// 4. Sphere vs OBB (MT3_02_08_EX / Sphere.cpp)
bool CheckSphereVsOBB(const SphereCollider& sphere, const OBBCollider& obb) {
    if (!sphere.enabled || !obb.enabled) return false;
    Matrix4x4 invWorld = Mathematics::Inverse(obb.GetWorldMatrix());
    Vector3 centerInLocal = Mathematics::Transform(sphere.center, invWorld);

    AABBCollider localAABB;
    localAABB.min = { -obb.size.x, -obb.size.y, -obb.size.z };
    localAABB.max = {  obb.size.x,  obb.size.y,  obb.size.z };

    SphereCollider localSphere;
    localSphere.center = centerInLocal;
    localSphere.radius = sphere.radius;

    return CheckSphereVsAABB(localSphere, localAABB);
}

// 5. AABB vs AABB (MT3_02_05 / Matrix.cpp)
bool CheckAABBvsAABB(const AABBCollider& a, const AABBCollider& b) {
    if (!a.enabled || !b.enabled) return false;
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// 6. AABB vs Segment (MT3_02_07 / Matrix.cpp - Slab Method)
bool CheckAABBvsSegment(const AABBCollider& aabb, const Segment& segment) {
    if (!aabb.enabled) return false;

    float tmin = 0.0f;
    float tmax = 1.0f;

    // Check X slab
    if (std::abs(segment.different.x) < 1e-6f) {
        if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) return false;
    } else {
        float t1 = (aabb.min.x - segment.origin.x) / segment.different.x;
        float t2 = (aabb.max.x - segment.origin.x) / segment.different.x;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Check Y slab
    if (std::abs(segment.different.y) < 1e-6f) {
        if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) return false;
    } else {
        float t1 = (aabb.min.y - segment.origin.y) / segment.different.y;
        float t2 = (aabb.max.y - segment.origin.y) / segment.different.y;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Check Z slab
    if (std::abs(segment.different.z) < 1e-6f) {
        if (segment.origin.z < aabb.min.z || segment.origin.z > aabb.max.z) return false;
    } else {
        float t1 = (aabb.min.z - segment.origin.z) / segment.different.z;
        float t2 = (aabb.max.z - segment.origin.z) / segment.different.z;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    return (tmin <= tmax && tmin <= 1.0f && tmax >= 0.0f);
}

// 7. Plane vs Segment (MT3_02_03 / Plane.cpp)
bool CheckPlaneVsSegment(const PlaneCollider& plane, const Segment& segment) {
    if (!plane.enabled) return false;
    float dot = Dot(plane.normal, segment.different);
    if (std::abs(dot) < 1e-6f) return false;
    float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;
    return (t >= 0.0f && t <= 1.0f);
}

// 8. Triangle vs Segment (MT3_02_04 / Triangle.cpp)
bool CheckTriangleVsSegment(const TriangleCollider& triangle, const Segment& segment) {
    if (!triangle.enabled) return false;

    Vector3 normal = triangle.Normal();
    float d = Dot(normal, triangle.vertices[0]);
    float dot = Dot(normal, segment.different);

    if (std::abs(dot) < 1e-6f) return false;

    float t = (d - Dot(normal, segment.origin)) / dot;
    if (t < 0.0f || t > 1.0f) return false;

    Vector3 p = segment.origin + segment.different * t;

    Vector3 v01 = triangle.vertices[1] - triangle.vertices[0];
    Vector3 v12 = triangle.vertices[2] - triangle.vertices[1];
    Vector3 v20 = triangle.vertices[0] - triangle.vertices[2];

    Vector3 v0p = p - triangle.vertices[0];
    Vector3 v1p = p - triangle.vertices[1];
    Vector3 v2p = p - triangle.vertices[2];

    Vector3 cross01 = Cross(v01, v0p);
    Vector3 cross12 = Cross(v12, v1p);
    Vector3 cross20 = Cross(v20, v2p);

    return (Dot(cross01, normal) >= 0.0f &&
            Dot(cross12, normal) >= 0.0f &&
            Dot(cross20, normal) >= 0.0f);
}

// 9. OBB vs Segment (MT3_02_09_EX / Matrix.cpp)
bool CheckOBBvsSegment(const OBBCollider& obb, const Segment& segment) {
    if (!obb.enabled) return false;
    Matrix4x4 invWorld = Mathematics::Inverse(obb.GetWorldMatrix());
    Vector3 localOrigin = Mathematics::Transform(segment.origin, invWorld);
    Vector3 localEnd = Mathematics::Transform(segment.End(), invWorld);

    AABBCollider localAABB;
    localAABB.min = { -obb.size.x, -obb.size.y, -obb.size.z };
    localAABB.max = {  obb.size.x,  obb.size.y,  obb.size.z };

    Segment localSegment;
    localSegment.origin = localOrigin;
    localSegment.different = localEnd - localOrigin;

    return CheckAABBvsSegment(localAABB, localSegment);
}

// 10. OBB vs OBB (MT3_02_10_EX / Matrix.cpp - 15 Axes SAT)
bool CheckOBBvsOBB(const OBBCollider& obb1, const OBBCollider& obb2) {
    if (!obb1.enabled || !obb2.enabled) return false;

    Vector3 axes[15];
    axes[0] = obb1.orientations[0];
    axes[1] = obb1.orientations[1];
    axes[2] = obb1.orientations[2];
    axes[3] = obb2.orientations[0];
    axes[4] = obb2.orientations[1];
    axes[5] = obb2.orientations[2];

    int idx = 6;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            axes[idx++] = Cross(obb1.orientations[i], obb2.orientations[j]);
        }
    }

    for (int i = 0; i < 15; ++i) {
        float len = axes[i].Length();
        if (len < 1e-4f) continue;

        Vector3 axis = axes[i] / len;
        float min1, max1, min2, max2;
        GetOBBProjection(obb1, axis, min1, max1);
        GetOBBProjection(obb2, axis, min2, max2);

        float L1 = max1 - min1;
        float L2 = max2 - min2;
        float sumSpan = L1 + L2;
        float longSpan = (std::max)(max1, max2) - (std::min)(min1, min2);

        if (sumSpan < longSpan) {
            return false; // Separating axis found: no overlap
        }
    }

    return true; // Overlapping on all 15 axes
}

// 11. OBB vs AABB
bool CheckOBBvsAABB(const OBBCollider& obb, const AABBCollider& aabb) {
    if (!obb.enabled || !aabb.enabled) return false;
    OBBCollider aabbAsOBB;
    aabbAsOBB.center = aabb.Center();
    aabbAsOBB.orientations[0] = { 1.0f, 0.0f, 0.0f };
    aabbAsOBB.orientations[1] = { 0.0f, 1.0f, 0.0f };
    aabbAsOBB.orientations[2] = { 0.0f, 0.0f, 1.0f };
    aabbAsOBB.size = aabb.HalfExtents();
    return CheckOBBvsOBB(obb, aabbAsOBB);
}

// =========================================================================
// Struct Member Dispatchers
// =========================================================================

bool SphereCollider::Intersects(const SphereCollider& other) const { return CheckSphereVsSphere(*this, other); }
bool SphereCollider::Intersects(const AABBCollider& aabb) const     { return CheckSphereVsAABB(*this, aabb); }
bool SphereCollider::Intersects(const OBBCollider& obb) const       { return CheckSphereVsOBB(*this, obb); }
bool SphereCollider::Intersects(const PlaneCollider& plane) const   { return CheckSphereVsPlane(*this, plane); }

bool AABBCollider::Intersects(const AABBCollider& other) const      { return CheckAABBvsAABB(*this, other); }
bool AABBCollider::Intersects(const SphereCollider& sphere) const   { return CheckSphereVsAABB(sphere, *this); }
bool AABBCollider::Intersects(const OBBCollider& obb) const         { return CheckOBBvsAABB(obb, *this); }
bool AABBCollider::Intersects(const Segment& segment) const         { return CheckAABBvsSegment(*this, segment); }

bool OBBCollider::Intersects(const OBBCollider& other) const        { return CheckOBBvsOBB(*this, other); }
bool OBBCollider::Intersects(const SphereCollider& sphere) const    { return CheckSphereVsOBB(sphere, *this); }
bool OBBCollider::Intersects(const AABBCollider& aabb) const        { return CheckOBBvsAABB(*this, aabb); }
bool OBBCollider::Intersects(const Segment& segment) const          { return CheckOBBvsSegment(*this, segment); }

bool PlaneCollider::Intersects(const SphereCollider& sphere) const  { return CheckSphereVsPlane(sphere, *this); }
bool PlaneCollider::Intersects(const Segment& segment) const        { return CheckPlaneVsSegment(*this, segment); }

bool TriangleCollider::Intersects(const Segment& segment) const     { return CheckTriangleVsSegment(*this, segment); }
