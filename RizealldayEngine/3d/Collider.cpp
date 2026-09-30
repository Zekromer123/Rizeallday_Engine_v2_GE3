#include "3d/Collider.h"
#include <cmath>

bool SphereCollider::Intersects(const SphereCollider& other) const {
    return CheckSphereVsSphere(*this, other);
}

bool AABBCollider::Intersects(const AABBCollider& other) const {
    return CheckAABBvsAABB(*this, other);
}

bool AABBCollider::Intersects(const SphereCollider& sphere) const {
    return CheckSphereVsAABB(sphere, *this);
}
