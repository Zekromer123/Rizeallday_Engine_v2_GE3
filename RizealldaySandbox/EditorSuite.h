#pragma once
#include <d3d12.h>
#include "ParticleQuad.h"
#include "base/RenderTexture.h"
#include "3d/Camera.h"
#include "3d/ShadowMap.h"
#include "3d/Skybox.h"
#include "3d/WorldTransform.h"
#include "base/DXCManager.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"
#include "math/Frustum.h"
#include "base/ScriptAPI.h"
#include "base/ScriptBase.h"
#include "base/ScriptModule.h"
#ifdef USE_IMGUI
#include <imgui.h>
#else
struct ImVec2 {
	float x = 0.0f;
	float y = 0.0f;
	constexpr ImVec2() = default;
	constexpr ImVec2(float _x, float _y) : x(_x), y(_y) {}
};
#endif
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <memory>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

class DirectXCommon;
class Input;
class LightGroup;
class Object3d;
class Model;

enum class EditorSceneState {
	Edit,
	Play,
	Pause
};

enum class GizmoType {
	None = -1,
	Hand = 0,
	Move = 1,
	Rotate = 2,
	Scale = 3,
	Bounds = 4,
	Collider = 5
};

enum class GizmoAxis {
	None = -1,
	X = 0,
	Y = 1,
	Z = 2,
	Center = 3
};

enum class GameObjectType {
	Empty,
	Cube,
	Sphere,
	Plane,
	Quad,
	Camera,
	DirectionalLight,
	PointLight,
	SpotLight,
	ParticleSystem,
	MapchipGrid,
	Sprite,
	AudioSource,
	Canvas
};

struct GameObject {
	GameObject();
	~GameObject();
	GameObject(const GameObject& other);
	GameObject(GameObject&& other) noexcept;
	GameObject& operator=(const GameObject& other);
	GameObject& operator=(GameObject&& other) noexcept;

	uint32_t id = 0;
	std::string name = "GameObject";
	bool isActive = true;
	bool isStatic = false;
	std::string tag = "Untagged";
	uint32_t layer = 0;              // 0..31, names in ScriptLayer (ScriptAPI.h) / GetCollisionLayerName()
	uint32_t collisionMask = 0xFFFFFFFF;
	// Set on objects created from a prefab (runtime only, not saved): used by object pooling
	std::string prefabSource;
	int prefabNodeIndex = -1;
	GameObjectType type = GameObjectType::Empty;
	int parentIndex = -1;
	int parentId = -1;
	std::vector<uint32_t> childrenIds;

	// Transform Component
	Vector3 translation = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f }; // Radians
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Quaternion rotationQuat = Quaternion::Identity();
	// Runtime only (not serialized): world matrix, refreshed by EditorSuite::UpdateWorldMatrices().
	// translation / rotation / rotationQuat / scale above are LOCAL (relative to the parent).
	Matrix4x4 worldMatrix = Mathematics::MakeIdentity4x4();

	// Mesh Filter & Mesh Renderer
	bool hasMesh = false;
	bool meshEnabled = true;
	enum class MeshType { Cube, Sphere, Plane, Quad, CustomModel };
	MeshType meshType = MeshType::Cube;
	Vector4 materialColor = { 0.8f, 0.8f, 0.82f, 1.0f };
	uint32_t textureHandle = 0;
	bool isLit = true;
	int lightingType = 4; // 0=Unlit, 1=Lambert, 2=HalfLambert, 3=Phong, 4=BlinnPhong
	float shininess = 32.0f;
	Vector3 specularColor = { 1.0f, 1.0f, 1.0f };
	bool castShadow = true;
	bool receiveShadow = true;
	bool useGeometryShader = false;
	int geometryShaderEffect = 0; // 0=None, 1=Standard Pass-Through, 2=Explode Faces Along Normals
	float explodeAmount = 0.5f;
	std::string texturePath = "";
	std::string modelPath = "";
	Vector2 uvOffset = { 0.0f, 0.0f };
	Vector2 uvTiling = { 1.0f, 1.0f };
	float uvRotation = 0.0f;
	std::unique_ptr<Object3d> editObject3d;
	std::unique_ptr<Object3d> gameObject3d;
	std::unique_ptr<WorldTransform> shadowTransform;

	// Animator / Skeletal Animation Component
	int animationIndex = 0;
	float animationSpeed = 1.0f;
	bool animationLoop = true;
	bool animationPlaying = true;

	// Box Collider Component
	bool hasBoxCollider = false;
	bool boxColliderEnabled = true;
	bool isTrigger = false;
	bool providesContacts = true;
	Vector3 boxCenter = { 0.0f, 0.0f, 0.0f };
	Vector3 boxSize = { 1.0f, 1.0f, 1.0f };
	bool isColliding = false;

	// Sphere Collider Component
	bool hasSphereCollider = false;
	bool sphereColliderEnabled = true;
	bool sphereIsTrigger = false;
	Vector3 sphereCenter = { 0.0f, 0.0f, 0.0f };
	float sphereRadius = 0.5f;

	// Camera Component
	bool hasCamera = false;
	bool cameraEnabled = true;
	float fov = 45.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;
	bool isOrthographic = false;

	// Light Component
	bool hasLight = false;
	bool lightEnabled = true;
	enum class LightType { Directional, Point, Spot };
	LightType lightType = LightType::Directional;
	Vector3 lightColor = { 1.0f, 0.95f, 0.9f };
	float lightIntensity = 1.0f;
	Vector3 lightDirection = { 0.5f, -1.0f, 0.5f };
	float lightRange = 10.0f;
	float lightDecay = 2.0f;
	float lightSpotAngle = 30.0f;
	float lightSpotFalloffStart = 20.0f;
	bool lightCastShadow = true;
	float lightShadowBias = 0.002f;
	float lightShadowStrength = 1.0f;
	float lightShadowNormalBias = 0.02f;

	// Particle System Component
	bool hasParticleSystem = false;
	int attachToIndex = -1;
	int attachedParentId = -1;
	Vector3 localOffset = { 0.0f, 0.0f, 0.0f };
	int particlePreset = 0; // 0: Custom, 1: Smoke, 2: Water Fountain, 3: Fireworks, 4: Explosion, 5: Torch

	// Particle System Parameters
	int particleShape = 1; // 0: Point, 1: Sphere, 2: Box, 3: Cone
	float particleSpawnRadius = 0.5f;
	Vector3 particleBoxExtents = { 1.0f, 1.0f, 1.0f };
	float particleConeAngle = 25.0f; // degrees

	float particleSpawnRate = 20.0f; // particles per second
	int particleMaxParticles = 100;
	int particleBurstCount = 15;

	Vector3 particleDirection = { 0.0f, 1.0f, 0.0f };
	float particleSpreadAngle = 30.0f; // degrees (0-180)
	float particleSpeedMin = 1.0f;
	float particleSpeedMax = 3.0f;

	float particleGravity = 0.0f;
	float particleDrag = 0.05f;

	bool particleWindEnabled = false;
	Vector3 particleWindAcceleration = { 15.0f, 0.0f, 0.0f };

	float particleLifetimeMin = 1.5f;
	float particleLifetimeMax = 3.0f;

	float particleStartScale = 0.25f;
	float particleEndScale = 0.5f;

	Vector4 particleStartColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 particleEndColor = { 1.0f, 1.0f, 1.0f, 0.0f };

	int particleBlendMode = 0; // 0: Normal Alpha, 1: Additive Glow
	int particleBillboardMode = 0; // 0: Camera Facing, 1: Y-Axis Cylindrical, 2: Velocity Aligned

	std::shared_ptr<ParticleQuad> particleQuad;

	// Mapchip Component
	bool hasMapchip = false;

	// Audio Source Component
	bool hasAudioSource = false;
	std::string audioClipName = "None";
	float audioVolume = 1.0f;
	bool audioPlayOnAwake = false;
	bool audioLoop = false;
	uint32_t runtimeSoundHandle = 0;
	uint32_t runtimeVoiceHandle = 0;

	// C++ Script Component
	struct ScriptSavedFieldValue {
		ScriptFieldType type = ScriptFieldType::Float;
		float floatVal = 0.0f;
		int intVal = 0;
		bool boolVal = false;
		Vector3 vec3Val = { 0.0f, 0.0f, 0.0f };
		std::string strVal = "";
	};

	struct ScriptComponent {
		std::string className = "";
		ScriptInstancePtr instance = nullptr;
		std::map<std::string, ScriptSavedFieldValue> savedFields;
		bool isMissingScript = false;
		bool isCompiling = false;

		ScriptComponent() = default;
		explicit ScriptComponent(const std::string& name) : className(name), instance(nullptr) {}
		ScriptComponent(const ScriptComponent& other)
			: className(other.className),
			  instance(nullptr),
			  savedFields(other.savedFields),
			  isMissingScript(other.isMissingScript),
			  isCompiling(other.isCompiling) {}
		ScriptComponent& operator=(const ScriptComponent& other) {
			if (this != &other) {
				className = other.className;
				instance.reset();
				savedFields = other.savedFields;
				isMissingScript = other.isMissingScript;
				isCompiling = other.isCompiling;
			}
			return *this;
		}
		ScriptComponent(ScriptComponent&&) noexcept = default;
		ScriptComponent& operator=(ScriptComponent&&) noexcept = default;
	};
	bool hasScript = false;
	std::string scriptClassName = "";
	std::vector<ScriptComponent> scripts;

	// Rigidbody Component
	bool hasRigidbody = false;
	bool rbEnabled = true;
	float mass = 1.0f;
	float drag = 0.05f;
	float gravityScale = 1.0f;
	bool useGravity = true;
	bool isKinematic = false;
	float bounciness = 0.1f;
	Vector3 velocity = { 0.0f, 0.0f, 0.0f };
	bool isGrounded = false;
};

class EditorSuite : public IScriptHost
{
public:
	EditorSuite();
	~EditorSuite() override;

	void Initialize(
		DirectXCommon* dxCommon,
		Camera* gameCamera,
		LightGroup* lights,
		ParticleQuad* particleSystem
	);

	void Update(float deltaTime, Input* input);

	// Render scene to EditViewRenderTexture and GameViewRenderTexture
	void RenderViewports(ID3D12GraphicsCommandList* cmdList, bool showParticles);

	// Draw all Editor GUI panels (Hierarchy, Inspector, Viewports, Project, Console)
	void DrawEditorUI(float deltaTime, bool& showParticles);

	EditorSceneState GetSceneState() const { return sceneState_; }
	Camera& GetEditorCamera() { return editorCamera_; }

	// Creation & Lookup helpers
	GameObject& CreateGameObject(const std::string& name, GameObjectType type);
	GameObject& CreateCube(const std::string& name = "Cube", const Vector3& pos = { 0.0f, 0.0f, 0.0f });
	GameObject& CreateSphere(const std::string& name = "Sphere", const Vector3& pos = { 0.0f, 0.0f, 0.0f });
	GameObject& CreatePlane(const std::string& name = "Plane", const Vector3& pos = { 0.0f, 0.0f, 0.0f });
	GameObject& CreateQuad(const std::string& name = "Textured Quad", const Vector3& pos = { 0.0f, 0.0f, 0.0f });
	GameObject* FindGameObjectById(uint32_t id, bool warnIfMissing = true);
	// O(1) id -> index lookup. The cache is validated on every hit and rebuilt on a miss,
	// so code that mutates gameObjects_ does not have to invalidate it.
	int FindGameObjectIndexById(uint32_t id);
	static std::string GetCollisionLayerName(uint32_t layer);
	GameObject* FindGameObjectByName(const char* name, bool warnIfMissing = true);
	GameObject* FindGameObjectByName(const std::string& name, bool warnIfMissing = true) {
		return FindGameObjectByName(name.c_str(), warnIfMissing);
	}
	// Transform hierarchy (EditorSuite_Hierarchy.cpp). parentId is the source of truth;
	// parentIndex / childrenIds / worldMatrix are derived caches.
	void RebuildHierarchyLinks();
	void UpdateWorldMatrices();
	bool IsDescendantOf(uint32_t candidateId, uint32_t ancestorId) const;
	bool ReparentGameObject(uint32_t childId, int newParentId, bool keepWorldTransform = true);
	void GetWorldTRS(const GameObject& obj, Vector3& t, Quaternion& q, Vector3& s) const;

	void DeleteSelectedGameObject();
	void QueueDeleteGameObject(int index);
	void QueueDeleteGameObjectById(uint32_t id) override;
	void ProcessPendingDeletes();
	void LogScriptMessage(const char* msg, int logType = 0) override {
		if (msg) LogMessage(msg, logType);
	}

	bool HasActiveParticleSystem(Vector3* outEmitterPos = nullptr) const;
	void StartPlayMode();
	void StopPlayMode();
	void UpdateScripts(float deltaTime);
	void DispatchScriptCollision(uint32_t selfId, uint32_t otherId);
	void SavePlayModeSnapshot();
	void RestorePlayModeSnapshot();

	// GameScripts DLL Hot Reload, Reflection & Build Pipeline
	void SetupEngineAPI();
	void HotReloadScripts();
	std::string SerializeAllScriptFieldsToJson();
	void DeserializeAllScriptFieldsFromJson(const std::string& json);
	void CaptureScriptFieldsToSaved(GameObject::ScriptComponent& sc);
	void ApplySavedFieldsToScript(GameObject::ScriptComponent& sc);
	void EnsureEditModeScriptInstances();
	void DestroyAllScriptInstances();
	void SyncGameObjectToView(const GameObject& src, ScriptGameObjectView& dst);
	void SyncViewToGameObject(const ScriptGameObjectView& src, GameObject& dst);
	void RefreshAllScriptObjectViews();
	ScriptGameObjectView* GetOrUpdateScriptViewById(uint32_t id);
	ScriptGameObjectView* FindPendingSpawnView(uint32_t id);
	ScriptGameObjectView* QueueScriptSpawn(const char* name, int primitiveType, const Vector3& position);
	// Prefabs + pooling (EditorSuite_Prefabs.cpp)
	std::filesystem::path GetPrefabDirectory() const;
	bool SavePrefab(uint32_t rootId, const std::string& prefabName);
	bool HasPrefab(const std::string& prefabName);
	void ClearPrefabCache();
	// Creates the prefab's objects right away (editor use / deferred spawn processing). Returns the root id or 0.
	uint32_t InstantiatePrefabNow(const std::string& prefabName, const Vector3& position, uint32_t rootId = 0, const ScriptGameObjectView* rootOverride = nullptr);
	void StartScriptsOnSpawnedObject(uint32_t id, bool createInstances);
	void ResetPooledPrefabInstance(uint32_t rootId, const ScriptGameObjectView* rootView);
	// Script entry points: deferred like QueueScriptSpawn. Reuses a despawned instance when one is pooled.
	ScriptGameObjectView* QueuePrefabSpawn(const char* prefabName, const Vector3& position);
	// Prefab instances go back to their pool (hidden, scripts paused); other objects are deleted
	void QueueDespawn(uint32_t id);
	void ProcessPendingDespawns();
	void ResetPrefabPools();
	size_t GetPooledCount(const std::string& prefabName) const;

	// Runtime scene switching + save data (EditorSuite_Runtime.cpp)
	std::filesystem::path ResolveSceneRequest(const std::string& nameOrPath) const;
	void RequestSceneLoad(const std::string& nameOrPath); // deferred to the end of the frame
	bool ProcessPendingSceneLoad();
	void RestoreRuntimeSceneOrigin();
	std::filesystem::path GetSaveDataPath() const;
	void SetSaveNumber(const std::string& key, double value);
	double GetSaveNumber(const std::string& key, double fallback);
	void SetSaveString(const std::string& key, const std::string& value);
	bool GetSaveString(const std::string& key, std::string& out);
	bool HasSaveKey(const std::string& key);
	void DeleteAllSaveData();

	// GameObject > Samples (EditorSuite_Samples.cpp)
	bool CreateSurvivorsDemo();
	bool CancelPendingSpawn(uint32_t id);
	void ProcessPendingSpawns();
	void RetireScriptView(uint32_t id);
	void ClearRetiredScriptViews();

	void TriggerScriptBuild();
	void PollScriptSourceWatcher(float deltaTime);
	void DrainScriptBuildLogs();
	void OpenFileInVisualStudio(const std::filesystem::path& filePath);
	std::filesystem::path ResolveGameScriptsDir() const;
	std::filesystem::path ResolveGameScriptsVcxproj() const;
	std::filesystem::path ResolveSolutionFilePath() const;
	std::string ResolveMsBuildExe();

	// Scene Management & Serialization (.scene.json)
	std::string SerializeSceneToString() const;
	bool DeserializeSceneFromString(const std::string& json);

	// Per-object / scene-settings JSON (shared by scene files and the undo system).
	// The void* parameters are SceneJson::Json* so this header does not depend on nlohmann/json.
	void WriteGameObjectJson(const GameObject& obj, void* outJson) const;
	void ReadGameObjectJson(const void* json, GameObject& obj);
	void WriteSceneSettingsJson(void* outJson) const;
	void ReadSceneSettingsJson(const void* json);
	bool SaveScene(const std::string& filePath);
	bool LoadScene(const std::string& filePath);
	void NewScene();
	void DiscardUnsavedChanges();
	const std::string& GetCurrentSceneName() const { return currentSceneName_; }
	bool IsSceneDirty() const { return isSceneDirty_; }
	void SetOnRequestTitleScene(std::function<void()> callback) { onRequestTitleScene_ = std::move(callback); }

	// Project Management & Native Folder Picker
	static std::string OpenNativeFolderDialog();
	bool CreateNewProject(const std::string& parentFolder, const std::string& projectName);
	bool OpenProject(const std::string& folderPath);

	// Project-relative asset paths: stored relative to the project root (or the working directory
	// when no project is open) and resolved back to a loadable path when the asset is loaded
	std::string ToProjectRelativePath(const std::filesystem::path& path) const;
	std::string ResolveAssetPath(const std::string& path) const;
	std::filesystem::path GetScenesDirectory() const;
	std::string NormalizeScenePath(const std::string& input) const;
	void ResetEditorHistory();
	bool IsPathInsideProject(const std::filesystem::path& path) const;

	// Project Hub (startup "create or open a project" popup) and recent-project list
	void DrawProjectHub();

	// Close confirmation: WinApp asks OnCloseRequested() on WM_CLOSE / File > Exit.
	// With unsaved scene changes it returns false and opens the Save / Don't Save / Cancel popup.
	bool OnCloseRequested();

	// Exported (standalone) games are recognised by the game_build.cfg the exporter writes next to the exe
	static constexpr const char* kExportedGameConfigFile = "game_build.cfg";
	static std::filesystem::path GetExecutableDirectory();
	static bool IsExportedGameFolder(const std::filesystem::path& dir);
	/// True when this process was started from an exported game folder (working dir or exe dir)
	static bool IsExportedGameLaunch();
	void DrawUnsavedChangesModal();
	bool showUnsavedCloseModal_ = false;
	std::string unsavedCloseError_;
	void LoadRecentProjects();
	void AddRecentProject(const std::filesystem::path& projectRoot);

	// Standalone Game Exporter & Preview Mode
	bool ExportStandaloneGame();
	bool IsStandaloneMode() const {
#ifdef USE_IMGUI
		return isStandaloneGameMode_ || isStandalonePreview_;
#else
		return true;
#endif
	}
	void SetStandaloneGameMode(bool enable) { isStandaloneGameMode_ = enable; }
	void ToggleStandalonePreview();

	// Undo / Redo (32-state in-memory ring buffer)
	/// Records what changed since the last recorded state as one undo command (call after every edit).
	void PushUndoSnapshot();
	void Undo();
	void Redo();
	/// Clears undo/redo and makes the current scene the new baseline (scene load, new scene, project switch).
	void ResetUndoHistory();

	// Built-in Playable 3D Tetris Template
	void LoadTetrisTemplate();
	void UpdateTetrisGame(float deltaTime, Input* input);
	void RestartTetrisGame();

#ifdef USE_IMGUI
	/// Headless self-tests (`--run-tests [filter]`, EditorSuite_Tests.cpp). Returns the number of failed tests.
	int RunSelfTests(const std::string& filter);
#endif
	/// Process exit code of a `--run-tests` run (0 = all passed, 1 = failures). Always 0 otherwise.
	static inline int sSelfTestExitCode = 0;

private:
	// Gives the self-tests (EditorSuite_Tests.cpp) access to private state
	friend struct EditorSuiteTestAccess;
	/// True while the self-tests run: nothing may touch the user's recent-project list
	bool isRunningTests_ = false;

	void UpdatePhysics(float deltaTime);
	void CalculateLightViewProjection();
	void RenderShadowPass(ID3D12GraphicsCommandList* cmdList);
	void RenderScene3D(ID3D12GraphicsCommandList* cmdList, Camera& camera, bool isEditView);
	void ApplyUnity6DarkTheme();
	// Height of the Play/Build toolbar below the main menu bar; panels are laid out under it
	static constexpr float kTopBarHeight = 40.0f;
	void DrawMainMenuBar();
	void DrawTopPlayBar();
	void DrawHierarchyPanel();
	void DrawInspectorAndDetailsPanel(bool& showParticles);

	// Removable Inspector components (Transform and Scripts are handled separately)
	enum class ComponentKind { Mesh, BoxCollider, SphereCollider, Rigidbody, Camera, Light, ParticleSystem, AudioSource, Mapchip };
	/// Component header with a close (x) button and a right-click "Remove Component" menu.
	/// A removal is only queued here and applied after the Inspector finished drawing.
	bool DrawComponentHeader(const char* label, ComponentKind kind);
	void RemoveComponent(GameObject& obj, ComponentKind kind);
	int pendingComponentRemoval_ = -1; // ComponentKind queued by DrawComponentHeader, -1 = none
	void DrawViewportsPanel();
	void DrawProjectAndConsolePanel();
	void DrawCppScriptGeneratorModal();
	void DrawNewProjectModal();
	void DrawOpenProjectModal();
	void DrawBuildSettingsModal();
	void DrawSaveSceneAsModal();
	void DrawOpenSceneModal();
	void DrawSkyboxSettingsUI();
	void RefreshAvailableCubemaps();
	void DrawTetrisHUD(const ImVec2& vpPos, const ImVec2& vpSize);
	void SelectAssetForPreview(const std::filesystem::path& assetPath);
	void ReleasePreviewResources();

	// Viewport Overlays and Gizmos
	void DrawSceneViewOverlays(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawFloatingToolStrip(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawOrientationCompass(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawFloatingBottomBar(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawTransformGizmo(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawColliderWireframes(const ImVec2& vpPos, const ImVec2& vpSize);
	void DrawCameraFrustum(const ImVec2& vpPos, const ImVec2& vpSize, const GameObject& camObj);
	void DrawLightGizmo(const ImVec2& vpPos, const ImVec2& vpSize, const GameObject& lightObj);

	// 3D Math Projection Helper
	bool WorldToScreen(const Vector3& worldPos, const Camera& cam, const ImVec2& vpPos, const ImVec2& vpSize, ImVec2& outScreenPos, float& outDepth);

	// Hazel-style Vec3 UI control with Reset buttons for X, Y, Z
	bool DrawVec3Control(const std::string& label, Vector3& values, float resetValue = 0.0f, float columnWidth = 85.0f);

	// Hierarchy Popup Menu
	void DrawHierarchyCreationMenu();

	// Mapchip Editor inside Inspector / Details
	void DrawMapchipInspectorSection();
	bool SaveMapchipCSV(const std::string& filePath);
	bool LoadMapchipCSV(const std::string& filePath);
	bool ExportMapchipJSON(const std::string& filePath);

	// Asset Browser File System helpers
	void DrawDirectoryTreeNode(const std::filesystem::path& dirPath);
	void RefreshAssetBrowser();

	// C++ Class Generator I/O
	bool GenerateCppScriptFiles(const std::string& className, const std::string& folderPath, bool includeCollision);

	void LogMessage(const std::string& msg, int logType = 0); // 0: Info, 1: Warning, 2: Error

private:
	DirectXCommon* dxCommon_ = nullptr;
	Camera* gameCamera_ = nullptr;
	Camera editorCamera_{};
	LightGroup* lights_ = nullptr;
	ParticleQuad* particleSystem_ = nullptr;
	Input* currentInput_ = nullptr;
	float currentDeltaTime_ = 0.016f;  // scaled by timeScale_ while the game runs
	float unscaledDeltaTime_ = 0.016f;
	float timeScale_ = 1.0f;           // set by scripts (SetTimeScale); reset when Play starts/stops
	double gameTime_ = 0.0;            // scaled seconds since Play started
	// Where the Game view image was drawn last frame (x, y, width, height in client pixels); 0 width = unknown
	float gameViewRect_[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	void RecordGameViewRect(float x, float y, float w, float h) {
		gameViewRect_[0] = x; gameViewRect_[1] = y; gameViewRect_[2] = w; gameViewRect_[3] = h;
	}
	// Mouse position inside the Game view in pixels (0,0 = top-left); false if the mouse is outside or the view is hidden
	bool GetGameViewMousePosition(float& outX, float& outY, float& outW, float& outH) const;
	// Intersects the game camera's ray through the mouse with the horizontal plane y = planeY
	bool GetMouseWorldOnPlane(float planeY, Vector3& outPoint) const;
public:
	// Pure helper (tested): ray through NDC (x right, y up) intersected with y = planeY
	static bool ScreenRayToPlaneY(const Matrix4x4& viewProjection, float ndcX, float ndcY, float planeY, Vector3& outPoint);
private:
	DXCManager dxc_{};
	uint32_t particleTextureHandle_ = 0;

	// Particle System Helpers
	void EnsureParticleQuadInitialized(GameObject& obj);
	void SyncGameObjectToParticleQuad(GameObject& obj);
	void ApplyParticlePreset(GameObject& obj, int presetIndex);

	RenderTexture editViewRT_{};
	RenderTexture gameViewRT_{};
	RenderTexture modelPreviewRT_{};
	Camera previewCamera_{};
	std::unique_ptr<LightGroup> previewLights_;
	std::unique_ptr<Model> previewModel_;
	std::unique_ptr<Object3d> previewObject3d_;
	std::string currentPreviewAssetPath_ = "";
	uint32_t currentPreviewTextureHandle_ = 0;
	uint32_t previewVertexCount_ = 0;
	uint32_t previewTriangleCount_ = 0;
	float previewModelScale_ = 1.0f;
	Vector3 previewModelOffset_ = { 0.0f, 0.0f, 0.0f };
	float previewRotationY_ = 0.0f;
	bool focusDetailsTab_ = false;
	Skybox skybox_{};
	bool showLightingSettings_ = false;
	bool showProfiler_ = false;
	void DrawProfilerWindow();
	int selectedCubemapIndex_ = 0;
	std::vector<std::string> availableCubemaps_;
	ShadowMap shadowMap_{};
	WorldTransform shadowWorldTransform_{};
	Matrix4x4 lightViewProjectionMatrix_{};

	EditorSceneState sceneState_ = EditorSceneState::Edit;

	// GameObjects
	std::vector<GameObject> gameObjects_;
	std::unordered_map<uint32_t, size_t> idIndexCache_; // see FindGameObjectIndexById
	int selectedObjectIndex_ = 0;
	uint32_t nextObjectId_ = 1;
	char hierarchySearchBuf_[128] = "";
	std::vector<int> pendingDeletes_;
	int pendingDuplicateIndex_ = -1;
	// Hierarchy cycle warning guard (one log per cycle set) and rigidbody-on-child warning guard
	bool hierarchyCycleWarned_ = false;
	std::unordered_set<uint32_t> childRigidbodyWarned_;

	// Objects instantiated by scripts are queued here and created at a safe point,
	// so gameObjects_ never reallocates while a script callback is running.
	struct PendingSpawn {
		uint32_t id = 0;
		std::string name;
		int primitiveType = 0;
		std::string prefabName;  // non-empty: instantiate this prefab instead of a primitive
		bool reusePooled = false; // id refers to an existing, despawned prefab instance
	};
	std::vector<PendingSpawn> pendingSpawns_;

	// Prefabs (EditorSuite_Prefabs.cpp). Files: <project>/Assets/Prefabs/<Name>.prefab.json
public:
	struct PrefabCache; // parsed prefab JSON by name (defined in the .cpp)
private:
	std::shared_ptr<PrefabCache> prefabCache_;
	std::unordered_map<std::string, std::vector<uint32_t>> prefabPools_; // despawned root ids per prefab
	std::vector<uint32_t> pendingDespawns_;

	// Runtime scene switching (EditorSuite_Runtime.cpp)
	std::string pendingSceneLoad_;
	bool runtimeSceneSwitched_ = false; // editor Play mode loaded another scene; Stop restores the origin
	std::string runtimeOriginScenePath_;
	std::string runtimeOriginSceneName_;
	std::string runtimeOriginSettings_;
	bool runtimeOriginDirty_ = false;

	// Save data (loaded lazily, written on change)
	void EnsureSaveDataLoaded();
	void WriteSaveData();
	std::string saveDataLoadedFrom_;
	std::map<std::string, double> saveNumbers_;
	std::map<std::string, std::string> saveStrings_;
	// Set while ProcessPendingSpawns() runs: skips undo snapshots, selection changes and per-object logs
	bool isSpawningFromScript_ = false;

	// Scene State & Serialization
	std::string currentSceneName_ = "SampleScene";
	std::string currentScenePath_ = "Assets/Scenes/SampleScene.scene.json";
	bool isSceneDirty_ = false;
	bool isEditingSceneName_ = false;
	char sceneNameEditBuf_[128] = "SampleScene";

	// Scene Modals
	bool showSaveSceneAsModal_ = false;
	char saveSceneAsPathBuf_[256] = "Assets/Scenes/SampleScene.scene.json";
	bool showOpenSceneModal_ = false;
	char openScenePathBuf_[256] = "Assets/Scenes/SampleScene.scene.json";
	std::function<void()> onRequestTitleScene_;

	// Undo / Redo System (32-state ring buffer storing JSON scene string)
	// ---- Command-based undo ----
	// A command stores only the GameObjects that were created / deleted / modified (as their scene JSON),
	// plus the scene settings (skybox, mapchip) when they changed. Undo/Redo patch just those objects.
	struct UndoObjectChange {
		uint32_t id = 0;
		int indexBefore = -1;   // position in gameObjects_ before the edit (-1: did not exist)
		int indexAfter = -1;    // position after the edit (-1: deleted)
		std::string before;     // JSON before (empty when created)
		std::string after;      // JSON after (empty when deleted)
	};
	struct UndoCommand {
		std::vector<UndoObjectChange> changes;
		std::string settingsBefore;  // empty when the scene settings did not change
		std::string settingsAfter;
		double timeSeconds = 0.0;    // when it was recorded (used to merge continuous drags)
	};
	struct UndoSceneState {
		std::vector<uint32_t> order;                          // object ids in gameObjects_ order
		std::unordered_map<uint32_t, std::string> objects;    // id -> object JSON
		std::string settings;                                 // skybox + mapchip JSON
	};
	UndoSceneState CaptureUndoState() const;
	void ApplyUndoCommand(const UndoCommand& cmd, bool undo);

	std::vector<UndoCommand> undoCommands_;
	std::vector<UndoCommand> redoCommands_;
	UndoSceneState undoBaseline_;
	bool undoMergeAllowed_ = false; // false right after Undo/Redo/reset so the next edit starts a new command
	bool undoPushPending_ = false;  // an edit was made that PushUndoSnapshot() has not recorded yet
	static constexpr size_t kMaxUndoCommands = 200;
	// Edits of the same objects closer together than this are merged into one command (e.g. one slider drag)
	static constexpr double kUndoMergeWindowSeconds = 0.6;

	// Standalone Game Exporter & Preview Mode
	bool isStandaloneGameMode_ = false;
	bool isStandalonePreview_ = false;
	bool showBuildSettingsModal_ = false;
	char buildOutputDirBuf_[512] = "Build";
	char buildGameTitleBuf_[128] = "RizealldayGame";
	int buildResolutionIndex_ = 0; // 0: 1920x1080, 1: 1280x720, 2: 2560x1440
	bool buildFullscreen_ = false;
	char buildStartupSceneBuf_[256] = "Assets/Scenes/SampleScene.scene.json";

	// New / Open Project Wizard State
	bool showNewProjectModal_ = false;
	char projectNameBuf_[128] = "NewGameProject";
	char projectFolderBuf_[512] = "D:\\Projects";
	bool showOpenProjectModal_ = false;
	char openProjectFolderBuf_[512] = "D:\\Projects\\NewGameProject";
	std::filesystem::path currentProjectRoot_ = "";
	std::string projectModalError_;
	bool showProjectHub_ = false;
	std::vector<std::filesystem::path> recentProjects_;
	int selectedRecentProject_ = -1;
	// Project of this editor session; EditorSuite is re-created on Title <-> Editor transitions
	static inline std::filesystem::path sSessionProjectRoot_;
	mutable std::unordered_map<std::string, std::string> resolvedAssetCache_;
	mutable std::filesystem::path resolvedAssetRoot_;

	// Physics State & Play Mode Snapshots
	struct TransformSnapshot {
		uint32_t id = 0;
		Vector3 translation = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		Vector3 scale = { 1.0f, 1.0f, 1.0f };
		Quaternion rotationQuat = Quaternion::Identity();
		Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		bool isGrounded = false;
	};
	std::vector<TransformSnapshot> playModeSnapshots_;
	std::vector<GameObject> playModeObjectSnapshots_;
	bool simulatePhysicsInEditMode_ = false;
	bool stepSingleFrame_ = false;

	// Viewport & Gizmo State
	GizmoType currentGizmo_ = GizmoType::Move;
	GizmoAxis activeGizmoAxis_ = GizmoAxis::None;
	bool isDraggingGizmo_ = false;
	ImVec2 gizmoDragStartMouse_{};
	Vector3 gizmoStartTranslation_{};
	Vector3 gizmoStartRotation_{};
	Vector3 gizmoStartScale_{};
	Vector3 gizmoStartLocalOffset_{};

	bool showGrid_ = true;
	bool showGizmos_ = true;
	bool showWireframe_ = false;
	bool enableLighting_ = true;
	bool enableFrustumCulling_ = true;
	int lastVisibleObjects_ = 0;
	int lastCulledObjects_ = 0;
	bool isEditingColliderBounds_ = false;

	// Navigation
	bool isFlightNavigating_ = false;
	ImVec2 prevMousePos_{};

	// Mapchip Data
	int mapRows_ = 10;
	int mapCols_ = 15;
	float mapTileSize_ = 1.0f;
	int selectedBrushTile_ = 1; // 0: Empty, 1: Wall, 2: Spike, 3: Spawn, 4: Goal
	std::vector<std::vector<int>> mapGrid_;
	char mapFilePathBuf_[256] = "Resources/levels/stage01.csv";
	char mapJsonPathBuf_[256] = "Resources/levels/stage01.json";
	bool renderMapchip3DBlocks_ = true;

	// C++ Script Generator & Hot Reload State
	ScriptModule scriptModule_{};
	EngineAPI engineApi_{};
	// Script views keyed by GameObject id. Heap-allocated so pointers handed to scripts stay valid
	// while gameObjects_ grows or shrinks. Views of deleted objects are retired (isActive = false)
	// instead of freed, and only released when no script can still hold them.
	std::unordered_map<uint32_t, std::unique_ptr<ScriptGameObjectView>> scriptObjectViews_;
	std::vector<std::unique_ptr<ScriptGameObjectView>> retiredScriptViews_;
	static constexpr size_t kMaxRetiredScriptViews = 4096;

	bool showScriptGenModal_ = false;
	int scriptGenTargetObjectIndex_ = -1; // >= 0 when opened via [Add Component -> Script -> New Script...]
	char scriptClassNameBuf_[128] = "PlayerController";
	char scriptOutputDirBuf_[256] = "GameScripts/Scripts";
	bool scriptIncludeCollision_ = true;
	std::string scriptGenErrorMsg_ = "";

	bool autoBuildScripts_ = true;
	std::atomic<bool> isBuildingScripts_{ false };
	std::atomic<bool> queuedScriptRebuild_{ false };
	std::atomic<int> lastScriptBuildStatus_{ 0 }; // 0: Idle/Ready, 1: Succeeded, 2: Failed
	std::thread scriptBuildThread_;
	std::mutex buildLogMutex_;
	struct PendingBuildLog {
		std::string line;
		int type = 0;
	};
	std::vector<PendingBuildLog> pendingBuildLogs_;
	char msbuildFallbackPathBuf_[512] = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe";
	std::string resolvedMsBuildPath_ = "";

	// Source File Watcher State (GameScripts/Scripts/*.cpp|*.h)
	float scriptSourcePollTimer_ = 0.0f;
	float scriptSourceDebounceTimer_ = 0.0f;
	bool scriptSourceChangePending_ = false;
	bool scriptSourceWatcherInitialized_ = false;
	std::map<std::string, std::filesystem::file_time_type> scriptSourceTimestamps_;

	// Asset Browser State
	std::filesystem::path currentDirectory_ = "Resources";
	std::filesystem::path selectedAssetPath_ = "";
	char assetSearchBuf_[128] = "";

	// Console State
	struct LogEntry {
		std::string message;
		int type = 0; // 0: Info, 1: Warning, 2: Error
		std::string timestamp;
	};
	std::vector<LogEntry> consoleLogs_;
	int infoCount_ = 0;
	int warningCount_ = 0;
	int errorCount_ = 0;
	bool collapseLogs_ = false;

	// Right panel active tab
	int rightPanelTab_ = 0; // 0: Inspector, 1: Details
	int bottomPanelTab_ = 0; // 0: Project, 1: Console

	// Built-in Playable 3D Tetris Game Mode
	bool isTetrisActive_ = false;
	int tetrisBoard_[20][10] = {}; // 0 = empty, 1..7 = tetromino block type
	uint32_t tetrisBlockIds_[20][10] = {}; // GameObject ID per grid cell
	int currentTetrominoType_ = 0; // 0: I, 1: O, 2: T, 3: S, 4: Z, 5: J, 6: L
	int currentTetrominoRot_ = 0;  // 0, 1, 2, 3
	int currentTetrominoX_ = 4;
	int currentTetrominoY_ = 18;
	float tetrisFallTimer_ = 0.0f;
	float tetrisFallInterval_ = 0.7f;
	int tetrisScore_ = 0;
	int tetrisLinesCleared_ = 0;
	int tetrisLevel_ = 1;
	bool tetrisGameOver_ = false;
	std::vector<uint32_t> currentPieceCubeIds_;
};
