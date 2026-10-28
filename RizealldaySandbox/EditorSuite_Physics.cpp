#include "EditorSuiteInternal.h"

// Adapts a GameObject's collider to ICollidable so CollisionManager hits are forwarded to its scripts
namespace {
	class GameObjectColliderAdapter : public ICollidable {
	public:
		GameObjectColliderAdapter(EditorSuite* suite, uint32_t objId, ColliderShape shape, const AABBCollider& box, const SphereCollider& sph)
			: suite_(suite), objectId_(objId), shape_(shape), aabb_(box), sphere_(sph) {}

		ColliderShape GetColliderShape() const override { return shape_; }
		const AABBCollider& GetAABBCollider() const override { return aabb_; }
		const SphereCollider& GetSphereCollider() const override { return sphere_; }

		void OnCollision(ICollidable* other) override {
			auto* otherAdapter = dynamic_cast<GameObjectColliderAdapter*>(other);
			if (suite_ && otherAdapter && otherAdapter->objectId_ != objectId_) {
				suite_->DispatchScriptCollision(objectId_, otherAdapter->objectId_);
			}
		}

		uint32_t GetObjectId() const { return objectId_; }

	private:
		EditorSuite* suite_ = nullptr;
		uint32_t objectId_ = 0;
		ColliderShape shape_ = ColliderShape::AABB;
		AABBCollider aabb_{};
		SphereCollider sphere_{};
	};
}

void EditorSuite::UpdatePhysics(float deltaTime)
{
	const float gravity = -9.81f;

	// Scripts may have moved objects earlier this frame
	UpdateWorldMatrices();

	// World-space AABB of an object's box collider. Roots keep the original math (translation + center);
	// children transform the center through the world matrix and use the world scale.
	auto computeBoxBounds = [this](const GameObject& o, bool useAbsSize, Vector3& outMin, Vector3& outMax) {
		Vector3 wPos{};
		Quaternion wRot = Quaternion::Identity();
		Vector3 wScale{};
		GetWorldTRS(o, wPos, wRot, wScale);
		const Vector3 center = (o.parentId == -1)
			? Vector3{ wPos.x + o.boxCenter.x, wPos.y + o.boxCenter.y, wPos.z + o.boxCenter.z }
			: TransformUtil::TransformPoint(o.boxCenter, o.worldMatrix);
		Vector3 half = { o.boxSize.x * wScale.x * 0.5f, o.boxSize.y * wScale.y * 0.5f, o.boxSize.z * wScale.z * 0.5f };
		if (useAbsSize) {
			half = { std::abs(half.x), std::abs(half.y), std::abs(half.z) };
		}
		outMin = { center.x - half.x, center.y - half.y, center.z - half.z };
		outMax = { center.x + half.x, center.y + half.y, center.z + half.z };
	};

	for (auto& obj : gameObjects_) {
		if (!obj.isActive) continue;
		obj.isColliding = false;

		if (!obj.hasRigidbody || !obj.rbEnabled || obj.isKinematic || obj.isStatic) {
			continue;
		}

		// Rigidbody integration is root-only: a child is treated as kinematic (its collider stays active)
		if (obj.parentId != -1) {
			if (childRigidbodyWarned_.insert(obj.id).second) {
				LogMessage(std::format("[Physics] '{}' has a Rigidbody but is a child object; it is treated as kinematic.", obj.name), 1);
			}
			continue;
		}

		if (obj.useGravity) {
			obj.velocity.y += gravity * obj.gravityScale * deltaTime;
		}

		float dragFactor = std::clamp(1.0f - obj.drag * deltaTime, 0.0f, 1.0f);
		obj.velocity.x *= dragFactor;
		obj.velocity.y *= dragFactor;
		obj.velocity.z *= dragFactor;

		obj.translation.x += obj.velocity.x * deltaTime;
		obj.translation.y += obj.velocity.y * deltaTime;
		obj.translation.z += obj.velocity.z * deltaTime;

		obj.isGrounded = false;
	}

	// Integration moved root objects; refresh the matrices their children are derived from
	UpdateWorldMatrices();

	// Dispatch CollisionManager events to ScriptBase::OnCollision(GameObject& other) in Play mode
	if (sceneState_ != EditorSceneState::Edit || IsStandaloneMode()) {
		CollisionManager* colMgr = CollisionManager::GetInstance();
		colMgr->Clear();

		std::vector<GameObjectColliderAdapter> adapters;
		adapters.reserve(gameObjects_.size() * 2);

		for (const auto& obj : gameObjects_) {
			if (!obj.isActive) continue;

			if (obj.hasBoxCollider && obj.boxColliderEnabled) {
				AABBCollider box{};
				computeBoxBounds(obj, true, box.min, box.max);
				box.attribute = 1u << (obj.layer & 31u);
				box.mask = obj.collisionMask;
				box.enabled = true;
				SphereCollider dummySphere{};
				adapters.emplace_back(this, obj.id, ColliderShape::AABB, box, dummySphere);
				colMgr->RegisterCollider(&adapters.back());
			}

			if (obj.hasSphereCollider && obj.sphereColliderEnabled) {
				Vector3 wPos{};
				Quaternion wRot = Quaternion::Identity();
				Vector3 wScale{};
				GetWorldTRS(obj, wPos, wRot, wScale);
				float maxScale = (std::max)({ std::abs(wScale.x), std::abs(wScale.y), std::abs(wScale.z) });
				SphereCollider sph{};
				sph.center = (obj.parentId == -1)
					? Vector3{ wPos.x + obj.sphereCenter.x, wPos.y + obj.sphereCenter.y, wPos.z + obj.sphereCenter.z }
					: TransformUtil::TransformPoint(obj.sphereCenter, obj.worldMatrix);
				sph.radius = obj.sphereRadius * maxScale;
				sph.attribute = 1u << (obj.layer & 31u);
				sph.mask = obj.collisionMask;
				sph.enabled = true;
				AABBCollider dummyBox{};
				adapters.emplace_back(this, obj.id, ColliderShape::Sphere, dummyBox, sph);
				colMgr->RegisterCollider(&adapters.back());
			}
		}

		colMgr->CheckAllCollisions();
		colMgr->Clear();
	}

	// Collision detection and resolution.
	// Bounds of every box collider are computed once (GetWorldTRS is not free) and kept current as
	// dynamic objects are pushed out, so the pair loop below only does cheap AABB math.
	struct BoxProxy {
		size_t index = 0;
		Vector3 min{};
		Vector3 max{};
	};
	std::vector<BoxProxy> boxProxies;
	boxProxies.reserve(gameObjects_.size());
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		const auto& o = gameObjects_[i];
		if (!o.isActive || !o.hasBoxCollider || !o.boxColliderEnabled) continue;
		BoxProxy proxy{};
		proxy.index = i;
		computeBoxBounds(o, false, proxy.min, proxy.max);
		boxProxies.push_back(proxy);
	}

	for (auto& dynamicProxy : boxProxies) {
		auto& dynamicObj = gameObjects_[dynamicProxy.index];
		if (!dynamicObj.hasRigidbody || !dynamicObj.rbEnabled || dynamicObj.isKinematic || dynamicObj.isStatic) {
			continue;
		}
		// Child rigidbodies are kinematic (see integration above)
		if (dynamicObj.parentId != -1) {
			continue;
		}

		// Dynamic objects are roots, so their bounds come straight from the (possibly just corrected) translation
		Vector3& dMin = dynamicProxy.min;
		Vector3& dMax = dynamicProxy.max;
		const uint32_t dynamicLayerBit = 1u << (dynamicObj.layer & 31u);

		for (const auto& otherProxy : boxProxies) {
			if (otherProxy.index == dynamicProxy.index) continue;
			auto& otherObj = gameObjects_[otherProxy.index];
			// Same layer rules as the script collision events
			if ((dynamicObj.collisionMask & (1u << (otherObj.layer & 31u))) == 0 ||
				(otherObj.collisionMask & dynamicLayerBit) == 0) {
				continue;
			}

			const Vector3& oMin = otherProxy.min;
			const Vector3& oMax = otherProxy.max;

			bool overlapX = (dMin.x < oMax.x) && (dMax.x > oMin.x);
			bool overlapY = (dMin.y < oMax.y) && (dMax.y > oMin.y);
			bool overlapZ = (dMin.z < oMax.z) && (dMax.z > oMin.z);

			if (overlapX && overlapY && overlapZ) {
				dynamicObj.isColliding = true;
				otherObj.isColliding = true;

				if (dynamicObj.isTrigger || otherObj.isTrigger) continue;

				float penX1 = oMax.x - dMin.x;
				float penX2 = dMax.x - oMin.x;
				float penX = (penX1 < penX2) ? penX1 : -penX2;

				float penY1 = oMax.y - dMin.y;
				float penY2 = dMax.y - oMin.y;
				float penY = (penY1 < penY2) ? penY1 : -penY2;

				float penZ1 = oMax.z - dMin.z;
				float penZ2 = dMax.z - oMin.z;
				float penZ = (penZ1 < penZ2) ? penZ1 : -penZ2;

				float absX = std::abs(penX);
				float absY = std::abs(penY);
				float absZ = std::abs(penZ);

				if (absY <= absX && absY <= absZ) {
					dynamicObj.translation.y += penY;
					if (penY > 0.0f) {
						if (dynamicObj.velocity.y < 0.0f) {
							dynamicObj.velocity.y = -dynamicObj.velocity.y * dynamicObj.bounciness;
							if (std::abs(dynamicObj.velocity.y) < 0.2f) dynamicObj.velocity.y = 0.0f;
						}
						dynamicObj.isGrounded = true;
					} else {
						if (dynamicObj.velocity.y > 0.0f) dynamicObj.velocity.y = -dynamicObj.velocity.y * dynamicObj.bounciness;
					}
				} else if (absX <= absY && absX <= absZ) {
					dynamicObj.translation.x += penX;
					dynamicObj.velocity.x = -dynamicObj.velocity.x * dynamicObj.bounciness;
					if (std::abs(dynamicObj.velocity.x) < 0.1f) dynamicObj.velocity.x = 0.0f;
				} else {
					dynamicObj.translation.z += penZ;
					dynamicObj.velocity.z = -dynamicObj.velocity.z * dynamicObj.bounciness;
					if (std::abs(dynamicObj.velocity.z) < 0.1f) dynamicObj.velocity.z = 0.0f;
				}

				computeBoxBounds(dynamicObj, false, dMin, dMax);
			}
		}
	}

	// Collision resolution moved root objects; keep world matrices current for whoever runs next
	UpdateWorldMatrices();
}

