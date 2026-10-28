#include "3d/CollisionManager.h"
#include <algorithm>
#include <cmath>

namespace {
    // Grid coordinates are packed into 21 bits per axis (about +-1M cells)
    constexpr int kCellCoordLimit = (1 << 20) - 1;
    // A collider spanning more cells than this (ground planes, walls) is tested against everything
    constexpr int64_t kMaxCellsPerProxy = 64;

    int ToCell(float v, float invCellSize) {
        const float c = std::floor(v * invCellSize);
        return static_cast<int>(std::clamp(c, -static_cast<float>(kCellCoordLimit), static_cast<float>(kCellCoordLimit)));
    }

    uint64_t PackCell(int x, int y, int z) {
        const uint64_t ux = static_cast<uint64_t>(x + kCellCoordLimit + 1) & 0x1FFFFFull;
        const uint64_t uy = static_cast<uint64_t>(y + kCellCoordLimit + 1) & 0x1FFFFFull;
        const uint64_t uz = static_cast<uint64_t>(z + kCellCoordLimit + 1) & 0x1FFFFFull;
        return (ux << 42) | (uy << 21) | uz;
    }

    void UnpackCell(uint64_t key, int& x, int& y, int& z) {
        x = static_cast<int>((key >> 42) & 0x1FFFFFull) - (kCellCoordLimit + 1);
        y = static_cast<int>((key >> 21) & 0x1FFFFFull) - (kCellCoordLimit + 1);
        z = static_cast<int>(key & 0x1FFFFFull) - (kCellCoordLimit + 1);
    }

    bool IsFinite(const Vector3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
}

CollisionManager* CollisionManager::GetInstance() {
    static CollisionManager instance;
    return &instance;
}

void CollisionManager::RegisterCollider(ICollidable* collidable) {
    if (collidable) {
        colliders_.push_back(collidable);
    }
}

void CollisionManager::RemoveCollider(ICollidable* collidable) {
    if (!collidable) return;
    for (auto it = colliders_.begin(); it != colliders_.end(); ) {
        if (*it == collidable) {
            it = colliders_.erase(it);
        } else {
            ++it;
        }
    }
}

void CollisionManager::CheckAllCollisions() {
    lastStats_ = {};
    lastStats_.colliders = static_cast<uint32_t>(colliders_.size());

    if (broadphaseEnabled_ && colliders_.size() >= kBroadphaseMinColliders) {
        lastStats_.usedBroadphase = true;
        CheckAllPairsBroadphase();
    } else {
        CheckAllPairsBruteForce();
    }
    // Clear registrations for next frame
    colliders_.clear();
}

void CollisionManager::Clear() {
    colliders_.clear();
}

void CollisionManager::CheckAllPairsBruteForce() {
    for (size_t i = 0; i < colliders_.size(); ++i) {
        for (size_t j = i + 1; j < colliders_.size(); ++j) {
            CheckCollisionPair(colliders_[i], colliders_[j]);
        }
    }
}

void CollisionManager::CheckAllPairsBroadphase() {
    const size_t count = colliders_.size();
    proxies_.assign(count, ProxyInfo{});
    cellEntries_.clear();
    largeProxies_.clear();

    // 1) World AABB + filter data of every collider
    std::vector<float> sizes;
    sizes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        ProxyInfo& p = proxies_[i];
        const ICollidable* c = colliders_[i];
        if (!c) continue;
        switch (c->GetColliderShape()) {
        case ColliderShape::Sphere: {
            const SphereCollider& s = c->GetSphereCollider();
            const float r = std::abs(s.radius);
            p.min = { s.center.x - r, s.center.y - r, s.center.z - r };
            p.max = { s.center.x + r, s.center.y + r, s.center.z + r };
            p.attribute = s.attribute; p.mask = s.mask; p.active = s.enabled;
            break;
        }
        case ColliderShape::AABB: {
            AABBCollider b = c->GetAABBCollider();
            b.EnsureValid();
            p.min = b.min; p.max = b.max;
            p.attribute = b.attribute; p.mask = b.mask; p.active = b.enabled;
            break;
        }
        case ColliderShape::OBB: {
            const OBBCollider& o = c->GetOBBCollider();
            Vector3 e{};
            for (int axis = 0; axis < 3; ++axis) {
                const Vector3& d = o.orientations[axis];
                const float h = (axis == 0) ? o.size.x : (axis == 1) ? o.size.y : o.size.z;
                e.x += std::abs(d.x * h); e.y += std::abs(d.y * h); e.z += std::abs(d.z * h);
            }
            p.min = { o.center.x - e.x, o.center.y - e.y, o.center.z - e.z };
            p.max = { o.center.x + e.x, o.center.y + e.y, o.center.z + e.z };
            p.attribute = o.attribute; p.mask = o.mask; p.active = o.enabled;
            break;
        }
        }
        // A collider nobody can hit (or that wants to hit nothing) never produces a pair
        if (p.attribute == 0 || p.mask == 0) p.active = false;
        if (!p.active) continue;
        if (!IsFinite(p.min) || !IsFinite(p.max)) {
            p.large = true; // let the narrowphase decide
            continue;
        }
        sizes.push_back((std::max)({ p.max.x - p.min.x, p.max.y - p.min.y, p.max.z - p.min.z }));
    }

    // 2) Cell size from the median collider size (robust against a few huge ground/wall colliders)
    float cellSize = 1.0f;
    if (!sizes.empty()) {
        auto mid = sizes.begin() + static_cast<std::ptrdiff_t>(sizes.size() / 2);
        std::nth_element(sizes.begin(), mid, sizes.end());
        cellSize = (std::max)(*mid * 2.0f, 0.05f);
    }
    const float invCell = 1.0f / cellSize;

    // 3) Insert every proxy into the cells its AABB covers
    for (size_t i = 0; i < count; ++i) {
        ProxyInfo& p = proxies_[i];
        if (!p.active) continue;
        if (!p.large) {
            p.cellMin[0] = ToCell(p.min.x, invCell); p.cellMax[0] = ToCell(p.max.x, invCell);
            p.cellMin[1] = ToCell(p.min.y, invCell); p.cellMax[1] = ToCell(p.max.y, invCell);
            p.cellMin[2] = ToCell(p.min.z, invCell); p.cellMax[2] = ToCell(p.max.z, invCell);
            const int64_t cells = int64_t{ p.cellMax[0] - p.cellMin[0] + 1 }
                * int64_t{ p.cellMax[1] - p.cellMin[1] + 1 }
                * int64_t{ p.cellMax[2] - p.cellMin[2] + 1 };
            p.large = cells > kMaxCellsPerProxy;
        }
        if (p.large) {
            largeProxies_.push_back(static_cast<uint32_t>(i));
            continue;
        }
        for (int x = p.cellMin[0]; x <= p.cellMax[0]; ++x) {
            for (int y = p.cellMin[1]; y <= p.cellMax[1]; ++y) {
                for (int z = p.cellMin[2]; z <= p.cellMax[2]; ++z) {
                    cellEntries_.push_back({ PackCell(x, y, z), static_cast<uint32_t>(i) });
                }
            }
        }
    }

    // 4) Sort so each cell's proxies are contiguous (deterministic order)
    std::sort(cellEntries_.begin(), cellEntries_.end(), [](const CellEntry& a, const CellEntry& b) {
        return (a.key != b.key) ? (a.key < b.key) : (a.proxy < b.proxy);
    });

    // 5) Pairs inside each cell. A pair sharing several cells is only tested in the lowest shared cell.
    for (size_t begin = 0; begin < cellEntries_.size(); ) {
        size_t end = begin + 1;
        while (end < cellEntries_.size() && cellEntries_[end].key == cellEntries_[begin].key) ++end;
        if (end - begin > 1) {
            int cx = 0, cy = 0, cz = 0;
            UnpackCell(cellEntries_[begin].key, cx, cy, cz);
            for (size_t i = begin; i < end; ++i) {
                const ProxyInfo& a = proxies_[cellEntries_[i].proxy];
                for (size_t j = i + 1; j < end; ++j) {
                    const ProxyInfo& b = proxies_[cellEntries_[j].proxy];
                    if ((std::max)(a.cellMin[0], b.cellMin[0]) != cx ||
                        (std::max)(a.cellMin[1], b.cellMin[1]) != cy ||
                        (std::max)(a.cellMin[2], b.cellMin[2]) != cz) {
                        continue;
                    }
                    TestProxyPair(cellEntries_[i].proxy, cellEntries_[j].proxy);
                }
            }
        }
        begin = end;
    }

    // 6) Large proxies against everything else (large-vs-large once)
    for (uint32_t l : largeProxies_) {
        for (size_t i = 0; i < count; ++i) {
            const ProxyInfo& other = proxies_[i];
            if (!other.active || i == l) continue;
            if (other.large && i < l) continue;
            TestProxyPair(l, static_cast<uint32_t>(i));
        }
    }
}

void CollisionManager::TestProxyPair(uint32_t a, uint32_t b) {
    const ProxyInfo& pa = proxies_[a];
    const ProxyInfo& pb = proxies_[b];
    // Layer filter first (cheap), then the bounds overlap
    if ((pa.mask & pb.attribute) == 0 || (pb.mask & pa.attribute) == 0) return;
    if (!pa.large && !pb.large) {
        if (pa.min.x > pb.max.x || pb.min.x > pa.max.x) return;
        if (pa.min.y > pb.max.y || pb.min.y > pa.max.y) return;
        if (pa.min.z > pb.max.z || pb.min.z > pa.max.z) return;
    }
    // Keep registration order so callbacks see the same (a, b) order as the brute-force path
    CheckCollisionPair(colliders_[(std::min)(a, b)], colliders_[(std::max)(a, b)]);
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

    ++lastStats_.narrowphaseTests;

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
        ++lastStats_.collisions;
        a->OnCollision(b);
        b->OnCollision(a);
        a->OnCollision();
        b->OnCollision();
    }
}
