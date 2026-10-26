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
	uint32_t layer = 0;              // 0: Default, 1: Player, 2: Enemy, 3: PlayerBullet, 4: Wall, 5: Trigger
	uint32_t collisionMask = 0xFFFFFFFF;
	GameObjectType type = GameObjectType::Empty;
	int parentIndex = -1;
	int parentId = -1;
	std::vector<uint32_t> childrenIds;

	// Transform Component
	Vector3 translation = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f }; // Radians
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Quaternion rotationQuat = Quaternion::Identity();

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
	GameObject* FindGameObjectById(uint32_t id, bool warnIfMissing = true) override;
	GameObject* FindGameObjectByName(const char* name, bool warnIfMissing = true) override;
	GameObject* FindGameObjectByName(const std::string& name, bool warnIfMissing = true) {
		return FindGameObjectByName(name.c_str(), warnIfMissing);
	}
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
	void PushUndoSnapshot();
	void Undo();
	void Redo();

	// Built-in Playable 3D Tetris Template
	void LoadTetrisTemplate();
	void UpdateTetrisGame(float deltaTime, Input* input);
	void RestartTetrisGame();

private:
	void UpdatePhysics(float deltaTime);
	void CalculateLightViewProjection();
	void RenderShadowPass(ID3D12GraphicsCommandList* cmdList);
	void RenderScene3D(ID3D12GraphicsCommandList* cmdList, Camera& camera, bool isEditView);
	void ApplyUnity6DarkTheme();
	void DrawMainMenuBar();
	void DrawTopPlayBar();
	void DrawHierarchyPanel();
	void DrawInspectorAndDetailsPanel(bool& showParticles);
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
	void DrawGroundGrid(const ImVec2& vpPos, const ImVec2& vpSize);
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
	float currentDeltaTime_ = 0.016f;
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
	int selectedCubemapIndex_ = 0;
	std::vector<std::string> availableCubemaps_;
	ShadowMap shadowMap_{};
	WorldTransform shadowWorldTransform_{};
	Matrix4x4 lightViewProjectionMatrix_{};

	EditorSceneState sceneState_ = EditorSceneState::Edit;

	// GameObjects
	std::vector<GameObject> gameObjects_;
	int selectedObjectIndex_ = 0;
	uint32_t nextObjectId_ = 1;
	char hierarchySearchBuf_[128] = "";
	std::vector<int> pendingDeletes_;
	int pendingDuplicateIndex_ = -1;

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
	std::vector<std::string> undoStack_;
	std::vector<std::string> redoStack_;
	const size_t maxUndoStates_ = 32;

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
	std::vector<ScriptGameObjectView> scriptObjectViews_;

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
