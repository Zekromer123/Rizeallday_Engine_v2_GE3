#include "EditorSuiteInternal.h"
#include "audio/Audio.h"

// Script API part 2 (Tahap H): collision enter/exit, scene queries, hierarchy, audio, particles,
// material and component access. EngineAPI entries are filled in by SetupEngineAPIExtensions().

namespace {
/// Animator of a script target, started at its entry state if the state machine has not run yet
/// (Initialize runs before the first animation update, and parameters set there must not be lost)
AnimatorComponent* ScriptAnimator(void* ctx, uint32_t id, std::shared_ptr<AnimatorController>& outController)
{
	if (!ctx) return nullptr;
	EditorSuite& self = *static_cast<EditorSuite*>(ctx);
	GameObject* obj = self.FindGameObjectById(id, false);
	AnimatorComponent* a = obj ? self.GetAnimator(*obj) : nullptr;
	if (!a || a->controller[0] == '\0') return nullptr;
	outController = self.GetAnimatorController(a->controller);
	if (!outController || outController->states.empty()) return nullptr;
	if (!a->runtime.started || a->runtimeController != a->controller) {
		a->runtime = AnimatorRuntime{};
		a->runtimeController = a->controller;
		const AnimatorRuntime::Change change = a->runtime.Start(*outController);
		if (const AnimState* s = change.changed ? outController->FindState(change.state) : nullptr) self.ApplyAnimatorState(*obj, *a, *s, 0.0f);
	}
	return a;
}

constexpr uint64_t ContactKey(uint32_t selfId, uint32_t otherId)
{
	return (static_cast<uint64_t>(selfId) << 32) | otherId;
}

float Dot3(const Vector3& a, const Vector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Objects about to be deleted or despawned at the end of this frame are invisible to queries,
// so two bullets fired in the same frame cannot both "hit" an enemy that the first one destroyed.
std::unordered_set<uint32_t> CollectQueryExclusions(const std::vector<GameObject>& objects,
	const std::vector<int>& pendingDeleteIndices, const std::vector<uint32_t>& pendingDespawnIds)
{
	std::unordered_set<uint32_t> excluded(pendingDespawnIds.begin(), pendingDespawnIds.end());
	for (int index : pendingDeleteIndices) {
		if (index >= 0 && static_cast<size_t>(index) < objects.size()) {
			excluded.insert(objects[static_cast<size_t>(index)].id);
		}
	}
	return excluded;
}

bool HasCollider(const GameObject& obj)
{
	return (obj.hasBoxCollider && obj.boxColliderEnabled) || (obj.hasSphereCollider && obj.sphereColliderEnabled);
}

bool TagMatches(const GameObject& obj, const char* tag)
{
	return !tag || tag[0] == '\0' || obj.tag == tag;
}
}

// ---------------------------------------------------------------------------
// Collision events
// ---------------------------------------------------------------------------
void EditorSuite::RecordCollisionContact(uint32_t selfId, uint32_t otherId)
{
	if (selfId == otherId) return;
	const uint64_t key = ContactKey(selfId, otherId);
	if (frameContactSet_.insert(key).second) {
		frameContacts_.push_back(key);
	}
}

void EditorSuite::DispatchCollisionEvents()
{
	std::vector<uint64_t> current;
	current.swap(frameContacts_);
	std::unordered_set<uint64_t> currentSet;
	currentSet.swap(frameContactSet_);
	lastCollisionEvents_ = CollisionEventCounts{};

	for (uint64_t key : current) {
		const uint32_t selfId = static_cast<uint32_t>(key >> 32);
		const uint32_t otherId = static_cast<uint32_t>(key & 0xFFFFFFFFu);
		if (previousContactSet_.count(key) == 0) {
			++lastCollisionEvents_.enter;
			DispatchScriptCollision(selfId, otherId, CollisionEvent::Enter);
		}
		++lastCollisionEvents_.stay;
		DispatchScriptCollision(selfId, otherId, CollisionEvent::Stay);
	}
	for (uint64_t key : previousContacts_) {
		if (currentSet.count(key) == 0) {
			++lastCollisionEvents_.exit;
			DispatchScriptCollision(static_cast<uint32_t>(key >> 32), static_cast<uint32_t>(key & 0xFFFFFFFFu), CollisionEvent::Exit);
		}
	}

	previousContacts_ = std::move(current);
	previousContactSet_ = std::move(currentSet);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------
Vector3 EditorSuite::GetWorldPositionOf(const GameObject& obj) const
{
	// Roots: translation is fresh even mid-frame. Children: the world matrix (refreshed every frame).
	if (obj.parentId == -1) return obj.translation;
	return { obj.worldMatrix.m[3][0], obj.worldMatrix.m[3][1], obj.worldMatrix.m[3][2] };
}

bool EditorSuite::ScriptRaycast(const Vector3& origin, const Vector3& direction, float maxDistance,
	uint32_t layerMask, uint32_t ignoreId, ScriptRaycastHit& outHit) const
{
	outHit = ScriptRaycastHit{};
	const float len = std::sqrt(Dot3(direction, direction));
	if (!(len > 1.0e-6f) || !(maxDistance > 0.0f) || !std::isfinite(len)) return false;
	const Vector3 dir = direction / len;

	const auto excluded = CollectQueryExclusions(gameObjects_, pendingDeletes_, pendingDespawns_);
	float best = maxDistance;
	bool found = false;

	for (const auto& obj : gameObjects_) {
		if (!obj.isActive || obj.id == ignoreId || !HasCollider(obj)) continue;
		if ((layerMask & LayerBit(obj.layer)) == 0 || excluded.count(obj.id) != 0) continue;

		if (obj.hasBoxCollider && obj.boxColliderEnabled) {
			Vector3 bmin{};
			Vector3 bmax{};
			ComputeBoxColliderBounds(obj, true, bmin, bmax);
			// Slab test; remembers which axis the ray entered through for the normal
			float tNear = -1.0e30f;
			float tFar = 1.0e30f;
			int nearAxis = -1;
			bool miss = false;
			const float o[3] = { origin.x, origin.y, origin.z };
			const float d[3] = { dir.x, dir.y, dir.z };
			const float mn[3] = { bmin.x, bmin.y, bmin.z };
			const float mx[3] = { bmax.x, bmax.y, bmax.z };
			for (int axis = 0; axis < 3 && !miss; ++axis) {
				if (std::abs(d[axis]) < 1.0e-8f) {
					if (o[axis] < mn[axis] || o[axis] > mx[axis]) miss = true;
					continue;
				}
				float t1 = (mn[axis] - o[axis]) / d[axis];
				float t2 = (mx[axis] - o[axis]) / d[axis];
				if (t1 > t2) std::swap(t1, t2);
				if (t1 > tNear) { tNear = t1; nearAxis = axis; }
				tFar = (std::min)(tFar, t2);
				if (tNear > tFar) miss = true;
			}
			// tNear < 0: the origin is inside the box (ignored, like the shooter's own collider)
			if (!miss && nearAxis >= 0 && tNear >= 0.0f && tNear <= best) {
				best = tNear;
				found = true;
				outHit.objectId = obj.id;
				outHit.distance = tNear;
				outHit.point = origin + dir * tNear;
				Vector3 n{ 0.0f, 0.0f, 0.0f };
				const float sign = d[nearAxis] > 0.0f ? -1.0f : 1.0f;
				if (nearAxis == 0) n.x = sign; else if (nearAxis == 1) n.y = sign; else n.z = sign;
				outHit.normal = n;
			}
		}

		if (obj.hasSphereCollider && obj.sphereColliderEnabled) {
			Vector3 c{};
			float r = 0.0f;
			ComputeSphereColliderWorld(obj, c, r);
			const Vector3 oc = origin - c;
			const float b = Dot3(oc, dir);
			const float cc = Dot3(oc, oc) - r * r;
			const float disc = b * b - cc;
			if (cc > 0.0f && disc >= 0.0f) {
				const float t = -b - std::sqrt(disc);
				if (t >= 0.0f && t <= best) {
					best = t;
					found = true;
					outHit.objectId = obj.id;
					outHit.distance = t;
					outHit.point = origin + dir * t;
					outHit.normal = (r > 1.0e-6f) ? (outHit.point - c) / r : Vector3{ 0.0f, 1.0f, 0.0f };
				}
			}
		}
	}
	return found;
}

int EditorSuite::ScriptOverlapSphere(const Vector3& center, float radius, uint32_t layerMask, uint32_t* outIds, int maxCount) const
{
	if (!outIds || maxCount <= 0 || !(radius >= 0.0f)) return 0;
	const auto excluded = CollectQueryExclusions(gameObjects_, pendingDeletes_, pendingDespawns_);
	int count = 0;
	for (const auto& obj : gameObjects_) {
		if (count >= maxCount) break;
		if (!obj.isActive || !HasCollider(obj)) continue;
		if ((layerMask & LayerBit(obj.layer)) == 0 || excluded.count(obj.id) != 0) continue;

		bool touches = false;
		if (obj.hasBoxCollider && obj.boxColliderEnabled) {
			Vector3 bmin{};
			Vector3 bmax{};
			ComputeBoxColliderBounds(obj, true, bmin, bmax);
			const Vector3 closest{ std::clamp(center.x, bmin.x, bmax.x), std::clamp(center.y, bmin.y, bmax.y), std::clamp(center.z, bmin.z, bmax.z) };
			const Vector3 delta = closest - center;
			touches = Dot3(delta, delta) <= radius * radius;
		}
		if (!touches && obj.hasSphereCollider && obj.sphereColliderEnabled) {
			Vector3 c{};
			float r = 0.0f;
			ComputeSphereColliderWorld(obj, c, r);
			const Vector3 delta = c - center;
			const float reach = r + radius;
			touches = Dot3(delta, delta) <= reach * reach;
		}
		if (touches) outIds[count++] = obj.id;
	}
	return count;
}

const GameObject* EditorSuite::FindNearestObject(const Vector3& position, const char* tag, float maxDistance, uint32_t ignoreId) const
{
	const auto excluded = CollectQueryExclusions(gameObjects_, pendingDeletes_, pendingDespawns_);
	const GameObject* nearest = nullptr;
	float bestSq = (maxDistance > 0.0f && std::isfinite(maxDistance)) ? maxDistance * maxDistance : 3.0e38f;
	for (const auto& obj : gameObjects_) {
		if (!obj.isActive || obj.id == ignoreId || !TagMatches(obj, tag) || excluded.count(obj.id) != 0) continue;
		const Vector3 delta = GetWorldPositionOf(obj) - position;
		const float distSq = Dot3(delta, delta);
		if (distSq <= bestSq) {
			bestSq = distSq;
			nearest = &obj;
		}
	}
	return nearest;
}

// ---------------------------------------------------------------------------
// Deferred requests & runtime state
// ---------------------------------------------------------------------------
void EditorSuite::ResetScriptRuntimeState()
{
	frameContacts_.clear();
	frameContactSet_.clear();
	previousContacts_.clear();
	previousContactSet_.clear();
	delayedDespawns_.clear();
	pendingReparents_.clear();
	pendingParticleBursts_.clear();
	if (musicVoice_ != 0) {
		Audio::GetInstance()->StopAudio(musicVoice_);
	}
	musicVoice_ = 0;
	shakeLeft_ = 0.0f;
	hitstopLeft_ = 0.0f;
	physicsAccumulator_ = 0.0;
	musicPath_.clear();
}

void EditorSuite::CancelDelayedDespawn(uint32_t id)
{
	std::erase_if(delayedDespawns_, [id](const DelayedDespawn& d) { return d.id == id; });
}

void EditorSuite::TickScriptTimers(float deltaTime)
{
	if (delayedDespawns_.empty()) return;
	std::vector<uint32_t> due;
	for (auto it = delayedDespawns_.begin(); it != delayedDespawns_.end();) {
		it->remaining -= deltaTime;
		if (it->remaining <= 0.0f) {
			due.push_back(it->id);
			it = delayedDespawns_.erase(it);
		} else {
			++it;
		}
	}
	for (uint32_t id : due) QueueDespawn(id);
}

void EditorSuite::BurstOneShotParticles(uint32_t id)
{
	const GameObject* obj = FindGameObjectById(id, false);
	if (!obj || !obj->isActive || !obj->hasParticleSystem || obj->particleEmitContinuously) return;
	pendingParticleBursts_.push_back({ id, obj->particleBurstCount, true });
}

void EditorSuite::ProcessDeferredScriptRequests()
{
	if (!pendingReparents_.empty()) {
		std::vector<PendingReparent> reparents;
		reparents.swap(pendingReparents_);
		// Play-mode hierarchy changes are runtime state: they must not mark the scene as edited
		const bool wasDirty = isSceneDirty_;
		for (const auto& r : reparents) {
			if (FindGameObjectById(r.childId, false)) {
				ReparentGameObject(r.childId, r.parentId, r.keepWorldTransform);
			}
		}
		isSceneDirty_ = wasDirty;
	}

	if (!pendingParticleBursts_.empty()) {
		std::vector<PendingParticleBurst> bursts;
		bursts.swap(pendingParticleBursts_);
		UpdateWorldMatrices(); // emit from the current world position (children of spawned prefabs)
		for (const auto& b : bursts) {
			GameObject* obj = FindGameObjectById(b.id, false);
			if (!obj || !obj->isActive || !obj->hasParticleSystem) continue;
			EnsureParticleQuadInitialized(*obj);
			if (!obj->particleQuad) continue;
			if (b.restart) obj->particleQuad->ClearParticles();
			obj->particleQuad->emitter.translate = GetWorldPositionOf(*obj);
			const int count = std::clamp(b.count, 1, (std::max)(1, obj->particleMaxParticles));
			obj->particleQuad->EmitBurst(static_cast<uint32_t>(count));
		}
	}
}

// ---------------------------------------------------------------------------
// EngineAPI entries
// ---------------------------------------------------------------------------
void EditorSuite::SetupEngineAPIExtensions()
{
	// ---- Audio ----
	engineApi_.playSound = [](void* ctx, const char* path, float volume, bool loop) -> uint32_t {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || !path || path[0] == '\0') return 0u;
		Audio* audio = Audio::GetInstance();
		const uint32_t sound = audio->LoadAudio(self->ResolveAssetPath(path));
		if (sound == 0) return 0u;
		const float v = std::isfinite(volume) ? std::clamp(volume, 0.0f, 4.0f) : 1.0f;
		return audio->PlayAudio(sound, loop, v);
	};
	engineApi_.stopSound = [](void*, uint32_t voice) {
		Audio::GetInstance()->StopAudio(voice);
	};
	engineApi_.setSoundVolume = [](void*, uint32_t voice, float volume) {
		Audio::GetInstance()->SetVolume(voice, std::isfinite(volume) ? volume : 1.0f);
	};
	engineApi_.isSoundPlaying = [](void*, uint32_t voice) -> bool {
		return Audio::GetInstance()->IsPlaying(voice);
	};
	engineApi_.preloadSound = [](void* ctx, const char* path) {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (self && path && path[0] != '\0') Audio::GetInstance()->LoadAudio(self->ResolveAssetPath(path));
	};
	engineApi_.playMusic = [](void* ctx, const char* path, float volume) {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || !path || path[0] == '\0') return;
		Audio* audio = Audio::GetInstance();
		const std::string resolved = self->ResolveAssetPath(path);
		const float v = std::isfinite(volume) ? std::clamp(volume, 0.0f, 4.0f) : 1.0f;
		if (self->musicVoice_ != 0 && self->musicPath_ == resolved && audio->IsPlaying(self->musicVoice_)) {
			audio->SetVolume(self->musicVoice_, v);
			return;
		}
		if (self->musicVoice_ != 0) audio->StopAudio(self->musicVoice_);
		const uint32_t sound = audio->LoadAudio(resolved);
		self->musicVoice_ = (sound != 0) ? audio->PlayAudio(sound, true, v) : 0u;
		self->musicPath_ = (self->musicVoice_ != 0) ? resolved : std::string();
	};
	engineApi_.stopMusic = [](void* ctx) {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self) return;
		if (self->musicVoice_ != 0) Audio::GetInstance()->StopAudio(self->musicVoice_);
		self->musicVoice_ = 0;
		self->musicPath_.clear();
	};

	// ---- Particles & timers ----
	engineApi_.emitParticles = [](void* ctx, uint32_t id, int count) {
		if (ctx && id != 0 && count > 0) static_cast<EditorSuite*>(ctx)->pendingParticleBursts_.push_back({ id, count, false });
	};
	engineApi_.setParticlesEmitting = [](void* ctx, uint32_t id, bool emitting) {
		if (!ctx) return;
		if (GameObject* obj = static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false)) {
			obj->particleEmitContinuously = emitting;
		}
	};
	engineApi_.despawnAfter = [](void* ctx, uint32_t id, float seconds) {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || id == 0 || std::isnan(seconds)) return;
		self->CancelDelayedDespawn(id);
		if (seconds <= 0.0f) {
			self->QueueDespawn(id);
			return;
		}
		self->delayedDespawns_.push_back({ id, seconds });
	};

	// ---- Material ----
	engineApi_.getMaterial = [](void* ctx, uint32_t id, ScriptMaterialData* out) -> bool {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasMesh || !out) return false;
		out->color = obj->materialColor;
		out->visible = obj->meshEnabled;
		out->lit = obj->isLit;
		out->lightingType = obj->lightingType;
		out->shininess = obj->shininess;
		out->uvOffset = obj->uvOffset;
		out->uvTiling = obj->uvTiling;
		return true;
	};
	engineApi_.setMaterial = [](void* ctx, uint32_t id, const ScriptMaterialData* in) -> bool {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasMesh || !in) return false;
		obj->materialColor = in->color;
		obj->meshEnabled = in->visible;
		obj->isLit = in->lit;
		obj->lightingType = std::clamp(in->lightingType, 0, 4);
		obj->shininess = (std::max)(0.0f, in->shininess);
		obj->uvOffset = in->uvOffset;
		obj->uvTiling = in->uvTiling;
		return true;
	};
	engineApi_.setTexture = [](void* ctx, uint32_t id, const char* texturePath) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		GameObject* obj = self ? self->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasMesh || !texturePath) return false;
		obj->texturePath = (texturePath[0] != '\0') ? texturePath : "Resources/Model/white.png";
		// LoadTexture caches by path, so switching back and forth is cheap
		obj->textureHandle = TextureManager::GetInstance()->LoadTexture(self->ResolveAssetPath(obj->texturePath));
		return true;
	};

	// ---- Queries ----
	engineApi_.raycast = [](void* ctx, Vector3 origin, Vector3 direction, float maxDistance, uint32_t layerMask, uint32_t ignoreId, ScriptRaycastHit* outHit) -> bool {
		if (!ctx || !outHit) return false;
		return static_cast<EditorSuite*>(ctx)->ScriptRaycast(origin, direction, maxDistance, layerMask, ignoreId, *outHit);
	};
	engineApi_.overlapSphere = [](void* ctx, Vector3 center, float radius, uint32_t layerMask, uint32_t* outIds, int maxCount) -> int {
		return ctx ? static_cast<EditorSuite*>(ctx)->ScriptOverlapSphere(center, radius, layerMask, outIds, maxCount) : 0;
	};
	engineApi_.findWithTag = [](void* ctx, const char* tag) -> ScriptGameObjectView* {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || !tag) return nullptr;
		const auto excluded = CollectQueryExclusions(self->gameObjects_, self->pendingDeletes_, self->pendingDespawns_);
		for (const auto& obj : self->gameObjects_) {
			if (obj.isActive && obj.tag == tag && excluded.count(obj.id) == 0) {
				return self->GetOrUpdateScriptViewById(obj.id);
			}
		}
		return nullptr;
	};
	engineApi_.findAllWithTag = [](void* ctx, const char* tag, uint32_t* outIds, int maxCount) -> int {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || !tag || !outIds || maxCount <= 0) return 0;
		const auto excluded = CollectQueryExclusions(self->gameObjects_, self->pendingDeletes_, self->pendingDespawns_);
		int count = 0;
		for (const auto& obj : self->gameObjects_) {
			if (count >= maxCount) break;
			if (obj.isActive && obj.tag == tag && excluded.count(obj.id) == 0) outIds[count++] = obj.id;
		}
		return count;
	};
	engineApi_.findNearest = [](void* ctx, Vector3 position, const char* tag, float maxDistance, uint32_t ignoreId) -> ScriptGameObjectView* {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self) return nullptr;
		const GameObject* nearest = self->FindNearestObject(position, tag, maxDistance, ignoreId);
		return nearest ? self->GetOrUpdateScriptViewById(nearest->id) : nullptr;
	};

	// ---- Hierarchy ----
	engineApi_.setParent = [](void* ctx, uint32_t childId, uint32_t parentId, bool keepWorldTransform) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || childId == 0 || childId == parentId) return false;
		const bool childKnown = self->FindGameObjectById(childId, false) || self->FindPendingSpawnView(childId);
		const bool parentKnown = parentId == 0 || self->FindGameObjectById(parentId, false) || self->FindPendingSpawnView(parentId);
		if (!childKnown || !parentKnown) return false;
		self->pendingReparents_.push_back({ childId, parentId == 0 ? -1 : static_cast<int>(parentId), keepWorldTransform });
		return true;
	};
	engineApi_.getParent = [](void* ctx, uint32_t id) -> uint32_t {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		return (obj && obj->parentId >= 0) ? static_cast<uint32_t>(obj->parentId) : 0u;
	};
	engineApi_.getChildren = [](void* ctx, uint32_t id, uint32_t* outIds, int maxCount) -> int {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !outIds || maxCount <= 0) return 0;
		const int count = (std::min)(maxCount, static_cast<int>(obj->childrenIds.size()));
		for (int i = 0; i < count; ++i) outIds[i] = obj->childrenIds[static_cast<size_t>(i)];
		return count;
	};
	engineApi_.getWorldPosition = [](void* ctx, uint32_t id, Vector3* out) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		if (!self || !out) return false;
		if (const GameObject* obj = self->FindGameObjectById(id, false)) {
			*out = self->GetWorldPositionOf(*obj);
			return true;
		}
		if (const ScriptGameObjectView* pending = self->FindPendingSpawnView(id)) {
			*out = pending->translation; // spawned objects start as roots
			return true;
		}
		return false;
	};

	// ---- Components ----
	engineApi_.getLight = [](void* ctx, uint32_t id, ScriptLightData* out) -> bool {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasLight || !out) return false;
		out->enabled = obj->lightEnabled;
		out->type = static_cast<int>(obj->lightType);
		out->color = obj->lightColor;
		out->intensity = obj->lightIntensity;
		out->range = obj->lightRange;
		out->spotAngle = obj->lightSpotAngle;
		return true;
	};
	engineApi_.setLight = [](void* ctx, uint32_t id, const ScriptLightData* in) -> bool {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasLight || !in) return false;
		obj->lightEnabled = in->enabled;
		obj->lightType = static_cast<GameObject::LightType>(std::clamp(in->type, 0, 2));
		obj->lightColor = in->color;
		obj->lightIntensity = (std::max)(0.0f, in->intensity);
		obj->lightRange = (std::max)(0.0f, in->range);
		obj->lightSpotAngle = std::clamp(in->spotAngle, 1.0f, 179.0f);
		return true;
	};
	engineApi_.getRigidbody = [](void* ctx, uint32_t id, ScriptRigidbodyData* out) -> bool {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasRigidbody || !out) return false;
		out->enabled = obj->rbEnabled;
		out->mass = obj->mass;
		out->drag = obj->drag;
		out->gravityScale = obj->gravityScale;
		out->useGravity = obj->useGravity;
		out->isKinematic = obj->isKinematic;
		out->bounciness = obj->bounciness;
		return true;
	};
	engineApi_.setRigidbody = [](void* ctx, uint32_t id, const ScriptRigidbodyData* in) -> bool {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasRigidbody || !in) return false;
		obj->rbEnabled = in->enabled;
		obj->mass = (std::max)(0.0001f, in->mass);
		obj->drag = (std::max)(0.0f, in->drag);
		obj->gravityScale = in->gravityScale;
		obj->useGravity = in->useGravity;
		obj->isKinematic = in->isKinematic;
		obj->bounciness = std::clamp(in->bounciness, 0.0f, 1.0f);
		return true;
	};
	engineApi_.getAnimation = [](void* ctx, uint32_t id, ScriptAnimationData* out) -> bool {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasMesh || !out) return false;
		out->clipIndex = obj->animationIndex;
		out->speed = obj->animationSpeed;
		out->loop = obj->animationLoop;
		out->playing = obj->animationPlaying;
		return true;
	};
	engineApi_.setAnimation = [](void* ctx, uint32_t id, const ScriptAnimationData* in) -> bool {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->hasMesh || !in) return false;
		obj->animationIndex = (std::max)(0, in->clipIndex);
		obj->animationSpeed = std::isfinite(in->speed) ? in->speed : 1.0f;
		obj->animationLoop = in->loop;
		obj->animationPlaying = in->playing;
		return true;
	};
	// Animator (API v4)
	engineApi_.animSetParam = [](void* ctx, uint32_t id, const char* name, float value) -> bool {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = name ? ScriptAnimator(ctx, id, c) : nullptr;
		return a && std::isfinite(value) && a->runtime.SetValue(*c, name, value);
	};
	engineApi_.animGetParam = [](void* ctx, uint32_t id, const char* name) -> float {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = name ? ScriptAnimator(ctx, id, c) : nullptr;
		return a ? a->runtime.GetValue(name) : 0.0f;
	};
	engineApi_.animResetTrigger = [](void* ctx, uint32_t id, const char* name) -> bool {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = name ? ScriptAnimator(ctx, id, c) : nullptr;
		if (!a || !c->FindParameter(name)) return false;
		a->runtime.ResetTrigger(name);
		return true;
	};
	engineApi_.animIsState = [](void* ctx, uint32_t id, const char* stateName) -> bool {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = stateName ? ScriptAnimator(ctx, id, c) : nullptr;
		const AnimState* s = a ? c->FindState(a->runtime.currentState) : nullptr;
		return s && s->name == stateName;
	};
	engineApi_.animGetState = [](void* ctx, uint32_t id, char* outName, int outSize) -> int {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = ScriptAnimator(ctx, id, c);
		if (!a) return -1;
		const AnimState* s = c->FindState(a->runtime.currentState);
		const std::string name = s ? s->name : std::string();
		if (outName && outSize > 0) strncpy_s(outName, static_cast<size_t>(outSize), name.c_str(), _TRUNCATE);
		return static_cast<int>(name.size());
	};
	engineApi_.animCrossFade = [](void* ctx, uint32_t id, const char* stateName, float seconds) -> bool {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = stateName ? ScriptAnimator(ctx, id, c) : nullptr;
		if (!a) return false;
		const AnimatorRuntime::Change change = a->runtime.ForceState(*c, stateName, std::isfinite(seconds) ? seconds : 0.0f);
		const AnimState* s = change.changed ? c->FindState(change.state) : nullptr;
		GameObject* obj = static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false);
		if (!s || !obj) return false;
		static_cast<EditorSuite*>(ctx)->ApplyAnimatorState(*obj, *a, *s, change.fadeSeconds);
		return true;
	};
	engineApi_.animGetNormalizedTime = [](void* ctx, uint32_t id) -> float {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !obj->editObject3d || !obj->editObject3d->GetModel()) return 0.0f;
		return obj->editObject3d->GetAnimationPlayer().NormalizedTime(obj->editObject3d->GetModel());
	};
	engineApi_.animIsInTransition = [](void* ctx, uint32_t id) -> bool {
		std::shared_ptr<AnimatorController> c;
		AnimatorComponent* a = ScriptAnimator(ctx, id, c);
		return a && a->runtime.InTransition();
	};
	// Input actions (API v5): the project's map (ProjectSettings/input.json or the defaults)
	engineApi_.isActionDown = [](void* ctx, const char* action) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		return self && action && self->GetInputActions().IsDown(action, self->InputSource());
	};
	engineApi_.isActionTriggered = [](void* ctx, const char* action) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		return self && action && self->GetInputActions().IsTriggered(action, self->InputSource());
	};
	engineApi_.isActionReleased = [](void* ctx, const char* action) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		return self && action && self->GetInputActions().IsReleased(action, self->InputSource());
	};
	engineApi_.getAxis = [](void* ctx, const char* action) -> float {
		auto* self = static_cast<EditorSuite*>(ctx);
		return (self && action) ? self->GetInputActions().GetAxis(action, self->InputSource()) : 0.0f;
	};
	// Game feel (API v5)
	engineApi_.shakeCamera = [](void* ctx, float strength, float seconds) {
		if (ctx) static_cast<EditorSuite*>(ctx)->ShakeCamera(strength, seconds);
	};
	engineApi_.hitstop = [](void* ctx, float seconds) {
		if (ctx) static_cast<EditorSuite*>(ctx)->Hitstop(seconds);
	};
	engineApi_.getCamera = [](void* ctx, uint32_t id, ScriptCameraData* out) -> bool {
		const GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !(obj->hasCamera || obj->type == GameObjectType::Camera) || !out) return false;
		out->fov = obj->fov;
		out->nearClip = obj->nearClip;
		out->farClip = obj->farClip;
		out->orthographic = obj->isOrthographic;
		return true;
	};
	engineApi_.setCamera = [](void* ctx, uint32_t id, const ScriptCameraData* in) -> bool {
		GameObject* obj = ctx ? static_cast<EditorSuite*>(ctx)->FindGameObjectById(id, false) : nullptr;
		if (!obj || !(obj->hasCamera || obj->type == GameObjectType::Camera) || !in) return false;
		obj->fov = std::clamp(in->fov, 1.0f, 179.0f);
		obj->nearClip = (std::max)(0.001f, in->nearClip);
		obj->farClip = (std::max)(obj->nearClip + 0.01f, in->farClip);
		obj->isOrthographic = in->orthographic;
		return true;
	};
}

// ---------------------------------------------------------------------------
// Game feel
// ---------------------------------------------------------------------------
void EditorSuite::ShakeCamera(float strength, float seconds)
{
	if (!std::isfinite(strength) || !std::isfinite(seconds) || strength <= 0.0f || seconds <= 0.0f) return;
	if (strength < CameraShakeStrength()) return; // a weak hit never cuts a strong shake short
	shakeStrength_ = strength;
	shakeDuration_ = seconds;
	shakeLeft_ = seconds;
}

void EditorSuite::Hitstop(float seconds)
{
	if (!std::isfinite(seconds) || seconds <= 0.0f) return;
	hitstopLeft_ = (std::max)(hitstopLeft_, seconds);
}

Vector3 EditorSuite::CameraShakeOffset() const
{
	const float a = CameraShakeStrength();
	if (a <= 0.0f) return { 0.0f, 0.0f, 0.0f };
	// Incommensurate sines: jittery like noise but deterministic (tests, replays)
	const float t = shakeTime_;
	return { a * std::sin(t * 47.0f), a * std::sin(t * 61.0f + 1.3f), 0.5f * a * std::sin(t * 53.0f + 2.1f) };
}
