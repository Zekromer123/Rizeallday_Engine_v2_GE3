#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"

#ifdef GAMESCRIPTS_EXPORTS
#define GAMESCRIPTS_API extern "C" __declspec(dllexport)
#else
#define GAMESCRIPTS_API extern "C" __declspec(dllimport)
#endif

/// Plain C-compatible GameObject view structure used across the DLL boundary.
/// Contains no std::string, std::vector, or DirectX 12 resources.
struct ScriptGameObjectView {
	uint32_t id = 0;
	char name[128] = "GameObject";
	char tag[64] = "Untagged";
	bool isActive = true;
	bool isStatic = false;
	uint32_t layer = 0;

	Vector3 translation = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
	Quaternion rotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 velocity = { 0.0f, 0.0f, 0.0f };

	bool isGrounded = false;
	bool isColliding = false;
	bool dirtyFromScript = false;
};

#ifdef GAMESCRIPTS_EXPORTS
using GameObject = ScriptGameObjectView;
#else
struct GameObject;
#endif

/// Plain transform data passed across the EngineAPI function-pointer table.
struct ScriptTransformData {
	Vector3 translation = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
	Quaternion rotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
};

/// Function-pointer table passed from the Engine to GameScripts.dll on load.
struct EngineAPI {
	void* hostContext = nullptr;

	// Object lookup (returns plain ScriptGameObjectView*; nullptr if missing/deleted)
	ScriptGameObjectView* (*findObjectById)(void* ctx, uint32_t id, bool warnIfMissing) = nullptr;
	ScriptGameObjectView* (*findObjectByName)(void* ctx, const char* name, bool warnIfMissing) = nullptr;

	// Transform accessors
	bool (*getTransform)(void* ctx, uint32_t id, ScriptTransformData* outTransform) = nullptr;
	bool (*setTransform)(void* ctx, uint32_t id, const ScriptTransformData* inTransform) = nullptr;

	// Console logging (0: Info, 1: Warning, 2: Error)
	void (*log)(void* ctx, const char* message, int logType) = nullptr;

	// Deferred object deletion
	void (*deleteObject)(void* ctx, uint32_t id) = nullptr;

	// Input queries (DIK_* key code or uppercase ASCII character such as 'W', 'A', 'S', 'D', ' ')
	bool (*isKeyDown)(void* ctx, int keyCode) = nullptr;
	bool (*isKeyTriggered)(void* ctx, int keyCode) = nullptr;

	// Frame delta time
	float (*getDeltaTime)(void* ctx) = nullptr;

	// Instantiate a new GameObject in the scene (primitiveType: 0=Cube, 1=Sphere, 2=Plane, 3=Quad, 4=Empty)
	ScriptGameObjectView* (*instantiate)(void* ctx, const char* name, int primitiveType, Vector3 position) = nullptr;
};

/// Supported exposed field types for Inspector reflection and hot-reload serialization.
enum class ScriptFieldType : int32_t {
	Float = 0,
	Int = 1,
	Bool = 2,
	Vector3 = 3,
	Vec3 = 3,
	CharBuffer = 4
};

/// Plain metadata descriptor for a single EXPOSE_FIELD member.
struct ScriptFieldInfo {
	const char* name = "";
	ScriptFieldType type = ScriptFieldType::Float;
	void* dataPtr = nullptr;
	uint32_t sizeInBytes = 0;
};

/// Convenience alias for fixed-size text buffers in EXPOSE_FIELD.
using ScriptText = char[128];

class EditorSuite;
class IScriptHost {
public:
	virtual ~IScriptHost() = default;
	virtual GameObject* FindGameObjectById(uint32_t id, bool warnIfMissing = true) = 0;
	virtual GameObject* FindGameObjectByName(const char* name, bool warnIfMissing = true) = 0;
	virtual void QueueDeleteGameObjectById(uint32_t id) = 0;
	virtual void LogScriptMessage(const char* msg, int logType = 0) = 0;
};

/// Base class for all C++ GameObject scripts in GameScripts.dll.
/// Never holds std::string or std::vector members so layout is 100% ABI-independent across DLL boundaries.
class ScriptBase {
public:
	static constexpr int kMaxExposedFields = 64;

	ScriptBase() = default;
	virtual ~ScriptBase() = default;

	virtual void Initialize() {}
	virtual void Update(float deltaTime) { (void)deltaTime; }
	virtual void OnCollision(GameObject& other) { (void)other; }
	virtual void OnDestroy() {}
	virtual void OnAfterReload() {}

	void SetOwner(GameObject* ownerObj, uint32_t id = 0) {
		gameObject = ownerObj;
		owner = ownerObj;
		gameObject_ = ownerObj;
		if (id != 0 || !ownerObj) {
			ownerId_ = id;
		}
	}

#ifndef GAMESCRIPTS_EXPORTS
	void SetOwner(ScriptGameObjectView* ownerView, uint32_t id = 0) {
		SetOwner(reinterpret_cast<GameObject*>(ownerView), id);
	}
#endif

	GameObject* GetOwner() const { return gameObject_; }
	GameObject* GetGameObject() const { return gameObject_; }
	uint32_t GetOwnerId() const { return ownerId_; }

	void SetEngineAPI(const EngineAPI* api) { engineApi_ = api; }
	const EngineAPI* GetEngineAPI() const { return engineApi_; }

	void SetHost(IScriptHost* host, EditorSuite* suite = nullptr) {
		host_ = host;
		editorSuite_ = suite;
	}
	IScriptHost* GetHost() const { return host_; }
	EditorSuite* GetEditorSuite() const { return editorSuite_; }

	// Exposed field registration (called automatically by EXPOSE_FIELD)
	void RegisterExposedField(const char* fieldName, ScriptFieldType type, void* dataPtr, uint32_t sizeInBytes) {
		if (!fieldName || !dataPtr || exposedFieldCount_ >= kMaxExposedFields) {
			return;
		}
		exposedFields_[exposedFieldCount_].name = fieldName;
		exposedFields_[exposedFieldCount_].type = type;
		exposedFields_[exposedFieldCount_].dataPtr = dataPtr;
		exposedFields_[exposedFieldCount_].sizeInBytes = sizeInBytes;
		++exposedFieldCount_;
	}

	int GetExposedFieldCount() const { return exposedFieldCount_; }
	const ScriptFieldInfo* GetExposedField(int index) const {
		if (index < 0 || index >= exposedFieldCount_) return nullptr;
		return &exposedFields_[index];
	}

	// EngineAPI convenience helpers for scripts
	GameObject* FindGameObjectById(uint32_t id) const {
		if (engineApi_ && engineApi_->findObjectById) {
			return reinterpret_cast<GameObject*>(engineApi_->findObjectById(engineApi_->hostContext, id, true));
		}
		if (host_) {
			return host_->FindGameObjectById(id, true);
		}
		return nullptr;
	}

	GameObject* FindGameObjectByName(const char* name) const {
		if (!name) return nullptr;
		if (engineApi_ && engineApi_->findObjectByName) {
			return reinterpret_cast<GameObject*>(engineApi_->findObjectByName(engineApi_->hostContext, name, true));
		}
		if (host_) {
			return host_->FindGameObjectByName(name, true);
		}
		return nullptr;
	}

	bool GetTransform(uint32_t id, ScriptTransformData& outTransform) const {
		if (engineApi_ && engineApi_->getTransform) {
			return engineApi_->getTransform(engineApi_->hostContext, id, &outTransform);
		}
		return false;
	}

	bool SetTransform(uint32_t id, const ScriptTransformData& inTransform) const {
		if (engineApi_ && engineApi_->setTransform) {
			return engineApi_->setTransform(engineApi_->hostContext, id, &inTransform);
		}
		return false;
	}

	Vector3 GetPosition() const {
		if (gameObject_) {
			const auto* view = reinterpret_cast<const ScriptGameObjectView*>(gameObject_);
			return view->translation;
		}
		ScriptTransformData t{};
		GetTransform(ownerId_, t);
		return t.translation;
	}

	void SetPosition(const Vector3& pos) {
		if (gameObject_) {
			auto* view = reinterpret_cast<ScriptGameObjectView*>(gameObject_);
			view->translation = pos;
			view->dirtyFromScript = true;
		}
		ScriptTransformData t{};
		if (GetTransform(ownerId_, t)) {
			t.translation = pos;
			SetTransform(ownerId_, t);
		}
	}

	Vector3 GetRotation() const {
		if (gameObject_) {
			const auto* view = reinterpret_cast<const ScriptGameObjectView*>(gameObject_);
			return view->rotation;
		}
		ScriptTransformData t{};
		GetTransform(ownerId_, t);
		return t.rotation;
	}

	void SetRotation(const Vector3& euler) {
		if (gameObject_) {
			auto* view = reinterpret_cast<ScriptGameObjectView*>(gameObject_);
			view->rotation = euler;
			view->rotationQuat = Quaternion::FromEuler(euler);
			view->dirtyFromScript = true;
		}
		ScriptTransformData t{};
		if (GetTransform(ownerId_, t)) {
			t.rotation = euler;
			t.rotationQuat = Quaternion::FromEuler(euler);
			SetTransform(ownerId_, t);
		}
	}

	Vector3 GetScale() const {
		if (gameObject_) {
			const auto* view = reinterpret_cast<const ScriptGameObjectView*>(gameObject_);
			return view->scale;
		}
		ScriptTransformData t{};
		GetTransform(ownerId_, t);
		return t.scale;
	}

	void SetScale(const Vector3& s) {
		if (gameObject_) {
			auto* view = reinterpret_cast<ScriptGameObjectView*>(gameObject_);
			view->scale = s;
			view->dirtyFromScript = true;
		}
		ScriptTransformData t{};
		if (GetTransform(ownerId_, t)) {
			t.scale = s;
			SetTransform(ownerId_, t);
		}
	}

	void RequestDeleteGameObject(uint32_t id) {
		if (engineApi_ && engineApi_->deleteObject) {
			engineApi_->deleteObject(engineApi_->hostContext, id);
		} else if (host_) {
			host_->QueueDeleteGameObjectById(id);
		}
	}
	void DestroyGameObject(uint32_t id) { RequestDeleteGameObject(id); }

	void RequestDeleteSelf() {
		if (ownerId_ != 0) {
			RequestDeleteGameObject(ownerId_);
		}
	}
	void DestroySelf() { RequestDeleteSelf(); }

	void Log(const char* message, int logType = 0) const {
		if (!message) return;
		if (engineApi_ && engineApi_->log) {
			engineApi_->log(engineApi_->hostContext, message, logType);
		} else if (host_) {
			host_->LogScriptMessage(message, logType);
		}
	}

	bool IsKeyDown(int keyCode) const {
		if (engineApi_ && engineApi_->isKeyDown) {
			return engineApi_->isKeyDown(engineApi_->hostContext, keyCode);
		}
		return false;
	}

	bool IsKeyTriggered(int keyCode) const {
		if (engineApi_ && engineApi_->isKeyTriggered) {
			return engineApi_->isKeyTriggered(engineApi_->hostContext, keyCode);
		}
		return false;
	}

	float GetDeltaTime() const {
		if (engineApi_ && engineApi_->getDeltaTime) {
			return engineApi_->getDeltaTime(engineApi_->hostContext);
		}
		return 0.016f;
	}

	GameObject* Instantiate(const char* name, int primitiveType = 0, Vector3 position = { 0.0f, 0.0f, 0.0f }) const {
		if (engineApi_ && engineApi_->instantiate) {
			return reinterpret_cast<GameObject*>(engineApi_->instantiate(engineApi_->hostContext, name ? name : "InstantiatedObject", primitiveType, position));
		}
		return nullptr;
	}

	// Pointer to owner GameObject view (synchronized by the engine)
	GameObject* gameObject = nullptr;
	GameObject* owner = nullptr;

protected:
	GameObject* gameObject_ = nullptr;
	uint32_t ownerId_ = 0;
	const EngineAPI* engineApi_ = nullptr;
	IScriptHost* host_ = nullptr;
	EditorSuite* editorSuite_ = nullptr;

private:
	ScriptFieldInfo exposedFields_[kMaxExposedFields]{};
	int exposedFieldCount_ = 0;
};

/// Helper struct used by EXPOSE_FIELD to register a member field at ScriptBase construction time.
struct ScriptFieldRegistrar {
	ScriptFieldRegistrar(ScriptBase* self, const char* name, float* ptr) {
		if (self) self->RegisterExposedField(name, ScriptFieldType::Float, ptr, static_cast<uint32_t>(sizeof(float)));
	}
	ScriptFieldRegistrar(ScriptBase* self, const char* name, int* ptr) {
		if (self) self->RegisterExposedField(name, ScriptFieldType::Int, ptr, static_cast<uint32_t>(sizeof(int)));
	}
	ScriptFieldRegistrar(ScriptBase* self, const char* name, bool* ptr) {
		if (self) self->RegisterExposedField(name, ScriptFieldType::Bool, ptr, static_cast<uint32_t>(sizeof(bool)));
	}
	ScriptFieldRegistrar(ScriptBase* self, const char* name, Vector3* ptr) {
		if (self) self->RegisterExposedField(name, ScriptFieldType::Vector3, ptr, static_cast<uint32_t>(sizeof(Vector3)));
	}
	template <size_t N>
	ScriptFieldRegistrar(ScriptBase* self, const char* name, char (*ptr)[N]) {
		if (self) self->RegisterExposedField(name, ScriptFieldType::CharBuffer, *ptr, static_cast<uint32_t>(N));
	}
};

/// Macro to expose a script field (float, int, bool, Vector3, or char[N]) to the Inspector and preserve it across hot reloads and scene saves.
/// Note: Fields not declared with EXPOSE_FIELD are not preserved across a script hot reload.
#define EXPOSE_FIELD(Type, name, ...) \
	::std::type_identity_t<Type> name = __VA_ARGS__; \
	::ScriptFieldRegistrar _rize_field_reg_##name{ this, #name, &this->name };

#ifdef GAMESCRIPTS_EXPORTS
/// Internal DLL registration entry point implemented in GameScripts/Scripts/dllmain.cpp
void GameScripts_RegisterClass(const char* className, ScriptBase* (*factory)());

#define REGISTER_SCRIPT(ClassName) \
	namespace { \
		ScriptBase* Create_##ClassName##_Instance() { \
			return new ClassName(); \
		} \
		struct ClassName##_DllRegistrar { \
			ClassName##_DllRegistrar() { \
				GameScripts_RegisterClass(#ClassName, &Create_##ClassName##_Instance); \
			} \
		}; \
		static ClassName##_DllRegistrar s_##ClassName##_dllRegistrar; \
	} \
	extern "C" __declspec(dllexport) const void* ForceLinkScript_##ClassName() { \
		return &s_##ClassName##_dllRegistrar; \
	} \
	__pragma(comment(linker, "/include:ForceLinkScript_" #ClassName))
#endif
