#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
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
	uint32_t layer = 0;                     // ScriptLayer value (0..31)
	uint32_t collisionMask = 0xFFFFFFFFu;   // bit N set = collides with layer N (use LayerBit())

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

/// Collision layers. Two colliders only collide when each one's collisionMask contains the other's layer bit.
/// Names are shown in the Inspector; layers 8..31 are free for game use.
enum ScriptLayer : uint32_t {
	LAYER_DEFAULT = 0,
	LAYER_PLAYER = 1,
	LAYER_ENEMY = 2,
	LAYER_PLAYER_BULLET = 3,
	LAYER_WALL = 4,
	LAYER_TRIGGER = 5,
	LAYER_ENEMY_BULLET = 6,
	LAYER_PICKUP = 7,
	LAYER_NAMED_COUNT = 8,
};
constexpr uint32_t LayerBit(uint32_t layer) { return 1u << (layer & 31u); }
constexpr uint32_t kCollideWithAll = 0xFFFFFFFFu;

/// Keyboard key codes for IsKeyDown / IsKeyTriggered (DirectInput scan codes, so scripts need no <dinput.h>).
/// Values equal the matching DIK_* constants; a self-test checks this.
enum ScriptKey : int {
	KEY_ESCAPE = 0x01,
	KEY_1 = 0x02, KEY_2 = 0x03, KEY_3 = 0x04, KEY_4 = 0x05, KEY_5 = 0x06,
	KEY_6 = 0x07, KEY_7 = 0x08, KEY_8 = 0x09, KEY_9 = 0x0A, KEY_0 = 0x0B,
	KEY_BACKSPACE = 0x0E, KEY_TAB = 0x0F,
	KEY_Q = 0x10, KEY_W = 0x11, KEY_E = 0x12, KEY_R = 0x13, KEY_T = 0x14, KEY_Y = 0x15,
	KEY_U = 0x16, KEY_I = 0x17, KEY_O = 0x18, KEY_P = 0x19,
	KEY_ENTER = 0x1C, KEY_LCTRL = 0x1D,
	KEY_A = 0x1E, KEY_S = 0x1F, KEY_D = 0x20, KEY_F = 0x21, KEY_G = 0x22, KEY_H = 0x23,
	KEY_J = 0x24, KEY_K = 0x25, KEY_L = 0x26,
	KEY_LSHIFT = 0x2A,
	KEY_Z = 0x2C, KEY_X = 0x2D, KEY_C = 0x2E, KEY_V = 0x2F, KEY_B = 0x30, KEY_N = 0x31, KEY_M = 0x32,
	KEY_RSHIFT = 0x36, KEY_LALT = 0x38, KEY_SPACE = 0x39,
	KEY_F1 = 0x3B, KEY_F2 = 0x3C, KEY_F3 = 0x3D, KEY_F4 = 0x3E, KEY_F5 = 0x3F,
	KEY_F6 = 0x40, KEY_F7 = 0x41, KEY_F8 = 0x42, KEY_F9 = 0x43, KEY_F10 = 0x44,
	KEY_F11 = 0x57, KEY_F12 = 0x58,
	KEY_RCTRL = 0x9D, KEY_RALT = 0xB8,
	KEY_UP = 0xC8, KEY_LEFT = 0xCB, KEY_RIGHT = 0xCD, KEY_DOWN = 0xD0,
};

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

	// Input queries: keyCode is a DirectInput scan code. Use the KEY_* constants below
	// (plain ASCII such as 'W' does NOT work: 'W' == 0x57 == the scan code of F11).
	bool (*isKeyDown)(void* ctx, int keyCode) = nullptr;
	bool (*isKeyTriggered)(void* ctx, int keyCode) = nullptr;

	// Frame delta time
	float (*getDeltaTime)(void* ctx) = nullptr;

	// Instantiate a new GameObject in the scene (primitiveType: 0=Cube, 1=Sphere, 2=Plane, 3=Quad, 4=Empty)
	ScriptGameObjectView* (*instantiate)(void* ctx, const char* name, int primitiveType, Vector3 position) = nullptr;

	// Mouse: button 0 = left, 1 = right, 2 = middle. Position is in Game view pixels (0,0 = top-left).
	bool (*isMouseDown)(void* ctx, int button) = nullptr;
	bool (*isMouseTriggered)(void* ctx, int button) = nullptr;
	bool (*getMousePosition)(void* ctx, Vector2* outPixels, Vector2* outViewSize) = nullptr;
	// Game camera ray through the mouse hits the plane y = planeY (top-down aiming)
	bool (*getMouseWorldOnPlane)(void* ctx, float planeY, Vector3* outPoint) = nullptr;

	// Gamepad (XInput, pad 0..3). button = ScriptPadButton. Sticks are -1..1 (Y up = +1), triggers 0..1.
	bool (*isPadConnected)(void* ctx, int pad) = nullptr;
	bool (*isPadButtonDown)(void* ctx, int button, int pad) = nullptr;
	bool (*isPadButtonTriggered)(void* ctx, int button, int pad) = nullptr;
	Vector2 (*getPadStick)(void* ctx, int stick, int pad) = nullptr;   // stick 0 = left, 1 = right
	float (*getPadTrigger)(void* ctx, int side, int pad) = nullptr;    // side 0 = left, 1 = right
	void (*setPadVibration)(void* ctx, int pad, float left, float right) = nullptr;

	// Time. getDeltaTime() is already scaled; timeScale 0 pauses scripts' dt, physics and animation.
	float (*getUnscaledDeltaTime)(void* ctx) = nullptr;
	float (*getTime)(void* ctx) = nullptr;              // scaled seconds since Play started
	float (*getTimeScale)(void* ctx) = nullptr;
	void (*setTimeScale)(void* ctx, float scale) = nullptr;

	// Prefabs (Assets/Prefabs/<name>.prefab.json). Deferred like instantiate; reuses despawned instances.
	ScriptGameObjectView* (*instantiatePrefab)(void* ctx, const char* prefabName, Vector3 position) = nullptr;
	// Prefab instance -> hidden and returned to its pool; any other object -> deleted
	void (*despawn)(void* ctx, uint32_t id) = nullptr;

	// Game UI on a 1280 x 720 canvas (origin top-left), drawn over the game view in every build.
	// Call every frame (immediate mode). align: 0 = left, 1 = center, 2 = right.
	void (*uiRect)(void* ctx, float x, float y, float w, float h, Vector4 color) = nullptr;
	void (*uiText)(void* ctx, const char* text, float x, float y, float size, Vector4 color, int align) = nullptr;
	void (*uiImage)(void* ctx, const char* texturePath, float x, float y, float w, float h, Vector4 color) = nullptr;
	float (*uiMeasureText)(void* ctx, const char* text, float size) = nullptr;

	// Scene switching: "Stage2" -> Assets/Scenes/Stage2.scene.json (or a project-relative .json path).
	// Happens at the end of the frame; all scripts of the old scene get OnDestroy().
	void (*loadScene)(void* ctx, const char* sceneNameOrPath) = nullptr;
	// Name of the scene that is running (e.g. "Stage1"); returns the length, writes a terminated string
	int (*getSceneName)(void* ctx, char* out, int outSize) = nullptr;

	// Persistent save data (SaveData/save.json in the project / game folder), written immediately
	void (*saveSetNumber)(void* ctx, const char* key, double value) = nullptr;
	double (*saveGetNumber)(void* ctx, const char* key, double fallback) = nullptr;
	void (*saveSetString)(void* ctx, const char* key, const char* value) = nullptr;
	// Copies the string into out (truncated, always terminated); returns false if the key is missing
	bool (*saveGetString)(void* ctx, const char* key, char* out, int outSize) = nullptr;
	bool (*saveHasKey)(void* ctx, const char* key) = nullptr;
	void (*saveDeleteAll)(void* ctx) = nullptr;
};

/// Engine <-> GameScripts.dll compatibility check. Bump kScriptApiVersion when the meaning of the API changes;
/// layout changes of EngineAPI / ScriptGameObjectView / ScriptBase are caught automatically by the sizes.
/// The fingerprint is defined at the end of this header (it needs the complete ScriptBase).
constexpr uint32_t kScriptApiVersion = 2;

/// Size of the UI canvas used by the UI* helpers (stretched over the game view)
constexpr float kUICanvasWidth = 1280.0f;
constexpr float kUICanvasHeight = 720.0f;

/// XInput button bits for isPadButtonDown / IsPadButtonDown (same values as XINPUT_GAMEPAD_*).
enum ScriptPadButton : int {
	PAD_DPAD_UP = 0x0001, PAD_DPAD_DOWN = 0x0002, PAD_DPAD_LEFT = 0x0004, PAD_DPAD_RIGHT = 0x0008,
	PAD_START = 0x0010, PAD_BACK = 0x0020, PAD_LEFT_THUMB = 0x0040, PAD_RIGHT_THUMB = 0x0080,
	PAD_LB = 0x0100, PAD_RB = 0x0200,
	PAD_A = 0x1000, PAD_B = 0x2000, PAD_X = 0x4000, PAD_Y = 0x8000,
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
/// Fallback host interface. It intentionally exposes no object lookup: on the engine side GameObject is the
/// editor struct, which must never reach the DLL. Scripts look objects up through EngineAPI (views only).
class IScriptHost {
public:
	virtual ~IScriptHost() = default;
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

	/// Only a ScriptGameObjectView can become the owner. In the DLL GameObject is the same type;
	/// on the engine side there is deliberately no overload taking the editor GameObject.
	void SetOwner(ScriptGameObjectView* ownerView, uint32_t id = 0) {
		GameObject* ownerObj = reinterpret_cast<GameObject*>(ownerView);
		gameObject = ownerObj;
		owner = ownerObj;
		gameObject_ = ownerObj;
		if (id != 0 || !ownerView) {
			ownerId_ = id;
		}
	}

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

	// EngineAPI convenience helpers for scripts (lookups only go through EngineAPI, which returns views)
	GameObject* FindGameObjectById(uint32_t id) const {
		if (engineApi_ && engineApi_->findObjectById) {
			return reinterpret_cast<GameObject*>(engineApi_->findObjectById(engineApi_->hostContext, id, true));
		}
		return nullptr;
	}

	GameObject* FindGameObjectByName(const char* name) const {
		if (!name) return nullptr;
		if (engineApi_ && engineApi_->findObjectByName) {
			return reinterpret_cast<GameObject*>(engineApi_->findObjectByName(engineApi_->hostContext, name, true));
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

	// ---- Prefabs & pooling ----------------------------------------------------
	/// Spawns Assets/Prefabs/<prefabName>.prefab.json at position (created at the end of this frame;
	/// the returned view can be edited right away). Reuses a despawned instance when available, in which
	/// case its scripts get their prefab field values back and Initialize() runs again.
	GameObject* InstantiatePrefab(const char* prefabName, Vector3 position = { 0.0f, 0.0f, 0.0f }) const {
		if (engineApi_ && engineApi_->instantiatePrefab && prefabName) {
			return reinterpret_cast<GameObject*>(engineApi_->instantiatePrefab(engineApi_->hostContext, prefabName, position));
		}
		return nullptr;
	}
	/// Returns a prefab instance to its pool (cheap, use for bullets/enemies/pickups); other objects are destroyed
	void Despawn(uint32_t id) const {
		if (engineApi_ && engineApi_->despawn) engineApi_->despawn(engineApi_->hostContext, id);
	}
	void DespawnSelf() const {
		if (ownerId_ != 0) Despawn(ownerId_);
	}

	// ---- Scenes & save data -------------------------------------------------
	/// Switches to Assets/Scenes/<name>.scene.json at the end of this frame (e.g. LoadScene("GameOver"))
	void LoadScene(const char* sceneNameOrPath) const {
		if (engineApi_ && engineApi_->loadScene && sceneNameOrPath) engineApi_->loadScene(engineApi_->hostContext, sceneNameOrPath);
	}
	/// Current scene name, e.g. "Stage1" (empty string if unavailable)
	const char* GetSceneName() const {
		static char buffer[128];
		buffer[0] = '\0';
		if (engineApi_ && engineApi_->getSceneName) engineApi_->getSceneName(engineApi_->hostContext, buffer, static_cast<int>(sizeof(buffer)));
		return buffer;
	}
	void SaveInt(const char* key, int value) const { SaveFloatImpl(key, static_cast<double>(value)); }
	void SaveFloat(const char* key, float value) const { SaveFloatImpl(key, static_cast<double>(value)); }
	int LoadInt(const char* key, int fallback = 0) const { return static_cast<int>(LoadFloatImpl(key, static_cast<double>(fallback))); }
	float LoadFloat(const char* key, float fallback = 0.0f) const { return static_cast<float>(LoadFloatImpl(key, static_cast<double>(fallback))); }
	void SaveText(const char* key, const char* value) const {
		if (engineApi_ && engineApi_->saveSetString && key && value) engineApi_->saveSetString(engineApi_->hostContext, key, value);
	}
	/// Copies the saved string into out; returns false (and leaves out empty) when the key is missing
	bool LoadText(const char* key, char* out, int outSize) const {
		if (out && outSize > 0) out[0] = '\0';
		return engineApi_ && engineApi_->saveGetString && key && out && engineApi_->saveGetString(engineApi_->hostContext, key, out, outSize);
	}
	bool HasSaveKey(const char* key) const {
		return engineApi_ && engineApi_->saveHasKey && key && engineApi_->saveHasKey(engineApi_->hostContext, key);
	}
	void DeleteAllSaveData() const {
		if (engineApi_ && engineApi_->saveDeleteAll) engineApi_->saveDeleteAll(engineApi_->hostContext);
	}

	// ---- Game UI (1280 x 720 canvas, call every frame from Update) ----------
	void UIRect(float x, float y, float w, float h, Vector4 color) const {
		if (engineApi_ && engineApi_->uiRect) engineApi_->uiRect(engineApi_->hostContext, x, y, w, h, color);
	}
	/// size = line height in canvas pixels; align 0 = left, 1 = center, 2 = right (x is the anchor)
	void UIText(const char* text, float x, float y, float size = 32.0f, Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }, int align = 0) const {
		if (engineApi_ && engineApi_->uiText && text) engineApi_->uiText(engineApi_->hostContext, text, x, y, size, color, align);
	}
	/// texturePath relative to the project folder, e.g. "Assets/Textures/heart.png" (loaded once, then cached)
	void UIImage(const char* texturePath, float x, float y, float w, float h, Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) const {
		if (engineApi_ && engineApi_->uiImage && texturePath) engineApi_->uiImage(engineApi_->hostContext, texturePath, x, y, w, h, color);
	}
	float UIMeasureText(const char* text, float size = 32.0f) const {
		return (engineApi_ && engineApi_->uiMeasureText && text) ? engineApi_->uiMeasureText(engineApi_->hostContext, text, size) : 0.0f;
	}
	/// HP / XP bar: value01 is clamped to 0..1
	void UIProgressBar(float x, float y, float w, float h, float value01,
		Vector4 fill = { 0.2f, 0.85f, 0.3f, 1.0f }, Vector4 back = { 0.0f, 0.0f, 0.0f, 0.6f }) const {
		const float v = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		UIRect(x, y, w, h, back);
		UIRect(x, y, w * v, h, fill);
	}
	/// Mouse position on the UI canvas; false when the mouse is outside the game view
	bool GetMouseCanvasPosition(Vector2& outCanvas) const {
		Vector2 pixels{};
		Vector2 size{};
		if (!GetMousePosition(pixels, &size) || size.x <= 0.0f || size.y <= 0.0f) return false;
		outCanvas = { pixels.x / size.x * kUICanvasWidth, pixels.y / size.y * kUICanvasHeight };
		return true;
	}
	/// Button: returns true when clicked with the mouse, or when `selected` and Enter / Space / pad A is pressed
	/// (keyboard/gamepad menus keep their own selected index and pass selected = (index == i)).
	bool UIButton(const char* label, float x, float y, float w, float h, bool selected = false, float textSize = 28.0f) const {
		Vector2 m{};
		const bool hover = GetMouseCanvasPosition(m) && m.x >= x && m.x < x + w && m.y >= y && m.y < y + h;
		const bool highlighted = hover || selected;
		UIRect(x, y, w, h, highlighted ? Vector4{ 0.95f, 0.75f, 0.2f, 0.95f } : Vector4{ 0.1f, 0.1f, 0.12f, 0.85f });
		UIText(label, x + w * 0.5f, y + (h - textSize) * 0.5f, textSize,
			highlighted ? Vector4{ 0.05f, 0.05f, 0.05f, 1.0f } : Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }, 1);
		if (hover && IsMouseTriggered(0)) return true;
		return selected && (IsKeyTriggered(KEY_ENTER) || IsKeyTriggered(KEY_SPACE) || IsPadButtonTriggered(PAD_A));
	}

	// ---- Mouse --------------------------------------------------------------
	bool IsMouseDown(int button = 0) const {
		return engineApi_ && engineApi_->isMouseDown && engineApi_->isMouseDown(engineApi_->hostContext, button);
	}
	bool IsMouseTriggered(int button = 0) const {
		return engineApi_ && engineApi_->isMouseTriggered && engineApi_->isMouseTriggered(engineApi_->hostContext, button);
	}
	/// Mouse position in Game view pixels; false when the mouse is outside the Game view
	bool GetMousePosition(Vector2& outPixels, Vector2* outViewSize = nullptr) const {
		Vector2 size{};
		const bool ok = engineApi_ && engineApi_->getMousePosition && engineApi_->getMousePosition(engineApi_->hostContext, &outPixels, &size);
		if (outViewSize) *outViewSize = size;
		return ok;
	}
	/// Point on the ground plane y = planeY under the mouse (aim direction for top-down shooters)
	bool GetMouseWorldPosition(Vector3& outPoint, float planeY = 0.0f) const {
		return engineApi_ && engineApi_->getMouseWorldOnPlane && engineApi_->getMouseWorldOnPlane(engineApi_->hostContext, planeY, &outPoint);
	}

	// ---- Gamepad ------------------------------------------------------------
	bool IsPadConnected(int pad = 0) const {
		return engineApi_ && engineApi_->isPadConnected && engineApi_->isPadConnected(engineApi_->hostContext, pad);
	}
	bool IsPadButtonDown(int button, int pad = 0) const {
		return engineApi_ && engineApi_->isPadButtonDown && engineApi_->isPadButtonDown(engineApi_->hostContext, button, pad);
	}
	bool IsPadButtonTriggered(int button, int pad = 0) const {
		return engineApi_ && engineApi_->isPadButtonTriggered && engineApi_->isPadButtonTriggered(engineApi_->hostContext, button, pad);
	}
	Vector2 GetLeftStick(int pad = 0) const {
		return (engineApi_ && engineApi_->getPadStick) ? engineApi_->getPadStick(engineApi_->hostContext, 0, pad) : Vector2{};
	}
	Vector2 GetRightStick(int pad = 0) const {
		return (engineApi_ && engineApi_->getPadStick) ? engineApi_->getPadStick(engineApi_->hostContext, 1, pad) : Vector2{};
	}
	float GetPadTrigger(int side, int pad = 0) const {
		return (engineApi_ && engineApi_->getPadTrigger) ? engineApi_->getPadTrigger(engineApi_->hostContext, side, pad) : 0.0f;
	}
	void SetPadVibration(float left, float right, int pad = 0) const {
		if (engineApi_ && engineApi_->setPadVibration) engineApi_->setPadVibration(engineApi_->hostContext, pad, left, right);
	}

	/// Movement input from WASD / arrow keys / left stick: x = right, y = forward (+Z in a top-down view).
	/// Length is at most 1, so diagonal movement is not faster.
	Vector2 GetMoveAxis(int pad = 0) const {
		float x = 0.0f;
		float y = 0.0f;
		if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) x += 1.0f;
		if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) x -= 1.0f;
		if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) y += 1.0f;
		if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) y -= 1.0f;
		const Vector2 stick = GetLeftStick(pad);
		x += stick.x;
		y += stick.y;
		const float len = std::sqrt(x * x + y * y);
		if (len > 1.0f) { x /= len; y /= len; }
		return { x, y };
	}

	// ---- Time ---------------------------------------------------------------
	float GetUnscaledDeltaTime() const {
		return (engineApi_ && engineApi_->getUnscaledDeltaTime) ? engineApi_->getUnscaledDeltaTime(engineApi_->hostContext) : GetDeltaTime();
	}
	/// Scaled seconds since Play started
	float GetTime() const {
		return (engineApi_ && engineApi_->getTime) ? engineApi_->getTime(engineApi_->hostContext) : 0.0f;
	}
	float GetTimeScale() const {
		return (engineApi_ && engineApi_->getTimeScale) ? engineApi_->getTimeScale(engineApi_->hostContext) : 1.0f;
	}
	/// 0 = pause the game (level-up menu), 1 = normal, 0.3 = slow motion. Takes effect next frame.
	void SetTimeScale(float scale) const {
		if (engineApi_ && engineApi_->setTimeScale) engineApi_->setTimeScale(engineApi_->hostContext, scale);
	}

	// Pointer to owner GameObject view (synchronized by the engine)
	GameObject* gameObject = nullptr;
	GameObject* owner = nullptr;

protected:
	void SaveFloatImpl(const char* key, double value) const {
		if (engineApi_ && engineApi_->saveSetNumber && key) engineApi_->saveSetNumber(engineApi_->hostContext, key, value);
	}
	double LoadFloatImpl(const char* key, double fallback) const {
		return (engineApi_ && engineApi_->saveGetNumber && key) ? engineApi_->saveGetNumber(engineApi_->hostContext, key, fallback) : fallback;
	}

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

/// Exported by GameScripts.dll (GameScripts_GetApiFingerprint) and compared by the engine on load
constexpr uint32_t ScriptApiFingerprint() {
	return (kScriptApiVersion << 24)
		^ (static_cast<uint32_t>(sizeof(EngineAPI)) << 12)
		^ (static_cast<uint32_t>(sizeof(ScriptGameObjectView)) << 4)
		^ static_cast<uint32_t>(sizeof(ScriptBase));
}

#ifdef GAMESCRIPTS_EXPORTS
/// Internal DLL registration entry point implemented in GameScripts/Scripts/dllmain.cpp
void GameScripts_RegisterClass(const char* className, std::unique_ptr<ScriptBase> (*factory)());

#define REGISTER_SCRIPT(ClassName) \
	namespace { \
		std::unique_ptr<ScriptBase> Create_##ClassName##_Instance() { \
			return std::make_unique<ClassName>(); \
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
