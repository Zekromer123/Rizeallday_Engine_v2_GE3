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

    const SphereCollider& ca = a->GetSphereCollider();
    const SphereCollider& cb = b->GetSphereCollider();

    // Attribute/Mask filter:
    // A must want to collide with B's attribute, AND
    // B must want to collide with A's attribute
    if ((ca.mask & cb.attribute) == 0) return;
    if ((cb.mask & ca.attribute) == 0) return;

    // Sphere vs Sphere check (Squared distance, no sqrt - from GE3 curriculum)
    if (!CheckSphereVsSphere(ca, cb)) return;

    // Collision detected! Call both callbacks
    a->OnCollision(b);
    b->OnCollision(a);
    a->OnCollision();
    b->OnCollision();
}
