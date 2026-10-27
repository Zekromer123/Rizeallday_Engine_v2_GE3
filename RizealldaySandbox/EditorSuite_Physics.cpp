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

	for (auto& obj : gameObjects_) {
		if (!obj.isActive) continue;
		obj.isColliding = false;

		if (!obj.hasRigidbody || !obj.rbEnabled || obj.isKinematic || obj.isStatic) {
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

	// Dispatch CollisionManager events to ScriptBase::OnCollision(GameObject& other) in Play mode
	if (sceneState_ != EditorSceneState::Edit || IsStandaloneMode()) {
		CollisionManager* colMgr = CollisionManager::GetInstance();
		colMgr->Clear();

		std::vector<GameObjectColliderAdapter> adapters;
		adapters.reserve(gameObjects_.size() * 2);

		for (const auto& obj : gameObjects_) {
			if (!obj.isActive) continue;

			if (obj.hasBoxCollider && obj.boxColliderEnabled) {
				Vector3 halfExt = {
					std::abs(obj.boxSize.x * obj.scale.x) * 0.5f,
					std::abs(obj.boxSize.y * obj.scale.y) * 0.5f,
					std::abs(obj.boxSize.z * obj.scale.z) * 0.5f
				};
				AABBCollider box{};
				box.min = {
					obj.translation.x + obj.boxCenter.x - halfExt.x,
					obj.translation.y + obj.boxCenter.y - halfExt.y,
					obj.translation.z + obj.boxCenter.z - halfExt.z
				};
				box.max = {
					obj.translation.x + obj.boxCenter.x + halfExt.x,
					obj.translation.y + obj.boxCenter.y + halfExt.y,
					obj.translation.z + obj.boxCenter.z + halfExt.z
				};
				box.attribute = 1u << (obj.layer & 31u);
				box.mask = obj.collisionMask;
				box.enabled = true;
				SphereCollider dummySphere{};
				adapters.emplace_back(this, obj.id, ColliderShape::AABB, box, dummySphere);
				colMgr->RegisterCollider(&adapters.back());
			}

			if (obj.hasSphereCollider && obj.sphereColliderEnabled) {
				float maxScale = (std::max)({ std::abs(obj.scale.x), std::abs(obj.scale.y), std::abs(obj.scale.z) });
				SphereCollider sph{};
				sph.center = {
					obj.translation.x + obj.sphereCenter.x,
					obj.translation.y + obj.sphereCenter.y,
					obj.translation.z + obj.sphereCenter.z
				};
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

	// Collision detection and resolution
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		auto& dynamicObj = gameObjects_[i];
		if (!dynamicObj.isActive || !dynamicObj.hasRigidbody || !dynamicObj.rbEnabled || dynamicObj.isKinematic || dynamicObj.isStatic) {
			continue;
		}
		if (!dynamicObj.hasBoxCollider || !dynamicObj.boxColliderEnabled) {
			continue;
		}

		Vector3 dMin = {
			dynamicObj.translation.x + dynamicObj.boxCenter.x - (dynamicObj.boxSize.x * dynamicObj.scale.x) * 0.5f,
			dynamicObj.translation.y + dynamicObj.boxCenter.y - (dynamicObj.boxSize.y * dynamicObj.scale.y) * 0.5f,
			dynamicObj.translation.z + dynamicObj.boxCenter.z - (dynamicObj.boxSize.z * dynamicObj.scale.z) * 0.5f
		};
		Vector3 dMax = {
			dynamicObj.translation.x + dynamicObj.boxCenter.x + (dynamicObj.boxSize.x * dynamicObj.scale.x) * 0.5f,
			dynamicObj.translation.y + dynamicObj.boxCenter.y + (dynamicObj.boxSize.y * dynamicObj.scale.y) * 0.5f,
			dynamicObj.translation.z + dynamicObj.boxCenter.z + (dynamicObj.boxSize.z * dynamicObj.scale.z) * 0.5f
		};

		for (size_t j = 0; j < gameObjects_.size(); ++j) {
			if (i == j) continue;
			auto& otherObj = gameObjects_[j];
			if (!otherObj.isActive || !otherObj.hasBoxCollider || !otherObj.boxColliderEnabled) {
				continue;
			}

			Vector3 oMin = {
				otherObj.translation.x + otherObj.boxCenter.x - (otherObj.boxSize.x * otherObj.scale.x) * 0.5f,
				otherObj.translation.y + otherObj.boxCenter.y - (otherObj.boxSize.y * otherObj.scale.y) * 0.5f,
				otherObj.translation.z + otherObj.boxCenter.z - (otherObj.boxSize.z * otherObj.scale.z) * 0.5f
			};
			Vector3 oMax = {
				otherObj.translation.x + otherObj.boxCenter.x + (otherObj.boxSize.x * otherObj.scale.x) * 0.5f,
				otherObj.translation.y + otherObj.boxCenter.y + (otherObj.boxSize.y * otherObj.scale.y) * 0.5f,
				otherObj.translation.z + otherObj.boxCenter.z + (otherObj.boxSize.z * otherObj.scale.z) * 0.5f
			};

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

				dMin = {
					dynamicObj.translation.x + dynamicObj.boxCenter.x - (dynamicObj.boxSize.x * dynamicObj.scale.x) * 0.5f,
					dynamicObj.translation.y + dynamicObj.boxCenter.y - (dynamicObj.boxSize.y * dynamicObj.scale.y) * 0.5f,
					dynamicObj.translation.z + dynamicObj.boxCenter.z - (dynamicObj.boxSize.z * dynamicObj.scale.z) * 0.5f
				};
				dMax = {
					dynamicObj.translation.x + dynamicObj.boxCenter.x + (dynamicObj.boxSize.x * dynamicObj.scale.x) * 0.5f,
					dynamicObj.translation.y + dynamicObj.boxCenter.y + (dynamicObj.boxSize.y * dynamicObj.scale.y) * 0.5f,
					dynamicObj.translation.z + dynamicObj.boxCenter.z + (dynamicObj.boxSize.z * dynamicObj.scale.z) * 0.5f
				};
			}
		}
	}
}

