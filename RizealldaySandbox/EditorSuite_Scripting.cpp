#include "EditorSuiteInternal.h"

void EditorSuite::UpdateScripts(float deltaTime)
{
	if (sceneState_ == EditorSceneState::Edit && !IsStandaloneMode()) {
		return;
	}

	RefreshAllScriptObjectViews();

	// Instantiate/Destroy from scripts are deferred, so gameObjects_ does not change size inside this loop.
	// Objects are still re-fetched by index after each callback instead of holding references across it.
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (!gameObjects_[i].isActive || gameObjects_[i].scripts.empty()) continue;

		// Skip objects already queued for deferred deletion (pendingDeletes_ stores indices)
		if (std::find(pendingDeletes_.begin(), pendingDeletes_.end(), static_cast<int>(i)) != pendingDeletes_.end()) {
			continue;
		}

		const uint32_t objId = gameObjects_[i].id;
		for (size_t s = 0; s < gameObjects_[i].scripts.size(); ++s) {
			ScriptBase* instance = gameObjects_[i].scripts[s].instance.get();
			if (instance) {
				ScriptGameObjectView* view = GetOrUpdateScriptViewById(objId);
				Vector3 prevEuler = view ? view->rotation : gameObjects_[i].rotation;
				Quaternion prevQuat = view ? view->rotationQuat : gameObjects_[i].rotationQuat;

				instance->SetOwner(view, objId);
				instance->SetEngineAPI(&engineApi_);
				instance->SetHost(this, this);
				instance->Update(deltaTime);

				if (view) {
					bool eulerChanged = (view->rotation.x != prevEuler.x || view->rotation.y != prevEuler.y || view->rotation.z != prevEuler.z);
					bool quatChanged = (view->rotationQuat.x != prevQuat.x || view->rotationQuat.y != prevQuat.y || view->rotationQuat.z != prevQuat.z || view->rotationQuat.w != prevQuat.w);
					if (eulerChanged && !quatChanged) {
						view->rotationQuat = Quaternion::FromEuler(view->rotation);
					} else if (quatChanged && !eulerChanged) {
						view->rotation = view->rotationQuat.ToEuler();
					}
					SyncViewToGameObject(*view, gameObjects_[i]);
				}
			}
		}
	}

	// Create objects instantiated during Update() now that no script callback is running
	ProcessPendingSpawns();
}

void EditorSuite::DispatchScriptCollision(uint32_t selfId, uint32_t otherId)
{
	if (sceneState_ == EditorSceneState::Edit && !IsStandaloneMode()) {
		return;
	}

	GameObject* selfObj = FindGameObjectById(selfId, false);
	GameObject* otherObj = FindGameObjectById(otherId, false);
	if (!selfObj || !otherObj || !selfObj->isActive || !otherObj->isActive) {
		return;
	}
	if (selfObj->scripts.empty()) {
		return;
	}

	ScriptGameObjectView* selfView = GetOrUpdateScriptViewById(selfId);
	ScriptGameObjectView* otherView = GetOrUpdateScriptViewById(otherId);
	if (!selfView || !otherView) {
		return;
	}

	for (size_t s = 0; s < selfObj->scripts.size(); ++s) {
		auto& sc = selfObj->scripts[s];
		if (sc.instance) {
			Vector3 prevEuler = selfView->rotation;
			Quaternion prevQuat = selfView->rotationQuat;

			sc.instance->SetOwner(selfView, selfObj->id);
			sc.instance->SetEngineAPI(&engineApi_);
			sc.instance->SetHost(this, this);
			// Inside the DLL GameObject is an alias of ScriptGameObjectView, so the DLL receives the view.
			// Never pass an editor GameObject here.
			sc.instance->OnCollision(*reinterpret_cast<GameObject*>(otherView));

			bool eulerChanged = (selfView->rotation.x != prevEuler.x || selfView->rotation.y != prevEuler.y || selfView->rotation.z != prevEuler.z);
			bool quatChanged = (selfView->rotationQuat.x != prevQuat.x || selfView->rotationQuat.y != prevQuat.y || selfView->rotationQuat.z != prevQuat.z || selfView->rotationQuat.w != prevQuat.w);
			if (eulerChanged && !quatChanged) {
				selfView->rotationQuat = Quaternion::FromEuler(selfView->rotation);
			} else if (quatChanged && !eulerChanged) {
				selfView->rotation = selfView->rotationQuat.ToEuler();
			}
			SyncViewToGameObject(*selfView, *selfObj);
			SyncViewToGameObject(*otherView, *otherObj);
		}
	}
}

void EditorSuite::SetupEngineAPI()
{
	engineApi_.hostContext = this;
	engineApi_.log = [](void* ctx, const char* msg, int logType) {
		if (ctx && msg) {
			static_cast<EditorSuite*>(ctx)->LogMessage(msg, logType);
		}
	};
	engineApi_.findObjectByName = [](void* ctx, const char* name, bool warnIfMissing) -> ScriptGameObjectView* {
		if (!ctx || !name) return nullptr;
		auto* self = static_cast<EditorSuite*>(ctx);
		if (GameObject* found = self->FindGameObjectByName(name, false)) {
			return self->GetOrUpdateScriptViewById(found->id);
		}
		for (const auto& spawn : self->pendingSpawns_) {
			if (spawn.name == name) {
				return self->FindPendingSpawnView(spawn.id);
			}
		}
		if (warnIfMissing) {
			self->FindGameObjectByName(name, true); // logs the missing-object warning
		}
		return nullptr;
	};
	engineApi_.findObjectById = [](void* ctx, uint32_t id, bool warnIfMissing) -> ScriptGameObjectView* {
		if (!ctx || id == 0) return nullptr;
		auto* self = static_cast<EditorSuite*>(ctx);
		if (ScriptGameObjectView* view = self->GetOrUpdateScriptViewById(id)) {
			return view;
		}
		if (warnIfMissing) {
			self->FindGameObjectById(id, true); // logs the missing-object warning
		}
		return nullptr;
	};
	engineApi_.getTransform = [](void* ctx, uint32_t id, ScriptTransformData* outTransform) -> bool {
		if (!ctx || id == 0 || !outTransform) return false;
		auto* self = static_cast<EditorSuite*>(ctx);
		GameObject* found = self->FindGameObjectById(id, false);
		if (!found) {
			const ScriptGameObjectView* pending = self->FindPendingSpawnView(id);
			if (!pending) return false;
			outTransform->translation = pending->translation;
			outTransform->rotation = pending->rotation;
			outTransform->rotationQuat = pending->rotationQuat;
			outTransform->scale = pending->scale;
			return true;
		}
		outTransform->translation = found->translation;
		outTransform->rotation = found->rotation;
		outTransform->rotationQuat = found->rotationQuat;
		outTransform->scale = found->scale;
		return true;
	};
	engineApi_.setTransform = [](void* ctx, uint32_t id, const ScriptTransformData* inTransform) -> bool {
		if (!ctx || id == 0 || !inTransform) return false;
		auto* self = static_cast<EditorSuite*>(ctx);
		GameObject* found = self->FindGameObjectById(id, false);
		if (!found) {
			// Not created yet: ProcessPendingSpawns() applies the view transform on creation
			ScriptGameObjectView* pending = self->FindPendingSpawnView(id);
			if (!pending) return false;
			pending->translation = inTransform->translation;
			pending->rotation = inTransform->rotation;
			pending->rotationQuat = inTransform->rotationQuat;
			pending->scale = inTransform->scale;
			return true;
		}
		found->translation = inTransform->translation;
		found->rotation = inTransform->rotation;
		found->rotationQuat = inTransform->rotationQuat;
		found->scale = inTransform->scale;
		if (ScriptGameObjectView* view = self->GetOrUpdateScriptViewById(id)) {
			view->translation = inTransform->translation;
			view->rotation = inTransform->rotation;
			view->rotationQuat = inTransform->rotationQuat;
			view->scale = inTransform->scale;
		}
		return true;
	};
	engineApi_.deleteObject = [](void* ctx, uint32_t id) {
		if (ctx && id != 0) {
			auto* self = static_cast<EditorSuite*>(ctx);
			// An object instantiated this frame has not been created yet: just drop the request
			if (!self->CancelPendingSpawn(id)) {
				self->QueueDeleteGameObjectById(id);
			}
		}
	};
	engineApi_.isKeyDown = [](void* ctx, int keyCode) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		return (self && self->currentInput_) ? self->currentInput_->PushKey(static_cast<BYTE>(keyCode & 0xFF)) : false;
	};
	engineApi_.isKeyTriggered = [](void* ctx, int keyCode) -> bool {
		auto* self = static_cast<EditorSuite*>(ctx);
		return (self && self->currentInput_) ? self->currentInput_->TriggerKey(static_cast<BYTE>(keyCode & 0xFF)) : false;
	};
	engineApi_.getDeltaTime = [](void* ctx) -> float {
		auto* self = static_cast<EditorSuite*>(ctx);
		return self ? self->currentDeltaTime_ : 0.016f;
	};
	// Deferred: the GameObject is created by ProcessPendingSpawns() after the current script callback returns.
	// The returned view is valid immediately and keeps its address after the object is created.
	engineApi_.instantiate = [](void* ctx, const char* name, int primitiveType, Vector3 position) -> ScriptGameObjectView* {
		if (!ctx) return nullptr;
		return static_cast<EditorSuite*>(ctx)->QueueScriptSpawn(name, primitiveType, position);
	};
}

void EditorSuite::SyncGameObjectToView(const GameObject& src, ScriptGameObjectView& dst)
{
	dst.id = src.id;
	strncpy_s(dst.name, src.name.c_str(), sizeof(dst.name) - 1);
	strncpy_s(dst.tag, src.tag.c_str(), sizeof(dst.tag) - 1);
	dst.isActive = src.isActive;
	dst.isStatic = src.isStatic;
	dst.layer = src.layer;
	dst.translation = src.translation;
	dst.rotation = src.rotation;
	dst.scale = src.scale;
	dst.rotationQuat = src.rotationQuat;
	dst.velocity = src.velocity;
	dst.isGrounded = src.isGrounded;
	dst.isColliding = src.isColliding;
	dst.dirtyFromScript = false;
}

void EditorSuite::SyncViewToGameObject(const ScriptGameObjectView& src, GameObject& dst)
{
	dst.isActive = src.isActive;
	dst.isStatic = src.isStatic;
	dst.layer = src.layer;
	dst.translation = src.translation;
	dst.rotation = src.rotation;
	dst.scale = src.scale;
	dst.rotationQuat = src.rotationQuat;
	dst.velocity = src.velocity;
	dst.isGrounded = src.isGrounded;
}

void EditorSuite::RefreshAllScriptObjectViews()
{
	std::unordered_set<uint32_t> liveIds;
	liveIds.reserve(gameObjects_.size() + pendingSpawns_.size());
	for (const auto& obj : gameObjects_) {
		auto& view = scriptObjectViews_[obj.id];
		if (!view) {
			view = std::make_unique<ScriptGameObjectView>();
		}
		SyncGameObjectToView(obj, *view);
		liveIds.insert(obj.id);
	}
	for (const auto& spawn : pendingSpawns_) {
		liveIds.insert(spawn.id);
	}

	// Retire views whose GameObject no longer exists (scripts may still hold the pointer)
	std::vector<uint32_t> staleIds;
	for (const auto& [id, view] : scriptObjectViews_) {
		if (liveIds.count(id) == 0) {
			staleIds.push_back(id);
		}
	}
	for (uint32_t id : staleIds) {
		RetireScriptView(id);
	}
}

ScriptGameObjectView* EditorSuite::GetOrUpdateScriptViewById(uint32_t id)
{
	for (const auto& obj : gameObjects_) {
		if (obj.id == id) {
			auto& view = scriptObjectViews_[id];
			if (!view) {
				view = std::make_unique<ScriptGameObjectView>();
			}
			SyncGameObjectToView(obj, *view);
			return view.get();
		}
	}
	return FindPendingSpawnView(id);
}

ScriptGameObjectView* EditorSuite::FindPendingSpawnView(uint32_t id)
{
	for (const auto& spawn : pendingSpawns_) {
		if (spawn.id == id) {
			auto it = scriptObjectViews_.find(id);
			return (it != scriptObjectViews_.end()) ? it->second.get() : nullptr;
		}
	}
	return nullptr;
}

ScriptGameObjectView* EditorSuite::QueueScriptSpawn(const char* name, int primitiveType, const Vector3& position)
{
	PendingSpawn spawn{};
	spawn.id = nextObjectId_++;
	spawn.name = (name && name[0] != '\0') ? name : "InstantiatedObject";
	spawn.primitiveType = primitiveType;

	// The view exists immediately so the script can move/scale the object before it is created
	auto view = std::make_unique<ScriptGameObjectView>();
	view->id = spawn.id;
	strncpy_s(view->name, spawn.name.c_str(), sizeof(view->name) - 1);
	view->translation = position;
	view->rotationQuat = Quaternion::Identity();
	view->scale = (primitiveType == 2) ? Vector3{ 10.0f, 1.0f, 10.0f } : Vector3{ 1.0f, 1.0f, 1.0f };

	ScriptGameObjectView* rawView = view.get();
	scriptObjectViews_[spawn.id] = std::move(view);
	pendingSpawns_.push_back(std::move(spawn));
	return rawView;
}

bool EditorSuite::CancelPendingSpawn(uint32_t id)
{
	auto it = std::find_if(pendingSpawns_.begin(), pendingSpawns_.end(),
		[id](const PendingSpawn& spawn) { return spawn.id == id; });
	if (it == pendingSpawns_.end()) {
		return false;
	}
	pendingSpawns_.erase(it);
	RetireScriptView(id);
	return true;
}

void EditorSuite::ProcessPendingSpawns()
{
	if (pendingSpawns_.empty()) {
		return;
	}

	std::vector<PendingSpawn> spawns;
	spawns.swap(pendingSpawns_);

	isSpawningFromScript_ = true;
	gameObjects_.reserve(gameObjects_.size() + spawns.size());
	for (const auto& spawn : spawns) {
		auto viewIt = scriptObjectViews_.find(spawn.id);
		ScriptGameObjectView* view = (viewIt != scriptObjectViews_.end()) ? viewIt->second.get() : nullptr;
		const Vector3 position = view ? view->translation : Vector3{ 0.0f, 0.0f, 0.0f };

		GameObject* created = nullptr;
		switch (spawn.primitiveType) {
		case 0: created = &CreateCube(spawn.name, position); break;
		case 1: created = &CreateSphere(spawn.name, position); break;
		case 2: created = &CreatePlane(spawn.name, position); break;
		case 3: created = &CreateQuad(spawn.name, position); break;
		default:
			created = &CreateGameObject(spawn.name, GameObjectType::Empty);
			created->translation = position;
			break;
		}

		// Keep the id that was already handed to the script
		created->id = spawn.id;
		if (view) {
			SyncViewToGameObject(*view, *created);
		}
	}
	isSpawningFromScript_ = false;

	RefreshAllScriptObjectViews();
}

void EditorSuite::RetireScriptView(uint32_t id)
{
	auto it = scriptObjectViews_.find(id);
	if (it == scriptObjectViews_.end()) {
		return;
	}
	if (it->second) {
		it->second->isActive = false;
		retiredScriptViews_.push_back(std::move(it->second));
	}
	scriptObjectViews_.erase(it);

	// Bound memory in long-running standalone games: drop the oldest half once the cap is reached
	if (retiredScriptViews_.size() > kMaxRetiredScriptViews) {
		const size_t dropCount = retiredScriptViews_.size() / 2;
		retiredScriptViews_.erase(retiredScriptViews_.begin(), retiredScriptViews_.begin() + static_cast<std::ptrdiff_t>(dropCount));
	}
}

void EditorSuite::ClearRetiredScriptViews()
{
	retiredScriptViews_.clear();
}

void EditorSuite::CaptureScriptFieldsToSaved(GameObject::ScriptComponent& sc)
{
	if (!sc.instance || !scriptModule_.IsLoaded()) return;

	int fieldCount = scriptModule_.GetFieldCount(sc.instance.get());
	for (int f = 0; f < fieldCount; ++f) {
		ScriptFieldInfo info{};
		if (!scriptModule_.GetFieldInfo(sc.instance.get(), f, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;

		GameObject::ScriptSavedFieldValue saved{};
		saved.type = info.type;
		switch (info.type) {
		case ScriptFieldType::Float:
			saved.floatVal = *static_cast<const float*>(info.dataPtr);
			break;
		case ScriptFieldType::Int:
			saved.intVal = *static_cast<const int*>(info.dataPtr);
			break;
		case ScriptFieldType::Bool:
			saved.boolVal = *static_cast<const bool*>(info.dataPtr);
			break;
		case ScriptFieldType::Vec3:
			saved.vec3Val = *static_cast<const Vector3*>(info.dataPtr);
			break;
		case ScriptFieldType::CharBuffer:
			saved.strVal = static_cast<const char*>(info.dataPtr);
			break;
		}
		sc.savedFields[info.name] = saved;
	}
}

void EditorSuite::ApplySavedFieldsToScript(GameObject::ScriptComponent& sc)
{
	if (!sc.instance || !scriptModule_.IsLoaded() || sc.savedFields.empty()) return;

	int fieldCount = scriptModule_.GetFieldCount(sc.instance.get());
	for (int f = 0; f < fieldCount; ++f) {
		ScriptFieldInfo info{};
		if (!scriptModule_.GetFieldInfo(sc.instance.get(), f, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;

		auto it = sc.savedFields.find(info.name);
		if (it != sc.savedFields.end() && it->second.type == info.type) {
			switch (info.type) {
			case ScriptFieldType::Float:
				*static_cast<float*>(info.dataPtr) = it->second.floatVal;
				break;
			case ScriptFieldType::Int:
				*static_cast<int*>(info.dataPtr) = it->second.intVal;
				break;
			case ScriptFieldType::Bool:
				*static_cast<bool*>(info.dataPtr) = it->second.boolVal;
				break;
			case ScriptFieldType::Vec3:
				*static_cast<Vector3*>(info.dataPtr) = it->second.vec3Val;
				break;
			case ScriptFieldType::CharBuffer:
				if (info.sizeInBytes > 0) {
					strncpy_s(static_cast<char*>(info.dataPtr), info.sizeInBytes, it->second.strVal.c_str(), info.sizeInBytes - 1);
				}
				break;
			}
		}
	}
}

std::string EditorSuite::SerializeAllScriptFieldsToJson()
{
	std::ostringstream ss;
	ss << "{\n  \"scriptComponents\": [\n";
	bool firstEntry = true;
	for (auto& obj : gameObjects_) {
		for (size_t s = 0; s < obj.scripts.size(); ++s) {
			auto& sc = obj.scripts[s];
			CaptureScriptFieldsToSaved(sc);
			if (!firstEntry) ss << ",\n";
			firstEntry = false;
			ss << "    { \"objectId\": " << obj.id
			   << ", \"componentIndex\": " << s
			   << ", \"className\": \"" << sc.className << "\""
			   << ", \"fields\": [";
			bool firstField = true;
			for (const auto& [fName, fVal] : sc.savedFields) {
				if (!firstField) ss << ", ";
				firstField = false;
				ss << "{ \"name\": \"" << fName << "\", \"type\": " << static_cast<int>(fVal.type);
				switch (fVal.type) {
				case ScriptFieldType::Float:
					ss << ", \"v0\": " << fVal.floatVal << " }";
					break;
				case ScriptFieldType::Int:
					ss << ", \"v0\": " << fVal.intVal << " }";
					break;
				case ScriptFieldType::Bool:
					ss << ", \"v0\": " << (fVal.boolVal ? 1 : 0) << " }";
					break;
				case ScriptFieldType::Vec3:
					ss << ", \"v0\": " << fVal.vec3Val.x << ", \"v1\": " << fVal.vec3Val.y << ", \"v2\": " << fVal.vec3Val.z << " }";
					break;
				case ScriptFieldType::CharBuffer:
					ss << ", \"str\": \"" << fVal.strVal << "\" }";
					break;
				}
			}
			ss << "] }";
		}
	}
	ss << "\n  ]\n}\n";
	return ss.str();
}

void EditorSuite::DeserializeAllScriptFieldsFromJson(const std::string& json)
{
	(void)json;
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			ApplySavedFieldsToScript(sc);
		}
	}
}

void EditorSuite::DestroyAllScriptInstances()
{
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				CaptureScriptFieldsToSaved(sc);
				sc.instance.reset();
			}
		}
	}
}

void EditorSuite::EnsureEditModeScriptInstances()
{
	if (sceneState_ != EditorSceneState::Edit) return;

	RefreshAllScriptObjectViews();
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			if (sc.className.empty()) continue;

			if (scriptModule_.IsLoaded() && scriptModule_.IsRegistered(sc.className)) {
				sc.isMissingScript = false;
				sc.isCompiling = false;
				if (!sc.instance) {
					sc.instance = ScriptInstancePtr(scriptModule_.CreateScript(sc.className), ScriptInstanceDeleter{ &scriptModule_ });
					if (sc.instance) {
						sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
						sc.instance->SetEngineAPI(&engineApi_);
						sc.instance->SetHost(this, this);
						ApplySavedFieldsToScript(sc);
						CaptureScriptFieldsToSaved(sc);
					}
				}
			} else if (!sc.isCompiling) {
				sc.instance.reset();
				sc.isMissingScript = true;
			}
		}
	}
}

void EditorSuite::HotReloadScripts()
{
	// 1. Serialize exposed fields of every script component in the scene (tagged by GameObject ID, component index, and class name)
	std::string savedJson = SerializeAllScriptFieldsToJson();

	// 2. Destroy all ScriptBase instances before unloading the old shadow DLL.
	//    Note: OnDestroy() is intentionally NOT called during hot reload because the GameObject is not being destroyed.
	DestroyAllScriptInstances();

	// 3. Unload old shadow DLL, copy new GameScripts.dll to a new shadow file, LoadLibrary, and resolve exports
	std::string reloadError;
	if (!scriptModule_.LoadShadowCopy(&reloadError)) {
		LogMessage(std::format("[Scripts] Hot reload failed: {}", reloadError), 2);
		return;
	}

	RefreshAllScriptObjectViews();

	// 4. Recreate each script component instance by class name via the new DLL's CreateScript
	size_t reloadedInstanceCount = 0;
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			sc.instance.reset();
			sc.isCompiling = false;
			if (sc.className.empty()) continue;

			if (scriptModule_.IsRegistered(sc.className)) {
				sc.isMissingScript = false;
				sc.instance = ScriptInstancePtr(scriptModule_.CreateScript(sc.className), ScriptInstanceDeleter{ &scriptModule_ });
				if (sc.instance) {
					sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					reloadedInstanceCount++;
				}
			} else {
				sc.isMissingScript = true;
				LogMessage(std::format("[Scripts] Warning: Class '{}' on '{}' is no longer in GameScripts.dll (marked Missing Script).", sc.className, obj.name), 1);
			}
		}
	}

	// 5. Restore serialized field values by matching field name and type
	DeserializeAllScriptFieldsFromJson(savedJson);

	// 6. In Play mode, call OnAfterReload() instead of Initialize() so one-shot setup does not repeat
	if (sceneState_ != EditorSceneState::Edit) {
		for (auto& obj : gameObjects_) {
			if (!obj.isActive) continue;
			for (auto& sc : obj.scripts) {
				if (sc.instance) {
					ScriptGameObjectView* view = GetOrUpdateScriptViewById(obj.id);
					sc.instance->SetOwner(view, obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					sc.instance->OnAfterReload();
					if (view) {
						SyncViewToGameObject(*view, obj);
					}
				}
			}
		}
	}

	LogMessage(std::format("[Scripts] Reloaded {} ({} registered classes, {} active instances).",
		scriptModule_.GetActiveShadowPath().filename().string(),
		scriptModule_.GetRegisteredNames().size(),
		reloadedInstanceCount), 0);
}

