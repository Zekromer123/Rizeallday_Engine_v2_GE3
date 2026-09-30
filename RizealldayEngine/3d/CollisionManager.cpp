#include "3d/CollisionManager.h"

CollisionManager* CollisionManager::GetInstance() {
    static CollisionManager instance;
    return &instance;
}

void CollisionManager::RegisterCollider(ICollidable* collidable) {
    if (collidable) {
        colliders_.push_back(collidable);
    }
}

void CollisionManager::CheckAllCollisions() {
    // Total Round-Robin (総当たり): check every pair once
    for (size_t i = 0; i < colliders_.size(); ++i) {
        for (size_t j = i + 1; j < colliders_.size(); ++j) {
            CheckCollisionPair(colliders_[i], colliders_[j]);
        }
    }
    // Clear registrations for next frame
    colliders_.clear();
}

void CollisionManager::Clear() {
    colliders_.clear();
}

void CollisionManager::CheckCollisionPair(ICollidable* a, ICollidable* b) {
    if (!a || !b) return;

    ColliderShape shapeA = a->GetColliderShape();
    ColliderShape shapeB = b->GetColliderShape();

    uint32_t attrA = 0, maskA = 0;
    bool enabledA = true;
    switch (shapeA) {
    case ColliderShape::Sphere:
        attrA = a->GetSphereCollider().attribute;
        maskA = a->GetSphereCollider().mask;
        enabledA = a->GetSphereCollider().enabled;
        break;
    case ColliderShape::AABB:
        attrA = a->GetAABBCollider().attribute;
        maskA = a->GetAABBCollider().mask;
        enabledA = a->GetAABBCollider().enabled;
        break;
    case ColliderShape::OBB:
        attrA = a->GetOBBCollider().attribute;
        maskA = a->GetOBBCollider().mask;
        enabledA = a->GetOBBCollider().enabled;
        break;
    }

    uint32_t attrB = 0, maskB = 0;
    bool enabledB = true;
    switch (shapeB) {
    case ColliderShape::Sphere:
        attrB = b->GetSphereCollider().attribute;
        maskB = b->GetSphereCollider().mask;
        enabledB = b->GetSphereCollider().enabled;
        break;
    case ColliderShape::AABB:
        attrB = b->GetAABBCollider().attribute;
        maskB = b->GetAABBCollider().mask;
        enabledB = b->GetAABBCollider().enabled;
        break;
    case ColliderShape::OBB:
        attrB = b->GetOBBCollider().attribute;
        maskB = b->GetOBBCollider().mask;
        enabledB = b->GetOBBCollider().enabled;
        break;
    }

    if (!enabledA || !enabledB) return;

    // Attribute/Mask filter:
    // A must want to collide with B's attribute, AND
    // B must want to collide with A's attribute
    if ((maskA & attrB) == 0) return;
    if ((maskB & attrA) == 0) return;

    // Dispatch intersection according to shapes
    bool collided = false;

    if (shapeA == ColliderShape::Sphere && shapeB == ColliderShape::Sphere) {
        collided = CheckSphereVsSphere(a->GetSphereCollider(), b->GetSphereCollider());
    }
    else if (shapeA == ColliderShape::Sphere && shapeB == ColliderShape::AABB) {
        collided = CheckSphereVsAABB(a->GetSphereCollider(), b->GetAABBCollider());
    }
    else if (shapeA == ColliderShape::AABB && shapeB == ColliderShape::Sphere) {
        collided = CheckSphereVsAABB(b->GetSphereCollider(), a->GetAABBCollider());
    }
    else if (shapeA == ColliderShape::Sphere && shapeB == ColliderShape::OBB) {
        collided = CheckSphereVsOBB(a->GetSphereCollider(), b->GetOBBCollider());
    }
    else if (shapeA == ColliderShape::OBB && shapeB == ColliderShape::Sphere) {
        collided = CheckSphereVsOBB(b->GetSphereCollider(), a->GetOBBCollider());
    }
    else if (shapeA == ColliderShape::AABB && shapeB == ColliderShape::AABB) {
        collided = CheckAABBvsAABB(a->GetAABBCollider(), b->GetAABBCollider());
    }
    else if (shapeA == ColliderShape::OBB && shapeB == ColliderShape::OBB) {
        collided = CheckOBBvsOBB(a->GetOBBCollider(), b->GetOBBCollider());
    }
    else if (shapeA == ColliderShape::AABB && shapeB == ColliderShape::OBB) {
        collided = CheckOBBvsAABB(b->GetOBBCollider(), a->GetAABBCollider());
    }
    else if (shapeA == ColliderShape::OBB && shapeB == ColliderShape::AABB) {
        collided = CheckOBBvsAABB(a->GetOBBCollider(), b->GetAABBCollider());
    }

    if (collided) {
        a->OnCollision(b);
        b->OnCollision(a);
        a->OnCollision();
        b->OnCollision();
    }
}
