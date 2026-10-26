#include "EditorSuite.h"
#include <windows.h>
#include <shobjidl.h>
#include <RizeEngine.h>
#include "3d/Object3d.h"
#include "3d/Model.h"
#include "3d/LightGroup.h"
#include "3d/CollisionManager.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <ctime>
#include <iomanip>
#include <objbase.h>
#include <oleauto.h>
#include <shellapi.h>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

// Tetromino definitions: 7 shapes (I, O, T, S, Z, J, L) in a 4x4 coordinate space
static const int kTetrominoShapes[7][4][2] = {
	{ {0, 1}, {1, 1}, {2, 1}, {3, 1} }, // I (Cyan)
	{ {1, 1}, {2, 1}, {1, 2}, {2, 2} }, // O (Yellow)
	{ {1, 1}, {0, 1}, {2, 1}, {1, 2} }, // T (Purple)
	{ {1, 1}, {2, 1}, {0, 2}, {1, 2} }, // S (Green)
	{ {0, 1}, {1, 1}, {1, 2}, {2, 2} }, // Z (Red)
	{ {0, 2}, {0, 1}, {1, 1}, {2, 1} }, // J (Blue)
	{ {2, 2}, {0, 1}, {1, 1}, {2, 1} }  // L (Orange)
};

static const Vector4 kTetrominoColors[7] = {
	{ 0.15f, 0.85f, 0.95f, 1.0f }, // I: Cyan
	{ 0.95f, 0.85f, 0.15f, 1.0f }, // O: Yellow
	{ 0.70f, 0.20f, 0.85f, 1.0f }, // T: Purple
	{ 0.20f, 0.85f, 0.25f, 1.0f }, // S: Green
	{ 0.90f, 0.20f, 0.20f, 1.0f }, // Z: Red
	{ 0.20f, 0.40f, 0.95f, 1.0f }, // J: Blue
	{ 0.95f, 0.55f, 0.15f, 1.0f }  // L: Orange
};

GameObject::GameObject() = default;
GameObject::~GameObject() = default;

GameObject::GameObject(const GameObject& other) {
	id = other.id;
	name = other.name;
	isActive = other.isActive;
	isStatic = other.isStatic;
	tag = other.tag;
	layer = other.layer;
	collisionMask = other.collisionMask;
	type = other.type;
	parentIndex = other.parentIndex;
	parentId = other.parentId;
	childrenIds = other.childrenIds;
	translation = other.translation;
	rotation = other.rotation;
	scale = other.scale;
	rotationQuat = other.rotationQuat;
	hasMesh = other.hasMesh;
	meshEnabled = other.meshEnabled;
	meshType = other.meshType;
	materialColor = other.materialColor;
	textureHandle = other.textureHandle;
	isLit = other.isLit;
	lightingType = other.lightingType;
	shininess = other.shininess;
	specularColor = other.specularColor;
	useGeometryShader = other.useGeometryShader;
	geometryShaderEffect = other.geometryShaderEffect;
	explodeAmount = other.explodeAmount;
	castShadow = other.castShadow;
	receiveShadow = other.receiveShadow;
	texturePath = other.texturePath;
	modelPath = other.modelPath;
	uvOffset = other.uvOffset;
	uvTiling = other.uvTiling;
	uvRotation = other.uvRotation;
	animationIndex = other.animationIndex;
	animationSpeed = other.animationSpeed;
	animationLoop = other.animationLoop;
	animationPlaying = other.animationPlaying;
	hasBoxCollider = other.hasBoxCollider;
	boxColliderEnabled = other.boxColliderEnabled;
	isTrigger = other.isTrigger;
	providesContacts = other.providesContacts;
	boxCenter = other.boxCenter;
	boxSize = other.boxSize;
	isColliding = other.isColliding;
	hasSphereCollider = other.hasSphereCollider;
	sphereColliderEnabled = other.sphereColliderEnabled;
	sphereIsTrigger = other.sphereIsTrigger;
	sphereCenter = other.sphereCenter;
	sphereRadius = other.sphereRadius;
	hasCamera = other.hasCamera;
	cameraEnabled = other.cameraEnabled;
	fov = other.fov;
	nearClip = other.nearClip;
	farClip = other.farClip;
	isOrthographic = other.isOrthographic;
	hasLight = other.hasLight;
	lightEnabled = other.lightEnabled;
	lightType = other.lightType;
	lightColor = other.lightColor;
	lightIntensity = other.lightIntensity;
	lightDirection = other.lightDirection;
	lightRange = other.lightRange;
	lightDecay = other.lightDecay;
	lightSpotAngle = other.lightSpotAngle;
	lightSpotFalloffStart = other.lightSpotFalloffStart;
	lightCastShadow = other.lightCastShadow;
	lightShadowBias = other.lightShadowBias;
	lightShadowStrength = other.lightShadowStrength;
	lightShadowNormalBias = other.lightShadowNormalBias;
	hasParticleSystem = other.hasParticleSystem;
	attachToIndex = other.attachToIndex;
	attachedParentId = other.attachedParentId;
	localOffset = other.localOffset;
	particlePreset = other.particlePreset;
	particleShape = other.particleShape;
	particleSpawnRadius = other.particleSpawnRadius;
	particleBoxExtents = other.particleBoxExtents;
	particleConeAngle = other.particleConeAngle;
	particleSpawnRate = other.particleSpawnRate;
	particleMaxParticles = other.particleMaxParticles;
	particleBurstCount = other.particleBurstCount;
	particleDirection = other.particleDirection;
	particleSpreadAngle = other.particleSpreadAngle;
	particleSpeedMin = other.particleSpeedMin;
	particleSpeedMax = other.particleSpeedMax;
	particleGravity = other.particleGravity;
	particleDrag = other.particleDrag;
	particleWindEnabled = other.particleWindEnabled;
	particleWindAcceleration = other.particleWindAcceleration;
	particleLifetimeMin = other.particleLifetimeMin;
	particleLifetimeMax = other.particleLifetimeMax;
	particleStartScale = other.particleStartScale;
	particleEndScale = other.particleEndScale;
	particleStartColor = other.particleStartColor;
	particleEndColor = other.particleEndColor;
	particleBlendMode = other.particleBlendMode;
	particleBillboardMode = other.particleBillboardMode;
	particleQuad = nullptr;
	hasMapchip = other.hasMapchip;
	hasAudioSource = other.hasAudioSource;
	audioClipName = other.audioClipName;
	audioVolume = other.audioVolume;
	audioPlayOnAwake = other.audioPlayOnAwake;
	audioLoop = other.audioLoop;
	hasScript = other.hasScript;
	scriptClassName = other.scriptClassName;
	scripts = other.scripts;
	hasRigidbody = other.hasRigidbody;
	rbEnabled = other.rbEnabled;
	mass = other.mass;
	drag = other.drag;
	gravityScale = other.gravityScale;
	useGravity = other.useGravity;
	isKinematic = other.isKinematic;
	bounciness = other.bounciness;
	velocity = other.velocity;
	isGrounded = other.isGrounded;
}

GameObject::GameObject(GameObject&& other) noexcept = default;

GameObject& GameObject::operator=(const GameObject& other) {
	if (this != &other) {
		id = other.id;
		name = other.name;
		isActive = other.isActive;
		isStatic = other.isStatic;
		tag = other.tag;
		layer = other.layer;
		collisionMask = other.collisionMask;
		type = other.type;
		parentIndex = other.parentIndex;
		parentId = other.parentId;
		childrenIds = other.childrenIds;
		translation = other.translation;
		rotation = other.rotation;
		scale = other.scale;
		rotationQuat = other.rotationQuat;
		hasMesh = other.hasMesh;
		meshEnabled = other.meshEnabled;
		meshType = other.meshType;
		materialColor = other.materialColor;
		textureHandle = other.textureHandle;
		isLit = other.isLit;
		lightingType = other.lightingType;
		shininess = other.shininess;
		specularColor = other.specularColor;
		useGeometryShader = other.useGeometryShader;
		geometryShaderEffect = other.geometryShaderEffect;
		explodeAmount = other.explodeAmount;
		castShadow = other.castShadow;
		receiveShadow = other.receiveShadow;
		texturePath = other.texturePath;
		modelPath = other.modelPath;
		uvOffset = other.uvOffset;
		uvTiling = other.uvTiling;
		uvRotation = other.uvRotation;
		animationIndex = other.animationIndex;
		animationSpeed = other.animationSpeed;
		animationLoop = other.animationLoop;
		animationPlaying = other.animationPlaying;
		hasBoxCollider = other.hasBoxCollider;
		boxColliderEnabled = other.boxColliderEnabled;
		isTrigger = other.isTrigger;
		providesContacts = other.providesContacts;
		boxCenter = other.boxCenter;
		boxSize = other.boxSize;
		isColliding = other.isColliding;
		hasSphereCollider = other.hasSphereCollider;
		sphereColliderEnabled = other.sphereColliderEnabled;
		sphereIsTrigger = other.sphereIsTrigger;
		sphereCenter = other.sphereCenter;
		sphereRadius = other.sphereRadius;
		hasCamera = other.hasCamera;
		cameraEnabled = other.cameraEnabled;
		fov = other.fov;
		nearClip = other.nearClip;
		farClip = other.farClip;
		isOrthographic = other.isOrthographic;
		hasLight = other.hasLight;
		lightEnabled = other.lightEnabled;
		lightType = other.lightType;
		lightColor = other.lightColor;
		lightIntensity = other.lightIntensity;
		lightDirection = other.lightDirection;
		lightRange = other.lightRange;
		lightDecay = other.lightDecay;
		lightSpotAngle = other.lightSpotAngle;
		lightSpotFalloffStart = other.lightSpotFalloffStart;
		lightCastShadow = other.lightCastShadow;
		lightShadowBias = other.lightShadowBias;
		lightShadowStrength = other.lightShadowStrength;
		lightShadowNormalBias = other.lightShadowNormalBias;
		hasParticleSystem = other.hasParticleSystem;
		attachToIndex = other.attachToIndex;
		attachedParentId = other.attachedParentId;
		localOffset = other.localOffset;
		particlePreset = other.particlePreset;
		particleShape = other.particleShape;
		particleSpawnRadius = other.particleSpawnRadius;
		particleBoxExtents = other.particleBoxExtents;
		particleConeAngle = other.particleConeAngle;
		particleSpawnRate = other.particleSpawnRate;
		particleMaxParticles = other.particleMaxParticles;
		particleBurstCount = other.particleBurstCount;
		particleDirection = other.particleDirection;
		particleSpreadAngle = other.particleSpreadAngle;
		particleSpeedMin = other.particleSpeedMin;
		particleSpeedMax = other.particleSpeedMax;
		particleGravity = other.particleGravity;
		particleDrag = other.particleDrag;
		particleWindEnabled = other.particleWindEnabled;
		particleWindAcceleration = other.particleWindAcceleration;
		particleLifetimeMin = other.particleLifetimeMin;
		particleLifetimeMax = other.particleLifetimeMax;
		particleStartScale = other.particleStartScale;
		particleEndScale = other.particleEndScale;
		particleStartColor = other.particleStartColor;
		particleEndColor = other.particleEndColor;
		particleBlendMode = other.particleBlendMode;
		particleBillboardMode = other.particleBillboardMode;
		particleQuad = nullptr;
		hasMapchip = other.hasMapchip;
		hasAudioSource = other.hasAudioSource;
		audioClipName = other.audioClipName;
		audioVolume = other.audioVolume;
		audioPlayOnAwake = other.audioPlayOnAwake;
		audioLoop = other.audioLoop;
		hasScript = other.hasScript;
		scriptClassName = other.scriptClassName;
		scripts = other.scripts;
		hasRigidbody = other.hasRigidbody;
		rbEnabled = other.rbEnabled;
		mass = other.mass;
		drag = other.drag;
		gravityScale = other.gravityScale;
		useGravity = other.useGravity;
		isKinematic = other.isKinematic;
		bounciness = other.bounciness;
		velocity = other.velocity;
		isGrounded = other.isGrounded;
		editObject3d.reset();
		gameObject3d.reset();
		shadowTransform.reset();
		runtimeSoundHandle = 0;
		runtimeVoiceHandle = 0;
	}
	return *this;
}

GameObject& GameObject::operator=(GameObject&& other) noexcept = default;

EditorSuite::EditorSuite() = default;

EditorSuite::~EditorSuite()
{
	if (scriptBuildThread_.joinable()) {
		scriptBuildThread_.join();
	}
	DestroyAllScriptInstances();
	scriptModule_.Unload();
	Object3d::StaticSetShadowMap(nullptr);
	Object3d::StaticSetLightGroup(nullptr);
}

void EditorSuite::Initialize(
	DirectXCommon* dxCommon,
	Camera* gameCamera,
	LightGroup* lights,
	ParticleQuad* particleSystem)
{
	dxCommon_ = dxCommon;
	gameCamera_ = gameCamera;
	lights_ = lights;
	particleSystem_ = particleSystem;

	if (lights_) {
		Object3d::StaticSetLightGroup(lights_);
	}

	// If launched from an exported standalone game directory containing game_build.cfg, align working directory
	{
		char exePathBuf[MAX_PATH] = {};
		if (GetModuleFileNameA(NULL, exePathBuf, MAX_PATH) > 0) {
			std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
			if (!std::filesystem::exists("game_build.cfg") && std::filesystem::exists(exeDir / "game_build.cfg")) {
				std::error_code ec;
				std::filesystem::current_path(exeDir, ec);
			}
		}
	}

	// Ensure standard asset browser subfolders exist
	{
		std::error_code ec;
		std::filesystem::create_directories("Resources/Texture", ec);
		std::filesystem::create_directories("Resources/Model", ec);
		std::filesystem::create_directories("Resources/Sound", ec);
		std::filesystem::create_directories("Resources/levels", ec);
		std::filesystem::create_directories("shaders", ec);

		if (std::filesystem::exists("Resources/circle.png") && !std::filesystem::exists("Resources/Texture/circle.png")) {
			std::filesystem::copy_file("Resources/circle.png", "Resources/Texture/circle.png", std::filesystem::copy_options::overwrite_existing, ec);
		}
		if (!std::filesystem::exists("Resources/Model/cube.obj")) {
			std::ofstream objFile("Resources/Model/cube.obj");
			if (objFile.is_open()) {
				objFile << "# Sample 3D Cube Mesh\n";
				objFile << "v -0.5 -0.5  0.5\nv  0.5 -0.5  0.5\nv  0.5  0.5  0.5\nv -0.5  0.5  0.5\n";
				objFile << "v -0.5 -0.5 -0.5\nv  0.5 -0.5 -0.5\nv  0.5  0.5 -0.5\nv -0.5  0.5 -0.5\n";
				objFile << "vn  0.0  0.0  1.0\nvn  0.0  0.0 -1.0\nvn  0.0  1.0  0.0\n";
				objFile << "vn  0.0 -1.0  0.0\nvn  1.0  0.0  0.0\nvn -1.0  0.0  0.0\n";
				objFile << "vt 0.0 0.0\nvt 1.0 0.0\nvt 1.0 1.0\nvt 0.0 1.0\n";
				objFile << "f 1/1/1 2/2/1 3/3/1\nf 1/1/1 3/3/1 4/4/1\n";
				objFile << "f 6/1/2 5/2/2 8/3/2\nf 6/1/2 8/3/2 7/4/2\n";
				objFile << "f 4/1/3 3/2/3 7/3/3\nf 4/1/3 7/3/3 8/4/3\n";
				objFile << "f 5/1/4 6/2/4 2/3/4\nf 5/1/4 2/3/4 1/4/4\n";
				objFile << "f 2/1/5 6/2/5 7/3/5\nf 2/1/5 7/3/5 3/4/5\n";
				objFile << "f 5/1/6 1/2/6 4/3/6\nf 5/1/6 4/3/6 8/4/6\n";
			}
		}
	}

	dxc_.Initialize();
	particleTextureHandle_ = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");

	// Setup Editor Camera (Free navigation camera)
	editorCamera_.Initialize();
	editorCamera_.translation = { 0.0f, 3.5f, -10.0f };
	editorCamera_.rotation = { 0.25f, 0.0f, 0.0f };

	// Setup Offscreen RenderTargets for Scene, Game, and 3D Model Preview views
	const float editClearColor[4] = { 0.157f, 0.157f, 0.157f, 1.0f }; // Unity 6 background
	const float gameClearColor[4] = { 0.08f, 0.18f, 0.35f, 1.0f };
	const float previewClearColor[4] = { 0.14f, 0.15f, 0.18f, 1.0f };
	editViewRT_.Initialize(dxCommon_, 1920, 1080, 2, 1, 128, editClearColor);
	gameViewRT_.Initialize(dxCommon_, 1920, 1080, 3, 2, 129, gameClearColor);
	modelPreviewRT_.Initialize(dxCommon_, 256, 256, 4, 4, 132, previewClearColor);

	previewCamera_.Initialize();
	previewCamera_.fovY = 0.45f;
	previewCamera_.aspectRatio = 1.0f;
	previewCamera_.nearClip = 0.05f;
	previewCamera_.farClip = 500.0f;
	previewCamera_.translation = { 0.0f, 0.0f, -4.5f };
	previewCamera_.rotation = { 0.0f, 0.0f, 0.0f };
	previewCamera_.Update();

	previewLights_ = LightGroup::Create();
	previewLights_->SetDefaultLighting();
	previewLights_->SetAmbientColor({ 0.42f, 0.42f, 0.46f });
	previewLights_->SetAmbientIntensity(0.80f);
	previewLights_->GetDirectionalLight().direction = { 0.35f, -0.65f, 0.65f };
	previewLights_->GetDirectionalLight().color = { 1.0f, 0.98f, 0.95f, 1.0f };
	previewLights_->GetDirectionalLight().intensity = 1.15f;
	previewLights_->SetShadowActive(0);
	previewLights_->SetCameraPosition(previewCamera_.translation);
	previewLights_->Update();

	previewObject3d_ = std::make_unique<Object3d>();
	previewObject3d_->Initialize();
	previewObject3d_->SetCamera(&previewCamera_);
	if (Material* mat = previewObject3d_->GetMaterial()) {
		mat->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		mat->enableLighting = 1;
		mat->lightingType = 4;
		mat->shininess = 32.0f;
		mat->specularColor = { 0.6f, 0.6f, 0.6f };
		mat->receiveShadows = 0;
	}

	// Setup Directional Light Shadow Map (2048x2048 depth texture at DSV #3, SRV #130)
	shadowWorldTransform_.Initialize();
	shadowMap_.Initialize(dxCommon_, &dxc_, nullptr, 3, 130);
	Object3d::StaticSetShadowMap(&shadowMap_);

	// Setup Skybox & Environment
	TextureManager::CreateProceduralSkyboxDDS("Resources/skybox.dds");
	skybox_.Initialize(dxCommon_, &dxc_);
	RefreshAvailableCubemaps();

	// Initialize Mapchip Grid
	mapGrid_.assign(mapRows_, std::vector<int>(mapCols_, 0));
	for (int c = 0; c < mapCols_; ++c) {
		mapGrid_[mapRows_ - 1][c] = 1; // Floor
	}

	// Initialize GameScripts DLL hot-reload module and EngineAPI table
	SetupEngineAPI();
	scriptModule_.Initialize(&engineApi_);
	{
		std::filesystem::path scriptsFolder = ResolveGameScriptsDir() / "Scripts";
		strncpy_s(scriptOutputDirBuf_, scriptsFolder.generic_string().c_str(), sizeof(scriptOutputDirBuf_) - 1);
	}

	// Initialize Default Scene GameObjects first as baseline
	NewScene();

#ifdef USE_IMGUI
	ApplyUnity6DarkTheme();
#endif

	// Check if running as a standalone build export via game_build.cfg
	if (std::filesystem::exists("game_build.cfg")) {
		std::ifstream cfg("game_build.cfg");
		std::string line;
		std::string startupScene = "";
		std::string gameTitle = "";
		bool fullscreen = false;

		auto TrimStr = [](std::string s) {
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(s.begin());
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
			return s;
		};

		while (std::getline(cfg, line)) {
			size_t eqPos = line.find('=');
			if (eqPos == std::string::npos) continue;
			std::string key = TrimStr(line.substr(0, eqPos));
			std::string val = TrimStr(line.substr(eqPos + 1));

			if (key == "standaloneGameMode" && (val == "true" || val == "1")) {
				isStandaloneGameMode_ = true;
			} else if (key == "startupScene") {
				startupScene = val;
			} else if (key == "gameTitle") {
				gameTitle = val;
			} else if (key == "fullscreen" && (val == "true" || val == "1")) {
				fullscreen = true;
			}
		}
		cfg.close();

		if (isStandaloneGameMode_) {
			HWND activeHwnd = GetActiveWindow();
			if (!gameTitle.empty() && activeHwnd) {
				SetWindowTextA(activeHwnd, gameTitle.c_str());
			}
			if (fullscreen && activeHwnd) {
				ShowWindow(activeHwnd, SW_MAXIMIZE);
			}
			if (!startupScene.empty() && std::filesystem::exists(startupScene)) {
				LoadScene(startupScene);
			}
			StartPlayMode();
			LogMessage(std::format("[Standalone] Engine launched in Standalone Game Mode. Loaded {}", startupScene), 0);
			return;
		}
	}

	LogMessage("[EditorSuite] Initialized Unity 6 Dark Workspace & 5-Region Layout.", 0);
	LogMessage("[EditorSuite] Viewports Ready: # Scene (SRV #128) & Game (SRV #129).", 0);
}

GameObject& EditorSuite::CreateGameObject(const std::string& name, GameObjectType type)
{
	GameObject obj{};
	obj.id = nextObjectId_++;
	obj.name = name;
	obj.type = type;
	obj.isActive = true;
	obj.isStatic = false;
	obj.tag = "Untagged";
	obj.layer = 0;
	obj.collisionMask = 0xFFFFFFFF;
	obj.translation = { 0.0f, 0.0f, 0.0f };
	obj.rotation = { 0.0f, 0.0f, 0.0f };
	obj.scale = { 1.0f, 1.0f, 1.0f };
	obj.rotationQuat = Quaternion::Identity();

	gameObjects_.push_back(std::move(obj));
	selectedObjectIndex_ = static_cast<int>(gameObjects_.size()) - 1;
	isSceneDirty_ = true;
	PushUndoSnapshot();
	return gameObjects_.back();
}

GameObject& EditorSuite::CreateCube(const std::string& name, const Vector3& pos)
{
	auto& obj = CreateGameObject(name, GameObjectType::Cube);
	obj.translation = pos;
	obj.hasMesh = true;
	obj.meshType = GameObject::MeshType::Cube;
	obj.meshEnabled = true;
	obj.isLit = true;
	obj.lightingType = 4; // Blinn-Phong
	obj.shininess = 32.0f;
	obj.specularColor = { 1.0f, 1.0f, 1.0f };
	obj.materialColor = { 0.75f, 0.75f, 0.78f, 1.0f };

	// Automatically attach active Box Collider
	obj.hasBoxCollider = true;
	obj.boxColliderEnabled = true;
	obj.boxCenter = { 0.0f, 0.0f, 0.0f };
	obj.boxSize = { 1.0f, 1.0f, 1.0f };
	obj.isTrigger = false;
	obj.providesContacts = true;

	LogMessage(std::format("[Hierarchy] Created 3D Cube with Box Collider: {}", name), 0);
	return obj;
}

GameObject& EditorSuite::CreateSphere(const std::string& name, const Vector3& pos)
{
	auto& obj = CreateGameObject(name, GameObjectType::Sphere);
	obj.translation = pos;
	obj.hasMesh = true;
	obj.meshType = GameObject::MeshType::Sphere;
	obj.meshEnabled = true;
	obj.isLit = true;
	obj.lightingType = 4; // Blinn-Phong
	obj.shininess = 48.0f;
	obj.specularColor = { 1.0f, 1.0f, 1.0f };
	obj.materialColor = { 0.35f, 0.65f, 0.85f, 1.0f };

	// Automatically attach active Sphere Collider
	obj.hasSphereCollider = true;
	obj.sphereColliderEnabled = true;
	obj.sphereCenter = { 0.0f, 0.0f, 0.0f };
	obj.sphereRadius = 0.5f;

	LogMessage(std::format("[Hierarchy] Created 3D Sphere with Sphere Collider: {}", name), 0);
	return obj;
}

GameObject& EditorSuite::CreatePlane(const std::string& name, const Vector3& pos)
{
	auto& obj = CreateGameObject(name, GameObjectType::Plane);
	obj.translation = pos;
	obj.scale = { 10.0f, 1.0f, 10.0f };
	obj.hasMesh = true;
	obj.meshType = GameObject::MeshType::Plane;
	obj.meshEnabled = true;
	obj.isLit = true;
	obj.lightingType = 4;
	obj.shininess = 16.0f;
	obj.specularColor = { 0.5f, 0.5f, 0.5f };
	obj.materialColor = { 0.30f, 0.32f, 0.35f, 1.0f };
	obj.castShadow = false;
	obj.receiveShadow = true;

	// Automatically attach active Box Collider for floor bounds
	obj.hasBoxCollider = true;
	obj.boxColliderEnabled = true;
	obj.boxCenter = { 0.0f, -0.05f, 0.0f };
	obj.boxSize = { 10.0f, 0.1f, 10.0f };

	LogMessage(std::format("[Hierarchy] Created 3D Plane with Box Collider: {}", name), 0);
	return obj;
}

GameObject& EditorSuite::CreateQuad(const std::string& name, const Vector3& pos)
{
	auto& obj = CreateGameObject(name, GameObjectType::Quad);
	obj.translation = pos;
	obj.scale = { 1.0f, 1.0f, 1.0f };
	obj.hasMesh = true;
	obj.meshType = GameObject::MeshType::Quad;
	obj.meshEnabled = true;
	obj.isLit = true;
	obj.lightingType = 0; // Unlit by default for textured quads/sprites
	obj.materialColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	obj.texturePath = "Resources/circle.png";
	obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);

	LogMessage(std::format("[Hierarchy] Created Textured Quad: {}", name), 0);
	return obj;
}

void EditorSuite::DeleteSelectedGameObject()
{
	if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
		QueueDeleteGameObject(selectedObjectIndex_);
	}
}

void EditorSuite::QueueDeleteGameObject(int index)
{
	if (index >= 0 && index < static_cast<int>(gameObjects_.size())) {
		if (std::find(pendingDeletes_.begin(), pendingDeletes_.end(), index) == pendingDeletes_.end()) {
			pendingDeletes_.push_back(index);
		}
	}
}

GameObject* EditorSuite::FindGameObjectById(uint32_t id, bool warnIfMissing)
{
	for (auto& obj : gameObjects_) {
		if (obj.id == id) {
			return &obj;
		}
	}
	if (warnIfMissing) {
		LogMessage(std::format("[Script] Warning: GameObject with ID {} was not found (may have been deleted).", id), 1);
	}
	return nullptr;
}

GameObject* EditorSuite::FindGameObjectByName(const char* name, bool warnIfMissing)
{
	if (!name || name[0] == '\0') {
		return nullptr;
	}
	for (auto& obj : gameObjects_) {
		if (obj.name == name) {
			return &obj;
		}
	}
	if (warnIfMissing) {
		LogMessage(std::format("[Script] Warning: GameObject '{}' was not found (may have been deleted).", name), 1);
	}
	return nullptr;
}

void EditorSuite::QueueDeleteGameObjectById(uint32_t id)
{
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (gameObjects_[i].id == id) {
			QueueDeleteGameObject(static_cast<int>(i));
			return;
		}
	}
	LogMessage(std::format("[Script] Warning: Cannot delete GameObject with ID {} (already deleted or not found).", id), 1);
}

void EditorSuite::ProcessPendingDeletes()
{
	// 1. Process deferred duplication if requested
	if (pendingDuplicateIndex_ >= 0 && pendingDuplicateIndex_ < static_cast<int>(gameObjects_.size())) {
		for (auto& sc : gameObjects_[pendingDuplicateIndex_].scripts) {
			CaptureScriptFieldsToSaved(sc);
		}
		GameObject clone = gameObjects_[pendingDuplicateIndex_];
		clone.id = nextObjectId_++;
		clone.name += " (Copy)";
		clone.translation.x += 1.0f;
		clone.childrenIds.clear();
		clone.parentIndex = -1;
		clone.parentId = -1;
		gameObjects_.push_back(std::move(clone));
		selectedObjectIndex_ = static_cast<int>(gameObjects_.size()) - 1;
		RefreshAllScriptObjectViews();
		EnsureEditModeScriptInstances();
		isSceneDirty_ = true;
		PushUndoSnapshot();
		pendingDuplicateIndex_ = -1;
	}

	if (pendingDeletes_.empty()) {
		return;
	}

	// 2. Recursive Children Deletion (Unity-style):
	// Transitively collect all children and descendants of deleted objects
	std::unordered_set<int> deleteIndices;
	for (int idx : pendingDeletes_) {
		if (idx >= 0 && idx < static_cast<int>(gameObjects_.size())) {
			deleteIndices.insert(idx);
		}
	}

	bool addedChild = true;
	while (addedChild) {
		addedChild = false;
		for (size_t i = 0; i < gameObjects_.size(); ++i) {
			if (deleteIndices.count(static_cast<int>(i)) > 0) continue;
			const auto& cand = gameObjects_[i];
			for (int delIdx : deleteIndices) {
				const auto& parent = gameObjects_[delIdx];
				bool isDescendant = false;
				if (cand.parentIndex == delIdx) {
					isDescendant = true;
				} else if (cand.parentId != -1 && cand.parentId == static_cast<int>(parent.id)) {
					isDescendant = true;
				} else {
					for (uint32_t cid : parent.childrenIds) {
						if (cand.id == cid) {
							isDescendant = true;
							break;
						}
					}
				}
				if (isDescendant) {
					deleteIndices.insert(static_cast<int>(i));
					addedChild = true;
					break;
				}
			}
		}
	}

	if (deleteIndices.empty()) {
		pendingDeletes_.clear();
		return;
	}

	// 3. Record all deleted IDs and log
	std::unordered_set<uint32_t> deletedIds;
	for (int delIdx : deleteIndices) {
		deletedIds.insert(gameObjects_[delIdx].id);
		LogMessage(std::format("[Hierarchy] Deleted GameObject: {}", gameObjects_[delIdx].name), 0);
	}

	// 4. Call OnDestroy() on script instances (in Play mode), release GPU resources, and clear collision references
	RefreshAllScriptObjectViews();
	const bool inPlay = (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode());
	for (int delIdx : deleteIndices) {
		auto& obj = gameObjects_[delIdx];
		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				if (inPlay) {
					ScriptGameObjectView* view = GetOrUpdateScriptViewById(obj.id);
					sc.instance->SetOwner(view, obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					sc.instance->OnDestroy();
					if (view && view->dirtyFromScript) {
						SyncViewToGameObject(*view, obj);
					}
				}
				sc.instance.reset();
			}
		}
		if (obj.editObject3d) {
			obj.editObject3d.reset();
		}
		if (obj.gameObject3d) {
			obj.gameObject3d.reset();
		}
		if (obj.shadowTransform) {
			obj.shadowTransform.reset();
		}
		if (obj.runtimeVoiceHandle != 0) {
			Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
			obj.runtimeVoiceHandle = 0;
		}
		if (obj.particleQuad) {
			obj.particleQuad.reset();
		}
	}
	CollisionManager::GetInstance()->Clear();

	// 5. Erase from highest index to lowest index so remaining indices remain valid during erasure
	std::vector<int> sortedDescending(deleteIndices.begin(), deleteIndices.end());
	std::sort(sortedDescending.rbegin(), sortedDescending.rend());
	for (int delIdx : sortedDescending) {
		gameObjects_.erase(gameObjects_.begin() + delIdx);
	}

	// 6. Sanitize selectedObjectIndex_ and Gizmo state
	std::vector<int> sortedAscending(deleteIndices.begin(), deleteIndices.end());
	std::sort(sortedAscending.begin(), sortedAscending.end());

	if (deleteIndices.count(selectedObjectIndex_) > 0) {
		selectedObjectIndex_ = -1;
		isDraggingGizmo_ = false;
		activeGizmoAxis_ = GizmoAxis::None;
		isEditingColliderBounds_ = false;
	} else if (selectedObjectIndex_ >= 0) {
		int countBelow = 0;
		for (int d : sortedAscending) {
			if (d < selectedObjectIndex_) countBelow++;
		}
		selectedObjectIndex_ -= countBelow;
		if (selectedObjectIndex_ >= static_cast<int>(gameObjects_.size())) {
			selectedObjectIndex_ = static_cast<int>(gameObjects_.size()) - 1;
		}
	}

	// 7. Sanitize all remaining GameObjects and refresh script owner pointers
	RefreshAllScriptObjectViews();
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		auto& obj = gameObjects_[i];

		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
				sc.instance->SetEngineAPI(&engineApi_);
				sc.instance->SetHost(this, this);
			}
		}

		// Parent reference sanitization
		if (obj.parentId != -1 && deletedIds.count(static_cast<uint32_t>(obj.parentId)) > 0) {
			obj.parentIndex = -1;
			obj.parentId = -1;
		} else if (obj.parentIndex >= 0) {
			if (deleteIndices.count(obj.parentIndex) > 0) {
				obj.parentIndex = -1;
				obj.parentId = -1;
			} else {
				int countBelow = 0;
				for (int d : sortedAscending) {
					if (d < obj.parentIndex) countBelow++;
				}
				obj.parentIndex -= countBelow;
				if (obj.parentIndex >= 0 && obj.parentIndex < static_cast<int>(gameObjects_.size())) {
					obj.parentId = static_cast<int>(gameObjects_[obj.parentIndex].id);
				} else {
					obj.parentIndex = -1;
					obj.parentId = -1;
				}
			}
		} else if (obj.parentId != -1) {
			// Resync parentIndex from parentId
			obj.parentIndex = -1;
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(obj.parentId)) {
					obj.parentIndex = static_cast<int>(p);
					break;
				}
			}
			if (obj.parentIndex == -1) {
				obj.parentId = -1;
			}
		}

		// Particle parent attachment sanitization
		if (obj.attachedParentId != -1 && deletedIds.count(static_cast<uint32_t>(obj.attachedParentId)) > 0) {
			obj.attachToIndex = -1;
			obj.attachedParentId = -1;
			obj.localOffset = { 0.0f, 0.0f, 0.0f };
		} else if (obj.attachToIndex >= 0) {
			if (deleteIndices.count(obj.attachToIndex) > 0) {
				obj.attachToIndex = -1;
				obj.attachedParentId = -1;
				obj.localOffset = { 0.0f, 0.0f, 0.0f };
			} else {
				int countBelow = 0;
				for (int d : sortedAscending) {
					if (d < obj.attachToIndex) countBelow++;
				}
				obj.attachToIndex -= countBelow;
				if (obj.attachToIndex >= 0 && obj.attachToIndex < static_cast<int>(gameObjects_.size())) {
					obj.attachedParentId = static_cast<int>(gameObjects_[obj.attachToIndex].id);
				} else {
					obj.attachToIndex = -1;
					obj.attachedParentId = -1;
					obj.localOffset = { 0.0f, 0.0f, 0.0f };
				}
			}
		} else if (obj.attachedParentId != -1) {
			// Resync attachToIndex from attachedParentId
			obj.attachToIndex = -1;
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(obj.attachedParentId)) {
					obj.attachToIndex = static_cast<int>(p);
					break;
				}
			}
			if (obj.attachToIndex == -1) {
				obj.attachedParentId = -1;
				obj.localOffset = { 0.0f, 0.0f, 0.0f };
			}
		}

		// Filter childrenIds
		obj.childrenIds.erase(
			std::remove_if(obj.childrenIds.begin(), obj.childrenIds.end(),
				[&](uint32_t cid) { return deletedIds.count(cid) > 0; }),
			obj.childrenIds.end()
		);
	}

	// 8. Sanitize tetris grid and piece cubes
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 10; ++x) {
			if (deletedIds.count(tetrisBlockIds_[y][x]) > 0) {
				tetrisBlockIds_[y][x] = 0;
				tetrisBoard_[y][x] = 0;
			}
		}
	}
	currentPieceCubeIds_.erase(
		std::remove_if(currentPieceCubeIds_.begin(), currentPieceCubeIds_.end(),
			[&](uint32_t cid) { return deletedIds.count(cid) > 0; }),
		currentPieceCubeIds_.end()
	);

	pendingDeletes_.clear();
	if (sceneState_ == EditorSceneState::Edit) {
		playModeSnapshots_.erase(
			std::remove_if(playModeSnapshots_.begin(), playModeSnapshots_.end(),
				[&](const TransformSnapshot& snap) { return deletedIds.count(snap.id) > 0; }),
			playModeSnapshots_.end()
		);
		isSceneDirty_ = true;
		PushUndoSnapshot();
	}
}

std::string EditorSuite::SerializeSceneToString() const
{
	std::ostringstream ss;
	ss << "{\n";
	ss << "  \"sceneName\": \"" << currentSceneName_ << "\",\n";
	ss << "  \"skybox\": {\n";
	ss << "    \"mode\": " << static_cast<int>(skybox_.GetMode()) << ",\n";
	ss << "    \"topColor\": [" << skybox_.GetTopColor().x << ", " << skybox_.GetTopColor().y << ", " << skybox_.GetTopColor().z << ", " << skybox_.GetTopColor().w << "],\n";
	ss << "    \"horizonColor\": [" << skybox_.GetHorizonColor().x << ", " << skybox_.GetHorizonColor().y << ", " << skybox_.GetHorizonColor().z << ", " << skybox_.GetHorizonColor().w << "],\n";
	ss << "    \"bottomColor\": [" << skybox_.GetBottomColor().x << ", " << skybox_.GetBottomColor().y << ", " << skybox_.GetBottomColor().z << ", " << skybox_.GetBottomColor().w << "],\n";
	ss << "    \"solidColor\": [" << skybox_.GetSolidColor().x << ", " << skybox_.GetSolidColor().y << ", " << skybox_.GetSolidColor().z << ", " << skybox_.GetSolidColor().w << "],\n";
	ss << "    \"cubemapPath\": \"" << skybox_.GetCubemapPath() << "\",\n";
	ss << "    \"rotation\": " << skybox_.GetRotation() << ",\n";
	ss << "    \"exposure\": " << skybox_.GetExposure() << "\n";
	ss << "  },\n";
	ss << "  \"gameObjects\": [\n";

	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		const auto& obj = gameObjects_[i];
		ss << "    {\n";
		ss << "      \"id\": " << obj.id << ",\n";
		ss << "      \"name\": \"" << obj.name << "\",\n";
		ss << "      \"type\": " << static_cast<int>(obj.type) << ",\n";
		ss << "      \"isActive\": " << (obj.isActive ? "true" : "false") << ",\n";
		ss << "      \"isStatic\": " << (obj.isStatic ? "true" : "false") << ",\n";
		ss << "      \"tag\": \"" << obj.tag << "\",\n";
		ss << "      \"layer\": " << obj.layer << ",\n";
		ss << "      \"parentId\": " << obj.parentId << ",\n";
		ss << "      \"translation\": [" << obj.translation.x << ", " << obj.translation.y << ", " << obj.translation.z << "],\n";
		ss << "      \"rotation\": [" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << "],\n";
		ss << "      \"rotationQuat\": [" << obj.rotationQuat.x << ", " << obj.rotationQuat.y << ", " << obj.rotationQuat.z << ", " << obj.rotationQuat.w << "],\n";
		ss << "      \"scale\": [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "],\n";

		// Mesh
		ss << "      \"hasMesh\": " << (obj.hasMesh ? "true" : "false") << ",\n";
		ss << "      \"meshEnabled\": " << (obj.meshEnabled ? "true" : "false") << ",\n";
		ss << "      \"meshType\": " << static_cast<int>(obj.meshType) << ",\n";
		ss << "      \"materialColor\": [" << obj.materialColor.x << ", " << obj.materialColor.y << ", " << obj.materialColor.z << ", " << obj.materialColor.w << "],\n";
		ss << "      \"isLit\": " << (obj.isLit ? "true" : "false") << ",\n";
		ss << "      \"lightingType\": " << obj.lightingType << ",\n";
		ss << "      \"shininess\": " << obj.shininess << ",\n";
		ss << "      \"specularColor\": [" << obj.specularColor.x << ", " << obj.specularColor.y << ", " << obj.specularColor.z << "],\n";
		ss << "      \"castShadow\": " << (obj.castShadow ? "true" : "false") << ",\n";
		ss << "      \"receiveShadow\": " << (obj.receiveShadow ? "true" : "false") << ",\n";
		ss << "      \"useGeometryShader\": " << (obj.useGeometryShader ? "true" : "false") << ",\n";
		ss << "      \"geometryShaderEffect\": " << obj.geometryShaderEffect << ",\n";
		ss << "      \"explodeAmount\": " << obj.explodeAmount << ",\n";
		ss << "      \"texturePath\": \"" << obj.texturePath << "\",\n";
		ss << "      \"modelPath\": \"" << obj.modelPath << "\",\n";
		ss << "      \"uvOffset\": [" << obj.uvOffset.x << ", " << obj.uvOffset.y << "],\n";
		ss << "      \"uvTiling\": [" << obj.uvTiling.x << ", " << obj.uvTiling.y << "],\n";
		ss << "      \"uvRotation\": " << obj.uvRotation << ",\n";

		// Colliders
		ss << "      \"hasBoxCollider\": " << (obj.hasBoxCollider ? "true" : "false") << ",\n";
		ss << "      \"boxColliderEnabled\": " << (obj.boxColliderEnabled ? "true" : "false") << ",\n";
		ss << "      \"isTrigger\": " << (obj.isTrigger ? "true" : "false") << ",\n";
		ss << "      \"providesContacts\": " << (obj.providesContacts ? "true" : "false") << ",\n";
		ss << "      \"boxCenter\": [" << obj.boxCenter.x << ", " << obj.boxCenter.y << ", " << obj.boxCenter.z << "],\n";
		ss << "      \"boxSize\": [" << obj.boxSize.x << ", " << obj.boxSize.y << ", " << obj.boxSize.z << "],\n";
		ss << "      \"hasSphereCollider\": " << (obj.hasSphereCollider ? "true" : "false") << ",\n";
		ss << "      \"sphereColliderEnabled\": " << (obj.sphereColliderEnabled ? "true" : "false") << ",\n";
		ss << "      \"sphereIsTrigger\": " << (obj.sphereIsTrigger ? "true" : "false") << ",\n";
		ss << "      \"sphereCenter\": [" << obj.sphereCenter.x << ", " << obj.sphereCenter.y << ", " << obj.sphereCenter.z << "],\n";
		ss << "      \"sphereRadius\": " << obj.sphereRadius << ",\n";

		// Rigidbody
		ss << "      \"hasRigidbody\": " << (obj.hasRigidbody ? "true" : "false") << ",\n";
		ss << "      \"rbEnabled\": " << (obj.rbEnabled ? "true" : "false") << ",\n";
		ss << "      \"mass\": " << obj.mass << ",\n";
		ss << "      \"drag\": " << obj.drag << ",\n";
		ss << "      \"gravityScale\": " << obj.gravityScale << ",\n";
		ss << "      \"useGravity\": " << (obj.useGravity ? "true" : "false") << ",\n";
		ss << "      \"isKinematic\": " << (obj.isKinematic ? "true" : "false") << ",\n";
		ss << "      \"bounciness\": " << obj.bounciness << ",\n";

		// Camera
		ss << "      \"hasCamera\": " << (obj.hasCamera ? "true" : "false") << ",\n";
		ss << "      \"cameraEnabled\": " << (obj.cameraEnabled ? "true" : "false") << ",\n";
		ss << "      \"fov\": " << obj.fov << ",\n";
		ss << "      \"nearClip\": " << obj.nearClip << ",\n";
		ss << "      \"farClip\": " << obj.farClip << ",\n";
		ss << "      \"isOrthographic\": " << (obj.isOrthographic ? "true" : "false") << ",\n";

		// Light
		ss << "      \"hasLight\": " << (obj.hasLight ? "true" : "false") << ",\n";
		ss << "      \"lightEnabled\": " << (obj.lightEnabled ? "true" : "false") << ",\n";
		ss << "      \"lightType\": " << static_cast<int>(obj.lightType) << ",\n";
		ss << "      \"lightColor\": [" << obj.lightColor.x << ", " << obj.lightColor.y << ", " << obj.lightColor.z << "],\n";
		ss << "      \"lightIntensity\": " << obj.lightIntensity << ",\n";
		ss << "      \"lightDirection\": [" << obj.lightDirection.x << ", " << obj.lightDirection.y << ", " << obj.lightDirection.z << "],\n";
		ss << "      \"lightRange\": " << obj.lightRange << ",\n";
		ss << "      \"lightDecay\": " << obj.lightDecay << ",\n";
		ss << "      \"lightSpotAngle\": " << obj.lightSpotAngle << ",\n";
		ss << "      \"lightSpotFalloffStart\": " << obj.lightSpotFalloffStart << ",\n";
		ss << "      \"lightCastShadow\": " << (obj.lightCastShadow ? "true" : "false") << ",\n";
		ss << "      \"lightShadowBias\": " << obj.lightShadowBias << ",\n";
		ss << "      \"lightShadowStrength\": " << obj.lightShadowStrength << ",\n";
		ss << "      \"lightShadowNormalBias\": " << obj.lightShadowNormalBias << ",\n";

		// Particle System
		ss << "      \"hasParticleSystem\": " << (obj.hasParticleSystem ? "true" : "false") << ",\n";
		ss << "      \"attachedParentId\": " << obj.attachedParentId << ",\n";
		ss << "      \"localOffset\": [" << obj.localOffset.x << ", " << obj.localOffset.y << ", " << obj.localOffset.z << "],\n";
		ss << "      \"particlePreset\": " << obj.particlePreset << ",\n";
		ss << "      \"particleShape\": " << obj.particleShape << ",\n";
		ss << "      \"particleSpawnRadius\": " << obj.particleSpawnRadius << ",\n";
		ss << "      \"particleBoxExtents\": [" << obj.particleBoxExtents.x << ", " << obj.particleBoxExtents.y << ", " << obj.particleBoxExtents.z << "],\n";
		ss << "      \"particleConeAngle\": " << obj.particleConeAngle << ",\n";
		ss << "      \"particleSpawnRate\": " << obj.particleSpawnRate << ",\n";
		ss << "      \"particleMaxParticles\": " << obj.particleMaxParticles << ",\n";
		ss << "      \"particleBurstCount\": " << obj.particleBurstCount << ",\n";
		ss << "      \"particleDirection\": [" << obj.particleDirection.x << ", " << obj.particleDirection.y << ", " << obj.particleDirection.z << "],\n";
		ss << "      \"particleSpreadAngle\": " << obj.particleSpreadAngle << ",\n";
		ss << "      \"particleSpeedMin\": " << obj.particleSpeedMin << ",\n";
		ss << "      \"particleSpeedMax\": " << obj.particleSpeedMax << ",\n";
		ss << "      \"particleGravity\": " << obj.particleGravity << ",\n";
		ss << "      \"particleDrag\": " << obj.particleDrag << ",\n";
		ss << "      \"particleWindEnabled\": " << (obj.particleWindEnabled ? "true" : "false") << ",\n";
		ss << "      \"particleWindAcceleration\": [" << obj.particleWindAcceleration.x << ", " << obj.particleWindAcceleration.y << ", " << obj.particleWindAcceleration.z << "],\n";
		ss << "      \"particleLifetimeMin\": " << obj.particleLifetimeMin << ",\n";
		ss << "      \"particleLifetimeMax\": " << obj.particleLifetimeMax << ",\n";
		ss << "      \"particleStartScale\": " << obj.particleStartScale << ",\n";
		ss << "      \"particleEndScale\": " << obj.particleEndScale << ",\n";
		ss << "      \"particleStartColor\": [" << obj.particleStartColor.x << ", " << obj.particleStartColor.y << ", " << obj.particleStartColor.z << ", " << obj.particleStartColor.w << "],\n";
		ss << "      \"particleEndColor\": [" << obj.particleEndColor.x << ", " << obj.particleEndColor.y << ", " << obj.particleEndColor.z << ", " << obj.particleEndColor.w << "],\n";
		ss << "      \"particleBlendMode\": " << obj.particleBlendMode << ",\n";
		ss << "      \"particleBillboardMode\": " << obj.particleBillboardMode << ",\n";

		// Mapchip, Audio, Animation & Scripts
		ss << "      \"hasMapchip\": " << (obj.hasMapchip ? "true" : "false") << ",\n";
		ss << "      \"hasAudioSource\": " << (obj.hasAudioSource ? "true" : "false") << ",\n";
		ss << "      \"audioClipName\": \"" << obj.audioClipName << "\",\n";
		ss << "      \"audioVolume\": " << obj.audioVolume << ",\n";
		ss << "      \"audioPlayOnAwake\": " << (obj.audioPlayOnAwake ? "true" : "false") << ",\n";
		ss << "      \"audioLoop\": " << (obj.audioLoop ? "true" : "false") << ",\n";
		ss << "      \"animationIndex\": " << obj.animationIndex << ",\n";
		ss << "      \"animationSpeed\": " << obj.animationSpeed << ",\n";
		ss << "      \"animationLoop\": " << (obj.animationLoop ? "true" : "false") << ",\n";
		ss << "      \"animationPlaying\": " << (obj.animationPlaying ? "true" : "false") << ",\n";
		ss << "      \"hasScript\": " << ((!obj.scripts.empty() || obj.hasScript) ? "true" : "false") << ",\n";
		ss << "      \"scriptClassName\": \"" << (!obj.scripts.empty() ? obj.scripts.front().className : obj.scriptClassName) << "\",\n";
		ss << "      \"scripts\": [";
		for (size_t sIdx = 0; sIdx < obj.scripts.size(); ++sIdx) {
			ss << "\"" << obj.scripts[sIdx].className << "\"" << (sIdx + 1 < obj.scripts.size() ? ", " : "");
		}
		ss << "],\n";
		ss << "      \"scriptComponents\": [";
		for (size_t sIdx = 0; sIdx < obj.scripts.size(); ++sIdx) {
			const auto& sc = obj.scripts[sIdx];
			std::map<std::string, GameObject::ScriptSavedFieldValue> fieldsToWrite = sc.savedFields;
			if (sc.instance) {
				int fCount = scriptModule_.GetFieldCount(sc.instance.get());
				for (int fIdx = 0; fIdx < fCount; ++fIdx) {
					ScriptFieldInfo info{};
					if (!scriptModule_.GetFieldInfo(sc.instance.get(), fIdx, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;
					GameObject::ScriptSavedFieldValue val{};
					val.type = info.type;
					switch (info.type) {
					case ScriptFieldType::Float: val.floatVal = *static_cast<const float*>(info.dataPtr); break;
					case ScriptFieldType::Int: val.intVal = *static_cast<const int*>(info.dataPtr); break;
					case ScriptFieldType::Bool: val.boolVal = *static_cast<const bool*>(info.dataPtr); break;
					case ScriptFieldType::Vec3: val.vec3Val = *static_cast<const Vector3*>(info.dataPtr); break;
					case ScriptFieldType::CharBuffer: val.strVal = static_cast<const char*>(info.dataPtr); break;
					}
					fieldsToWrite[info.name] = val;
				}
			}

			ss << "{\"className\":\"" << sc.className << "\",\"fields\":[";
			size_t fWritten = 0;
			for (const auto& [fName, fVal] : fieldsToWrite) {
				if (fWritten > 0) ss << ",";
				ss << "{\"name\":\"" << fName << "\",\"type\":" << static_cast<int>(fVal.type);
				switch (fVal.type) {
				case ScriptFieldType::Float:
					ss << ",\"v0\":" << fVal.floatVal << "}";
					break;
				case ScriptFieldType::Int:
					ss << ",\"v0\":" << fVal.intVal << "}";
					break;
				case ScriptFieldType::Bool:
					ss << ",\"v0\":" << (fVal.boolVal ? 1 : 0) << "}";
					break;
				case ScriptFieldType::Vec3:
					ss << ",\"v0\":" << fVal.vec3Val.x << ",\"v1\":" << fVal.vec3Val.y << ",\"v2\":" << fVal.vec3Val.z << "}";
					break;
				case ScriptFieldType::CharBuffer: {
					std::string escaped;
					for (char ch : fVal.strVal) {
						if (ch == '\"' || ch == '\\') escaped.push_back('\\');
						escaped.push_back(ch);
					}
					ss << ",\"str\":\"" << escaped << "\"}";
					break;
				}
				}
				++fWritten;
			}
			ss << "]}" << (sIdx + 1 < obj.scripts.size() ? "," : "");
		}
		ss << "]\n";

		ss << "    }" << (i < gameObjects_.size() - 1 ? "," : "") << "\n";
	}

	ss << "  ],\n";

	// Mapchip Data
	ss << "  \"mapchip\": {\n";
	ss << "    \"rows\": " << mapRows_ << ",\n";
	ss << "    \"cols\": " << mapCols_ << ",\n";
	ss << "    \"tileSize\": " << mapTileSize_ << ",\n";
	ss << "    \"data\": [\n";
	for (int r = 0; r < mapRows_; ++r) {
		ss << "      [";
		for (int c = 0; c < mapCols_; ++c) {
			ss << mapGrid_[r][c] << (c < mapCols_ - 1 ? ", " : "");
		}
		ss << "]" << (r < mapRows_ - 1 ? "," : "") << "\n";
	}
	ss << "    ]\n";
	ss << "  }\n";

	ss << "}\n";
	return ss.str();
}

bool EditorSuite::DeserializeSceneFromString(const std::string& content)
{
	if (content.empty()) return false;

	// Destroy any active script instances before clearing scene objects
	RefreshAllScriptObjectViews();
	const bool inPlay = (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode());
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				if (inPlay) {
					sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					sc.instance->OnDestroy();
				}
				sc.instance.reset();
			}
		}
	}
	gameObjects_.clear();

	// Read sceneName with fallback to SampleScene
	size_t namePos = content.find("\"sceneName\": \"");
	if (namePos != std::string::npos) {
		size_t endPos = content.find("\"", namePos + 14);
		if (endPos != std::string::npos) {
			currentSceneName_ = content.substr(namePos + 14, endPos - (namePos + 14));
		}
	} else {
		currentSceneName_ = "SampleScene";
	}

	// Helper lambdas for robust fallback parsing
	auto GetStringField = [](const std::string& block, const std::string& key, std::string& outVal) {
		std::string token = "\"" + key + "\": \"";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			size_t ep = block.find("\"", p + token.length());
			if (ep != std::string::npos) outVal = block.substr(p + token.length(), ep - (p + token.length()));
		}
	};

	auto GetBoolField = [](const std::string& block, const std::string& key, bool& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			if (block.substr(p + token.length(), 4) == "true") outVal = true;
			else if (block.substr(p + token.length(), 5) == "false") outVal = false;
		}
	};

	auto GetIntField = [](const std::string& block, const std::string& key, int& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%d", &outVal);
		}
	};

	auto GetFloatField = [](const std::string& block, const std::string& key, float& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f", &outVal);
		}
	};

	auto GetVec2Field = [](const std::string& block, const std::string& key, Vector2& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f", &outVal.x, &outVal.y);
		}
	};

	auto GetVec3Field = [](const std::string& block, const std::string& key, Vector3& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f, %f", &outVal.x, &outVal.y, &outVal.z);
		}
	};

	auto GetVec4Field = [](const std::string& block, const std::string& key, Vector4& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f, %f, %f", &outVal.x, &outVal.y, &outVal.z, &outVal.w);
		}
	};

	// Parse Skybox block
	size_t skyboxPos = content.find("\"skybox\": {");
	if (skyboxPos != std::string::npos) {
		size_t skyboxEnd = content.find("}", skyboxPos);
		if (skyboxEnd != std::string::npos) {
			std::string skyBlock = content.substr(skyboxPos, skyboxEnd - skyboxPos + 1);
			int modeInt = 0;
			GetIntField(skyBlock, "mode", modeInt);
			skybox_.SetMode(static_cast<SkyboxMode>(modeInt));

			Vector4 top = skybox_.GetTopColor();
			GetVec4Field(skyBlock, "topColor", top);
			skybox_.SetTopColor(top);

			Vector4 horiz = skybox_.GetHorizonColor();
			GetVec4Field(skyBlock, "horizonColor", horiz);
			skybox_.SetHorizonColor(horiz);

			Vector4 bot = skybox_.GetBottomColor();
			GetVec4Field(skyBlock, "bottomColor", bot);
			skybox_.SetBottomColor(bot);

			Vector4 solid = skybox_.GetSolidColor();
			GetVec4Field(skyBlock, "solidColor", solid);
			skybox_.SetSolidColor(solid);

			std::string cubemap = skybox_.GetCubemapPath();
			GetStringField(skyBlock, "cubemapPath", cubemap);
			skybox_.SetCubemapPath(cubemap);
			if (skybox_.GetMode() == SkyboxMode::Cubemap) {
				skybox_.LoadCubemap(cubemap);
			}

			float rot = skybox_.GetRotation();
			GetFloatField(skyBlock, "rotation", rot);
			skybox_.SetRotation(rot);

			float exp = skybox_.GetExposure();
			GetFloatField(skyBlock, "exposure", exp);
			skybox_.SetExposure(exp);
		}
	}

	// Parse individual GameObjects
	size_t objStart = content.find("    {");
	while (objStart != std::string::npos) {
		size_t objEnd = content.find("\n    }", objStart);
		if (objEnd != std::string::npos) {
			objEnd += 1; // Advance past '\n' to "    }"
		} else {
			objEnd = content.find("    }", objStart);
		}
		if (objEnd == std::string::npos) break;

		std::string block = content.substr(objStart, objEnd - objStart);

		// Default-initialized GameObject guarantees safe fallback for any missing field
		GameObject obj;
		obj.id = nextObjectId_++;

		int typeInt = 0;
		GetIntField(block, "type", typeInt);
		obj.type = static_cast<GameObjectType>(typeInt);

		GetStringField(block, "name", obj.name);
		GetBoolField(block, "isActive", obj.isActive);
		GetBoolField(block, "isStatic", obj.isStatic);
		GetStringField(block, "tag", obj.tag);

		int layerInt = 0;
		GetIntField(block, "layer", layerInt);
		obj.layer = static_cast<uint32_t>(layerInt);

		GetIntField(block, "parentId", obj.parentId);

		// Transform
		GetVec3Field(block, "translation", obj.translation);
		GetVec3Field(block, "rotation", obj.rotation);
		GetVec3Field(block, "scale", obj.scale);

		Vector4 qVec = { 0.0f, 0.0f, 0.0f, 1.0f };
		GetVec4Field(block, "rotationQuat", qVec);
		obj.rotationQuat = { qVec.x, qVec.y, qVec.z, qVec.w };
		if (obj.rotationQuat.x == 0.0f && obj.rotationQuat.y == 0.0f && obj.rotationQuat.z == 0.0f && obj.rotationQuat.w == 0.0f) {
			obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
		}

		// Mesh
		GetBoolField(block, "hasMesh", obj.hasMesh);
		GetBoolField(block, "meshEnabled", obj.meshEnabled);
		int meshTypeInt = 0;
		GetIntField(block, "meshType", meshTypeInt);
		obj.meshType = static_cast<GameObject::MeshType>(meshTypeInt);
		GetVec4Field(block, "materialColor", obj.materialColor);
		GetBoolField(block, "isLit", obj.isLit);
		GetIntField(block, "lightingType", obj.lightingType);
		GetFloatField(block, "shininess", obj.shininess);
		GetVec3Field(block, "specularColor", obj.specularColor);
		GetBoolField(block, "castShadow", obj.castShadow);
		GetBoolField(block, "receiveShadow", obj.receiveShadow);
		GetBoolField(block, "useGeometryShader", obj.useGeometryShader);
		GetIntField(block, "geometryShaderEffect", obj.geometryShaderEffect);
		GetFloatField(block, "explodeAmount", obj.explodeAmount);
		if (obj.geometryShaderEffect == 0 && obj.useGeometryShader) {
			obj.geometryShaderEffect = 1;
		}
		GetStringField(block, "texturePath", obj.texturePath);
		if (!obj.texturePath.empty()) {
			obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);
		}
		GetStringField(block, "modelPath", obj.modelPath);
		GetVec2Field(block, "uvOffset", obj.uvOffset);
		GetVec2Field(block, "uvTiling", obj.uvTiling);
		GetFloatField(block, "uvRotation", obj.uvRotation);

		// Box Collider
		GetBoolField(block, "hasBoxCollider", obj.hasBoxCollider);
		GetBoolField(block, "boxColliderEnabled", obj.boxColliderEnabled);
		GetBoolField(block, "isTrigger", obj.isTrigger);
		GetBoolField(block, "providesContacts", obj.providesContacts);
		GetVec3Field(block, "boxCenter", obj.boxCenter);
		GetVec3Field(block, "boxSize", obj.boxSize);

		// Sphere Collider
		GetBoolField(block, "hasSphereCollider", obj.hasSphereCollider);
		GetBoolField(block, "sphereColliderEnabled", obj.sphereColliderEnabled);
		GetBoolField(block, "sphereIsTrigger", obj.sphereIsTrigger);
		GetVec3Field(block, "sphereCenter", obj.sphereCenter);
		GetFloatField(block, "sphereRadius", obj.sphereRadius);

		// Rigidbody
		GetBoolField(block, "hasRigidbody", obj.hasRigidbody);
		GetBoolField(block, "rbEnabled", obj.rbEnabled);
		GetFloatField(block, "mass", obj.mass);
		GetFloatField(block, "drag", obj.drag);
		GetFloatField(block, "gravityScale", obj.gravityScale);
		GetBoolField(block, "useGravity", obj.useGravity);
		GetBoolField(block, "isKinematic", obj.isKinematic);
		GetFloatField(block, "bounciness", obj.bounciness);

		// Camera
		GetBoolField(block, "hasCamera", obj.hasCamera);
		GetBoolField(block, "cameraEnabled", obj.cameraEnabled);
		GetFloatField(block, "fov", obj.fov);
		GetFloatField(block, "nearClip", obj.nearClip);
		GetFloatField(block, "farClip", obj.farClip);
		GetBoolField(block, "isOrthographic", obj.isOrthographic);

		// Light
		GetBoolField(block, "hasLight", obj.hasLight);
		GetBoolField(block, "lightEnabled", obj.lightEnabled);
		int lightTypeInt = 0;
		GetIntField(block, "lightType", lightTypeInt);
		obj.lightType = static_cast<GameObject::LightType>(lightTypeInt);
		GetVec3Field(block, "lightColor", obj.lightColor);
		GetFloatField(block, "lightIntensity", obj.lightIntensity);
		GetVec3Field(block, "lightDirection", obj.lightDirection);
		GetFloatField(block, "lightRange", obj.lightRange);
		GetFloatField(block, "lightDecay", obj.lightDecay);
		GetFloatField(block, "lightSpotAngle", obj.lightSpotAngle);
		GetFloatField(block, "lightSpotFalloffStart", obj.lightSpotFalloffStart);
		GetBoolField(block, "lightCastShadow", obj.lightCastShadow);
		GetFloatField(block, "lightShadowBias", obj.lightShadowBias);
		GetFloatField(block, "lightShadowStrength", obj.lightShadowStrength);
		GetFloatField(block, "lightShadowNormalBias", obj.lightShadowNormalBias);
		if (obj.hasLight && (obj.lightType == GameObject::LightType::Directional || obj.lightType == GameObject::LightType::Spot)) {
			if (std::abs(obj.rotation.x) < 0.0001f && std::abs(obj.rotation.y) < 0.0001f && std::abs(obj.rotation.z) < 0.0001f) {
				Vector3 d = obj.lightDirection;
				float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
				if (dLen > 0.0001f) {
					d = { d.x / dLen, d.y / dLen, d.z / dLen };
					obj.rotation.x = std::asin(std::clamp(-d.y, -1.0f, 1.0f));
					obj.rotation.y = std::atan2(d.x, d.z);
					obj.rotation.z = 0.0f;
					obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
				}
			}
		}

		// Particle System
		GetBoolField(block, "hasParticleSystem", obj.hasParticleSystem);
		GetIntField(block, "attachedParentId", obj.attachedParentId);
		GetVec3Field(block, "localOffset", obj.localOffset);
		GetIntField(block, "particlePreset", obj.particlePreset);
		GetIntField(block, "particleShape", obj.particleShape);
		GetFloatField(block, "particleSpawnRadius", obj.particleSpawnRadius);
		GetVec3Field(block, "particleBoxExtents", obj.particleBoxExtents);
		GetFloatField(block, "particleConeAngle", obj.particleConeAngle);
		GetFloatField(block, "particleSpawnRate", obj.particleSpawnRate);
		GetIntField(block, "particleMaxParticles", obj.particleMaxParticles);
		GetIntField(block, "particleBurstCount", obj.particleBurstCount);
		GetVec3Field(block, "particleDirection", obj.particleDirection);
		GetFloatField(block, "particleSpreadAngle", obj.particleSpreadAngle);
		GetFloatField(block, "particleSpeedMin", obj.particleSpeedMin);
		GetFloatField(block, "particleSpeedMax", obj.particleSpeedMax);
		GetFloatField(block, "particleGravity", obj.particleGravity);
		GetFloatField(block, "particleDrag", obj.particleDrag);
		GetBoolField(block, "particleWindEnabled", obj.particleWindEnabled);
		GetVec3Field(block, "particleWindAcceleration", obj.particleWindAcceleration);
		GetFloatField(block, "particleLifetimeMin", obj.particleLifetimeMin);
		GetFloatField(block, "particleLifetimeMax", obj.particleLifetimeMax);
		GetFloatField(block, "particleStartScale", obj.particleStartScale);
		GetFloatField(block, "particleEndScale", obj.particleEndScale);
		GetVec4Field(block, "particleStartColor", obj.particleStartColor);
		GetVec4Field(block, "particleEndColor", obj.particleEndColor);
		GetIntField(block, "particleBlendMode", obj.particleBlendMode);
		GetIntField(block, "particleBillboardMode", obj.particleBillboardMode);

		// Mapchip, Audio, Animation & Scripts
		GetBoolField(block, "hasMapchip", obj.hasMapchip);
		GetBoolField(block, "hasAudioSource", obj.hasAudioSource);
		GetStringField(block, "audioClipName", obj.audioClipName);
		GetFloatField(block, "audioVolume", obj.audioVolume);
		GetBoolField(block, "audioPlayOnAwake", obj.audioPlayOnAwake);
		GetBoolField(block, "audioLoop", obj.audioLoop);
		GetIntField(block, "animationIndex", obj.animationIndex);
		GetFloatField(block, "animationSpeed", obj.animationSpeed);
		GetBoolField(block, "animationLoop", obj.animationLoop);
		GetBoolField(block, "animationPlaying", obj.animationPlaying);
		GetBoolField(block, "hasScript", obj.hasScript);
		GetStringField(block, "scriptClassName", obj.scriptClassName);

		bool parsedScriptComponents = false;
		size_t scPos = block.find("\"scriptComponents\": [");
		if (scPos != std::string::npos) {
			parsedScriptComponents = true;
			size_t cursor = scPos + 21;
			while (cursor < block.size()) {
				size_t clsTok = block.find("{\"className\":\"", cursor);
				if (clsTok == std::string::npos) break;
				size_t clsEnd = block.find("\"", clsTok + 14);
				if (clsEnd == std::string::npos) break;
				std::string clsName = block.substr(clsTok + 14, clsEnd - (clsTok + 14));

				GameObject::ScriptComponent sc(clsName);
				sc.isMissingScript = !scriptModule_.IsRegistered(clsName);
				if (sc.isMissingScript) {
					LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving saved field data.", clsName, obj.name), 1);
				}

				size_t fieldsTok = block.find("\"fields\":[", clsEnd);
				size_t fieldsClose = (fieldsTok != std::string::npos) ? block.find("]}", fieldsTok + 10) : std::string::npos;
				if (fieldsTok != std::string::npos && fieldsClose != std::string::npos) {
					std::string fieldsSub = block.substr(fieldsTok + 10, fieldsClose - (fieldsTok + 10));
					size_t fCur = 0;
					while (fCur < fieldsSub.size()) {
						size_t fOpen = fieldsSub.find("{\"name\":\"", fCur);
						if (fOpen == std::string::npos) break;
						size_t fEnd = fieldsSub.find("}", fOpen);
						if (fEnd == std::string::npos) break;
						std::string fEntry = fieldsSub.substr(fOpen, fEnd - fOpen + 1);

						size_t nEnd = fEntry.find("\"", 9);
						if (nEnd != std::string::npos) {
							std::string fName = fEntry.substr(9, nEnd - 9);
							int fTypeInt = 0;
							size_t tPos = fEntry.find("\"type\":");
							if (tPos != std::string::npos) {
								sscanf_s(fEntry.c_str() + tPos + 7, "%d", &fTypeInt);
							}
							GameObject::ScriptSavedFieldValue fVal{};
							fVal.type = static_cast<ScriptFieldType>(fTypeInt);
							switch (fVal.type) {
							case ScriptFieldType::Float: {
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%f", &fVal.floatVal);
								break;
							}
							case ScriptFieldType::Int: {
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%d", &fVal.intVal);
								break;
							}
							case ScriptFieldType::Bool: {
								int bInt = 0;
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%d", &bInt);
								fVal.boolVal = (bInt != 0);
								break;
							}
							case ScriptFieldType::Vec3: {
								size_t v0Pos = fEntry.find("\"v0\":");
								size_t v1Pos = fEntry.find("\"v1\":");
								size_t v2Pos = fEntry.find("\"v2\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%f", &fVal.vec3Val.x);
								if (v1Pos != std::string::npos) sscanf_s(fEntry.c_str() + v1Pos + 5, "%f", &fVal.vec3Val.y);
								if (v2Pos != std::string::npos) sscanf_s(fEntry.c_str() + v2Pos + 5, "%f", &fVal.vec3Val.z);
								break;
							}
							case ScriptFieldType::CharBuffer: {
								size_t sPos = fEntry.find("\"str\":\"");
								if (sPos != std::string::npos) {
									std::string unesc;
									for (size_t si = sPos + 7; si < fEntry.size(); ++si) {
										if (fEntry[si] == '\\' && si + 1 < fEntry.size()) {
											unesc.push_back(fEntry[++si]);
										} else if (fEntry[si] == '\"') {
											break;
										} else {
											unesc.push_back(fEntry[si]);
										}
									}
									fVal.strVal = unesc;
								}
								break;
							}
							}
							sc.savedFields[fName] = fVal;
						}
						fCur = fEnd + 1;
					}
					cursor = fieldsClose + 2;
				} else {
					cursor = clsEnd + 1;
				}

				if (!clsName.empty()) {
					obj.scripts.push_back(std::move(sc));
				}
			}
		}

		if (!parsedScriptComponents) {
			size_t scriptsArrPos = block.find("\"scripts\": [");
			if (scriptsArrPos != std::string::npos) {
				size_t scriptsArrEnd = block.find("]", scriptsArrPos + 12);
				if (scriptsArrEnd != std::string::npos) {
					std::string arrContent = block.substr(scriptsArrPos + 12, scriptsArrEnd - (scriptsArrPos + 12));
					size_t qStart = arrContent.find("\"");
					while (qStart != std::string::npos) {
						size_t qEnd = arrContent.find("\"", qStart + 1);
						if (qEnd == std::string::npos) break;
						std::string clsName = arrContent.substr(qStart + 1, qEnd - (qStart + 1));
						if (!clsName.empty()) {
							GameObject::ScriptComponent sc(clsName);
							sc.isMissingScript = !scriptModule_.IsRegistered(clsName);
							if (sc.isMissingScript) {
								LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving script name on save.", clsName, obj.name), 1);
							}
							obj.scripts.push_back(std::move(sc));
						}
						qStart = arrContent.find("\"", qEnd + 1);
					}
				}
			} else if (obj.hasScript && !obj.scriptClassName.empty()) {
				GameObject::ScriptComponent sc(obj.scriptClassName);
				sc.isMissingScript = !scriptModule_.IsRegistered(obj.scriptClassName);
				if (sc.isMissingScript) {
					LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving script name on save.", obj.scriptClassName, obj.name), 1);
				}
				obj.scripts.push_back(std::move(sc));
			}
		}
		obj.hasScript = !obj.scripts.empty();
		if (!obj.scripts.empty()) {
			obj.scriptClassName = obj.scripts.front().className;
		}

		if (obj.hasParticleSystem) {
			EnsureParticleQuadInitialized(obj);
		}

		gameObjects_.push_back(std::move(obj));
		objStart = content.find("    {", objEnd);
	}

	// Sync parentIndex, childrenIds, and attachToIndex for loaded objects
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		gameObjects_[i].childrenIds.clear();
		gameObjects_[i].parentIndex = -1;
		gameObjects_[i].attachToIndex = -1;
	}
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (gameObjects_[i].parentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].parentId)) {
					gameObjects_[i].parentIndex = static_cast<int>(p);
					gameObjects_[p].childrenIds.push_back(gameObjects_[i].id);
					break;
				}
			}
		}
		if (gameObjects_[i].attachedParentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].attachedParentId)) {
					gameObjects_[i].attachToIndex = static_cast<int>(p);
					break;
				}
			}
		}
	}

	// Parse Mapchip section if present
	size_t mapPos = content.find("\"mapchip\": {");
	if (mapPos != std::string::npos) {
		std::string mapBlock = content.substr(mapPos);
		GetIntField(mapBlock, "rows", mapRows_);
		GetIntField(mapBlock, "cols", mapCols_);
		GetFloatField(mapBlock, "tileSize", mapTileSize_);

		size_t dataPos = mapBlock.find("\"data\": [");
		if (dataPos != std::string::npos) {
			mapGrid_.assign(mapRows_, std::vector<int>(mapCols_, 0));
			size_t rowStart = mapBlock.find("[", dataPos + 9);
			for (int r = 0; r < mapRows_ && rowStart != std::string::npos; ++r) {
				size_t rowEnd = mapBlock.find("]", rowStart);
				if (rowEnd == std::string::npos) break;
				std::string rowStr = mapBlock.substr(rowStart + 1, rowEnd - (rowStart + 1));
				std::stringstream rss(rowStr);
				std::string cell;
				int c = 0;
				while (std::getline(rss, cell, ',') && c < mapCols_) {
					try { mapGrid_[r][c++] = std::stoi(cell); } catch (...) {}
				}
				rowStart = mapBlock.find("[", rowEnd);
			}
		}
	}

	EnsureEditModeScriptInstances();
	selectedObjectIndex_ = gameObjects_.empty() ? -1 : 0;
	return true;
}

bool EditorSuite::SaveScene(const std::string& filePath)
{
	std::string targetPath = filePath.empty() ? currentScenePath_ : filePath;
	if (targetPath.empty()) {
		showSaveSceneAsModal_ = true;
		return false;
	}

	std::filesystem::path p(targetPath);
	if (p.has_parent_path()) {
		std::filesystem::create_directories(p.parent_path());
	}

	std::ofstream file(targetPath);
	if (!file.is_open()) return false;

	file << SerializeSceneToString();
	file.close();

	isSceneDirty_ = false;
	currentScenePath_ = targetPath;
	currentSceneName_ = p.stem().string();
	LogMessage(std::format("[Scene] Saved scene to: {}", targetPath), 0);
	return true;
}

bool EditorSuite::LoadScene(const std::string& filePath)
{
	if (sceneState_ != EditorSceneState::Edit && !isStandaloneGameMode_) {
		StopPlayMode();
	}

	std::ifstream file(filePath);
	if (!file.is_open()) return false;

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	if (!DeserializeSceneFromString(buffer.str())) return false;

	currentScenePath_ = filePath;
	currentSceneName_ = std::filesystem::path(filePath).stem().string();
	isSceneDirty_ = false;

	undoStack_.clear();
	redoStack_.clear();
	PushUndoSnapshot();
	isSceneDirty_ = false;

	LogMessage(std::format("[Scene] Loaded scene from: {} ({} objects)", filePath, gameObjects_.size()), 0);
	return true;
}

void EditorSuite::NewScene()
{
	if (sceneState_ != EditorSceneState::Edit) {
		StopPlayMode();
	}
	pendingDeletes_.clear();
	pendingDuplicateIndex_ = -1;
	CollisionManager::GetInstance()->Clear();
	gameObjects_.clear();
	currentSceneName_ = "SampleScene";
	currentScenePath_ = "Assets/Scenes/SampleScene.scene.json";
	isSceneDirty_ = false;

	skybox_.SetMode(SkyboxMode::Gradient);
	skybox_.SetTopColor({ 0.22f, 0.45f, 0.78f, 1.0f });
	skybox_.SetHorizonColor({ 0.72f, 0.82f, 0.92f, 1.0f });
	skybox_.SetBottomColor({ 0.35f, 0.37f, 0.40f, 1.0f });
	skybox_.SetSolidColor({ 0.157f, 0.157f, 0.157f, 1.0f });
	skybox_.SetCubemapPath("Resources/skybox.dds");
	skybox_.SetRotation(0.0f);
	skybox_.SetExposure(1.0f);

	// 1. Main Camera
	{
		auto& cam = CreateGameObject("Main Camera", GameObjectType::Camera);
		cam.tag = "MainCamera";
		cam.translation = gameCamera_ ? gameCamera_->translation : Vector3{ 0.0f, 1.5f, -8.0f };
		cam.rotation = gameCamera_ ? gameCamera_->rotation : Vector3{ 0.15f, 0.0f, 0.0f };
		cam.hasCamera = true;
		cam.fov = 45.0f;
		cam.nearClip = 0.1f;
		cam.farClip = 1000.0f;
	}

	// 2. Directional Light
	{
		auto& light = CreateGameObject("Directional Light", GameObjectType::DirectionalLight);
		light.translation = { 0.0f, 10.0f, 0.0f };
		light.hasLight = true;
		light.lightType = GameObject::LightType::Directional;
		light.lightDirection = { 0.5f, -1.0f, 0.5f };
		{
			Vector3 d = light.lightDirection;
			float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (dLen > 0.0001f) { d.x /= dLen; d.y /= dLen; d.z /= dLen; }
			light.rotation = { std::asin(std::clamp(-d.y, -1.0f, 1.0f)), std::atan2(d.x, d.z), 0.0f };
			light.rotationQuat = Quaternion::FromEuler(light.rotation);
		}
		light.lightColor = { 1.0f, 0.95f, 0.9f };
		light.lightIntensity = 1.0f;
		light.lightCastShadow = true;
		light.lightShadowBias = 0.002f;
		light.lightShadowStrength = 1.0f;
		light.lightShadowNormalBias = 0.02f;
	}

	// 3. Ground Plane
	{
		auto& plane = CreatePlane("Ground Plane", { 0.0f, -0.5f, 0.0f });
		plane.scale = { 20.0f, 1.0f, 20.0f };
		plane.isStatic = true;
		plane.materialColor = { 0.25f, 0.27f, 0.30f, 1.0f };
		plane.castShadow = false;
		plane.receiveShadow = true;
	}

	selectedObjectIndex_ = 2; // Ground Plane
	undoStack_.clear();
	redoStack_.clear();
	PushUndoSnapshot();
	isSceneDirty_ = false;

	LogMessage("[Scene] Created Clean Default Scene (SampleScene).", 0);
}

void EditorSuite::DiscardUnsavedChanges()
{
	if (std::filesystem::exists(currentScenePath_)) {
		LoadScene(currentScenePath_);
	} else {
		NewScene();
	}
	LogMessage("[Scene] Discarded unsaved changes.", 0);
}

void EditorSuite::PushUndoSnapshot()
{
	std::string stateJson = SerializeSceneToString();
	if (!undoStack_.empty() && undoStack_.back() == stateJson) {
		return;
	}

	undoStack_.push_back(stateJson);
	if (undoStack_.size() > maxUndoStates_) {
		undoStack_.erase(undoStack_.begin());
	}
	redoStack_.clear();
}

void EditorSuite::Undo()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput) return; // Must not run while text field is active
#endif
	if (sceneState_ == EditorSceneState::Play) return; // Disabled in Play mode

	if (undoStack_.size() > 1) {
		redoStack_.push_back(undoStack_.back());
		undoStack_.pop_back();

		DeserializeSceneFromString(undoStack_.back());
		isSceneDirty_ = true;
		LogMessage("[Undo] Reverted to previous scene state.", 0);
	}
}

void EditorSuite::Redo()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput) return;
#endif
	if (sceneState_ == EditorSceneState::Play) return;

	if (!redoStack_.empty()) {
		std::string nextState = redoStack_.back();
		redoStack_.pop_back();
		undoStack_.push_back(nextState);

		DeserializeSceneFromString(nextState);
		isSceneDirty_ = true;
		LogMessage("[Redo] Restored undone scene state.", 0);
	}
}

std::string EditorSuite::OpenNativeFolderDialog()
{
	std::string folderPath = "";
	HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool shouldUninit = SUCCEEDED(hrInit);
	{
		Microsoft::WRL::ComPtr<IFileOpenDialog> pFileOpen = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOpen));
		if (SUCCEEDED(hr) && pFileOpen) {
			DWORD dwOptions;
			if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
				pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			}
			if (SUCCEEDED(pFileOpen->Show(NULL))) {
				Microsoft::WRL::ComPtr<IShellItem> pItem = nullptr;
				if (SUCCEEDED(pFileOpen->GetResult(&pItem)) && pItem) {
					PWSTR pszFilePath = nullptr;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) && pszFilePath) {
						int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
						if (len > 1) {
							folderPath.resize(len - 1);
							WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, folderPath.data(), len, NULL, NULL);
						}
						CoTaskMemFree(pszFilePath);
					}
				}
			}
		}
	}
	if (shouldUninit) {
		CoUninitialize();
	}
	return folderPath;
}

bool EditorSuite::CreateNewProject(const std::string& parentFolder, const std::string& projectName)
{
	try {
		std::filesystem::path projPath = std::filesystem::path(parentFolder) / projectName;
		std::filesystem::create_directories(projPath / "Assets" / "Scenes");
		std::filesystem::create_directories(projPath / "Assets" / "Models");
		std::filesystem::create_directories(projPath / "Assets" / "Textures");
		std::filesystem::create_directories(projPath / "Assets" / "Scripts");
		std::filesystem::create_directories(projPath / "Assets" / "Audio");
		std::filesystem::create_directories(projPath / "ProjectSettings");

		// Write ProjectSettings/project.json
		{
			std::ofstream pj((projPath / "ProjectSettings" / "project.json").string());
			pj << "{\n  \"projectName\": \"" << projectName << "\",\n  \"engineVersion\": \"v2.0\",\n  \"startupScene\": \"Assets/Scenes/SampleScene.scene.json\"\n}\n";
		}

		// Populate default SampleScene.scene.json containing Main Camera, Directional Light, and Ground Plane
		NewScene();
		currentSceneName_ = "SampleScene";
		currentScenePath_ = (projPath / "Assets" / "Scenes" / "SampleScene.scene.json").string();
		SaveScene(currentScenePath_);

		// Copy starter sample assets so project folders are immediately populated
		std::error_code ec;
		if (std::filesystem::exists("Resources/circle.png")) {
			std::filesystem::copy_file("Resources/circle.png", projPath / "Assets" / "Textures" / "circle.png", std::filesystem::copy_options::overwrite_existing, ec);
		}
		if (std::filesystem::exists("Resources/Model/cube.obj")) {
			std::filesystem::copy_file("Resources/Model/cube.obj", projPath / "Assets" / "Models" / "cube.obj", std::filesystem::copy_options::overwrite_existing, ec);
		}

		currentProjectRoot_ = projPath;
		currentDirectory_ = projPath;
		strncpy_s(openProjectFolderBuf_, projPath.string().c_str(), sizeof(openProjectFolderBuf_) - 1);
		strncpy_s(buildStartupSceneBuf_, currentScenePath_.c_str(), sizeof(buildStartupSceneBuf_) - 1);
		strncpy_s(buildGameTitleBuf_, projectName.c_str(), sizeof(buildGameTitleBuf_) - 1);

		LogMessage(std::format("[Project] Created new project '{}' at: {}", projectName, projPath.string()), 0);
		return true;
	} catch (const std::exception& e) {
		LogMessage(std::format("[Project] Failed to create project: {}", e.what()), 2);
		return false;
	}
}

bool EditorSuite::OpenProject(const std::string& folderPath)
{
	if (folderPath.empty()) return false;

	std::filesystem::path projPath(folderPath);
	if (!std::filesystem::exists(projPath / "ProjectSettings" / "project.json") &&
		projPath.has_parent_path() &&
		std::filesystem::exists(projPath.parent_path() / "ProjectSettings" / "project.json")) {
		projPath = projPath.parent_path();
	}

	std::filesystem::path projJsonPath = projPath / "ProjectSettings" / "project.json";
	if (!std::filesystem::exists(projJsonPath)) {
		LogMessage(std::format("[Project] Folder does not contain ProjectSettings/project.json: {}", folderPath), 1);
		if (std::filesystem::exists(projPath) && std::filesystem::is_directory(projPath)) {
			currentDirectory_ = projPath;
		}
		return false;
	}

	std::string projectName = projPath.filename().string();
	std::string startupScene = "Assets/Scenes/SampleScene.scene.json";

	std::ifstream pj(projJsonPath);
	if (pj.is_open()) {
		std::stringstream buf;
		buf << pj.rdbuf();
		std::string content = buf.str();

		auto ExtractJsonString = [&](const std::string& key, std::string& outVal) {
			std::string token = "\"" + key + "\": \"";
			size_t p = content.find(token);
			if (p != std::string::npos) {
				size_t ep = content.find("\"", p + token.length());
				if (ep != std::string::npos) {
					outVal = content.substr(p + token.length(), ep - (p + token.length()));
				}
			}
		};

		ExtractJsonString("projectName", projectName);
		ExtractJsonString("startupScene", startupScene);
	}

	currentProjectRoot_ = projPath;
	currentDirectory_ = projPath;
	strncpy_s(openProjectFolderBuf_, projPath.string().c_str(), sizeof(openProjectFolderBuf_) - 1);
	strncpy_s(buildGameTitleBuf_, projectName.c_str(), sizeof(buildGameTitleBuf_) - 1);

	std::filesystem::path sceneFullPath = projPath / startupScene;
	if (!std::filesystem::exists(sceneFullPath)) {
		sceneFullPath = projPath / "Assets" / "Scenes" / "SampleScene.scene.json";
	}
	if (std::filesystem::exists(sceneFullPath)) {
		LoadScene(sceneFullPath.string());
		strncpy_s(buildStartupSceneBuf_, sceneFullPath.string().c_str(), sizeof(buildStartupSceneBuf_) - 1);
	}

	LogMessage(std::format("[Project] Opened project '{}' at: {}", projectName, projPath.string()), 0);
	return true;
}

void EditorSuite::ToggleStandalonePreview()
{
	if (!isStandalonePreview_) {
		isStandalonePreview_ = true;
		if (sceneState_ == EditorSceneState::Edit) {
			StartPlayMode();
		} else {
			sceneState_ = EditorSceneState::Play;
		}
		LogMessage("[Standalone] Entered Standalone Game Preview (F11 / Esc to exit).", 0);
	} else {
		isStandalonePreview_ = false;
		if (sceneState_ != EditorSceneState::Edit) {
			StopPlayMode();
		}
		LogMessage("[Standalone] Exited Standalone Game Preview.", 0);
	}
}

bool EditorSuite::ExportStandaloneGame()
{
	try {
		std::filesystem::path outDir(buildOutputDirBuf_);
		std::filesystem::create_directories(outDir);

		char exePath[MAX_PATH] = {};
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		std::filesystem::path sourceExe(exePath);

		std::string gameTitle = (buildGameTitleBuf_[0] != '\0') ? buildGameTitleBuf_ : "RizealldayGame";
		std::string targetExeName = gameTitle + ".exe";
		std::filesystem::copy_file(sourceExe, outDir / targetExeName, std::filesystem::copy_options::overwrite_existing);

		auto binDir = sourceExe.parent_path();
		std::vector<std::filesystem::path> dllSearchDirs = {
			binDir,
			std::filesystem::current_path(),
			binDir.parent_path().parent_path()
		};
		for (const auto& dir : dllSearchDirs) {
			if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
				for (const auto& entry : std::filesystem::directory_iterator(dir)) {
					if (entry.is_regular_file() && entry.path().extension() == ".dll") {
						std::filesystem::copy_file(entry.path(), outDir / entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
					}
				}
			}
		}

		// Copy bin/Scripts/GameScripts.dll into exported standalone directory
		{
			std::filesystem::path srcScriptsDll = scriptModule_.ResolveDllPath();
			if (std::filesystem::exists(srcScriptsDll)) {
				std::error_code ec;
				std::filesystem::path dstScriptsDll = outDir / ScriptModule::kScriptsDllSubpath;
				std::filesystem::create_directories(dstScriptsDll.parent_path(), ec);
				std::filesystem::copy_file(srcScriptsDll, dstScriptsDll, std::filesystem::copy_options::overwrite_existing, ec);
			}
		}

		// Copy Resources/ and shaders/
		if (std::filesystem::exists("Resources")) {
			std::filesystem::copy("Resources", outDir / "Resources", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		} else if (std::filesystem::exists(binDir / "Resources")) {
			std::filesystem::copy(binDir / "Resources", outDir / "Resources", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		}

		if (std::filesystem::exists("shaders")) {
			std::filesystem::copy("shaders", outDir / "shaders", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		} else if (std::filesystem::exists(binDir / "shaders")) {
			std::filesystem::copy(binDir / "shaders", outDir / "shaders", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		}

		// Determine relative path for startup scene inside exported directory
		std::string rawStartup = (buildStartupSceneBuf_[0] != '\0') ? buildStartupSceneBuf_ : "Assets/Scenes/SampleScene.scene.json";
		std::filesystem::path srcScenePath(rawStartup);
		std::string relStartup = "Assets/Scenes/SampleScene.scene.json";
		if (!srcScenePath.is_absolute()) {
			relStartup = srcScenePath.generic_string();
		} else if (srcScenePath.has_filename()) {
			relStartup = (std::filesystem::path("Assets/Scenes") / srcScenePath.filename()).generic_string();
		}

		std::filesystem::path destScenePath = outDir / relStartup;
		std::filesystem::create_directories(destScenePath.parent_path());

		if (std::filesystem::exists(srcScenePath)) {
			std::error_code ec;
			if (!std::filesystem::equivalent(srcScenePath, destScenePath, ec)) {
				std::filesystem::copy_file(srcScenePath, destScenePath, std::filesystem::copy_options::overwrite_existing, ec);
			}
		} else {
			std::ofstream sf(destScenePath);
			if (sf.is_open()) {
				sf << SerializeSceneToString();
			}
		}

		int resW = 1920, resH = 1080;
		if (buildResolutionIndex_ == 1) { resW = 1280; resH = 720; }
		else if (buildResolutionIndex_ == 2) { resW = 2560; resH = 1440; }

		{
			std::ofstream cfg((outDir / "game_build.cfg").string());
			cfg << "standaloneGameMode = true\n";
			cfg << "gameTitle = " << gameTitle << "\n";
			cfg << "resolutionWidth = " << resW << "\n";
			cfg << "resolutionHeight = " << resH << "\n";
			cfg << "fullscreen = " << (buildFullscreen_ ? "true" : "false") << "\n";
			cfg << "startupScene = " << relStartup << "\n";
		}

		LogMessage(std::format("[Build] Successfully exported standalone game '{}' to: {}", targetExeName, outDir.string()), 0);
		return true;
	} catch (const std::exception& e) {
		LogMessage(std::format("[Build] Standalone export failed: {}", e.what()), 2);
		return false;
	}
}

void EditorSuite::LoadTetrisTemplate()
{
	gameObjects_.clear();
	currentSceneName_ = "3DTetrisScene";
	isTetrisActive_ = true;
	tetrisScore_ = 0;
	tetrisLinesCleared_ = 0;
	tetrisLevel_ = 1;
	tetrisGameOver_ = false;
	tetrisFallTimer_ = 0.0f;
	currentPieceCubeIds_.clear();
	std::memset(tetrisBoard_, 0, sizeof(tetrisBoard_));
	std::memset(tetrisBlockIds_, 0, sizeof(tetrisBlockIds_));

	// 1. Camera centered directly in front of 10x20 Tetris Grid
	{
		auto& cam = CreateGameObject("Tetris Camera", GameObjectType::Camera);
		cam.translation = { 4.5f, 9.5f, -19.5f };
		cam.rotation = { 0.0f, 0.0f, 0.0f };
		cam.hasCamera = true;
		cam.fov = 50.0f;
		if (gameCamera_) {
			gameCamera_->translation = cam.translation;
			gameCamera_->rotation = cam.rotation;
		}
	}

	// 2. Directional Sun Light
	{
		auto& sun = CreateGameObject("Sun Light", GameObjectType::DirectionalLight);
		sun.hasLight = true;
		sun.lightType = GameObject::LightType::Directional;
		sun.lightDirection = { 0.3f, -1.0f, 0.4f };
		{
			Vector3 d = sun.lightDirection;
			float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (dLen > 0.0001f) { d.x /= dLen; d.y /= dLen; d.z /= dLen; }
			sun.rotation = { std::asin(std::clamp(-d.y, -1.0f, 1.0f)), std::atan2(d.x, d.z), 0.0f };
			sun.rotationQuat = Quaternion::FromEuler(sun.rotation);
		}
		sun.lightColor = { 1.0f, 0.95f, 0.9f };
		sun.lightIntensity = 1.2f;
	}

	// 3. Point Light
	{
		auto& pt = CreateGameObject("Board Glow Light", GameObjectType::PointLight);
		pt.translation = { 4.5f, 10.0f, -6.0f };
		pt.hasLight = true;
		pt.lightType = GameObject::LightType::Point;
		pt.lightColor = { 0.6f, 0.8f, 1.0f };
		pt.lightIntensity = 1.5f;
		pt.lightRange = 30.0f;
	}

	// 4. Board Frame: Left Border, Right Border, and Floor
	Vector4 frameColor = { 0.15f, 0.16f, 0.18f, 1.0f };
	for (int y = 0; y < 20; ++y) {
		auto& cube = CreateCube("Wall_Left", { -1.0f, static_cast<float>(y), 0.0f });
		cube.materialColor = frameColor;
		cube.isStatic = true;
	}
	for (int y = 0; y < 20; ++y) {
		auto& cube = CreateCube("Wall_Right", { 10.0f, static_cast<float>(y), 0.0f });
		cube.materialColor = frameColor;
		cube.isStatic = true;
	}
	for (int x = -1; x <= 10; ++x) {
		auto& cube = CreateCube("Floor", { static_cast<float>(x), -1.0f, 0.0f });
		cube.materialColor = frameColor;
		cube.isStatic = true;
	}

	// 5. Backboard Plane
	{
		auto& back = CreateCube("BackBoard", { 4.5f, 9.5f, 0.6f });
		back.scale = { 12.0f, 21.0f, 0.2f };
		back.materialColor = { 0.07f, 0.08f, 0.10f, 1.0f };
		back.isStatic = true;
	}

	// 6. Row Clear Particle Explosion System
	if (particleSystem_) {
		auto& ps = CreateGameObject("Row Clear Fireworks", GameObjectType::ParticleSystem);
		ps.translation = { 4.5f, 10.0f, 0.0f };
		ps.hasParticleSystem = true;
		ps.particlePreset = 3; // Fireworks preset
		particleSystem_->ApplyPreset(3);
	}

	RestartTetrisGame();
	isSceneDirty_ = false;
	LogMessage("[Tetris] Loaded 3D Tetris Template. Press [ > Play ] to start playing!", 0);
}

void EditorSuite::RestartTetrisGame()
{
	for (int r = 0; r < 20; ++r) {
		for (int c = 0; c < 10; ++c) {
			if (tetrisBlockIds_[r][c] != 0) {
				for (auto it = gameObjects_.begin(); it != gameObjects_.end(); ++it) {
					if (it->id == tetrisBlockIds_[r][c]) {
						gameObjects_.erase(it);
						break;
					}
				}
			}
			tetrisBoard_[r][c] = 0;
			tetrisBlockIds_[r][c] = 0;
		}
	}

	tetrisScore_ = 0;
	tetrisLinesCleared_ = 0;
	tetrisLevel_ = 1;
	tetrisGameOver_ = false;
	tetrisFallTimer_ = 0.0f;

	currentTetrominoType_ = rand() % 7;
	currentTetrominoRot_ = 0;
	currentTetrominoX_ = 3;
	currentTetrominoY_ = 18;

	currentPieceCubeIds_.clear();
	for (int i = 0; i < 4; ++i) {
		auto& cube = CreateCube("FallingBlock", { 0.0f, 0.0f, 0.0f });
		cube.materialColor = kTetrominoColors[currentTetrominoType_];
		cube.lightingType = 4;
		cube.shininess = 64.0f;
		cube.specularColor = { 1.0f, 1.0f, 1.0f };
		cube.hasRigidbody = false;
		cube.hasBoxCollider = false;
		currentPieceCubeIds_.push_back(cube.id);
	}
}

void EditorSuite::UpdateTetrisGame(float deltaTime, Input* input)
{
	if (!isTetrisActive_ || tetrisGameOver_ || sceneState_ != EditorSceneState::Play) {
		return;
	}

	auto IsValidPosition = [&](int testX, int testY, int testRot) -> bool {
		for (int i = 0; i < 4; ++i) {
			int bx = kTetrominoShapes[currentTetrominoType_][i][0];
			int by = kTetrominoShapes[currentTetrominoType_][i][1];

			for (int r = 0; r < testRot % 4; ++r) {
				int tempX = bx;
				bx = by;
				by = 3 - tempX;
			}

			int gridX = testX + bx;
			int gridY = testY + by;

			if (gridX < 0 || gridX >= 10 || gridY < 0) return false;
			if (gridY < 20 && tetrisBoard_[gridY][gridX] != 0) return false;
		}
		return true;
	};

#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsKeyPressed(ImGuiKey_A) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
		if (IsValidPosition(currentTetrominoX_ - 1, currentTetrominoY_, currentTetrominoRot_)) {
			currentTetrominoX_--;
		}
	}
	if (ImGui::IsKeyPressed(ImGuiKey_D) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
		if (IsValidPosition(currentTetrominoX_ + 1, currentTetrominoY_, currentTetrominoRot_)) {
			currentTetrominoX_++;
		}
	}
	if (ImGui::IsKeyPressed(ImGuiKey_W) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
		int nextRot = (currentTetrominoRot_ + 1) % 4;
		if (IsValidPosition(currentTetrominoX_, currentTetrominoY_, nextRot)) {
			currentTetrominoRot_ = nextRot;
		}
	}
	float fallSpeed = (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) ? 0.08f : tetrisFallInterval_;

	if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
		while (IsValidPosition(currentTetrominoX_, currentTetrominoY_ - 1, currentTetrominoRot_)) {
			currentTetrominoY_--;
			tetrisScore_ += 2;
		}
		tetrisFallTimer_ = 999.0f;
	}
#else
	float fallSpeed = tetrisFallInterval_;
#endif

	tetrisFallTimer_ += deltaTime;
	if (tetrisFallTimer_ >= fallSpeed) {
		tetrisFallTimer_ = 0.0f;
		if (IsValidPosition(currentTetrominoX_, currentTetrominoY_ - 1, currentTetrominoRot_)) {
			currentTetrominoY_--;
		} else {
			for (int i = 0; i < 4; ++i) {
				int bx = kTetrominoShapes[currentTetrominoType_][i][0];
				int by = kTetrominoShapes[currentTetrominoType_][i][1];
				for (int r = 0; r < currentTetrominoRot_ % 4; ++r) {
					int tempX = bx;
					bx = by;
					by = 3 - tempX;
				}
				int gx = currentTetrominoX_ + bx;
				int gy = currentTetrominoY_ + by;
				if (gx >= 0 && gx < 10 && gy >= 0 && gy < 20) {
					tetrisBoard_[gy][gx] = currentTetrominoType_ + 1;
					if (i < static_cast<int>(currentPieceCubeIds_.size())) {
						tetrisBlockIds_[gy][gx] = currentPieceCubeIds_[i];
					}
				}
			}

			int clearedRows = 0;
			for (int y = 0; y < 20; ++y) {
				bool rowFull = true;
				for (int x = 0; x < 10; ++x) {
					if (tetrisBoard_[y][x] == 0) { rowFull = false; break; }
				}

				if (rowFull) {
					clearedRows++;
					if (particleSystem_) {
						particleSystem_->emitter.translate = { 4.5f, static_cast<float>(y), 0.0f };
						particleSystem_->EmitBurst(35);
					}

					for (int x = 0; x < 10; ++x) {
						uint32_t cubeId = tetrisBlockIds_[y][x];
						for (auto it = gameObjects_.begin(); it != gameObjects_.end(); ++it) {
							if (it->id == cubeId) { gameObjects_.erase(it); break; }
						}
						tetrisBoard_[y][x] = 0;
						tetrisBlockIds_[y][x] = 0;
					}

					for (int ry = y; ry < 19; ++ry) {
						for (int rx = 0; rx < 10; ++rx) {
							tetrisBoard_[ry][rx] = tetrisBoard_[ry + 1][rx];
							tetrisBlockIds_[ry][rx] = tetrisBlockIds_[ry + 1][rx];
							if (tetrisBlockIds_[ry][rx] != 0) {
								for (auto& obj : gameObjects_) {
									if (obj.id == tetrisBlockIds_[ry][rx]) {
										obj.translation.y -= 1.0f;
										break;
									}
								}
							}
						}
					}
					for (int rx = 0; rx < 10; ++rx) {
						tetrisBoard_[19][rx] = 0;
						tetrisBlockIds_[19][rx] = 0;
					}
					y--;
				}
			}

			if (clearedRows > 0) {
				tetrisLinesCleared_ += clearedRows;
				tetrisScore_ += (clearedRows == 1 ? 100 : (clearedRows == 2 ? 300 : (clearedRows == 3 ? 500 : 800))) * tetrisLevel_;
				tetrisLevel_ = 1 + (tetrisLinesCleared_ / 10);
				tetrisFallInterval_ = (std::max)(0.15f, 0.7f - (tetrisLevel_ - 1) * 0.06f);
			}

			currentTetrominoType_ = rand() % 7;
			currentTetrominoRot_ = 0;
			currentTetrominoX_ = 3;
			currentTetrominoY_ = 18;

			if (!IsValidPosition(currentTetrominoX_, currentTetrominoY_, currentTetrominoRot_)) {
				tetrisGameOver_ = true;
				LogMessage(std::format("[Tetris] GAME OVER! Final Score: {}", tetrisScore_), 0);
				return;
			}

			currentPieceCubeIds_.clear();
			for (int i = 0; i < 4; ++i) {
				auto& cube = CreateCube("FallingBlock", { 0.0f, 0.0f, 0.0f });
				cube.materialColor = kTetrominoColors[currentTetrominoType_];
				cube.lightingType = 4;
				cube.shininess = 64.0f;
				cube.specularColor = { 1.0f, 1.0f, 1.0f };
				cube.hasRigidbody = false;
				cube.hasBoxCollider = false;
				currentPieceCubeIds_.push_back(cube.id);
			}
		}
	}

	Quaternion pieceQuat = Quaternion::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, currentTetrominoRot_ * (3.14159265f / 2.0f));
	for (int i = 0; i < 4 && i < static_cast<int>(currentPieceCubeIds_.size()); ++i) {
		int bx = kTetrominoShapes[currentTetrominoType_][i][0];
		int by = kTetrominoShapes[currentTetrominoType_][i][1];
		for (int r = 0; r < currentTetrominoRot_ % 4; ++r) {
			int tempX = bx;
			bx = by;
			by = 3 - tempX;
		}

		for (auto& obj : gameObjects_) {
			if (obj.id == currentPieceCubeIds_[i]) {
				obj.translation = { static_cast<float>(currentTetrominoX_ + bx), static_cast<float>(currentTetrominoY_ + by), 0.0f };
				obj.rotationQuat = pieceQuat;
				obj.rotation = pieceQuat.ToEuler();
				break;
			}
		}
	}
}

void EditorSuite::DrawTetrisHUD(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	if (!isTetrisActive_) return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 hudPos = ImVec2(vpPos.x + 16.0f, vpPos.y + 16.0f);

	drawList->AddRectFilled(hudPos, ImVec2(hudPos.x + 200.0f, hudPos.y + 130.0f), IM_COL32(20, 24, 30, 220), 6.0f);
	drawList->AddRect(hudPos, ImVec2(hudPos.x + 200.0f, hudPos.y + 130.0f), IM_COL32(0, 180, 255, 200), 6.0f);

	std::string scoreStr = std::format("SCORE: {}", tetrisScore_);
	std::string linesStr = std::format("LINES: {}", tetrisLinesCleared_);
	std::string levelStr = std::format("LEVEL: {}", tetrisLevel_);

	drawList->AddText(ImVec2(hudPos.x + 12.0f, hudPos.y + 10.0f), IM_COL32(255, 255, 255, 255), "3D TETRIS DX12");
	drawList->AddText(ImVec2(hudPos.x + 12.0f, hudPos.y + 36.0f), IM_COL32(255, 215, 0, 255), scoreStr.c_str());
	drawList->AddText(ImVec2(hudPos.x + 12.0f, hudPos.y + 60.0f), IM_COL32(100, 220, 255, 255), linesStr.c_str());
	drawList->AddText(ImVec2(hudPos.x + 12.0f, hudPos.y + 84.0f), IM_COL32(100, 255, 120, 255), levelStr.c_str());
	drawList->AddText(ImVec2(hudPos.x + 12.0f, hudPos.y + 106.0f), IM_COL32(180, 180, 180, 255), "A/D: Move | W: Rot | Space: Drop");

	if (tetrisGameOver_) {
		ImVec2 center = ImVec2(vpPos.x + vpSize.x * 0.5f, vpPos.y + vpSize.y * 0.5f);
		drawList->AddRectFilled(ImVec2(center.x - 140.0f, center.y - 45.0f), ImVec2(center.x + 140.0f, center.y + 45.0f), IM_COL32(150, 0, 0, 230), 8.0f);
		drawList->AddText(ImVec2(center.x - 50.0f, center.y - 30.0f), IM_COL32(255, 255, 255, 255), "GAME OVER");

		ImGui::SetCursorScreenPos(ImVec2(center.x - 60.0f, center.y + 2.0f));
		if (ImGui::Button("Restart Tetris", ImVec2(120.0f, 26.0f))) {
			RestartTetrisGame();
		}
	}
#endif
}

void EditorSuite::Update(float deltaTime, Input* input)
{
	currentInput_ = input;
	currentDeltaTime_ = deltaTime;

	// Drain any MSBuild output lines produced by background script compilation
	DrainScriptBuildLogs();

	// Poll GameScripts/Scripts/*.cpp|*.h for file saves when Auto Build Scripts is enabled
	if (!IsStandaloneMode()) {
		PollScriptSourceWatcher(deltaTime);
	}

	// Poll bin/Scripts/GameScripts.dll for timestamp changes every 0.5s; hot-reload at a safe point between frames
	if (scriptModule_.PollDllChanged(deltaTime, isBuildingScripts_.load())) {
		HotReloadScripts();
	}

	ProcessPendingDeletes();

#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();

	// Global Hotkeys
	if (!io.WantTextInput) {
		if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
			ToggleStandalonePreview();
		}
		if (isStandalonePreview_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ToggleStandalonePreview();
		}

		// Ctrl+Shift+B: Build Scripts (only when !io.WantTextInput)
		if (!IsStandaloneMode() && io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_B)) {
			TriggerScriptBuild();
		}

		// Undo / Redo (Disabled in Play mode)
		if (sceneState_ != EditorSceneState::Play && !IsStandaloneMode()) {
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
			if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) Redo();
		}

		if (!IsStandaloneMode()) {
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveScene(currentScenePath_);
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) NewScene();
			if (ImGui::IsKeyPressed(ImGuiKey_Delete)) DeleteSelectedGameObject();

			if (!isFlightNavigating_) {
				if (ImGui::IsKeyPressed(ImGuiKey_Q)) currentGizmo_ = GizmoType::Hand;
				if (ImGui::IsKeyPressed(ImGuiKey_W)) currentGizmo_ = GizmoType::Move;
				if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmo_ = GizmoType::Rotate;
				if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmo_ = GizmoType::Scale;
				if (ImGui::IsKeyPressed(ImGuiKey_T)) currentGizmo_ = GizmoType::Bounds;
				if (ImGui::IsKeyPressed(ImGuiKey_C)) isEditingColliderBounds_ = !isEditingColliderBounds_;
			}
		}
	}
#endif

	// Slowly rotate 3D model preview if active
	if (previewModel_) {
		previewRotationY_ += deltaTime * 1.1f;
		if (previewRotationY_ > 6.2831853f) {
			previewRotationY_ -= 6.2831853f;
		}
	}

	// 1. Particle System Parent Attachment & World-Space Simulation Update
	for (auto& obj : gameObjects_) {
		if (obj.isActive && obj.hasParticleSystem) {
			if (obj.attachedParentId != -1) {
				bool parentFound = false;
				for (const auto& parent : gameObjects_) {
					if (parent.id == static_cast<uint32_t>(obj.attachedParentId)) {
						obj.translation = parent.translation + obj.localOffset;
						parentFound = true;
						break;
					}
				}
				if (!parentFound) {
					// Fall back to None (World) safely if parent was deleted
					obj.attachedParentId = -1;
					obj.localOffset = { 0.0f, 0.0f, 0.0f };
				}
			}
			EnsureParticleQuadInitialized(obj);
			if (obj.particleQuad) {
				obj.particleQuad->emitter.translate = obj.translation;
				obj.particleQuad->Simulate(deltaTime);
			}
			if (particleSystem_) {
				particleSystem_->emitter.translate = obj.translation;
			}
		}
	}

	// 2. 3D Tetris Game Loop
	if (isTetrisActive_) {
		UpdateTetrisGame(deltaTime, input);
	}

	// 3. Authoritative Main Camera Sync to gameCamera_
	for (const auto& obj : gameObjects_) {
		if (obj.isActive && (obj.hasCamera || obj.type == GameObjectType::Camera)) {
			if (gameCamera_) {
				gameCamera_->translation = obj.translation;
				gameCamera_->rotation = obj.rotation;
				gameCamera_->fovY = obj.fov * 3.14159265f / 180.0f;
				gameCamera_->nearClip = obj.nearClip;
				gameCamera_->farClip = obj.farClip;
				gameCamera_->Update();
			}
			break;
		}
	}

	// 4. Authoritative Punctual Lights Sync to lights_ (LightGroup)
	if (lights_) {
		int ptCount = 0;
		int spCount = 0;
		for (int i = 0; i < LightGroup::MaxPointLights; ++i) lights_->GetPointLight(i).active = 0;
		for (int i = 0; i < LightGroup::MaxSpotLights; ++i) lights_->GetSpotLight(i).active = 0;

		for (const auto& obj : gameObjects_) {
			if (!obj.isActive || !obj.hasLight) continue;

			if (obj.lightType == GameObject::LightType::Directional) {
				// Sync direction with Transform rotation (local forward is +Z) so rotating Directional Light in Scene/Inspector rotates illumination
				Matrix4x4 rotMat = Mathematics::Multiply(Mathematics::MakeRotateXMatrix(obj.rotation.x),
				                   Mathematics::Multiply(Mathematics::MakeRotateYMatrix(obj.rotation.y),
				                                         Mathematics::MakeRotateZMatrix(obj.rotation.z)));
				Vector3 dir = Mathematics::Transform({ 0.0f, 0.0f, 1.0f }, rotMat);
				float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
				if (len > 0.0001f) { dir.x /= len; dir.y /= len; dir.z /= len; }
				const_cast<GameObject&>(obj).lightDirection = dir;
				lights_->GetDirectionalLight().direction = dir;
				lights_->GetDirectionalLight().color = { obj.lightColor.x, obj.lightColor.y, obj.lightColor.z, 1.0f };
				lights_->GetDirectionalLight().intensity = obj.lightIntensity;
				lights_->GetDirectionalLight().active = obj.lightEnabled ? 1 : 0;
				lights_->SetShadowActive((obj.lightEnabled && obj.lightCastShadow) ? 1 : 0);
				lights_->SetShadowBias(obj.lightShadowBias);
				lights_->SetShadowStrength(obj.lightShadowStrength);
				lights_->SetShadowNormalBias(obj.lightShadowNormalBias);
			} else if (obj.lightType == GameObject::LightType::Point && ptCount < LightGroup::MaxPointLights) {
				auto& pl = lights_->GetPointLight(ptCount++);
				pl.position = obj.translation;
				pl.color = { obj.lightColor.x, obj.lightColor.y, obj.lightColor.z, 1.0f };
				pl.intensity = obj.lightIntensity;
				pl.radius = obj.lightRange;
				pl.decay = obj.lightDecay;
				pl.active = obj.lightEnabled ? 1 : 0;
			} else if (obj.lightType == GameObject::LightType::Spot && spCount < LightGroup::MaxSpotLights) {
				auto& sl = lights_->GetSpotLight(spCount++);
				sl.position = obj.translation;

				// Sync direction with Transform rotation (local forward is +Z)
				Matrix4x4 rotMat = Mathematics::Multiply(Mathematics::MakeRotateXMatrix(obj.rotation.x),
				                   Mathematics::Multiply(Mathematics::MakeRotateYMatrix(obj.rotation.y),
				                                         Mathematics::MakeRotateZMatrix(obj.rotation.z)));
				Vector3 fwd = Mathematics::Transform({ 0.0f, 0.0f, 1.0f }, rotMat);
				float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
				if (len > 0.0001f) { fwd.x /= len; fwd.y /= len; fwd.z /= len; }
				sl.direction = fwd;
				const_cast<GameObject&>(obj).lightDirection = fwd;

				sl.color = { obj.lightColor.x, obj.lightColor.y, obj.lightColor.z, 1.0f };
				sl.intensity = obj.lightIntensity;
				sl.distance = obj.lightRange;
				sl.decay = obj.lightDecay;
				sl.cosAngle = std::cos(obj.lightSpotAngle * 3.14159265f / 180.0f);
				sl.cosFalloffStart = std::cos(obj.lightSpotFalloffStart * 3.14159265f / 180.0f);
				// In C++ enforce cosFalloffStart > cosAngle + 0.001f
				if (sl.cosFalloffStart <= sl.cosAngle + 0.001f) {
					sl.cosFalloffStart = sl.cosAngle + 0.001f;
				}
				sl.active = obj.lightEnabled ? 1 : 0;
			}
		}
		lights_->Update();
	}

	// 5. Script & Physics Simulation (Play mode only for Scripts; respects Pause and Step)
	const bool advancePlayFrame =
		(sceneState_ == EditorSceneState::Play) ||
		(sceneState_ == EditorSceneState::Pause && stepSingleFrame_) ||
		IsStandaloneMode();
	stepSingleFrame_ = false;

	if (advancePlayFrame) {
		UpdateScripts(deltaTime);
	}

	if (advancePlayFrame || (sceneState_ == EditorSceneState::Edit && simulatePhysicsInEditMode_)) {
		UpdatePhysics(deltaTime);
	}

	// Process any deferred GameObject deletions requested by scripts during Update/OnCollision
	if (!pendingDeletes_.empty()) {
		ProcessPendingDeletes();
	}
}

void EditorSuite::CalculateLightViewProjection()
{
	Vector3 lightDir = { 0.5f, -1.0f, 0.5f };
	for (const auto& obj : gameObjects_) {
		if (obj.isActive && obj.hasLight && obj.lightEnabled && obj.lightType == GameObject::LightType::Directional) {
			lightDir = obj.lightDirection;
			break;
		}
	}
	float len = std::sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
	if (len > 0.0001f) {
		lightDir = { lightDir.x / len, lightDir.y / len, lightDir.z / len };
	} else {
		lightDir = { 0.0f, -1.0f, 0.0f };
	}

	Vector3 minBound = { 1e9f, 1e9f, 1e9f };
	Vector3 maxBound = { -1e9f, -1e9f, -1e9f };
	bool hasCasters = false;

	for (const auto& obj : gameObjects_) {
		if (obj.isActive && obj.hasMesh && obj.meshEnabled && obj.castShadow) {
			hasCasters = true;
			float maxExtent = (std::max)({ obj.scale.x, obj.scale.y, obj.scale.z });
			minBound.x = (std::min)(minBound.x, obj.translation.x - maxExtent);
			minBound.y = (std::min)(minBound.y, obj.translation.y - maxExtent);
			minBound.z = (std::min)(minBound.z, obj.translation.z - maxExtent);

			maxBound.x = (std::max)(maxBound.x, obj.translation.x + maxExtent);
			maxBound.y = (std::max)(maxBound.y, obj.translation.y + maxExtent);
			maxBound.z = (std::max)(maxBound.z, obj.translation.z + maxExtent);
		}
	}

	Vector3 center = { 0.0f, 0.0f, 0.0f };
	float orthoSize = 25.0f;
	if (hasCasters) {
		center = { (minBound.x + maxBound.x) * 0.5f, (minBound.y + maxBound.y) * 0.5f, (minBound.z + maxBound.z) * 0.5f };
		float dx = (maxBound.x - minBound.x) * 0.5f;
		float dy = (maxBound.y - minBound.y) * 0.5f;
		float dz = (maxBound.z - minBound.z) * 0.5f;
		float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
		orthoSize = (std::max)(radius * 1.2f, 20.0f);
	}

	float lightDist = 50.0f;
	Vector3 lightPos = { center.x - lightDir.x * lightDist, center.y - lightDir.y * lightDist, center.z - lightDir.z * lightDist };

	Vector3 fwd = lightDir;
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (std::abs(fwd.y) > 0.99f) {
		up = { 0.0f, 0.0f, 1.0f };
	}

	Vector3 right = {
		up.y * fwd.z - up.z * fwd.y,
		up.z * fwd.x - up.x * fwd.z,
		up.x * fwd.y - up.y * fwd.x
	};
	float rightLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
	if (rightLen > 0.0001f) {
		right = { right.x / rightLen, right.y / rightLen, right.z / rightLen };
	} else {
		right = { 1.0f, 0.0f, 0.0f };
	}

	Vector3 realUp = {
		fwd.y * right.z - fwd.z * right.y,
		fwd.z * right.x - fwd.x * right.z,
		fwd.x * right.y - fwd.y * right.x
	};

	Matrix4x4 viewMat = {};
	viewMat.m[0][0] = right.x;
	viewMat.m[1][0] = right.y;
	viewMat.m[2][0] = right.z;
	viewMat.m[3][0] = -(lightPos.x * right.x + lightPos.y * right.y + lightPos.z * right.z);

	viewMat.m[0][1] = realUp.x;
	viewMat.m[1][1] = realUp.y;
	viewMat.m[2][1] = realUp.z;
	viewMat.m[3][1] = -(lightPos.x * realUp.x + lightPos.y * realUp.y + lightPos.z * realUp.z);

	viewMat.m[0][2] = fwd.x;
	viewMat.m[1][2] = fwd.y;
	viewMat.m[2][2] = fwd.z;
	viewMat.m[3][2] = -(lightPos.x * fwd.x + lightPos.y * fwd.y + lightPos.z * fwd.z);

	viewMat.m[3][3] = 1.0f;

	float nearClip = 0.1f;
	float farClip = 120.0f;
	Matrix4x4 projMat = Mathematics::MakeOrthographicMatrix(-orthoSize, orthoSize, orthoSize, -orthoSize, nearClip, farClip);

	lightViewProjectionMatrix_ = Mathematics::Multiply(viewMat, projMat);
}

void EditorSuite::RenderShadowPass(ID3D12GraphicsCommandList* cmdList)
{
	bool dirFound = false;
	bool castShadows = true;
	float shadowBias = 0.002f;
	float shadowStrength = 1.0f;
	float shadowNormalBias = 0.02f;

	for (const auto& obj : gameObjects_) {
		if (obj.isActive && obj.hasLight && obj.lightEnabled && obj.lightType == GameObject::LightType::Directional) {
			dirFound = true;
			castShadows = obj.lightCastShadow;
			shadowBias = obj.lightShadowBias;
			shadowStrength = obj.lightShadowStrength;
			shadowNormalBias = obj.lightShadowNormalBias;
			break;
		}
	}

	if (!dirFound || !castShadows) {
		if (lights_) {
			lights_->SetShadowActive(0);
			lights_->Update();
		}
		shadowMap_.BeginShadowPass(cmdList);
		shadowMap_.EndShadowPass(cmdList);
		return;
	}

	CalculateLightViewProjection();

	if (lights_) {
		lights_->SetLightViewProjection(lightViewProjectionMatrix_);
		lights_->SetShadowActive(1);
		lights_->SetShadowBias(shadowBias);
		lights_->SetShadowStrength(shadowStrength);
		lights_->SetShadowNormalBias(shadowNormalBias);
		lights_->Update();
	}

	shadowMap_.BeginShadowPass(cmdList);

	Frustum shadowFrustum = Frustum::ExtractFromViewProjection(lightViewProjectionMatrix_);

	for (auto& obj : gameObjects_) {
		if (!obj.isActive || !obj.hasMesh || !obj.meshEnabled || !obj.castShadow) {
			continue;
		}

		Model* model = nullptr;
		if (obj.meshType == GameObject::MeshType::Cube) {
			model = Model::CreateCube();
		} else if (obj.meshType == GameObject::MeshType::Sphere) {
			model = Model::CreateSphere();
		} else if (obj.meshType == GameObject::MeshType::Plane) {
			model = Model::CreatePlane();
		} else if (obj.meshType == GameObject::MeshType::Quad) {
			model = Model::CreateQuad();
		} else if (obj.meshType == GameObject::MeshType::CustomModel && !obj.modelPath.empty()) {
			std::filesystem::path mp(obj.modelPath);
			model = Model::CreateFromFile(mp.parent_path().string(), mp.filename().string());
		}

		if (!model) continue;

		if (!obj.shadowTransform) {
			obj.shadowTransform = std::make_unique<WorldTransform>();
			obj.shadowTransform->Initialize();
		}
		obj.shadowTransform->translation = obj.translation;
		obj.shadowTransform->rotation = obj.rotation;
		obj.shadowTransform->scale = obj.scale;
		obj.shadowTransform->rotationQuat = obj.rotationQuat;
		obj.shadowTransform->UpdateMatrix(lightViewProjectionMatrix_);

		if (enableFrustumCulling_) {
			AABB worldBox = Frustum::TransformAABB(model->GetLocalAABB(), obj.shadowTransform->matWorld);
			if (!shadowFrustum.IsAABBVisible(worldBox)) {
				continue;
			}
		}

		cmdList->SetGraphicsRootConstantBufferView(1, obj.shadowTransform->GetGPUAddress());
		if (obj.editObject3d && obj.editObject3d->GetBonePaletteGPUAddress() != 0) {
			cmdList->SetGraphicsRootConstantBufferView(4, obj.editObject3d->GetBonePaletteGPUAddress());
		} else if (obj.gameObject3d && obj.gameObject3d->GetBonePaletteGPUAddress() != 0) {
			cmdList->SetGraphicsRootConstantBufferView(4, obj.gameObject3d->GetBonePaletteGPUAddress());
		}
		model->Draw(cmdList);
	}

	shadowMap_.EndShadowPass(cmdList);
}

void EditorSuite::RenderScene3D(ID3D12GraphicsCommandList* cmdList, Camera& camera, bool isEditView)
{
	camera.Update();
	if (lights_) {
		lights_->SetCameraPosition(camera.translation);
	}

	Frustum camFrustum = Frustum::ExtractFromViewProjection(camera.GetViewProjectionMatrix());
	int visibleCount = 0;
	int culledCount = 0;

	for (auto& obj : gameObjects_) {
		if (!obj.isActive || !obj.hasMesh || !obj.meshEnabled) {
			continue;
		}

		auto& obj3d = isEditView ? obj.editObject3d : obj.gameObject3d;
		if (!obj3d) {
			obj3d = std::make_unique<Object3d>();
			obj3d->Initialize();
		}

		// Bind Model Primitive or Custom 3D Mesh
		Model* model = nullptr;
		if (obj.meshType == GameObject::MeshType::Cube) {
			model = Model::CreateCube();
		} else if (obj.meshType == GameObject::MeshType::Sphere) {
			model = Model::CreateSphere();
		} else if (obj.meshType == GameObject::MeshType::Plane) {
			model = Model::CreatePlane();
		} else if (obj.meshType == GameObject::MeshType::Quad) {
			model = Model::CreateQuad();
		} else if (obj.meshType == GameObject::MeshType::CustomModel && !obj.modelPath.empty()) {
			std::filesystem::path mp(obj.modelPath);
			model = Model::CreateFromFile(mp.parent_path().string(), mp.filename().string());
		}

		if (model && obj3d->GetModel() != model) {
			obj3d->SetModel(model);
		}

		Material* mat = obj3d->GetMaterial();
		if (mat) {
			mat->color = obj.materialColor;
			mat->enableLighting = (obj.isLit && enableLighting_) ? 1 : 0;
			mat->lightingType = obj.lightingType; // Unlit, Lambert, Half-Lambert, Phong, Blinn-Phong
			mat->shininess = obj.shininess;
			mat->specularColor = obj.specularColor;
			mat->receiveShadows = (obj.receiveShadow && obj.lightingType != 0) ? 1 : 0;
			mat->explodeAmount = obj.explodeAmount;
			mat->uvOffset = obj.uvOffset;
			mat->uvTiling = obj.uvTiling;
			mat->uvRotation = obj.uvRotation;
		}

		obj3d->SetTextureHandle(obj.textureHandle);
		GeometryShaderMode gsMode = static_cast<GeometryShaderMode>(obj.geometryShaderEffect);
		if (gsMode == GeometryShaderMode::None && obj.useGeometryShader) {
			gsMode = GeometryShaderMode::PassThrough;
		}
		obj3d->SetGeometryShaderMode(gsMode);
		obj3d->SetUseGeometryShader(obj.useGeometryShader);
		obj3d->worldTransform.translation = obj.translation;
		obj3d->worldTransform.rotation = obj.rotation;
		obj3d->worldTransform.scale = obj.scale;
		obj3d->worldTransform.rotationQuat = obj.rotationQuat;
		obj3d->SetCamera(&camera);

		obj3d->SetCurrentAnimationIndex(static_cast<uint32_t>((std::max)(0, obj.animationIndex)));
		obj3d->SetAnimationSpeed(obj.animationSpeed);
		obj3d->SetLoopAnimation(obj.animationLoop);

		if (isEditView) {
			float animDt = obj.animationPlaying ? currentDeltaTime_ : 0.0f;
			obj3d->Update(animDt);
		} else {
			obj3d->Update(0.0f);
			if (obj.editObject3d) {
				obj3d->CopyBonePaletteFrom(*obj.editObject3d);
			}
		}

		if (enableFrustumCulling_ && model) {
			AABB localBox = model->GetLocalAABB();
			if (model->GetAnimationCount() > 0) {
				// Expand bounding box slightly for animated meshes so bone motion does not clip prematurely
				Vector3 halfExt = (localBox.max - localBox.min) * 0.5f;
				localBox.min = localBox.min - halfExt;
				localBox.max = localBox.max + halfExt;
			}
			AABB worldBox = Frustum::TransformAABB(localBox, obj3d->worldTransform.matWorld);
			if (!camFrustum.IsAABBVisible(worldBox)) {
				culledCount++;
				continue;
			}
		}

		visibleCount++;
		obj3d->Draw(cmdList);
	}

	if (isEditView) {
		lastVisibleObjects_ = visibleCount;
		lastCulledObjects_ = culledCount;
	}
}

void EditorSuite::RenderViewports(ID3D12GraphicsCommandList* cmdList, bool showParticles)
{
	if (!dxCommon_ || dxCommon_->IsDeviceLost() || !cmdList) {
		return;
	}

	bool activeParticles = HasActiveParticleSystem() && showParticles;

	// 0. Directional Light Shadow Pass (Render active casters into shadow map)
	RenderShadowPass(cmdList);

#ifdef USE_IMGUI
	// 1. Render to EDIT VIEW (Scene Camera)
	editViewRT_.BeginRender();
	editorCamera_.Update();
	skybox_.Draw(cmdList, editorCamera_);
	RenderScene3D(cmdList, editorCamera_, true);
	if (activeParticles) {
		for (auto& obj : gameObjects_) {
			if (obj.isActive && obj.hasParticleSystem && obj.particleQuad) {
				obj.particleQuad->PrepareForCamera(editorCamera_, ParticleQuad::kPassScene);
				obj.particleQuad->Draw(cmdList, ParticleQuad::kPassScene);
			}
		}
	}
	editViewRT_.EndRender();

	// 2. Render to GAME VIEW (Player Game Camera)
	gameViewRT_.BeginRender();
	if (gameCamera_) {
		gameCamera_->Update();
		skybox_.Draw(cmdList, *gameCamera_);
		RenderScene3D(cmdList, *gameCamera_, false);
		if (activeParticles) {
			for (auto& obj : gameObjects_) {
				if (obj.isActive && obj.hasParticleSystem && obj.particleQuad) {
					obj.particleQuad->PrepareForCamera(*gameCamera_, ParticleQuad::kPassGame);
					obj.particleQuad->Draw(cmdList, ParticleQuad::kPassGame);
				}
			}
		}
	}
	gameViewRT_.EndRender();

	// 3. Render 3D Asset Preview (if a 3D model is currently selected in the Asset Browser)
	if (previewModel_ && previewObject3d_ && modelPreviewRT_.IsInitialized() && !IsStandaloneMode()) {
		modelPreviewRT_.BeginRender();
		TextureManager::GetInstance()->SetDescriptorHeaps(cmdList);
		previewCamera_.Update();

		if (previewLights_) {
			previewLights_->SetCameraPosition(previewCamera_.translation);
			previewLights_->Update();
			Object3d::StaticSetLightGroup(previewLights_.get());
		}

		previewObject3d_->worldTransform.scale = { previewModelScale_, previewModelScale_, previewModelScale_ };
		previewObject3d_->worldTransform.rotation = { 0.0f, previewRotationY_, 0.0f };
		previewObject3d_->worldTransform.rotationQuat = Quaternion::FromEuler(previewObject3d_->worldTransform.rotation);

		// Center the scaled model around the origin regardless of its Y rotation angle
		const float cx = previewModelOffset_.x * previewModelScale_;
		const float cy = previewModelOffset_.y * previewModelScale_;
		const float cz = previewModelOffset_.z * previewModelScale_;
		const float cosY = std::cos(previewRotationY_);
		const float sinY = std::sin(previewRotationY_);
		const float rx = cx * cosY + cz * sinY;
		const float ry = cy;
		const float rz = -cx * sinY + cz * cosY;
		previewObject3d_->worldTransform.translation = { -rx, -ry, -rz };

		previewObject3d_->SetCamera(&previewCamera_);
		previewObject3d_->Update(currentDeltaTime_);
		previewObject3d_->Draw(cmdList);

		if (lights_) {
			Object3d::StaticSetLightGroup(lights_);
		}
		modelPreviewRT_.EndRender();
	}

	// 4. Restore to swapchain backbuffer for ImGui UI drawing
	dxCommon_->BindMainRenderTarget();
#else
	// Standalone Release build without ImGui: render Game View directly to the main swapchain backbuffer
	dxCommon_->BindMainRenderTarget();
	if (gameCamera_) {
		gameCamera_->Update();
		skybox_.Draw(cmdList, *gameCamera_);
		RenderScene3D(cmdList, *gameCamera_, true);
		if (activeParticles) {
			for (auto& obj : gameObjects_) {
				if (obj.isActive && obj.hasParticleSystem && obj.particleQuad) {
					obj.particleQuad->PrepareForCamera(*gameCamera_, ParticleQuad::kPassGame);
					obj.particleQuad->Draw(cmdList, ParticleQuad::kPassGame);
				}
			}
		}
	}
#endif
}

void EditorSuite::DrawEditorUI(float deltaTime, bool& showParticles)
{
	if (!dxCommon_ || dxCommon_->IsDeviceLost()) {
		return;
	}
#ifdef USE_IMGUI
	if (IsStandaloneMode()) {
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("##StandaloneFullscreenGameView", nullptr, flags)) {
			ImVec2 avail = ImGui::GetContentRegionAvail();
			ImGui::Image(ImTextureRef((ImTextureID)gameViewRT_.GetSrvGPUHandle().ptr), avail);

			if (isStandalonePreview_) {
				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddRectFilled(ImVec2(10.0f, 10.0f), ImVec2(360.0f, 38.0f), IM_COL32(0, 0, 0, 180), 4.0f);
				dl->AddText(ImVec2(18.0f, 16.0f), IM_COL32(0, 220, 255, 255), "Standalone Game Preview | Press F11 or Esc to exit");
			}

			if (isTetrisActive_) {
				DrawTetrisHUD(ImVec2(0.0f, 0.0f), avail);
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
		return;
	}

	DrawMainMenuBar();
	DrawTopPlayBar();
	DrawHierarchyPanel();
	DrawViewportsPanel();
	DrawProjectAndConsolePanel();
	DrawInspectorAndDetailsPanel(showParticles);

	if (showScriptGenModal_) DrawCppScriptGeneratorModal();
	if (showNewProjectModal_) DrawNewProjectModal();
	if (showOpenProjectModal_) DrawOpenProjectModal();
	if (showBuildSettingsModal_) DrawBuildSettingsModal();
	if (showSaveSceneAsModal_) DrawSaveSceneAsModal();
	if (showOpenSceneModal_) DrawOpenSceneModal();

	if (showLightingSettings_) {
		ImGui::SetNextWindowSize(ImVec2(380.0f, 420.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Lighting & Environment", &showLightingSettings_)) {
			DrawSkyboxSettingsUI();
		}
		ImGui::End();
	}
#endif
}

void EditorSuite::ApplyUnity6DarkTheme()
{
#ifdef USE_IMGUI
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.ChildRounding = 2.0f;
	style.FrameRounding = 3.0f;
	style.PopupRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 3.0f;
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.FramePadding = ImVec2(6.0f, 4.0f);
	style.ItemSpacing = ImVec2(6.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg]             = ImVec4(0.157f, 0.157f, 0.157f, 1.0f); // #282828
	colors[ImGuiCol_ChildBg]              = ImVec4(0.118f, 0.118f, 0.118f, 1.0f); // #1E1E1E
	colors[ImGuiCol_PopupBg]              = ImVec4(0.140f, 0.140f, 0.140f, 0.98f);
	colors[ImGuiCol_Border]               = ImVec4(0.100f, 0.100f, 0.100f, 1.0f);
	colors[ImGuiCol_FrameBg]              = ImVec4(0.118f, 0.118f, 0.118f, 1.0f); // #1E1E1E
	colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);
	colors[ImGuiCol_FrameBgActive]        = ImVec4(0.240f, 0.240f, 0.240f, 1.0f);
	colors[ImGuiCol_TitleBg]              = ImVec4(0.130f, 0.130f, 0.130f, 1.0f);
	colors[ImGuiCol_TitleBgActive]        = ImVec4(0.160f, 0.160f, 0.160f, 1.0f);
	colors[ImGuiCol_MenuBarBg]            = ImVec4(0.137f, 0.137f, 0.137f, 1.0f);
	colors[ImGuiCol_Header]               = ImVec4(0.220f, 0.220f, 0.220f, 1.0f); // #383838
	colors[ImGuiCol_HeaderHovered]        = ImVec4(0.280f, 0.280f, 0.280f, 1.0f);
	colors[ImGuiCol_HeaderActive]         = ImVec4(0.176f, 0.447f, 0.851f, 1.0f); // Unity Blue
	colors[ImGuiCol_Button]               = ImVec4(0.220f, 0.220f, 0.220f, 1.0f); // #383838
	colors[ImGuiCol_ButtonHovered]        = ImVec4(0.280f, 0.280f, 0.280f, 1.0f);
	colors[ImGuiCol_ButtonActive]         = ImVec4(0.176f, 0.447f, 0.851f, 1.0f);
	colors[ImGuiCol_CheckMark]            = ImVec4(0.200f, 0.600f, 1.000f, 1.0f);
	colors[ImGuiCol_SliderGrab]           = ImVec4(0.350f, 0.350f, 0.350f, 1.0f);
	colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.176f, 0.447f, 0.851f, 1.0f);
	colors[ImGuiCol_Tab]                  = ImVec4(0.150f, 0.150f, 0.150f, 1.0f);
	colors[ImGuiCol_TabHovered]           = ImVec4(0.280f, 0.280f, 0.280f, 1.0f);
	colors[ImGuiCol_TabSelected]          = ImVec4(0.220f, 0.220f, 0.220f, 1.0f);
	colors[ImGuiCol_TabDimmed]            = ImVec4(0.120f, 0.120f, 0.120f, 1.0f);
	colors[ImGuiCol_TabDimmedSelected]    = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);
#endif
}

void EditorSuite::DrawMainMenuBar()
{
#ifdef USE_IMGUI
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Scene", "Ctrl+N")) { NewScene(); }
			if (ImGui::MenuItem("Open Scene...")) { showOpenSceneModal_ = true; }
			if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
				if (currentScenePath_.empty()) showSaveSceneAsModal_ = true;
				else SaveScene(currentScenePath_);
			}
			if (ImGui::MenuItem("Save Scene As...")) { showSaveSceneAsModal_ = true; }
			if (ImGui::MenuItem("Revert Unsaved Changes")) { DiscardUnsavedChanges(); }
			ImGui::Separator();
			if (ImGui::MenuItem("New Project...")) { showNewProjectModal_ = true; }
			if (ImGui::MenuItem("Open Project...")) { showOpenProjectModal_ = true; }
			ImGui::Separator();
			if (ImGui::MenuItem("Load Game Template > Playable 3D Tetris")) { LoadTetrisTemplate(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Build & Export Game...")) { showBuildSettingsModal_ = true; }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) { Undo(); }
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) { Redo(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Delete Selected", "Del")) { DeleteSelectedGameObject(); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Assets")) {
			if (ImGui::MenuItem("Create C++ Script...")) {
				scriptGenTargetObjectIndex_ = -1;
				scriptGenErrorMsg_.clear();
				showScriptGenModal_ = true;
			}
			if (ImGui::MenuItem("Build Scripts (GameScripts.dll)", "Ctrl+Shift+B")) { TriggerScriptBuild(); }
			ImGui::MenuItem("Auto Build Scripts on Save", nullptr, &autoBuildScripts_);
			ImGui::Separator();
			if (ImGui::MenuItem("Refresh Asset Database", "Ctrl+R")) { RefreshAssetBrowser(); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("GameObject")) {
			DrawHierarchyCreationMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window")) {
			if (ImGui::MenuItem("Preview Standalone Game Mode", "F11")) { ToggleStandalonePreview(); }
			if (ImGui::MenuItem("Lighting / Environment", nullptr, showLightingSettings_)) {
				showLightingSettings_ = !showLightingSettings_;
			}
			if (ImGui::BeginMenu("Application Scene")) {
				bool canSwitch = (sceneState_ == EditorSceneState::Edit);
				if (ImGui::MenuItem("Title Scene (Demo)", nullptr, false, canSwitch)) {
					if (onRequestTitleScene_) {
						onRequestTitleScene_();
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About Rizeallday Engine v2")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
#endif
}

void EditorSuite::DrawTopPlayBar()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float barH = 38.0f;
	float menuH = ImGui::GetFrameHeight();

	ImGui::SetNextWindowPos(ImVec2(0.0f, menuH));
	ImGui::SetNextWindowSize(ImVec2(screenW, barH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
	if (ImGui::Begin("##TopPlayBar", nullptr, flags)) {
		// Left: Engine Badge & Script Hot Reload Status
		ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Rizeallday v2");
		ImGui::SameLine();
		if (isBuildingScripts_.load()) {
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.25f, 1.0f), "| Building scripts...");
		} else if (lastScriptBuildStatus_.load() == 2) {
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "| Script Build Failed");
		} else {
			ImGui::TextColored(ImVec4(0.35f, 0.9f, 0.45f, 1.0f), "| Scripts Ready");
		}

		// Center: Play, Pause, Step, Stop Buttons
		float centerPos = (screenW - 580.0f) * 0.42f;
		if (centerPos < 230.0f) centerPos = 230.0f;
		ImGui::SameLine(centerPos);

		bool isPlay = (sceneState_ == EditorSceneState::Play);
		bool isPause = (sceneState_ == EditorSceneState::Pause);
		bool inPlaySession = (isPlay || isPause);
		if (inPlaySession) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.22f, 1.0f));
		if (ImGui::Button(inPlaySession ? "[ > PLAYING ]" : "[ > Play ]", ImVec2(92.0f, 24.0f))) {
			if (!inPlaySession) {
				StartPlayMode();
			} else {
				StopPlayMode();
			}
		}
		if (inPlaySession) ImGui::PopStyleColor();
		ImGui::SetItemTooltip("Play / stop scene simulation");

		ImGui::SameLine();
		if (isPause) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.45f, 0.12f, 1.0f));
		if (ImGui::Button("[ || Pause ]", ImVec2(78.0f, 24.0f))) {
			if (sceneState_ == EditorSceneState::Play) {
				sceneState_ = EditorSceneState::Pause;
			} else if (sceneState_ == EditorSceneState::Pause) {
				sceneState_ = EditorSceneState::Play;
			}
		}
		if (isPause) ImGui::PopStyleColor();
		ImGui::SetItemTooltip("Pause running game simulation");

		ImGui::SameLine();
		if (ImGui::Button("[ >| Step ]", ImVec2(76.0f, 24.0f))) {
			if (sceneState_ == EditorSceneState::Edit) {
				StartPlayMode();
			}
			sceneState_ = EditorSceneState::Pause;
			stepSingleFrame_ = true;
		}
		ImGui::SetItemTooltip("Step forward by a single frame");

		ImGui::SameLine();
		if (ImGui::Button("[ [] Stop ]", ImVec2(76.0f, 24.0f))) {
			if (sceneState_ != EditorSceneState::Edit) {
				StopPlayMode();
			}
		}
		ImGui::SetItemTooltip("Stop play mode and restore original transforms");

		ImGui::SameLine();
		ImGui::Checkbox("Sim Physics", &simulatePhysicsInEditMode_);
		ImGui::SetItemTooltip("Toggle real-time physics simulation in Edit mode");

		// Right: Script Hot-Reload & Build Actions
		ImGui::SameLine();
		bool building = isBuildingScripts_.load();
		if (building) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.45f, 0.15f, 1.0f));
		if (ImGui::Button(building ? "[ Building... ]" : "[ Build Scripts ]", ImVec2(118.0f, 24.0f))) {
			TriggerScriptBuild();
		}
		if (building) ImGui::PopStyleColor();
		ImGui::SetItemTooltip("Compile GameScripts.dll in background via MSBuild (Ctrl+Shift+B)");

		ImGui::SameLine();
		ImGui::Checkbox("Auto Build Scripts", &autoBuildScripts_);
		ImGui::SetItemTooltip("Automatically rebuild and hot-reload GameScripts.dll ~1s after saving any .cpp/.h in GameScripts/Scripts");

		ImGui::SameLine();
		if (ImGui::Button("[ Build / Export ]", ImVec2(115.0f, 24.0f))) {
			showBuildSettingsModal_ = true;
		}
		ImGui::SetItemTooltip("Open Build Settings & Standalone Game Exporter");

		ImGui::SameLine();
		if (ImGui::Button("+ New C++ Script", ImVec2(112.0f, 24.0f))) {
			scriptGenTargetObjectIndex_ = selectedObjectIndex_;
			scriptGenErrorMsg_.clear();
			showScriptGenModal_ = true;
		}
		ImGui::SetItemTooltip("Generate new C++ GameObject script skeleton in GameScripts/Scripts");
	}
	ImGui::End();
	ImGui::PopStyleVar();
#endif
}

void EditorSuite::DrawHierarchyPanel()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float screenH = io.DisplaySize.y;
	float topY = ImGui::GetFrameHeight() + 38.0f;
	float leftW = screenW * 0.18f;
	if (leftW < 220.0f) leftW = 220.0f;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(0.0f, topY));
	ImGui::SetNextWindowSize(ImVec2(leftW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Hierarchy", nullptr, flags)) {
		// Sub-bar: [ + v ] creation button and search filter
		if (ImGui::Button(" + v ", ImVec2(36.0f, 22.0f))) {
			ImGui::OpenPopup("HierarchyCreationPopup");
		}
		ImGui::SetItemTooltip("Create new GameObject in Scene");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##SearchHierarchy", "Search All...", hierarchySearchBuf_, sizeof(hierarchySearchBuf_));

		ImGui::Separator();

		if (ImGui::BeginPopup("HierarchyCreationPopup")) {
			DrawHierarchyCreationMenu();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupContextWindow("HierarchyContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			DrawHierarchyCreationMenu();
			ImGui::EndPopup();
		}

		// Hierarchy Scene Name with dynamic name and dirty flag '*'
		std::string sceneLabel = currentSceneName_ + (isSceneDirty_ ? "*" : "");

		if (isEditingSceneName_) {
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##EditSceneName", sceneNameEditBuf_, sizeof(sceneNameEditBuf_), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				currentSceneName_ = sceneNameEditBuf_;
				isEditingSceneName_ = false;
				isSceneDirty_ = true;
				PushUndoSnapshot();
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				currentSceneName_ = sceneNameEditBuf_;
				isEditingSceneName_ = false;
				isSceneDirty_ = true;
				PushUndoSnapshot();
			}
		} else {
			bool nodeOpen = ImGui::TreeNodeEx("##SceneTreeHeader", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", sceneLabel.c_str());

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
				isEditingSceneName_ = true;
				strncpy_s(sceneNameEditBuf_, currentSceneName_.c_str(), sizeof(sceneNameEditBuf_) - 1);
			}

			if (ImGui::BeginPopupContextItem("SceneHeaderContext")) {
				if (ImGui::MenuItem("Rename Scene")) {
					isEditingSceneName_ = true;
					strncpy_s(sceneNameEditBuf_, currentSceneName_.c_str(), sizeof(sceneNameEditBuf_) - 1);
				}
				if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
					if (currentScenePath_.empty()) showSaveSceneAsModal_ = true;
					else SaveScene(currentScenePath_);
				}
				ImGui::EndPopup();
			}

			if (nodeOpen) {
				for (size_t i = 0; i < gameObjects_.size(); ++i) {
					auto& obj = gameObjects_[i];
					if (hierarchySearchBuf_[0] != '\0' && obj.name.find(hierarchySearchBuf_) == std::string::npos) continue;

					ImGui::PushID(static_cast<int>(obj.id));
					if (ImGui::Checkbox("##Active", &obj.isActive)) {
						isSceneDirty_ = true;
						PushUndoSnapshot();
					}
					ImGui::SameLine();

					const char* icon = "[Obj]";
					if (obj.type == GameObjectType::Cube) icon = "[Cube]";
					else if (obj.type == GameObjectType::Sphere) icon = "[Sph]";
					else if (obj.type == GameObjectType::Plane) icon = "[Pln]";
					else if (obj.type == GameObjectType::Quad) icon = "[Quad]";
					else if (obj.type == GameObjectType::Camera) icon = "[Cam]";
					else if (obj.type == GameObjectType::DirectionalLight) icon = "[Sun]";
					else if (obj.type == GameObjectType::PointLight) icon = "[PtLt]";
					else if (obj.type == GameObjectType::SpotLight) icon = "[SpLt]";
					else if (obj.type == GameObjectType::ParticleSystem) icon = "[Prt]";
					else if (obj.type == GameObjectType::MapchipGrid) icon = "[Map]";
					else if (obj.type == GameObjectType::AudioSource || obj.hasAudioSource) icon = "[Aud]";

					std::string label = std::format("{} {}", icon, obj.name);
					ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (static_cast<int>(i) == selectedObjectIndex_) nodeFlags |= ImGuiTreeNodeFlags_Selected;

					if (!obj.isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					ImGui::TreeNodeEx(label.c_str(), nodeFlags);
					if (!obj.isActive) ImGui::PopStyleColor();

					if (ImGui::IsItemClicked()) {
						selectedObjectIndex_ = static_cast<int>(i);
					}

					if (ImGui::BeginPopupContextItem()) {
						selectedObjectIndex_ = static_cast<int>(i);
						if (ImGui::MenuItem("Delete GameObject", "Del")) {
							QueueDeleteGameObject(static_cast<int>(i));
						}
						if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
							pendingDuplicateIndex_ = static_cast<int>(i);
						}
						ImGui::EndPopup();
					}

					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
#endif
}

void EditorSuite::DrawHierarchyCreationMenu()
{
#ifdef USE_IMGUI
	if (ImGui::MenuItem("Create Empty", "Ctrl+Shift+N")) {
		CreateGameObject("GameObject", GameObjectType::Empty);
	}
	if (ImGui::MenuItem("Create Empty Child", "Alt+Shift+N")) {
		int parentIdx = selectedObjectIndex_;
		int parentId = (parentIdx >= 0 && parentIdx < static_cast<int>(gameObjects_.size())) ? static_cast<int>(gameObjects_[parentIdx].id) : -1;
		auto& child = CreateGameObject("ChildGameObject", GameObjectType::Empty);
		child.parentIndex = parentIdx;
		child.parentId = parentId;
		if (parentIdx >= 0 && parentIdx < static_cast<int>(gameObjects_.size()) - 1) {
			gameObjects_[parentIdx].childrenIds.push_back(child.id);
		}
	}
	if (ImGui::MenuItem("Create Empty Parent", "Ctrl+Shift+G")) {
		int childIdx = selectedObjectIndex_;
		auto& parent = CreateGameObject("ParentGameObject", GameObjectType::Empty);
		if (childIdx >= 0 && childIdx < static_cast<int>(gameObjects_.size()) - 1) {
			parent.translation = gameObjects_[childIdx].translation;
			gameObjects_[childIdx].parentId = static_cast<int>(parent.id);
			gameObjects_[childIdx].parentIndex = static_cast<int>(gameObjects_.size()) - 1;
			parent.childrenIds.push_back(gameObjects_[childIdx].id);
		}
	}
	ImGui::Separator();

	if (ImGui::BeginMenu("2D Object")) {
		if (ImGui::MenuItem("Sprite")) {
			CreateGameObject("Sprite", GameObjectType::Sprite);
		}
		if (ImGui::MenuItem("Textured Rectangle")) {
			CreateQuad("Textured Rectangle", { 0.0f, 0.5f, 0.0f });
		}
		if (ImGui::MenuItem("Mapchip Grid")) {
			auto& map = CreateGameObject("Level Mapchip Grid", GameObjectType::MapchipGrid);
			map.hasMapchip = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("3D Object")) {
		if (ImGui::MenuItem("Cube (with Box Collider)")) {
			CreateCube("Cube", { 0.0f, 0.5f, 0.0f });
		}
		if (ImGui::MenuItem("Sphere (with Sphere Collider)")) {
			CreateSphere("Sphere", { 0.0f, 0.5f, 0.0f });
		}
		if (ImGui::MenuItem("Plane (Ground)")) {
			CreatePlane("Plane", { 0.0f, 0.0f, 0.0f });
		}
		if (ImGui::MenuItem("Textured Quad")) {
			CreateQuad("Textured Quad", { 0.0f, 0.5f, 0.0f });
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Visual Effects")) {
		if (ImGui::MenuItem("Particle System")) {
			auto& ps = CreateGameObject("Particle System", GameObjectType::ParticleSystem);
			ps.hasParticleSystem = true;
			EnsureParticleQuadInitialized(ps);
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Light")) {
		if (ImGui::MenuItem("Directional Light")) {
			auto& light = CreateGameObject("Directional Light", GameObjectType::DirectionalLight);
			light.hasLight = true;
			light.lightType = GameObject::LightType::Directional;
			light.lightDirection = { 0.5f, -1.0f, 0.5f };
			Vector3 d = light.lightDirection;
			float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (dLen > 0.0001f) { d.x /= dLen; d.y /= dLen; d.z /= dLen; }
			light.rotation = { std::asin(std::clamp(-d.y, -1.0f, 1.0f)), std::atan2(d.x, d.z), 0.0f };
			light.rotationQuat = Quaternion::FromEuler(light.rotation);
		}
		if (ImGui::MenuItem("Point Light")) {
			auto& light = CreateGameObject("Point Light", GameObjectType::PointLight);
			light.hasLight = true;
			light.lightType = GameObject::LightType::Point;
			light.lightRange = 10.0f;
			light.lightDecay = 2.0f;
		}
		if (ImGui::MenuItem("Spot Light")) {
			auto& light = CreateGameObject("Spot Light", GameObjectType::SpotLight);
			light.hasLight = true;
			light.lightType = GameObject::LightType::Spot;
			light.lightRange = 7.0f;
			light.lightDecay = 2.0f;
			light.lightSpotAngle = 30.0f;
			light.lightSpotFalloffStart = 20.0f;
			light.rotation = { 0.5f, 0.0f, 0.0f };
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Audio")) {
		if (ImGui::MenuItem("Audio Source")) {
			auto& audio = CreateGameObject("Audio Source", GameObjectType::AudioSource);
			audio.hasAudioSource = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("UI (Canvas)")) {
		if (ImGui::MenuItem("Canvas")) {
			CreateGameObject("Canvas", GameObjectType::Canvas);
		}
		ImGui::EndMenu();
	}
#endif
}

void EditorSuite::DrawViewportsPanel()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float screenH = io.DisplaySize.y;
	float topY = ImGui::GetFrameHeight() + 38.0f;
	float leftW = screenW * 0.18f;
	if (leftW < 220.0f) leftW = 220.0f;
	float rightW = screenW * 0.24f;
	if (rightW < 300.0f) rightW = 300.0f;
	float centerW = screenW - leftW - rightW;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(leftW, topY));
	ImGui::SetNextWindowSize(ImVec2(centerW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Viewport Workspace", nullptr, flags)) {
		if (ImGui::BeginTabBar("ViewportTabs")) {
			// TAB 1: # SCENE VIEW (Interactive 3D Viewport)
			if (ImGui::BeginTabItem("# Scene")) {
				ImVec2 avail = ImGui::GetContentRegionAvail();
				ImVec2 imgPos = ImGui::GetCursorScreenPos();

				ImGui::Image(ImTextureRef((ImTextureID)editViewRT_.GetSrvGPUHandle().ptr), avail);

				bool isHovered = ImGui::IsItemHovered();

				// Flight Camera Navigation (RMB held inside # Scene)
				if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
					isFlightNavigating_ = true;
					ImVec2 mouseDelta = io.MouseDelta;
					float rotSpeed = 0.003f;
					editorCamera_.rotation.y += mouseDelta.x * rotSpeed;
					editorCamera_.rotation.x += mouseDelta.y * rotSpeed;
					editorCamera_.rotation.x = std::clamp(editorCamera_.rotation.x, -1.5f, 1.5f);

					float moveSpeed = 8.0f * io.DeltaTime;
					if (io.KeyShift) moveSpeed *= 2.5f;

					Vector3 forward = { std::sin(editorCamera_.rotation.y), 0.0f, std::cos(editorCamera_.rotation.y) };
					Vector3 right = { std::cos(editorCamera_.rotation.y), 0.0f, -std::sin(editorCamera_.rotation.y) };

					if (ImGui::IsKeyDown(ImGuiKey_W)) editorCamera_.translation = editorCamera_.translation + (forward * moveSpeed);
					if (ImGui::IsKeyDown(ImGuiKey_S)) editorCamera_.translation = editorCamera_.translation - (forward * moveSpeed);
					if (ImGui::IsKeyDown(ImGuiKey_D)) editorCamera_.translation = editorCamera_.translation + (right * moveSpeed);
					if (ImGui::IsKeyDown(ImGuiKey_A)) editorCamera_.translation = editorCamera_.translation - (right * moveSpeed);
					if (ImGui::IsKeyDown(ImGuiKey_E)) editorCamera_.translation.y += moveSpeed;
					if (ImGui::IsKeyDown(ImGuiKey_Q)) editorCamera_.translation.y -= moveSpeed;
				} else {
					isFlightNavigating_ = false;
				}

				// MMB Pan
				if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
					float panSpeed = 0.015f;
					Vector3 right = { std::cos(editorCamera_.rotation.y), 0.0f, -std::sin(editorCamera_.rotation.y) };
					editorCamera_.translation = editorCamera_.translation - (right * (io.MouseDelta.x * panSpeed));
					editorCamera_.translation.y += io.MouseDelta.y * panSpeed;
				}

				// Mouse Wheel Zoom
				if (isHovered && io.MouseWheel != 0.0f) {
					Vector3 forward = { std::sin(editorCamera_.rotation.y), -std::sin(editorCamera_.rotation.x), std::cos(editorCamera_.rotation.y) };
					editorCamera_.translation = editorCamera_.translation + (forward * (io.MouseWheel * 1.5f));
				}

				DrawSceneViewOverlays(imgPos, avail);
				ImGui::EndTabItem();
			}

			// TAB 2: GAME VIEW (Player Camera)
			if (ImGui::BeginTabItem("Game")) {
				ImVec2 avail = ImGui::GetContentRegionAvail();
				ImVec2 imgPos = ImGui::GetCursorScreenPos();
				ImGui::Image(ImTextureRef((ImTextureID)gameViewRT_.GetSrvGPUHandle().ptr), avail);

				if (isTetrisActive_) {
					DrawTetrisHUD(imgPos, avail);
				}
				ImGui::EndTabItem();
			}

			// TAB 3: DUAL SPLIT VIEW
			if (ImGui::BeginTabItem("Dual Split")) {
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float halfW = (avail.x - 8.0f) * 0.5f;
				ImGui::Image(ImTextureRef((ImTextureID)editViewRT_.GetSrvGPUHandle().ptr), ImVec2(halfW, avail.y));
				ImGui::SameLine();
				ImGui::Image(ImTextureRef((ImTextureID)gameViewRT_.GetSrvGPUHandle().ptr), ImVec2(halfW, avail.y));
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif
}

bool EditorSuite::WorldToScreen(const Vector3& worldPos, const Camera& cam, const ImVec2& vpPos, const ImVec2& vpSize, ImVec2& outScreenPos, float& outDepth)
{
	const Matrix4x4& vp = cam.GetViewProjectionMatrix();
	Vector4 clip = {
		worldPos.x * vp.m[0][0] + worldPos.y * vp.m[1][0] + worldPos.z * vp.m[2][0] + vp.m[3][0],
		worldPos.x * vp.m[0][1] + worldPos.y * vp.m[1][1] + worldPos.z * vp.m[2][1] + vp.m[3][1],
		worldPos.x * vp.m[0][2] + worldPos.y * vp.m[1][2] + worldPos.z * vp.m[2][2] + vp.m[3][2],
		worldPos.x * vp.m[0][3] + worldPos.y * vp.m[1][3] + worldPos.z * vp.m[2][3] + vp.m[3][3]
	};

	if (clip.w <= 0.01f) {
		return false;
	}

	float invW = 1.0f / clip.w;
	float ndcX = clip.x * invW;
	float ndcY = clip.y * invW;

	outScreenPos.x = vpPos.x + (ndcX * 0.5f + 0.5f) * vpSize.x;
	outScreenPos.y = vpPos.y + (-ndcY * 0.5f + 0.5f) * vpSize.y;
	outDepth = clip.w;
	return true;
}

void EditorSuite::DrawSceneViewOverlays(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	if (showGrid_) DrawGroundGrid(vpPos, vpSize);
	if (showWireframe_ || isEditingColliderBounds_) DrawColliderWireframes(vpPos, vpSize);

	if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
		const auto& sel = gameObjects_[selectedObjectIndex_];
		if (sel.type == GameObjectType::Camera || sel.hasCamera) {
			DrawCameraFrustum(vpPos, vpSize, sel);
		} else if (sel.type == GameObjectType::DirectionalLight || sel.type == GameObjectType::PointLight || sel.type == GameObjectType::SpotLight || sel.hasLight) {
			DrawLightGizmo(vpPos, vpSize, sel);
		}
	}

	if (showGizmos_ && selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
		DrawTransformGizmo(vpPos, vpSize);
	}

	DrawFloatingToolStrip(vpPos, vpSize);
	DrawOrientationCompass(vpPos, vpSize);
	DrawFloatingBottomBar(vpPos, vpSize);
#endif
}

void EditorSuite::DrawGroundGrid(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const int gridLines = 20;
	const float step = 1.0f;
	const float half = (gridLines * step) * 0.5f;

	ImU32 gridCol = IM_COL32(80, 80, 85, 90);
	ImU32 axisColX = IM_COL32(200, 60, 60, 160);
	ImU32 axisColZ = IM_COL32(60, 100, 220, 160);

	for (int i = -gridLines / 2; i <= gridLines / 2; ++i) {
		float x = i * step;
		Vector3 p0 = { x, 0.0f, -half };
		Vector3 p1 = { x, 0.0f,  half };

		ImVec2 s0, s1; float d0, d1;
		if (WorldToScreen(p0, editorCamera_, vpPos, vpSize, s0, d0) &&
			WorldToScreen(p1, editorCamera_, vpPos, vpSize, s1, d1)) {
			drawList->AddLine(s0, s1, (i == 0) ? axisColZ : gridCol, 1.0f);
		}
	}

	for (int i = -gridLines / 2; i <= gridLines / 2; ++i) {
		float z = i * step;
		Vector3 p0 = { -half, 0.0f, z };
		Vector3 p1 = {  half, 0.0f, z };

		ImVec2 s0, s1; float d0, d1;
		if (WorldToScreen(p0, editorCamera_, vpPos, vpSize, s0, d0) &&
			WorldToScreen(p1, editorCamera_, vpPos, vpSize, s1, d1)) {
			drawList->AddLine(s0, s1, (i == 0) ? axisColX : gridCol, 1.0f);
		}
	}
#endif
}

void EditorSuite::DrawFloatingToolStrip(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 stripPos = ImVec2(vpPos.x + 12.0f, vpPos.y + 12.0f);
	float btnW = 28.0f;
	float btnH = 24.0f;

	drawList->AddRectFilled(stripPos, ImVec2(stripPos.x + btnW * 6.0f + 20.0f, stripPos.y + btnH + 8.0f), IM_COL32(28, 28, 30, 220), 4.0f);

	const char* toolLabels[] = { "Q", "W", "E", "R", "T", "C" };
	const char* toolTips[] = {
		"Hand / Pan Tool (Q)",
		"Move / Translate Gizmo (W)",
		"Rotate Gizmo (E)",
		"Scale Gizmo (R)",
		"Rect / Bounds Transform Tool (T)",
		"Edit Box Collider Bounds (C)"
	};
	GizmoType toolTypes[] = { GizmoType::Hand, GizmoType::Move, GizmoType::Rotate, GizmoType::Scale, GizmoType::Bounds, GizmoType::Collider };

	for (int t = 0; t < 6; ++t) {
		ImVec2 bMin = ImVec2(stripPos.x + 4.0f + t * (btnW + 2.0f), stripPos.y + 4.0f);
		ImVec2 bMax = ImVec2(bMin.x + btnW, bMin.y + btnH);

		bool active = (currentGizmo_ == toolTypes[t]);
		if (t == 5 && isEditingColliderBounds_) active = true;

		ImU32 col = active ? IM_COL32(45, 115, 215, 255) : IM_COL32(45, 45, 48, 255);
		drawList->AddRectFilled(bMin, bMax, col, 2.0f);
		drawList->AddText(ImVec2(bMin.x + 9.0f, bMin.y + 4.0f), IM_COL32(240, 240, 240, 255), toolLabels[t]);

		ImGui::SetCursorScreenPos(bMin);
		if (ImGui::InvisibleButton(std::format("##ToolStripBtn{}", t).c_str(), ImVec2(btnW, btnH))) {
			if (t == 5) isEditingColliderBounds_ = !isEditingColliderBounds_;
			else currentGizmo_ = toolTypes[t];
		}
		ImGui::SetItemTooltip(toolTips[t]);
	}
#endif
}

void EditorSuite::DrawOrientationCompass(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 center = ImVec2(vpPos.x + vpSize.x - 38.0f, vpPos.y + 38.0f);
	float radius = 24.0f;

	drawList->AddCircleFilled(center, radius, IM_COL32(28, 28, 30, 200), 24);

	Vector3 right = { std::cos(editorCamera_.rotation.y), 0.0f, -std::sin(editorCamera_.rotation.y) };
	Vector3 forward = { std::sin(editorCamera_.rotation.y), 0.0f, std::cos(editorCamera_.rotation.y) };

	ImVec2 xTip = ImVec2(center.x + right.x * 18.0f, center.y - right.z * 18.0f);
	ImVec2 zTip = ImVec2(center.x + forward.x * 18.0f, center.y - forward.z * 18.0f);

	drawList->AddLine(center, xTip, IM_COL32(220, 60, 60, 255), 2.0f);
	drawList->AddText(ImVec2(xTip.x - 3.0f, xTip.y - 7.0f), IM_COL32(255, 100, 100, 255), "X");

	drawList->AddLine(center, zTip, IM_COL32(60, 120, 240, 255), 2.0f);
	drawList->AddText(ImVec2(zTip.x - 3.0f, zTip.y - 7.0f), IM_COL32(100, 160, 255, 255), "Z");
#endif
}

void EditorSuite::DrawFloatingBottomBar(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 barPos = ImVec2(vpPos.x + 12.0f, vpPos.y + vpSize.y - 32.0f);
	drawList->AddRectFilled(barPos, ImVec2(barPos.x + 415.0f, barPos.y + 24.0f), IM_COL32(28, 28, 30, 220), 4.0f);

	ImGui::SetCursorScreenPos(ImVec2(barPos.x + 8.0f, barPos.y + 2.0f));
	ImGui::Checkbox("Grid", &showGrid_);
	ImGui::SetItemTooltip("Toggle 3D ground plane grid");
	ImGui::SameLine();
	ImGui::Checkbox("Gizmos", &showGizmos_);
	ImGui::SetItemTooltip("Toggle 3D transform gizmos");
	ImGui::SameLine();
	ImGui::Checkbox("Wire", &showWireframe_);
	ImGui::SetItemTooltip("Toggle collider wireframe rendering");
	ImGui::SameLine();
	ImGui::Checkbox("Light", &enableLighting_);
	ImGui::SetItemTooltip("Toggle viewport scene lighting");
	ImGui::SameLine();
	ImGui::Checkbox("Cull", &enableFrustumCulling_);
	ImGui::SetItemTooltip("Toggle view frustum AABB culling");
	ImGui::SameLine();
	ImGui::TextDisabled("| Vis:%d Cul:%d", lastVisibleObjects_, lastCulledObjects_);
#endif
}

void EditorSuite::DrawTransformGizmo(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(gameObjects_.size())) return;
	auto& sel = gameObjects_[selectedObjectIndex_];

	ImVec2 originScreen; float originDepth;
	if (!WorldToScreen(sel.translation, editorCamera_, vpPos, vpSize, originScreen, originDepth)) return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float axisLength = 55.0f;

	Vector3 xTarget = sel.translation + Vector3{ 1.0f, 0.0f, 0.0f };
	Vector3 yTarget = sel.translation + Vector3{ 0.0f, 1.0f, 0.0f };
	Vector3 zTarget = sel.translation + Vector3{ 0.0f, 0.0f, 1.0f };

	ImVec2 xScreen, yScreen, zScreen; float xd, yd, zd;
	if (!WorldToScreen(xTarget, editorCamera_, vpPos, vpSize, xScreen, xd)) return;
	if (!WorldToScreen(yTarget, editorCamera_, vpPos, vpSize, yScreen, yd)) return;
	if (!WorldToScreen(zTarget, editorCamera_, vpPos, vpSize, zScreen, zd)) return;

	ImVec2 dirX = ImVec2(xScreen.x - originScreen.x, xScreen.y - originScreen.y);
	ImVec2 dirY = ImVec2(yScreen.x - originScreen.x, yScreen.y - originScreen.y);
	ImVec2 dirZ = ImVec2(zScreen.x - originScreen.x, zScreen.y - originScreen.y);

	float lenX = std::sqrt(dirX.x * dirX.x + dirX.y * dirX.y);
	float lenY = std::sqrt(dirY.x * dirY.x + dirY.y * dirY.y);
	float lenZ = std::sqrt(dirZ.x * dirZ.x + dirZ.y * dirZ.y);

	if (lenX > 0.01f) { dirX.x = dirX.x / lenX * axisLength; dirX.y = dirX.y / lenX * axisLength; }
	if (lenY > 0.01f) { dirY.x = dirY.x / lenY * axisLength; dirY.y = dirY.y / lenY * axisLength; }
	if (lenZ > 0.01f) { dirZ.x = dirZ.x / lenZ * axisLength; dirZ.y = dirZ.y / lenZ * axisLength; }

	ImVec2 tipX = ImVec2(originScreen.x + dirX.x, originScreen.y + dirX.y);
	ImVec2 tipY = ImVec2(originScreen.x + dirY.x, originScreen.y + dirY.y);
	ImVec2 tipZ = ImVec2(originScreen.x + dirZ.x, originScreen.y + dirZ.y);

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mousePos = io.MousePos;

	auto DistToSegment = [](const ImVec2& p, const ImVec2& a, const ImVec2& b) -> float {
		float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
		if (l2 < 0.001f) return std::sqrt((p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y));
		float t = std::clamp(((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2, 0.0f, 1.0f);
		ImVec2 proj = ImVec2(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
		return std::sqrt((p.x - proj.x) * (p.x - proj.x) + (p.y - proj.y) * (p.y - proj.y));
	};

	if (!isDraggingGizmo_) {
		activeGizmoAxis_ = GizmoAxis::None;
		if (DistToSegment(mousePos, originScreen, tipX) < 8.0f) activeGizmoAxis_ = GizmoAxis::X;
		else if (DistToSegment(mousePos, originScreen, tipY) < 8.0f) activeGizmoAxis_ = GizmoAxis::Y;
		else if (DistToSegment(mousePos, originScreen, tipZ) < 8.0f) activeGizmoAxis_ = GizmoAxis::Z;

		if (activeGizmoAxis_ != GizmoAxis::None && io.MouseClicked[0]) {
			isDraggingGizmo_ = true;
			gizmoDragStartMouse_ = mousePos;
			gizmoStartTranslation_ = sel.translation;
			gizmoStartRotation_ = sel.rotation;
			gizmoStartScale_ = sel.scale;
			gizmoStartLocalOffset_ = sel.localOffset;
		}
	} else {
		if (io.MouseDown[0]) {
			float dx = (mousePos.x - gizmoDragStartMouse_.x) * 0.03f;
			float dy = -(mousePos.y - gizmoDragStartMouse_.y) * 0.03f;

			if (currentGizmo_ == GizmoType::Move) {
				if (sel.hasParticleSystem && sel.attachedParentId != -1) {
					int pIdx = -1;
					for (size_t p = 0; p < gameObjects_.size(); ++p) {
						if (gameObjects_[p].id == static_cast<uint32_t>(sel.attachedParentId)) {
							pIdx = static_cast<int>(p);
							break;
						}
					}
					if (pIdx >= 0) {
						if (activeGizmoAxis_ == GizmoAxis::X) sel.localOffset.x = gizmoStartLocalOffset_.x + dx;
						else if (activeGizmoAxis_ == GizmoAxis::Y) sel.localOffset.y = gizmoStartLocalOffset_.y + dy;
						else if (activeGizmoAxis_ == GizmoAxis::Z) sel.localOffset.z = gizmoStartLocalOffset_.z + dx;
						sel.translation = gameObjects_[pIdx].translation + sel.localOffset;
					} else {
						sel.attachedParentId = -1;
						sel.localOffset = { 0.0f, 0.0f, 0.0f };
						if (activeGizmoAxis_ == GizmoAxis::X) sel.translation.x = gizmoStartTranslation_.x + dx;
						else if (activeGizmoAxis_ == GizmoAxis::Y) sel.translation.y = gizmoStartTranslation_.y + dy;
						else if (activeGizmoAxis_ == GizmoAxis::Z) sel.translation.z = gizmoStartTranslation_.z + dx;
					}
				} else {
					if (activeGizmoAxis_ == GizmoAxis::X) sel.translation.x = gizmoStartTranslation_.x + dx;
					else if (activeGizmoAxis_ == GizmoAxis::Y) sel.translation.y = gizmoStartTranslation_.y + dy;
					else if (activeGizmoAxis_ == GizmoAxis::Z) sel.translation.z = gizmoStartTranslation_.z + dx;
				}
			} else if (currentGizmo_ == GizmoType::Rotate) {
				if (activeGizmoAxis_ == GizmoAxis::X) sel.rotation.x = gizmoStartRotation_.x + dy;
				else if (activeGizmoAxis_ == GizmoAxis::Y) sel.rotation.y = gizmoStartRotation_.y + dx;
				else if (activeGizmoAxis_ == GizmoAxis::Z) sel.rotation.z = gizmoStartRotation_.z + dx;
				sel.rotationQuat = Quaternion::FromEuler(sel.rotation);
			} else if (currentGizmo_ == GizmoType::Scale || currentGizmo_ == GizmoType::Bounds) {
				float delta = dx + dy;
				if (activeGizmoAxis_ == GizmoAxis::X) sel.scale.x = (std::max)(0.01f, gizmoStartScale_.x + delta);
				else if (activeGizmoAxis_ == GizmoAxis::Y) sel.scale.y = (std::max)(0.01f, gizmoStartScale_.y + delta);
				else if (activeGizmoAxis_ == GizmoAxis::Z) sel.scale.z = (std::max)(0.01f, gizmoStartScale_.z + delta);
			}
			isSceneDirty_ = true;
		} else {
			// Gizmo drag ended: push snapshot and mark scene dirty
			isDraggingGizmo_ = false;
			activeGizmoAxis_ = GizmoAxis::None;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
	}

	ImU32 colX = (activeGizmoAxis_ == GizmoAxis::X) ? IM_COL32(255, 230, 50, 255) : IM_COL32(230, 50, 50, 255);
	drawList->AddLine(originScreen, tipX, colX, (activeGizmoAxis_ == GizmoAxis::X) ? 3.0f : 2.0f);
	drawList->AddCircleFilled(tipX, 4.0f, colX);

	ImU32 colY = (activeGizmoAxis_ == GizmoAxis::Y) ? IM_COL32(255, 230, 50, 255) : IM_COL32(50, 220, 60, 255);
	drawList->AddLine(originScreen, tipY, colY, (activeGizmoAxis_ == GizmoAxis::Y) ? 3.0f : 2.0f);
	drawList->AddCircleFilled(tipY, 4.0f, colY);

	ImU32 colZ = (activeGizmoAxis_ == GizmoAxis::Z) ? IM_COL32(255, 230, 50, 255) : IM_COL32(50, 110, 240, 255);
	drawList->AddLine(originScreen, tipZ, colZ, (activeGizmoAxis_ == GizmoAxis::Z) ? 3.0f : 2.0f);
	drawList->AddCircleFilled(tipZ, 4.0f, colZ);
#endif
}

void EditorSuite::DrawColliderWireframes(const ImVec2& vpPos, const ImVec2& vpSize)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (const auto& obj : gameObjects_) {
		if (!obj.isActive || !obj.hasBoxCollider || !obj.boxColliderEnabled) continue;

		Vector3 halfSize = { obj.boxSize.x * obj.scale.x * 0.5f, obj.boxSize.y * obj.scale.y * 0.5f, obj.boxSize.z * obj.scale.z * 0.5f };
		Vector3 minP = obj.translation + obj.boxCenter - halfSize;
		Vector3 maxP = obj.translation + obj.boxCenter + halfSize;

		Vector3 corners[8] = {
			{ minP.x, minP.y, minP.z }, { maxP.x, minP.y, minP.z },
			{ maxP.x, maxP.y, minP.z }, { minP.x, maxP.y, minP.z },
			{ minP.x, minP.y, maxP.z }, { maxP.x, minP.y, maxP.z },
			{ maxP.x, maxP.y, maxP.z }, { minP.x, maxP.y, maxP.z }
		};

		ImVec2 sc[8]; float sd[8]; bool allOk = true;
		for (int i = 0; i < 8; ++i) {
			if (!WorldToScreen(corners[i], editorCamera_, vpPos, vpSize, sc[i], sd[i])) { allOk = false; break; }
		}

		if (allOk) {
			ImU32 col = obj.isColliding ? IM_COL32(255, 50, 50, 220) : IM_COL32(50, 255, 100, 180);
			int edges[12][2] = {
				{0,1},{1,2},{2,3},{3,0},
				{4,5},{5,6},{6,7},{7,4},
				{0,4},{1,5},{2,6},{3,7}
			};
			for (int e = 0; e < 12; ++e) {
				drawList->AddLine(sc[edges[e][0]], sc[edges[e][1]], col, 1.2f);
			}
		}
	}
#endif
}

void EditorSuite::DrawCameraFrustum(const ImVec2& vpPos, const ImVec2& vpSize, const GameObject& camObj)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 camPos; float camDepth;
	if (!WorldToScreen(camObj.translation, editorCamera_, vpPos, vpSize, camPos, camDepth)) return;

	Vector3 forward = { std::sin(camObj.rotation.y), -std::sin(camObj.rotation.x), std::cos(camObj.rotation.y) };
	Vector3 right = { std::cos(camObj.rotation.y), 0.0f, -std::sin(camObj.rotation.y) };
	Vector3 up = { 0.0f, 1.0f, 0.0f };

	float dist = 4.0f;
	float halfH = dist * std::tan(camObj.fov * 0.5f * 3.14159265f / 180.0f);
	float halfW = halfH * (16.0f / 9.0f);

	Vector3 center = camObj.translation + (forward * dist);
	Vector3 fCorners[4] = {
		center + (right * -halfW) + (up *  halfH),
		center + (right *  halfW) + (up *  halfH),
		center + (right *  halfW) + (up * -halfH),
		center + (right * -halfW) + (up * -halfH)
	};

	ImVec2 sc[4]; float sd[4];
	for (int i = 0; i < 4; ++i) {
		if (WorldToScreen(fCorners[i], editorCamera_, vpPos, vpSize, sc[i], sd[i])) {
			drawList->AddLine(camPos, sc[i], IM_COL32(240, 240, 240, 160), 1.0f);
		}
	}
	for (int i = 0; i < 4; ++i) {
		drawList->AddLine(sc[i], sc[(i + 1) % 4], IM_COL32(255, 255, 255, 200), 1.2f);
	}
#endif
}

void EditorSuite::DrawLightGizmo(const ImVec2& vpPos, const ImVec2& vpSize, const GameObject& lightObj)
{
#ifdef USE_IMGUI
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 lPos; float lDepth;
	if (!WorldToScreen(lightObj.translation, editorCamera_, vpPos, vpSize, lPos, lDepth)) return;

	ImU32 lightCol = IM_COL32(255, 220, 80, 240);
	drawList->AddCircleFilled(lPos, 6.0f, lightCol);
	drawList->AddCircle(lPos, 7.5f, IM_COL32(40, 40, 40, 200), 16, 1.5f);

	if (lightObj.lightType == GameObject::LightType::Directional) {
		// 1. 2D radiating sun rays around center icon
		for (int k = 0; k < 8; ++k) {
			float angle = k * (3.14159265f / 4.0f);
			ImVec2 rStart = ImVec2(lPos.x + std::cos(angle) * 10.0f, lPos.y + std::sin(angle) * 10.0f);
			ImVec2 rEnd = ImVec2(lPos.x + std::cos(angle) * 17.0f, lPos.y + std::sin(angle) * 17.0f);
			drawList->AddLine(rStart, rEnd, IM_COL32(255, 230, 90, 220), 1.5f);
		}

		// 2. 3D directional sun rays (central ray + 4 surrounding parallel rays)
		Vector3 dir = lightObj.lightDirection;
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len > 0.001f) { dir.x /= len; dir.y /= len; dir.z /= len; }
		else { dir = { 0.0f, -1.0f, 0.0f }; }

		Vector3 up = (std::abs(dir.y) < 0.99f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
		Vector3 u = { dir.y * up.z - dir.z * up.y, dir.z * up.x - dir.x * up.z, dir.x * up.y - dir.y * up.x };
		float ulen = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
		if (ulen > 0.0001f) { u.x /= ulen; u.y /= ulen; u.z /= ulen; }
		Vector3 v = { dir.y * u.z - dir.z * u.y, dir.z * u.x - dir.x * u.z, dir.x * u.y - dir.y * u.x };
		float vlen = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (vlen > 0.0001f) { v.x /= vlen; v.y /= vlen; v.z /= vlen; }

		Vector3 offsets[5] = {
			{ 0.0f, 0.0f, 0.0f },
			u * 1.0f,
			u * -1.0f,
			v * 1.0f,
			v * -1.0f
		};

		for (int i = 0; i < 5; ++i) {
			Vector3 rStart = lightObj.translation + offsets[i];
			Vector3 rEnd = rStart + (dir * 3.5f);
			ImVec2 s0, s1; float d0, d1;
			if (WorldToScreen(rStart, editorCamera_, vpPos, vpSize, s0, d0) &&
				WorldToScreen(rEnd, editorCamera_, vpPos, vpSize, s1, d1)) {
				drawList->AddLine(s0, s1, (i == 0) ? IM_COL32(255, 240, 100, 255) : IM_COL32(255, 220, 80, 180), (i == 0) ? 2.0f : 1.2f);
			}
		}
	} else if (lightObj.lightType == GameObject::LightType::Point) {
		// Wireframe radius sphere (XZ, XY, YZ orthogonal circles)
		const int segs = 32;
		float r = (std::max)(0.1f, lightObj.lightRange);
		Vector3 center = lightObj.translation;
		ImU32 sphereCol = IM_COL32(255, 220, 80, 180);

		// 1. Equator circle (XZ plane)
		ImVec2 prevP;
		for (int i = 0; i <= segs; ++i) {
			float theta = i * 2.0f * 3.14159265f / segs;
			Vector3 pt = center + Vector3{ std::cos(theta) * r, 0.0f, std::sin(theta) * r };
			ImVec2 sp; float sdep;
			if (WorldToScreen(pt, editorCamera_, vpPos, vpSize, sp, sdep)) {
				if (i > 0) drawList->AddLine(prevP, sp, sphereCol, 1.2f);
				prevP = sp;
			}
		}

		// 2. Vertical front circle (XY plane)
		for (int i = 0; i <= segs; ++i) {
			float theta = i * 2.0f * 3.14159265f / segs;
			Vector3 pt = center + Vector3{ std::cos(theta) * r, std::sin(theta) * r, 0.0f };
			ImVec2 sp; float sdep;
			if (WorldToScreen(pt, editorCamera_, vpPos, vpSize, sp, sdep)) {
				if (i > 0) drawList->AddLine(prevP, sp, sphereCol, 1.2f);
				prevP = sp;
			}
		}

		// 3. Vertical side circle (YZ plane)
		for (int i = 0; i <= segs; ++i) {
			float theta = i * 2.0f * 3.14159265f / segs;
			Vector3 pt = center + Vector3{ 0.0f, std::sin(theta) * r, std::cos(theta) * r };
			ImVec2 sp; float sdep;
			if (WorldToScreen(pt, editorCamera_, vpPos, vpSize, sp, sdep)) {
				if (i > 0) drawList->AddLine(prevP, sp, sphereCol, 1.2f);
				prevP = sp;
			}
		}
	} else if (lightObj.lightType == GameObject::LightType::Spot) {
		// Wireframe cone matching Distance and Spot Angle
		float dist = (std::max)(0.1f, lightObj.lightRange);
		float angleRad = lightObj.lightSpotAngle * 3.14159265f / 180.0f;
		float baseRadius = dist * std::tan(angleRad);

		Vector3 dir = lightObj.lightDirection;
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len > 0.001f) { dir.x /= len; dir.y /= len; dir.z /= len; }
		else { dir = { 0.0f, -1.0f, 0.0f }; }

		Vector3 up = (std::abs(dir.y) < 0.99f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
		Vector3 u = { dir.y * up.z - dir.z * up.y, dir.z * up.x - dir.x * up.z, dir.x * up.y - dir.y * up.x };
		float ulen = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
		if (ulen > 0.0001f) { u.x /= ulen; u.y /= ulen; u.z /= ulen; }
		Vector3 v = { dir.y * u.z - dir.z * u.y, dir.z * u.x - dir.x * u.z, dir.x * u.y - dir.y * u.x };
		float vlen = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (vlen > 0.0001f) { v.x /= vlen; v.y /= vlen; v.z /= vlen; }

		Vector3 apex = lightObj.translation;
		Vector3 baseCenter = apex + (dir * dist);
		const int segs = 32;
		ImU32 coneCol = IM_COL32(255, 220, 80, 200);

		// Outer base circle
		ImVec2 prevP;
		for (int i = 0; i <= segs; ++i) {
			float theta = i * 2.0f * 3.14159265f / segs;
			Vector3 pt = baseCenter + (u * (std::cos(theta) * baseRadius)) + (v * (std::sin(theta) * baseRadius));
			ImVec2 sp; float sdep;
			if (WorldToScreen(pt, editorCamera_, vpPos, vpSize, sp, sdep)) {
				if (i > 0) drawList->AddLine(prevP, sp, coneCol, 1.2f);
				prevP = sp;
			}
		}

		// Inner falloff circle (if falloff start angle < spot angle)
		if (lightObj.lightSpotFalloffStart > 0.0f && lightObj.lightSpotFalloffStart < lightObj.lightSpotAngle) {
			float innerRadius = dist * std::tan(lightObj.lightSpotFalloffStart * 3.14159265f / 180.0f);
			ImU32 innerCol = IM_COL32(255, 240, 140, 130);
			for (int i = 0; i <= segs; ++i) {
				float theta = i * 2.0f * 3.14159265f / segs;
				Vector3 pt = baseCenter + (u * (std::cos(theta) * innerRadius)) + (v * (std::sin(theta) * innerRadius));
				ImVec2 sp; float sdep;
				if (WorldToScreen(pt, editorCamera_, vpPos, vpSize, sp, sdep)) {
					if (i > 0) drawList->AddLine(prevP, sp, innerCol, 1.0f);
					prevP = sp;
				}
			}
		}

		// 4 Side generator lines connecting apex to base cardinal points
		Vector3 cardinalPts[4] = {
			baseCenter + (u * baseRadius),
			baseCenter - (u * baseRadius),
			baseCenter + (v * baseRadius),
			baseCenter - (v * baseRadius)
		};
		for (int i = 0; i < 4; ++i) {
			ImVec2 s0, s1; float d0, d1;
			if (WorldToScreen(apex, editorCamera_, vpPos, vpSize, s0, d0) &&
				WorldToScreen(cardinalPts[i], editorCamera_, vpPos, vpSize, s1, d1)) {
				drawList->AddLine(s0, s1, coneCol, 1.2f);
			}
		}

		// Central axis line
		ImVec2 a0, a1; float da0, da1;
		if (WorldToScreen(apex, editorCamera_, vpPos, vpSize, a0, da0) &&
			WorldToScreen(baseCenter, editorCamera_, vpPos, vpSize, a1, da1)) {
			drawList->AddLine(a0, a1, IM_COL32(255, 240, 100, 140), 1.0f);
		}
	}
#endif
}

void EditorSuite::DrawInspectorAndDetailsPanel(bool& showParticles)
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float screenH = io.DisplaySize.y;
	float topY = ImGui::GetFrameHeight() + 38.0f;
	float rightW = screenW * 0.24f;
	if (rightW < 300.0f) rightW = 300.0f;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(screenW - rightW, topY));
	ImGui::SetNextWindowSize(ImVec2(rightW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Inspector Workspace", nullptr, flags)) {
		if (ImGui::BeginTabBar("InspectorTabs")) {
			// TAB 1: INSPECTOR
			if (ImGui::BeginTabItem("Inspector")) {
				rightPanelTab_ = 0;
				if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(gameObjects_.size())) {
					ImGui::TextDisabled("No GameObject Selected");
					ImGui::Separator();
					DrawSkyboxSettingsUI();
					ImGui::EndTabItem();
					ImGui::EndTabBar();
					ImGui::End();
					return;
				}

				auto& obj = gameObjects_[selectedObjectIndex_];

				// Header: Active Checkbox + Object Name + Static
				if (ImGui::Checkbox("##Active", &obj.isActive)) {
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}
				ImGui::SameLine();
				char nameBuf[128]{};
				strncpy_s(nameBuf, obj.name.c_str(), sizeof(nameBuf) - 1);
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
				if (ImGui::InputText("##ObjName", nameBuf, sizeof(nameBuf))) {
					obj.name = nameBuf;
					isSceneDirty_ = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					PushUndoSnapshot();
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("Static", &obj.isStatic)) {
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}

				// 1. TRANSFORM COMPONENT (Always Present)
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					DrawVec3Control("Position", obj.translation, 0.0f);
					ImGui::SetItemTooltip("World position (X, Y, Z)");

					if (DrawVec3Control("Rotation", obj.rotation, 0.0f)) {
						obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
					}
					ImGui::SetItemTooltip("Euler rotation in radians (X, Y, Z)");

					DrawVec3Control("Scale", obj.scale, 1.0f);
					ImGui::SetItemTooltip("Local scale vector (X, Y, Z)");

					ImGui::Separator();
					float qVal[4] = { obj.rotationQuat.x, obj.rotationQuat.y, obj.rotationQuat.z, obj.rotationQuat.w };
					ImGui::InputFloat4("Quaternion (Live)", qVal, "%.4f", ImGuiInputTextFlags_ReadOnly);
					ImGui::SetItemTooltip("Direct 4D Unit Quaternion rotation readout (X, Y, Z, W)");

					if (ImGui::Button("Test 90-deg Roll Slerp", ImVec2(-1.0f, 22.0f))) {
						Quaternion target = Quaternion::FromAxisAngle({ 0, 0, 1 }, 3.14159265f / 2.0f);
						obj.rotationQuat = Quaternion::Slerp(obj.rotationQuat, target, 0.5f);
						obj.rotation = obj.rotationQuat.ToEuler();
						isSceneDirty_ = true;
						PushUndoSnapshot();
					}
					ImGui::SetItemTooltip("Interpolate orientation using spherical linear interpolation (Slerp)");
				}

				// 2. MESH FILTER & MESH RENDERER
				if (obj.hasMesh) {
					if (ImGui::CollapsingHeader("Mesh Filter & Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##Mesh", &obj.meshEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						const char* meshTypes[] = { "Cube", "Sphere", "Plane", "Quad", "CustomModel" };
						int currentMesh = static_cast<int>(obj.meshType);
						if (ImGui::Combo("Mesh Prototype", &currentMesh, meshTypes, 5)) {
							obj.meshType = static_cast<GameObject::MeshType>(currentMesh);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						if (obj.meshType == GameObject::MeshType::CustomModel) {
							char modelBuf[256] = {};
							strncpy_s(modelBuf, obj.modelPath.c_str(), sizeof(modelBuf) - 1);
							if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf))) {
								obj.modelPath = modelBuf;
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						}

						if (ImGui::ColorEdit4("Material Color", &obj.materialColor.x)) { isSceneDirty_ = true; }
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

						if (ImGui::Checkbox("Lit Material", &obj.isLit)) { isSceneDirty_ = true; PushUndoSnapshot(); }

						const char* reflectionModels[] = { "0: Unlit", "1: Lambert", "2: Half-Lambert", "3: Phong", "4: Blinn-Phong" };
						if (ImGui::Combo("Reflection Model", &obj.lightingType, reflectionModels, 5)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Select illumination shading equation (02_00 Phong, 02_01 Blinn-Phong, Half-Lambert, or Unlit)");

						if (obj.lightingType >= 3) {
							if (ImGui::SliderFloat("Shininess", &obj.shininess, 1.0f, 256.0f, "%.1f")) { isSceneDirty_ = true; }
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Specular exponent power factor");

							if (ImGui::ColorEdit3("Specular Color", &obj.specularColor.x)) { isSceneDirty_ = true; }
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						}

						if (ImGui::Checkbox("Cast Shadows##Mesh", &obj.castShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Render this mesh into the directional light shadow map");
						ImGui::SameLine();
						if (ImGui::Checkbox("Receive Shadows##Mesh", &obj.receiveShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Receive shadows cast by other geometry (Unlit objects never receive shadows)");

						const char* gsEffects[] = { "None", "Standard Pass-Through", "Explode Faces Along Normals" };
						if (ImGui::Combo("Geometry Shader Effect", &obj.geometryShaderEffect, gsEffects, 3)) {
							obj.useGeometryShader = (obj.geometryShaderEffect != 0);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("DirectX 12 SM 6.0 Geometry Shader pipeline variants");

						if (obj.geometryShaderEffect == 2) {
							if (ImGui::SliderFloat("Explode Amount", &obj.explodeAmount, 0.0f, 2.0f, "%.2f")) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Displacement distance along polygon face normals in the geometry shader");
						}

						ImGui::Separator();
						ImGui::Text("Textured Surface / UV Controls:");

						char texBuf[256];
						strncpy_s(texBuf, obj.texturePath.c_str(), sizeof(texBuf));
						if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf))) {
							obj.texturePath = texBuf;
							obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Path to texture file (e.g. Resources/circle.png)");

						const char* texPresets[] = { "(Choose Preset...)", "Resources/circle.png", "Resources/Model/white.png", "Resources/uvChecker.png" };
						int selectedPreset = 0;
						if (ImGui::Combo("Preset Selector", &selectedPreset, texPresets, 4)) {
							if (selectedPreset == 1) {
								obj.texturePath = "Resources/circle.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							} else if (selectedPreset == 2) {
								obj.texturePath = "Resources/Model/white.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							} else if (selectedPreset == 3) {
								obj.texturePath = "Resources/uvChecker.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(obj.texturePath);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}

						if (ImGui::ColorEdit4("Tint Color", &obj.materialColor.x)) { isSceneDirty_ = true; }
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Color multiplier tint applied to the texture (RGBA)");

						if (ImGui::DragFloat2("UV Offset XY", &obj.uvOffset.x, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV texture coordinate offset (U, V)");

						if (ImGui::DragFloat2("UV Tiling XY", &obj.uvTiling.x, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV texture coordinate scaling / repeat (U, V)");

						if (ImGui::DragFloat("UV Rotation", &obj.uvRotation, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV coordinate rotation angle in radians");
					}

					// ANIMATOR / SKELETAL ANIMATION SECTION
					Model* activeModel = obj.editObject3d ? obj.editObject3d->GetModel() : (obj.gameObject3d ? obj.gameObject3d->GetModel() : nullptr);
					if (activeModel && activeModel->GetAnimationCount() > 0) {
						if (ImGui::CollapsingHeader("Animator (FBX / glTF Animation)", ImGuiTreeNodeFlags_DefaultOpen)) {
							uint32_t animCount = static_cast<uint32_t>(activeModel->GetAnimationCount());
							std::vector<std::string> clipNames;
							std::vector<const char*> clipNamePtrs;
							for (uint32_t a = 0; a < animCount; ++a) {
								clipNames.push_back(std::format("[{}] {}", a, activeModel->GetAnimationName(static_cast<int>(a))));
							}
							for (const auto& cn : clipNames) {
								clipNamePtrs.push_back(cn.c_str());
							}

							int curClip = std::clamp(obj.animationIndex, 0, static_cast<int>(animCount) - 1);
							if (ImGui::Combo("Animation Clip", &curClip, clipNamePtrs.data(), static_cast<int>(clipNamePtrs.size()))) {
								obj.animationIndex = curClip;
								if (obj.editObject3d) {
									obj.editObject3d->SetCurrentAnimationIndex(static_cast<uint32_t>(curClip));
									obj.editObject3d->ResetAnimationTime();
								}
								if (obj.gameObject3d) {
									obj.gameObject3d->SetCurrentAnimationIndex(static_cast<uint32_t>(curClip));
									obj.gameObject3d->ResetAnimationTime();
								}
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}

							if (ImGui::Checkbox("Play Animation", &obj.animationPlaying)) {
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
							ImGui::SameLine();
							if (ImGui::Checkbox("Loop##Anim", &obj.animationLoop)) {
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
							ImGui::SameLine();
							if (ImGui::Button("Restart Clip")) {
								if (obj.editObject3d) obj.editObject3d->ResetAnimationTime();
								if (obj.gameObject3d) obj.gameObject3d->ResetAnimationTime();
							}

							if (ImGui::SliderFloat("Playback Speed", &obj.animationSpeed, 0.0f, 3.0f, "%.2fx")) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

							if (obj.editObject3d) {
								ImGui::TextDisabled("Current Time: %.2f s  |  Clips: %u", obj.editObject3d->GetAnimationTime(), animCount);
							}
						}
					}
				}

				// 3. BOX COLLIDER COMPONENT
				if (obj.hasBoxCollider) {
					if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##BoxCol", &obj.boxColliderEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SameLine();
						if (ImGui::Button(isEditingColliderBounds_ ? " [ Editing Bounds ] " : " Edit Bounds ")) {
							isEditingColliderBounds_ = !isEditingColliderBounds_;
						}
						ImGui::SetItemTooltip("Enable interactive handle editing of box collider extents");

						if (ImGui::Checkbox("Is Trigger", &obj.isTrigger)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Trigger volume: detects overlaps without generating physical collision response");

						ImGui::SameLine();
						if (ImGui::Checkbox("Provides Contacts", &obj.providesContacts)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Generate contact points and impulse resolution");

						DrawVec3Control("Center", obj.boxCenter, 0.0f);
						ImGui::SetItemTooltip("Offset of box collider center relative to transform position");

						DrawVec3Control("Size", obj.boxSize, 1.0f);
						ImGui::SetItemTooltip("Dimensions of box collider along X, Y, Z axes");
					}
				}

				// 4. SPHERE COLLIDER COMPONENT
				if (obj.hasSphereCollider) {
					if (ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##SphCol", &obj.sphereColliderEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						if (ImGui::Checkbox("Is Trigger##Sph", &obj.sphereIsTrigger)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						DrawVec3Control("Center##Sph", obj.sphereCenter, 0.0f);
						ImGui::SetItemTooltip("Center offset of sphere collider");

						if (ImGui::DragFloat("Radius", &obj.sphereRadius, 0.05f, 0.01f, 100.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Radius of sphere collider");
					}
				}

				// 5. RIGIDBODY COMPONENT
				if (obj.hasRigidbody) {
					if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##RB", &obj.rbEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }

						if (ImGui::DragFloat("Mass (kg)", &obj.mass, 0.1f, 0.01f, 1000.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Mass of the object in kilograms");

						if (ImGui::DragFloat("Linear Drag", &obj.drag, 0.01f, 0.0f, 10.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Linear air resistance damping");

						if (ImGui::DragFloat("Gravity Scale", &obj.gravityScale, 0.05f, -10.0f, 10.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Multiplier for gravitational acceleration");

						if (ImGui::Checkbox("Use Gravity", &obj.useGravity)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Enable constant downward gravitational acceleration");

						ImGui::SameLine();
						if (ImGui::Checkbox("Is Kinematic", &obj.isKinematic)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Ignore physical forces and move only via script or transform");

						if (ImGui::SliderFloat("Bounciness", &obj.bounciness, 0.0f, 1.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Coefficient of restitution for collisions");
					}
				}

				// 6. CAMERA COMPONENT
				if (obj.hasCamera) {
					if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##Cam", &obj.cameraEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						if (ImGui::SliderFloat("Field of View", &obj.fov, 10.0f, 150.0f, "%.1f deg")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						if (ImGui::DragFloat("Near Clip", &obj.nearClip, 0.01f, 0.001f, 10.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						if (ImGui::DragFloat("Far Clip", &obj.farClip, 5.0f, 10.0f, 5000.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
					}
				}

				// 7. LIGHT COMPONENT (Directional + Point Light 03_00 + Spot Light 03_01)
				if (obj.hasLight) {
					if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Checkbox("Enabled##Light", &obj.lightEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						const char* lightTypes[] = { "Directional", "Point", "Spot" };
						int currentLight = static_cast<int>(obj.lightType);
						if (ImGui::Combo("Light Type", &currentLight, lightTypes, 3)) {
							obj.lightType = static_cast<GameObject::LightType>(currentLight);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Light emitter type: Directional, Point Light (03_00), or Spot Light (03_01)");

						if (ImGui::ColorEdit3("Color", &obj.lightColor.x)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("RGB light emission color");

						if (ImGui::DragFloat("Intensity", &obj.lightIntensity, 0.05f, 0.0f, 100.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Light brightness multiplier");

						if (obj.lightType == GameObject::LightType::Directional) {
							if (DrawVec3Control("Direction", obj.lightDirection, 0.0f)) {
								Vector3 d = obj.lightDirection;
								float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
								if (dLen > 0.0001f) {
									d = { d.x / dLen, d.y / dLen, d.z / dLen };
									obj.rotation.x = std::asin(std::clamp(-d.y, -1.0f, 1.0f));
									obj.rotation.y = std::atan2(d.x, d.z);
									obj.rotation.z = 0.0f;
									obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
								}
							}
							ImGui::SetItemTooltip("Direction vector of light emission (linked with Transform Rotation)");

							if (ImGui::Checkbox("Cast Shadows##Light", &obj.lightCastShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
							ImGui::SetItemTooltip("Enable directional light shadow map rendering and attenuation");

							if (ImGui::SliderFloat("Shadow Bias", &obj.lightShadowBias, 0.0001f, 0.02f, "%.4f")) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Depth bias offset to prevent shadow acne (self-shadow artifacts)");

							if (ImGui::SliderFloat("Shadow Strength", &obj.lightShadowStrength, 0.0f, 1.0f, "%.2f")) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Intensity of the shadows cast by this light (1.0 = pitch black shadows, 0.0 = no shadow attenuation)");
						} else {
							if (ImGui::DragFloat(obj.lightType == GameObject::LightType::Point ? "Radius (Max Dist)" : "Distance", &obj.lightRange, 0.2f, 0.1f, 200.0f)) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Maximum illumination radius / distance");

							if (ImGui::DragFloat("Decay", &obj.lightDecay, 0.1f, 0.0f, 10.0f)) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Attenuation decay exponent");

							if (obj.lightType == GameObject::LightType::Spot) {
								if (DrawVec3Control("Direction", obj.lightDirection, 0.0f)) {
									Vector3 d = obj.lightDirection;
									float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
									if (dLen > 0.0001f) {
										d = { d.x / dLen, d.y / dLen, d.z / dLen };
										obj.rotation.x = std::asin(std::clamp(-d.y, -1.0f, 1.0f));
										obj.rotation.y = std::atan2(d.x, d.z);
										obj.rotation.z = 0.0f;
										obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
									}
								}
								ImGui::SetItemTooltip("Direction vector of spot cone (linked with Transform Rotation)");
								if (ImGui::SliderFloat("Spot Angle", &obj.lightSpotAngle, 1.0f, 90.0f)) isSceneDirty_ = true;
								if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
								ImGui::SetItemTooltip("Cone half-angle of spot light in degrees");

								if (ImGui::SliderFloat("Falloff Start Angle", &obj.lightSpotFalloffStart, 1.0f, obj.lightSpotAngle)) isSceneDirty_ = true;
								if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							}
						}
					}
				}

				// 8. PARTICLE SYSTEM COMPONENT (Parent Attachment, Presets & Controls)
				if (obj.hasParticleSystem) {
					EnsureParticleQuadInitialized(obj);
					if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen)) {
						// 1. Parent Attachment
						std::vector<std::string> parentNames;
						std::vector<int> parentIds;
						parentNames.push_back("None (World)");
						parentIds.push_back(-1);

						int selectedParentIdx = 0;
						for (size_t p = 0; p < gameObjects_.size(); ++p) {
							if (gameObjects_[p].id != obj.id) {
								if (static_cast<int>(gameObjects_[p].id) == obj.attachedParentId) {
									selectedParentIdx = static_cast<int>(parentIds.size());
								}
								parentNames.push_back("[" + std::to_string(gameObjects_[p].id) + "] " + gameObjects_[p].name);
								parentIds.push_back(static_cast<int>(gameObjects_[p].id));
							}
						}

						std::vector<const char*> parentNamePtrs;
						for (const auto& name : parentNames) parentNamePtrs.push_back(name.c_str());

						if (ImGui::Combo("Attach To GameObject", &selectedParentIdx, parentNamePtrs.data(), static_cast<int>(parentNamePtrs.size()))) {
							obj.attachedParentId = parentIds[selectedParentIdx];
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Attach particle emitter to track a parent GameObject");

						if (obj.attachedParentId != -1) {
							if (DrawVec3Control("Local Offset", obj.localOffset, 0.0f)) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Position offset relative to parent GameObject");
						}

						ImGui::Separator();

						// 2. Presets
						const char* presetNames[] = {
							"Select Preset...",
							"Smoke",
							"Water Fountain",
							"Fireworks",
							"Explosion",
							"Torch / Campfire"
						};
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 110.0f);
						if (ImGui::Combo("Preset Template", &obj.particlePreset, presetNames, 6)) {
							if (obj.particlePreset > 0) {
								ApplyParticlePreset(obj, obj.particlePreset);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::SetItemTooltip("Select quick particle preset template");
						ImGui::SameLine();
						if (ImGui::Button("Apply Preset", ImVec2(100.0f, 0.0f))) {
							if (obj.particlePreset > 0) {
								ApplyParticlePreset(obj, obj.particlePreset);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::SetItemTooltip("Apply selected preset to fill all parameters above");

						ImGui::Separator();

						// 3. Emitter Shape & Volume
						const char* shapeNames[] = { "Point", "Sphere", "Box", "Cone" };
						if (ImGui::Combo("Emitter Shape", &obj.particleShape, shapeNames, 4)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Shape geometry of particle emission source");

						if (obj.particleShape == 1 || obj.particleShape == 3) {
							if (ImGui::DragFloat("Spawn Radius", &obj.particleSpawnRadius, 0.01f, 0.0f, 20.0f, "%.2f")) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Radius of emission volume");
						}
						if (obj.particleShape == 2) {
							if (DrawVec3Control("Box Extents", obj.particleBoxExtents, 1.0f)) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Width, height, and depth extents of emission box");
						}
						if (obj.particleShape == 3) {
							if (ImGui::DragFloat("Cone Angle", &obj.particleConeAngle, 0.1f, 0.0f, 90.0f, "%.1f deg")) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Angle of cone emission apex");
						}

						ImGui::Separator();

						// 4. Emission Rate & Capacity
						if (ImGui::DragFloat("Spawn Rate", &obj.particleSpawnRate, 0.5f, 0.1f, 200.0f, "%.1f /s")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Particles spawned per second");

						if (ImGui::DragInt("Max Particles", &obj.particleMaxParticles, 1.0f, 1, 1000)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Maximum particle pool capacity for this emitter");

						// Burst Now button with count
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
						if (ImGui::DragInt("##BurstCount", &obj.particleBurstCount, 1.0f, 1, 200, "Count: %d")) {
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SameLine();
						if (ImGui::Button("Burst Now", ImVec2(110.0f, 0.0f))) {
							if (obj.particleQuad) {
								obj.particleQuad->EmitBurst(static_cast<uint32_t>(obj.particleBurstCount));
							}
						}
						ImGui::SetItemTooltip("Instantly emit a burst of particles");

						ImGui::Separator();

						// 5. Move Direction & Velocity
						if (DrawVec3Control("Move Direction", obj.particleDirection, 0.0f)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Primary emission travel direction vector");

						if (ImGui::SliderFloat("Spread Angle", &obj.particleSpreadAngle, 0.0f, 180.0f, "%.1f deg")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Velocity dispersion cone (0 = straight beam, 180 = full spherical omni)");

						float speedRange[2] = { obj.particleSpeedMin, obj.particleSpeedMax };
						if (ImGui::DragFloat2("Speed Min / Max", speedRange, 0.05f, 0.0f, 100.0f, "%.2f")) {
							obj.particleSpeedMin = speedRange[0];
							obj.particleSpeedMax = (std::max)(speedRange[0], speedRange[1]);
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Initial emission velocity speed range");

						ImGui::Separator();

						// 6. Forces & Drag
						if (ImGui::DragFloat("Gravity Modifier", &obj.particleGravity, 0.05f, -50.0f, 50.0f, "%.2f")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Vertical acceleration (negative = downward fall, positive = upward draft)");

						if (ImGui::DragFloat("Linear Drag", &obj.particleDrag, 0.01f, 0.0f, 10.0f, "%.2f")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Air resistance deceleration factor");

						// Wind Field
						if (ImGui::Checkbox("Enable Wind Field", &obj.particleWindEnabled)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Enable global directional wind force on particles");
						if (obj.particleWindEnabled) {
							if (DrawVec3Control("Wind Accel", obj.particleWindAcceleration, 0.0f)) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Continuous directional acceleration applied to active particles");
						}

						ImGui::Separator();

						// 7. Lifetime & Scale
						float lifeRange[2] = { obj.particleLifetimeMin, obj.particleLifetimeMax };
						if (ImGui::DragFloat2("Lifetime Min / Max", lifeRange, 0.05f, 0.05f, 30.0f, "%.2f s")) {
							obj.particleLifetimeMin = lifeRange[0];
							obj.particleLifetimeMax = (std::max)(lifeRange[0], lifeRange[1]);
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Lifespan duration of individual particles");

						float scaleRange[2] = { obj.particleStartScale, obj.particleEndScale };
						if (ImGui::DragFloat2("Start -> End Scale", scaleRange, 0.01f, 0.001f, 10.0f, "%.2f")) {
							obj.particleStartScale = scaleRange[0];
							obj.particleEndScale = scaleRange[1];
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Particle scale at birth (left) interpolated to death (right)");

						ImGui::Separator();

						// 8. Color & Blending
						float startCol[4] = { obj.particleStartColor.x, obj.particleStartColor.y, obj.particleStartColor.z, obj.particleStartColor.w };
						if (ImGui::ColorEdit4("Start Color", startCol)) {
							obj.particleStartColor = { startCol[0], startCol[1], startCol[2], startCol[3] };
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Initial particle color and opacity at birth");

						float endCol[4] = { obj.particleEndColor.x, obj.particleEndColor.y, obj.particleEndColor.z, obj.particleEndColor.w };
						if (ImGui::ColorEdit4("End Color", endCol)) {
							obj.particleEndColor = { endCol[0], endCol[1], endCol[2], endCol[3] };
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Final particle color and opacity at death");

						const char* blendModeNames[] = { "Normal Alpha", "Additive Glow" };
						if (ImGui::Combo("Blend Mode", &obj.particleBlendMode, blendModeNames, 2)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Alpha blending: Normal Alpha vs Additive Glow");

						const char* billboardModeNames[] = { "Camera Facing", "Y-Axis Cylindrical", "Velocity Aligned" };
						if (ImGui::Combo("Billboard Mode", &obj.particleBillboardMode, billboardModeNames, 3)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Billboard alignment: Camera Facing, Cylindrical Y, or Velocity Aligned");
					}
				}

				// 9. MAPCHIP LEVEL EDITOR
				if (obj.hasMapchip || obj.type == GameObjectType::MapchipGrid) {
					DrawMapchipInspectorSection();
				}

				// 9b. AUDIO SOURCE COMPONENT (XAudio2 + Windows Media Foundation: .wav / .mp3 / .m4a / .wma)
				if (obj.hasAudioSource || obj.type == GameObjectType::AudioSource) {
					obj.hasAudioSource = true;
					if (ImGui::CollapsingHeader("Audio Source (XAudio2 + Media Foundation)", ImGuiTreeNodeFlags_DefaultOpen)) {
						// Discover available audio files in Resources/Sound and Assets/Audio
						std::vector<std::string> audioClips;
						audioClips.push_back("None");
						std::vector<std::string> soundDirs = { "Resources/Sound", "Resources", "Assets/Audio", "Assets" };
						for (const auto& dir : soundDirs) {
							std::error_code ec;
							if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
								for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
									if (!entry.is_regular_file(ec)) continue;
									std::string ext = entry.path().extension().string();
									std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
									if (ext == ".wav" || ext == ".mp3" || ext == ".m4a" || ext == ".wma") {
										std::string relPath = entry.path().generic_string();
										if (std::find(audioClips.begin(), audioClips.end(), relPath) == audioClips.end()) {
											audioClips.push_back(relPath);
										}
									}
								}
							}
						}

						int curClipIdx = 0;
						for (size_t cIdx = 0; cIdx < audioClips.size(); ++cIdx) {
							if (audioClips[cIdx] == obj.audioClipName) {
								curClipIdx = static_cast<int>(cIdx);
								break;
							}
						}

						std::vector<const char*> clipPtrs;
						for (const auto& s : audioClips) clipPtrs.push_back(s.c_str());

						if (ImGui::Combo("Audio ClipPreset", &curClipIdx, clipPtrs.data(), static_cast<int>(clipPtrs.size()))) {
							obj.audioClipName = audioClips[curClipIdx];
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(obj.audioClipName);
							}
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Select audio file (.wav, .mp3, .m4a, .wma) decoded via Media Foundation & cached in Audio::LoadAudio");

						char clipBuf[256] = {};
						strncpy_s(clipBuf, obj.audioClipName.c_str(), sizeof(clipBuf) - 1);
						if (ImGui::InputText("Clip Path", clipBuf, sizeof(clipBuf))) {
							obj.audioClipName = clipBuf;
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) {
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(obj.audioClipName);
							}
							PushUndoSnapshot();
						}

						if (ImGui::SliderFloat("Volume", &obj.audioVolume, 0.0f, 1.0f, "%.2f")) {
							if (obj.runtimeVoiceHandle != 0) {
								Audio::GetInstance()->SetVolume(obj.runtimeVoiceHandle, obj.audioVolume);
							}
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

						if (ImGui::Checkbox("Play On Awake", &obj.audioPlayOnAwake)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SameLine();
						if (ImGui::Checkbox("Loop##Audio", &obj.audioLoop)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}

						bool isPlayingNow = (obj.runtimeVoiceHandle != 0) && Audio::GetInstance()->IsPlaying(obj.runtimeVoiceHandle);
						if (isPlayingNow) {
							ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.45f, 1.0f), "Status: Playing (Voice #%u)", obj.runtimeVoiceHandle);
						} else {
							ImGui::TextDisabled("Status: Stopped");
						}

						if (ImGui::Button("[ > Play Preview ]", ImVec2(135.0f, 24.0f))) {
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								if (obj.runtimeVoiceHandle != 0) {
									Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
								}
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(obj.audioClipName);
								obj.runtimeVoiceHandle = Audio::GetInstance()->PlayAudio(obj.runtimeSoundHandle, obj.audioLoop, obj.audioVolume);
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("[ [] Stop ]##AudioStop", ImVec2(100.0f, 24.0f))) {
							if (obj.runtimeVoiceHandle != 0) {
								Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
								obj.runtimeVoiceHandle = 0;
							}
						}
					}
				}

				// 10. SCRIPT COMPONENTS
				EnsureEditModeScriptInstances();
				for (size_t sIdx = 0; sIdx < obj.scripts.size(); ) {
					ImGui::PushID(static_cast<int>(sIdx));
					std::string headerLabel = std::format("{} (Script)##ScriptHeader_{}", obj.scripts[sIdx].className, sIdx);
					bool open = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
					bool removeScript = false;
					if (ImGui::BeginPopupContextItem("ScriptCompCtx")) {
						if (ImGui::MenuItem("Open Script (.cpp)")) {
							std::filesystem::path cppPath = ResolveGameScriptsDir() / "Scripts" / (obj.scripts[sIdx].className + ".cpp");
							OpenFileInVisualStudio(cppPath);
						}
						if (ImGui::MenuItem("Open Header (.h)")) {
							std::filesystem::path hPath = ResolveGameScriptsDir() / "Scripts" / (obj.scripts[sIdx].className + ".h");
							OpenFileInVisualStudio(hPath);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Remove Component")) {
							removeScript = true;
						}
						ImGui::EndPopup();
					}
					if (open) {
						ImGui::Indent(8.0f);
						auto& sc = obj.scripts[sIdx];
						ImGui::Text("Script Class: %s", sc.className.c_str());
						bool isReg = scriptModule_.IsRegistered(sc.className);
						if (sc.isCompiling || (isBuildingScripts_.load() && !isReg)) {
							ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Status: Compiling...");
						} else if (!isReg || sc.isMissingScript) {
							ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.25f, 1.0f), "Status: Missing Script (Click [ Build Scripts ] or fix class)");
						} else if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause) {
							ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Status: Active (Running in Play Mode)");
						} else {
							ImGui::TextDisabled("Status: Ready (GameScripts.dll)");
						}

						// [ Open Script ] and [ .h ] buttons on the same line, plus [ Remove ]
						float availW = ImGui::GetContentRegionAvail().x;
						float openW = (std::max)(90.0f, availW - 135.0f);
						if (ImGui::Button("[ Open Script ]", ImVec2(openW, 24.0f))) {
							std::filesystem::path cppPath = ResolveGameScriptsDir() / "Scripts" / (sc.className + ".cpp");
							OpenFileInVisualStudio(cppPath);
						}
						ImGui::SetItemTooltip("Open %s.cpp in Visual Studio", sc.className.c_str());

						ImGui::SameLine();
						if (ImGui::Button("[ .h ]", ImVec2(45.0f, 24.0f))) {
							std::filesystem::path hPath = ResolveGameScriptsDir() / "Scripts" / (sc.className + ".h");
							OpenFileInVisualStudio(hPath);
						}
						ImGui::SetItemTooltip("Open %s.h in Visual Studio", sc.className.c_str());

						ImGui::SameLine();
						if (ImGui::Button("Remove", ImVec2(75.0f, 24.0f))) {
							removeScript = true;
						}
						ImGui::SetItemTooltip("Remove this Script component from GameObject");

						// Exposed Fields UI (EXPOSE_FIELD reflection)
						if (sc.instance && isReg) {
							int fieldCount = scriptModule_.GetFieldCount(sc.instance.get());
							if (fieldCount > 0) {
								ImGui::Spacing();
								ImGui::Separator();
								ImGui::TextDisabled("Exposed Fields:");
								for (int fIdx = 0; fIdx < fieldCount; ++fIdx) {
									ScriptFieldInfo info{};
									if (!scriptModule_.GetFieldInfo(sc.instance.get(), fIdx, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;

									ImGui::PushID(fIdx);
									bool fieldEdited = false;
									bool fieldCommitted = false;

									switch (info.type) {
									case ScriptFieldType::Float: {
										float* ptr = static_cast<float*>(info.dataPtr);
										if (ImGui::DragFloat(info.name, ptr, 0.05f, 0.0f, 0.0f, "%.3f")) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Int: {
										int* ptr = static_cast<int*>(info.dataPtr);
										if (ImGui::DragInt(info.name, ptr, 1.0f)) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Bool: {
										bool* ptr = static_cast<bool*>(info.dataPtr);
										if (ImGui::Checkbox(info.name, ptr)) {
											fieldEdited = true;
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Vec3: {
										Vector3* ptr = static_cast<Vector3*>(info.dataPtr);
										if (DrawVec3Control(info.name, *ptr, 0.0f)) {
											fieldEdited = true;
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::CharBuffer: {
										char* ptr = static_cast<char*>(info.dataPtr);
										if (ImGui::InputText(info.name, ptr, info.sizeInBytes)) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									}

									if (fieldEdited || fieldCommitted) {
										CaptureScriptFieldsToSaved(sc);
										isSceneDirty_ = true;
									}
									if (fieldCommitted) {
										PushUndoSnapshot();
									}
									ImGui::PopID();
								}
							}
						}

						ImGui::Unindent(8.0f);
					}
					ImGui::PopID();

					if (removeScript) {
						if (obj.scripts[sIdx].instance) {
							if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode()) {
								obj.scripts[sIdx].instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
								obj.scripts[sIdx].instance->SetEngineAPI(&engineApi_);
								obj.scripts[sIdx].instance->SetHost(this, this);
								obj.scripts[sIdx].instance->OnDestroy();
							}
							obj.scripts[sIdx].instance.reset();
						}
						obj.scripts.erase(obj.scripts.begin() + static_cast<std::ptrdiff_t>(sIdx));
						obj.hasScript = !obj.scripts.empty();
						obj.scriptClassName = obj.scripts.empty() ? "" : obj.scripts.front().className;
						isSceneDirty_ = true;
						PushUndoSnapshot();
					} else {
						++sIdx;
					}
				}

				// Centered [ Add Component ] Button
				ImGui::Spacing();
				ImGui::Separator();
				float btnWidth = 180.0f;
				float availWidth = ImGui::GetContentRegionAvail().x;
				ImGui::SetCursorPosX((availWidth - btnWidth) * 0.5f);
				if (ImGui::Button("Add Component", ImVec2(btnWidth, 28.0f))) {
					ImGui::OpenPopup("AddComponentPopup");
				}

				if (ImGui::BeginPopup("AddComponentPopup")) {
					if (ImGui::MenuItem("Mesh Filter & Renderer (Cube)")) { obj.hasMesh = true; obj.meshType = GameObject::MeshType::Cube; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Textured Quad")) { obj.hasMesh = true; obj.meshType = GameObject::MeshType::Quad; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Box Collider")) { obj.hasBoxCollider = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Sphere Collider")) { obj.hasSphereCollider = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Rigidbody")) { obj.hasRigidbody = true; obj.rbEnabled = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Camera")) { obj.hasCamera = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Light")) { obj.hasLight = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Particle System")) { obj.hasParticleSystem = true; EnsureParticleQuadInitialized(obj); isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Audio Source")) { obj.hasAudioSource = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					ImGui::Separator();
					if (ImGui::BeginMenu("Script")) {
						const auto& registeredScripts = scriptModule_.GetRegisteredNames();
						if (registeredScripts.empty()) {
							ImGui::TextDisabled("No registered scripts in GameScripts.dll");
						} else {
							for (const auto& scriptName : registeredScripts) {
								if (ImGui::MenuItem(scriptName.c_str())) {
									GameObject::ScriptComponent sc(scriptName);
									sc.instance = ScriptInstancePtr(scriptModule_.CreateScript(scriptName), ScriptInstanceDeleter{ &scriptModule_ });
									if (sc.instance) {
										RefreshAllScriptObjectViews();
										ScriptGameObjectView* view = GetOrUpdateScriptViewById(obj.id);
										sc.instance->SetOwner(view, obj.id);
										sc.instance->SetEngineAPI(&engineApi_);
										sc.instance->SetHost(this, this);
										CaptureScriptFieldsToSaved(sc);
										if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode()) {
											sc.instance->Initialize();
											if (view && view->dirtyFromScript) {
												SyncViewToGameObject(*view, obj);
											}
										}
									}
									obj.scripts.push_back(std::move(sc));
									obj.hasScript = true;
									obj.scriptClassName = obj.scripts.front().className;
									isSceneDirty_ = true;
									PushUndoSnapshot();
								}
							}
						}
						ImGui::Separator();
						if (ImGui::MenuItem("New Script...")) {
							scriptGenTargetObjectIndex_ = selectedObjectIndex_;
							scriptGenErrorMsg_.clear();
							showScriptGenModal_ = true;
						}
						ImGui::EndMenu();
					}
					ImGui::EndPopup();
				}

				ImGui::EndTabItem();
			}

			// DETAILS TAB (Live Asset Previews & Engine Statistics)
			ImGuiTabItemFlags detailsTabFlags = focusDetailsTab_ ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
			if (ImGui::BeginTabItem("Details", nullptr, detailsTabFlags)) {
				focusDetailsTab_ = false;
				rightPanelTab_ = 1;
				ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Asset & Scene Details");
				ImGui::Separator();

				if (!selectedAssetPath_.empty() && std::filesystem::exists(selectedAssetPath_) && !std::filesystem::is_directory(selectedAssetPath_)) {
					std::string ext = selectedAssetPath_.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					ImGui::Text("Selected Asset: %s", selectedAssetPath_.filename().string().c_str());
					ImGui::TextDisabled("Path: %s", selectedAssetPath_.string().c_str());

					try {
						auto sizeBytes = std::filesystem::file_size(selectedAssetPath_);
						if (sizeBytes < 1024) {
							ImGui::Text("File Size: %llu Bytes (%.2f KB)", static_cast<unsigned long long>(sizeBytes), sizeBytes / 1024.0f);
						} else {
							ImGui::Text("File Size: %.2f KB (%llu Bytes)", sizeBytes / 1024.0f, static_cast<unsigned long long>(sizeBytes));
						}
					} catch (...) {}

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
						ImGui::Separator();
						ImGui::Text("2D Texture Preview:");
						if (currentPreviewTextureHandle_ == 0 && currentPreviewAssetPath_ != selectedAssetPath_.string()) {
							SelectAssetForPreview(selectedAssetPath_);
						}
						if (currentPreviewTextureHandle_ != 0) {
							ImGui::Image(
								ImTextureRef((ImTextureID)TextureManager::GetInstance()->GetGPUHandle(currentPreviewTextureHandle_).ptr),
								ImVec2(140.0f, 140.0f));
						}

						if (ImGui::Button("[ Create Textured Rectangle in Scene ]", ImVec2(-1.0f, 26.0f))) {
							uint32_t permHandle = TextureManager::GetInstance()->LoadTexture(selectedAssetPath_.string());
							auto& q = CreateQuad(selectedAssetPath_.stem().string(), { 0.0f, 1.0f, 0.0f });
							q.texturePath = selectedAssetPath_.string();
							q.textureHandle = permHandle;
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
							if (ImGui::Button("[ Assign to Selected Object Material ]", ImVec2(-1.0f, 26.0f))) {
								uint32_t permHandle = TextureManager::GetInstance()->LoadTexture(selectedAssetPath_.string());
								gameObjects_[selectedObjectIndex_].texturePath = selectedAssetPath_.string();
								gameObjects_[selectedObjectIndex_].textureHandle = permHandle;
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
					} else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
						ImGui::Separator();
						ImGui::Text("3D Model Preview (Assimp):");
						if (!previewModel_ && currentPreviewAssetPath_ != selectedAssetPath_.string()) {
							SelectAssetForPreview(selectedAssetPath_);
						}
						ImGui::Text("Format: %s", ext.c_str());
						ImGui::Text("Vertices: %u  |  Triangles: %u", previewVertexCount_, previewTriangleCount_);
						if (previewModel_) {
							ImGui::Image(
								ImTextureRef((ImTextureID)modelPreviewRT_.GetSrvGPUHandle().ptr),
								ImVec2(180.0f, 180.0f));
						} else {
							ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.4f, 1.0f), "Unable to parse 3D geometry.");
						}
						if (ImGui::Button("[ Spawn 3D Model in Scene ]", ImVec2(-1.0f, 26.0f))) {
							auto& obj = CreateGameObject(selectedAssetPath_.stem().string(), GameObjectType::Cube);
							obj.hasMesh = true;
							obj.meshEnabled = true;
							obj.meshType = GameObject::MeshType::CustomModel;
							obj.modelPath = selectedAssetPath_.string();
							obj.translation = { 0.0f, 1.0f, 0.0f };
							isSceneDirty_ = true;
							PushUndoSnapshot();
							LogMessage(std::format("[Scene] Spawned 3D Model '{}' from '{}'", obj.name, obj.modelPath), 0);
						}
					} else if (ext == ".wav" || ext == ".mp3" || ext == ".m4a" || ext == ".wma") {
						ImGui::Separator();
						ImGui::Text("Audio Clip Preview (XAudio2 + Media Foundation):");
						if (ImGui::Button("[ > Play Audio Clip ]", ImVec2(-1.0f, 26.0f))) {
							uint32_t sh = Audio::GetInstance()->LoadAudio(selectedAssetPath_.string());
							Audio::GetInstance()->PlayAudio(sh, false, 1.0f);
						}
						if (ImGui::Button("[ [] Stop All Audio ]", ImVec2(-1.0f, 26.0f))) {
							Audio::GetInstance()->StopAllAudio();
						}
						if (ImGui::Button("[ Create Audio Source in Scene ]", ImVec2(-1.0f, 26.0f))) {
							auto& aud = CreateGameObject(selectedAssetPath_.stem().string(), GameObjectType::AudioSource);
							aud.hasAudioSource = true;
							aud.audioClipName = selectedAssetPath_.generic_string();
							aud.audioPlayOnAwake = true;
							aud.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(aud.audioClipName);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
					} else if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
						ImGui::Separator();
						ImGui::Text("C++ Script Source File:");
						if (ImGui::Button("[ Open in Visual Studio ]", ImVec2(-1.0f, 26.0f))) {
							OpenFileInVisualStudio(selectedAssetPath_);
						}
					}
					ImGui::Separator();
				} else {
					ImGui::TextDisabled("Select a texture (.png, .jpg, .dds) or 3D model (.obj, .fbx, .gltf) in the Project browser to inspect.");
					ImGui::Separator();
				}

				ImGui::Text("Total GameObjects: %zu", gameObjects_.size());
				ImGui::Text("Active Cameras: %s", gameCamera_ ? "1" : "0");
				ImGui::Text("Lighting: %s", enableLighting_ ? "Enabled" : "Disabled");
				ImGui::Checkbox("Frustum Culling", &enableFrustumCulling_);
				ImGui::Text("Frustum Culling Stats: %d Visible / %d Culled", lastVisibleObjects_, lastCulledObjects_);

				ImGui::EndTabItem();
			} else {
				focusDetailsTab_ = false;
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif
}

void EditorSuite::SelectAssetForPreview(const std::filesystem::path& assetPath)
{
	if (!dxCommon_ || dxCommon_->IsDeviceLost()) {
		return;
	}

	if (currentPreviewAssetPath_ == assetPath.string() && (currentPreviewTextureHandle_ != 0 || previewModel_ != nullptr)) {
		selectedAssetPath_ = assetPath;
		focusDetailsTab_ = true;
		return;
	}

	ReleasePreviewResources();
	selectedAssetPath_ = assetPath;
	currentPreviewAssetPath_ = assetPath.string();

	if (assetPath.empty() || !std::filesystem::exists(assetPath) || std::filesystem::is_directory(assetPath)) {
		return;
	}

	std::string ext = assetPath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
		currentPreviewTextureHandle_ = TextureManager::GetInstance()->LoadPreviewTexture(assetPath.string());
		focusDetailsTab_ = true;
	} else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
		previewModel_ = Model::CreatePreviewFromFile(assetPath.string());
		if (previewModel_) {
			previewVertexCount_ = previewModel_->GetTotalVertexCount();
			previewTriangleCount_ = previewModel_->GetTotalTriangleCount();
			Vector3 center = { 0.0f, 0.0f, 0.0f };
			float maxExtent = 2.0f;
			previewModel_->GetBounds(center, maxExtent);
			previewModelScale_ = 1.65f / (std::max)(maxExtent, 0.001f);
			previewModelOffset_ = center;
			previewRotationY_ = 0.0f;

			if (!previewObject3d_) {
				previewObject3d_ = std::make_unique<Object3d>();
				previewObject3d_->Initialize();
			}
			previewObject3d_->SetCamera(&previewCamera_);
			previewObject3d_->SetModel(previewModel_.get(), true);
			if (Material* mat = previewObject3d_->GetMaterial()) {
				mat->color = { 1.0f, 1.0f, 1.0f, 1.0f };
				mat->enableLighting = 1;
				mat->lightingType = 4;
				mat->shininess = 32.0f;
				mat->specularColor = { 0.6f, 0.6f, 0.6f };
				mat->receiveShadows = 0;
			}
		}
		focusDetailsTab_ = true;
	}
}

void EditorSuite::ReleasePreviewResources()
{
	if (currentPreviewTextureHandle_ != 0) {
		TextureManager::GetInstance()->ReleasePreviewTexture();
		currentPreviewTextureHandle_ = 0;
	}
	if (previewObject3d_) {
		previewObject3d_->SetModel(nullptr, true);
	}
	if (previewModel_) {
		// Model::~Model() queues its vertex and index buffers into dxCommon_->DeferRelease()
		previewModel_.reset();
	}
	previewVertexCount_ = 0;
	previewTriangleCount_ = 0;
	currentPreviewAssetPath_.clear();
}

void EditorSuite::DrawProjectAndConsolePanel()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float screenH = io.DisplaySize.y;
	float topY = ImGui::GetFrameHeight() + 38.0f;
	float midH = (screenH - topY) * 0.65f;
	float bottomH = screenH - topY - midH;

	ImGui::SetNextWindowPos(ImVec2(0.0f, topY + midH));
	ImGui::SetNextWindowSize(ImVec2(screenW, bottomH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Project / Console Workspace", nullptr, flags)) {
		if (ImGui::BeginTabBar("BottomPanelTabs")) {
			// TAB 1: PROJECT (Asset Browser with Subfolder Navigation)
			if (ImGui::BeginTabItem("Project")) {
				bottomPanelTab_ = 0;

				if (ImGui::Button("[ <- Back ]", ImVec2(95.0f, 22.0f))) {
					std::filesystem::path parent = currentDirectory_.parent_path();
					if (!parent.empty() && parent != currentDirectory_) {
						currentDirectory_ = parent;
					} else if (currentDirectory_ != ".") {
						currentDirectory_ = ".";
					}
				}
				ImGui::SetItemTooltip("Navigate to parent directory");

				ImGui::SameLine();
				if (ImGui::Button("Root", ImVec2(50.0f, 22.0f))) {
					currentDirectory_ = ".";
				}
				ImGui::SetItemTooltip("Browse workspace root (Resources, shaders, Assets)");

				ImGui::SameLine();
				if (ImGui::Button("Resources", ImVec2(78.0f, 22.0f))) {
					if (std::filesystem::exists("Resources")) currentDirectory_ = "Resources";
				}
				ImGui::SameLine();
				if (ImGui::Button("Scripts", ImVec2(68.0f, 22.0f))) {
					std::filesystem::path scriptsDir = ResolveGameScriptsDir() / "Scripts";
					if (std::filesystem::exists(scriptsDir)) {
						currentDirectory_ = scriptsDir;
					}
				}
				ImGui::SetItemTooltip("Browse GameScripts/Scripts (.cpp / .h)");

				ImGui::SameLine();
				if (ImGui::Button("shaders", ImVec2(65.0f, 22.0f))) {
					if (std::filesystem::exists("shaders")) currentDirectory_ = "shaders";
					else if (std::filesystem::exists("../RizealldayEngine/shaders")) currentDirectory_ = "../RizealldayEngine/shaders";
				}
				if (!currentProjectRoot_.empty() && std::filesystem::exists(currentProjectRoot_)) {
					ImGui::SameLine();
					if (ImGui::Button("Project Root", ImVec2(90.0f, 22.0f))) {
						currentDirectory_ = currentProjectRoot_;
					}
				}

				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Directory: %s", currentDirectory_.string().c_str());
				ImGui::SameLine((std::max)(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - 200.0f));
				ImGui::SetNextItemWidth(190.0f);
				ImGui::InputTextWithHint("##AssetSearch", "Search Assets...", assetSearchBuf_, sizeof(assetSearchBuf_));

				ImGui::Separator();

				ImGui::BeginChild("ProjectAssetsScrollArea", ImVec2(0, 0), false);
				try {
					if (std::filesystem::exists(currentDirectory_) && std::filesystem::is_directory(currentDirectory_)) {
						std::vector<std::filesystem::directory_entry> dirs;
						std::vector<std::filesystem::directory_entry> files;
						for (const auto& entry : std::filesystem::directory_iterator(currentDirectory_)) {
							std::string filename = entry.path().filename().string();
							if (assetSearchBuf_[0] != '\0' && filename.find(assetSearchBuf_) == std::string::npos) continue;
							if (entry.is_directory()) dirs.push_back(entry);
							else files.push_back(entry);
						}

						// Render directories first (double-click to enter subfolder)
						for (const auto& entry : dirs) {
							std::string filename = entry.path().filename().string();
							ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "[DIR]");
							ImGui::SameLine();
							bool isSel = (selectedAssetPath_ == entry.path());
							if (ImGui::Selectable(filename.c_str(), isSel, ImGuiSelectableFlags_AllowDoubleClick)) {
								if (selectedAssetPath_ != entry.path()) {
									ReleasePreviewResources();
									selectedAssetPath_ = entry.path();
								}
								if (ImGui::IsMouseDoubleClicked(0)) {
									currentDirectory_ = entry.path();
								}
							}
							ImGui::SetItemTooltip("Double-click to open folder '%s'", filename.c_str());
						}

						// Render files
						for (const auto& entry : files) {
							std::string filename = entry.path().filename().string();
							std::string ext = entry.path().extension().string();
							std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

							std::string badge = "[FILE]";
							ImVec4 badgeCol = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

							if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
								badge = "[TEX]";
								badgeCol = ImVec4(0.9f, 0.4f, 0.6f, 1.0f);
							} else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
								badge = "[3D]";
								badgeCol = ImVec4(0.3f, 0.8f, 0.9f, 1.0f);
							} else if (filename.find(".scene.json") != std::string::npos || ext == ".json") {
								badge = "[SCENE]";
								badgeCol = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
							} else if (ext == ".csv") {
								badge = "[MAP]";
								badgeCol = ImVec4(0.2f, 0.8f, 0.3f, 1.0f);
							} else if (ext == ".hlsl" || ext == ".hlsli") {
								badge = "[HLSL]";
								badgeCol = ImVec4(0.7f, 0.5f, 0.95f, 1.0f);
							} else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
								badge = "[AUDIO]";
								badgeCol = ImVec4(0.4f, 0.9f, 0.7f, 1.0f);
							} else if (ext == ".cpp") {
								badge = "[CPP]";
								badgeCol = ImVec4(0.35f, 0.75f, 1.0f, 1.0f);
							} else if (ext == ".h" || ext == ".hpp") {
								badge = "[H]";
								badgeCol = ImVec4(0.65f, 0.55f, 1.0f, 1.0f);
							}

							ImGui::TextColored(badgeCol, "%s", badge.c_str());
							ImGui::SameLine();
							bool isSel = (selectedAssetPath_ == entry.path());
							if (ImGui::Selectable(filename.c_str(), isSel, ImGuiSelectableFlags_AllowDoubleClick)) {
								SelectAssetForPreview(entry.path());
								if (ImGui::IsMouseDoubleClicked(0)) {
									if (filename.find(".scene.json") != std::string::npos) {
										LoadScene(entry.path().string());
									} else if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
										OpenFileInVisualStudio(entry.path());
									}
								}
							}
						}
					}
				} catch (...) {}
				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			// TAB 2: CONSOLE
			if (ImGui::BeginTabItem("Console")) {
				bottomPanelTab_ = 1;
				if (ImGui::Button("Clear")) {
					consoleLogs_.clear();
					infoCount_ = 0; warningCount_ = 0; errorCount_ = 0;
				}
				ImGui::SameLine();
				ImGui::Checkbox("Collapse", &collapseLogs_);
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 180.0f);
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Info: %d", infoCount_);
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Warn: %d", warningCount_);
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %d", errorCount_);

				ImGui::Separator();
				ImGui::BeginChild("ConsoleScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
				for (const auto& log : consoleLogs_) {
					ImVec4 col = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
					if (log.type == 1) col = ImVec4(0.95f, 0.85f, 0.3f, 1.0f);
					else if (log.type == 2) col = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
					ImGui::TextColored(col, "%s", log.message.c_str());
				}
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif
}

void EditorSuite::DrawSaveSceneAsModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Save Scene As...");
	if (ImGui::BeginPopupModal("Save Scene As...", &showSaveSceneAsModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter Scene File Path (.scene.json):");
		ImGui::Separator();

		ImGui::InputText("File Path", saveSceneAsPathBuf_, sizeof(saveSceneAsPathBuf_));

		ImGui::Separator();
		if (ImGui::Button("Save", ImVec2(100.0f, 26.0f))) {
			SaveScene(saveSceneAsPathBuf_);
			showSaveSceneAsModal_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(90.0f, 26.0f))) {
			showSaveSceneAsModal_ = false;
		}
		ImGui::EndPopup();
	}
#endif
}

void EditorSuite::DrawOpenSceneModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Open Scene...");
	if (ImGui::BeginPopupModal("Open Scene...", &showOpenSceneModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Select or enter a .scene.json file to load:");
		ImGui::Separator();

		// List of scenes in project directory
		ImGui::BeginChild("SceneListArea", ImVec2(360.0f, 150.0f), true);
		try {
			std::vector<std::string> searchPaths = { "Assets/Scenes", "Assets", "." };
			if (!currentProjectRoot_.empty()) {
				searchPaths.insert(searchPaths.begin(), (currentProjectRoot_ / "Assets" / "Scenes").string());
			}
			for (const auto& sp : searchPaths) {
				if (std::filesystem::exists(sp) && std::filesystem::is_directory(sp)) {
					for (const auto& entry : std::filesystem::directory_iterator(sp)) {
						std::string fn = entry.path().filename().string();
						if (fn.find(".scene.json") != std::string::npos) {
							bool isSel = (openScenePathBuf_ == entry.path().string());
							if (ImGui::Selectable(entry.path().string().c_str(), isSel)) {
								strncpy_s(openScenePathBuf_, entry.path().string().c_str(), sizeof(openScenePathBuf_) - 1);
							}
						}
					}
				}
			}
		} catch (...) {}
		ImGui::EndChild();

		ImGui::InputText("Path", openScenePathBuf_, sizeof(openScenePathBuf_));

		ImGui::Separator();
		if (ImGui::Button("Open", ImVec2(100.0f, 26.0f))) {
			if (openScenePathBuf_[0] != '\0') {
				LoadScene(openScenePathBuf_);
				showOpenSceneModal_ = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(90.0f, 26.0f))) {
			showOpenSceneModal_ = false;
		}
		ImGui::EndPopup();
	}
#endif
}

void EditorSuite::DrawNewProjectModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("New Project Wizard");
	if (ImGui::BeginPopupModal("New Project Wizard", &showNewProjectModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create a New Engine Project Structure");
		ImGui::Separator();

		ImGui::InputText("Project Name", projectNameBuf_, sizeof(projectNameBuf_));
		ImGui::InputText("Parent Directory", projectFolderBuf_, sizeof(projectFolderBuf_));
		ImGui::SameLine();
		if (ImGui::Button("[ Browse Folder... ]")) {
			std::string chosen = OpenNativeFolderDialog();
			if (!chosen.empty()) strncpy_s(projectFolderBuf_, chosen.c_str(), sizeof(projectFolderBuf_) - 1);
		}

		ImGui::Separator();
		if (ImGui::Button("Create Project", ImVec2(130.0f, 26.0f))) {
			if (CreateNewProject(projectFolderBuf_, projectNameBuf_)) {
				showNewProjectModal_ = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(90.0f, 26.0f))) {
			showNewProjectModal_ = false;
		}
		ImGui::EndPopup();
	}
#endif
}

void EditorSuite::DrawOpenProjectModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Open Project");
	if (ImGui::BeginPopupModal("Open Project", &showOpenProjectModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Select a Project Folder (containing ProjectSettings/project.json):");
		ImGui::Separator();

		ImGui::InputText("Project Folder", openProjectFolderBuf_, sizeof(openProjectFolderBuf_));
		ImGui::SameLine();
		if (ImGui::Button("[ Browse Folder... ]")) {
			std::string chosen = OpenNativeFolderDialog();
			if (!chosen.empty()) strncpy_s(openProjectFolderBuf_, chosen.c_str(), sizeof(openProjectFolderBuf_) - 1);
		}

		ImGui::Separator();
		if (ImGui::Button("Open Project", ImVec2(130.0f, 26.0f))) {
			if (OpenProject(openProjectFolderBuf_)) {
				showOpenProjectModal_ = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(90.0f, 26.0f))) {
			showOpenProjectModal_ = false;
		}
		ImGui::EndPopup();
	}
#endif
}

void EditorSuite::DrawBuildSettingsModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Build Settings & Standalone Game Exporter");
	if (ImGui::BeginPopupModal("Build Settings & Standalone Game Exporter", &showBuildSettingsModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Export Standalone DirectX 12 Game Build");
		ImGui::Separator();

		ImGui::InputText("Target Output Folder", buildOutputDirBuf_, sizeof(buildOutputDirBuf_));
		ImGui::SameLine();
		if (ImGui::Button("[ Browse Folder... ]")) {
			std::string chosen = OpenNativeFolderDialog();
			if (!chosen.empty()) strncpy_s(buildOutputDirBuf_, chosen.c_str(), sizeof(buildOutputDirBuf_) - 1);
		}

		ImGui::InputText("Game Title", buildGameTitleBuf_, sizeof(buildGameTitleBuf_));

		const char* resolutions[] = { "1920 x 1080 (FHD)", "1280 x 720 (HD)", "2560 x 1440 (2K QHD)" };
		ImGui::Combo("Window Resolution", &buildResolutionIndex_, resolutions, 3);
		ImGui::Checkbox("Start in Fullscreen", &buildFullscreen_);

		ImGui::InputText("Startup Scene", buildStartupSceneBuf_, sizeof(buildStartupSceneBuf_));

		ImGui::Separator();
		if (ImGui::Button("[ Export Standalone Game ]", ImVec2(200.0f, 28.0f))) {
			if (ExportStandaloneGame()) {
				showBuildSettingsModal_ = false;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("[ Open Output Folder ]", ImVec2(160.0f, 28.0f))) {
			std::filesystem::path outDir(buildOutputDirBuf_);
			std::error_code ec;
			std::filesystem::create_directories(outDir, ec);
			std::filesystem::path absOut = std::filesystem::absolute(outDir, ec);
			ShellExecuteW(NULL, L"open", absOut.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(90.0f, 28.0f))) {
			showBuildSettingsModal_ = false;
		}
		ImGui::EndPopup();
	}
#endif
}

bool EditorSuite::DrawVec3Control(const std::string& label, Vector3& values, float resetValue, float columnWidth)
{
	bool modified = false;
#ifdef USE_IMGUI
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("Vec3ControlTable", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("LabelCol", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("ControlCol", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(label.c_str());

		ImGui::TableNextColumn();
		float availWidth = ImGui::GetContentRegionAvail().x;
		float itemWidth = (availWidth - 54.0f) / 3.0f;
		ImVec2 buttonSize = ImVec2(18.0f, 0.0f);

		// X Axis (Red)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
		if (ImGui::Button("X", buttonSize)) {
			values.x = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}
		ImGui::SameLine(0.0f, 4.0f);

		// Y Axis (Green)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.20f, 1.0f));
		if (ImGui::Button("Y", buttonSize)) {
			values.y = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}
		ImGui::SameLine(0.0f, 4.0f);

		// Z Axis (Blue)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.78f, 1.0f));
		if (ImGui::Button("Z", buttonSize)) {
			values.z = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}

		ImGui::EndTable();
	}

	ImGui::PopID();
#endif
	return modified;
}

void EditorSuite::DrawMapchipInspectorSection()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("Mapchip Level Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Render 3D Blocks in Viewport", &renderMapchip3DBlocks_);
		ImGui::Text("Palette:");
		const char* tileNames[] = { "0: Empty", "1: Wall Block", "2: Spike", "3: Spawn", "4: Goal" };
		for (int t = 0; t < 5; ++t) {
			ImGui::RadioButton(tileNames[t], &selectedBrushTile_, t);
			if (t < 4) ImGui::SameLine();
		}

		ImGui::Spacing();
		ImGui::Text("Level Grid (15x10):");

		float cellW = 18.0f;
		float cellH = 18.0f;
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 startPos = ImGui::GetCursorScreenPos();

		for (int r = 0; r < mapRows_; ++r) {
			for (int c = 0; c < mapCols_; ++c) {
				ImVec2 pMin = ImVec2(startPos.x + c * (cellW + 2.0f), startPos.y + r * (cellH + 2.0f));
				ImVec2 pMax = ImVec2(pMin.x + cellW, pMin.y + cellH);

				int tile = mapGrid_[r][c];
				ImU32 col = IM_COL32(40, 40, 45, 255);
				if (tile == 1) col = IM_COL32(180, 120, 70, 255); // Wall
				else if (tile == 2) col = IM_COL32(220, 60, 60, 255); // Spike
				else if (tile == 3) col = IM_COL32(60, 180, 220, 255); // Spawn
				else if (tile == 4) col = IM_COL32(60, 220, 100, 255); // Goal

				drawList->AddRectFilled(pMin, pMax, col, 2.0f);

				ImGui::SetCursorScreenPos(pMin);
				if (ImGui::InvisibleButton(std::format("##Tile_{}_{}", r, c).c_str(), ImVec2(cellW, cellH))) {
					mapGrid_[r][c] = selectedBrushTile_;
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}
			}
		}

		ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + mapRows_ * (cellH + 2.0f) + 8.0f));
		if (ImGui::Button("Save Mapchip CSV", ImVec2(140.0f, 24.0f))) {
			SaveMapchipCSV(mapFilePathBuf_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Mapchip CSV", ImVec2(140.0f, 24.0f))) {
			LoadMapchipCSV(mapFilePathBuf_);
		}
	}
#endif
}

bool EditorSuite::SaveMapchipCSV(const std::string& filePath)
{
	std::filesystem::path p(filePath);
	if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

	std::ofstream file(filePath);
	if (!file.is_open()) return false;

	for (int r = 0; r < mapRows_; ++r) {
		for (int c = 0; c < mapCols_; ++c) {
			file << mapGrid_[r][c];
			if (c < mapCols_ - 1) file << ",";
		}
		file << "\n";
	}
	file.close();
	LogMessage(std::format("[Mapchip] Saved: {}", filePath), 0);
	return true;
}

bool EditorSuite::LoadMapchipCSV(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open()) return false;

	std::vector<std::vector<int>> newGrid;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		std::vector<int> row;
		std::stringstream ss(line);
		std::string val;
		while (std::getline(ss, val, ',')) {
			try { row.push_back(std::stoi(val)); } catch (...) { row.push_back(0); }
		}
		newGrid.push_back(row);
	}
	file.close();

	if (!newGrid.empty()) {
		mapRows_ = static_cast<int>(newGrid.size());
		mapCols_ = static_cast<int>(newGrid[0].size());
		mapGrid_ = newGrid;
		LogMessage(std::format("[Mapchip] Loaded ({}x{}): {}", mapCols_, mapRows_, filePath), 0);
		return true;
	}
	return false;
}

bool EditorSuite::ExportMapchipJSON(const std::string& filePath)
{
	std::filesystem::path p(filePath);
	if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

	std::ofstream file(filePath);
	if (!file.is_open()) return false;

	file << "{\n";
	file << "  \"cols\": " << mapCols_ << ",\n";
	file << "  \"rows\": " << mapRows_ << ",\n";
	file << "  \"tileSize\": " << mapTileSize_ << ",\n";
	file << "  \"data\": [\n";
	for (int r = 0; r < mapRows_; ++r) {
		file << "    [";
		for (int c = 0; c < mapCols_; ++c) {
			file << mapGrid_[r][c];
			if (c < mapCols_ - 1) file << ", ";
		}
		file << "]";
		if (r < mapRows_ - 1) file << ",";
		file << "\n";
	}
	file << "  ]\n";
	file << "}\n";
	file.close();
	LogMessage(std::format("[Mapchip] Exported JSON: {}", filePath), 0);
	return true;
}

// Built-in example ScriptBase components so [ Add Component ] -> Script has ready-to-use scripts out of the box
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

void EditorSuite::DrawCppScriptGeneratorModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Unity-Style C++ Script Generator");
	if (ImGui::BeginPopupModal("Unity-Style C++ Script Generator", &showScriptGenModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create New C++ Script in GameScripts/Scripts/ (Derives from ScriptBase)");
		ImGui::Separator();

		if (ImGui::InputText("Class Name", scriptClassNameBuf_, sizeof(scriptClassNameBuf_))) {
			scriptGenErrorMsg_.clear();
		}
		ImGui::Checkbox("Include OnCollision Override", &scriptIncludeCollision_);
		std::filesystem::path targetScriptsDir = ResolveGameScriptsDir() / "Scripts";
		ImGui::TextDisabled("Target folder: %s", targetScriptsDir.string().c_str());
		ImGui::TextDisabled("Generates <ClassName>.h and <ClassName>.cpp, builds GameScripts.dll, and opens in Visual Studio.");

		if (!scriptGenErrorMsg_.empty()) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", scriptGenErrorMsg_.c_str());
		}

		ImGui::Separator();
		if (ImGui::Button("Create & Open", ImVec2(140.0f, 0.0f))) {
			if (GenerateCppScriptFiles(scriptClassNameBuf_, targetScriptsDir.string(), scriptIncludeCollision_)) {
				showScriptGenModal_ = false;
				scriptGenTargetObjectIndex_ = -1;
				scriptGenErrorMsg_.clear();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
			showScriptGenModal_ = false;
			scriptGenTargetObjectIndex_ = -1;
			scriptGenErrorMsg_.clear();
		}
		ImGui::EndPopup();
	}
#endif
}

bool EditorSuite::GenerateCppScriptFiles(const std::string& className, const std::string& folderPath, bool includeCollision)
{
	// Validate C++ identifier
	if (className.empty()) {
		scriptGenErrorMsg_ = "Error: Class Name cannot be empty.";
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}
	if (!(std::isalpha(static_cast<unsigned char>(className.front())) || className.front() == '_')) {
		scriptGenErrorMsg_ = "Error: Class Name must start with a letter or underscore.";
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}
	for (char c : className) {
		if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
			scriptGenErrorMsg_ = "Error: Class Name must contain only letters, digits, or underscores.";
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
	}

	std::filesystem::path targetDir = folderPath.empty() ? (ResolveGameScriptsDir() / "Scripts") : std::filesystem::path(folderPath);
	std::error_code ec;
	std::filesystem::create_directories(targetDir, ec);
	std::filesystem::path headerPath = targetDir / (className + ".h");
	std::filesystem::path sourcePath = targetDir / (className + ".cpp");

	if (scriptModule_.IsRegistered(className) || std::filesystem::exists(headerPath, ec) || std::filesystem::exists(sourcePath, ec)) {
		scriptGenErrorMsg_ = std::format("Error: Script '{}' already exists.", className);
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}

	// Generate <ClassName>.h deriving from ScriptBase with EXPOSE_FIELD sample
	{
		std::ofstream h(headerPath);
		if (!h.is_open()) {
			scriptGenErrorMsg_ = std::format("Error: Failed to write header file: {}", headerPath.string());
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
		h << "#pragma once\n";
		h << "#include \"base/ScriptAPI.h\"\n\n";
		h << "// Note: Fields not marked EXPOSE_FIELD are not preserved across a script hot reload.\n";
		h << "class " << className << " : public ScriptBase {\n";
		h << "public:\n";
		h << "    EXPOSE_FIELD(float, speed, 1.0f);\n\n";
		h << "    void Initialize() override;\n";
		h << "    void Update(float deltaTime) override;\n";
		if (includeCollision) {
			h << "    void OnCollision(GameObject& other) override;\n";
		}
		h << "    void OnDestroy() override;\n";
		h << "};\n";
		h.close();
	}

	// Generate <ClassName>.cpp with method stubs and REGISTER_SCRIPT(ClassName)
	{
		std::ofstream cpp(sourcePath);
		if (!cpp.is_open()) {
			scriptGenErrorMsg_ = std::format("Error: Failed to write source file: {}", sourcePath.string());
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
		cpp << "#include \"" << className << ".h\"\n\n";
		cpp << "void " << className << "::Initialize() {\n";
		cpp << "    if (GameObject* self = GetOwner()) {\n";
		cpp << "        (void)self;\n";
		cpp << "    }\n";
		cpp << "}\n\n";
		cpp << "void " << className << "::Update(float deltaTime) {\n";
		cpp << "    if (GameObject* self = GetOwner()) {\n";
		cpp << "        self->rotation.y += speed * deltaTime;\n";
		cpp << "        self->rotationQuat = Quaternion::FromEuler(self->rotation);\n";
		cpp << "    }\n";
		cpp << "}\n\n";
		if (includeCollision) {
			cpp << "void " << className << "::OnCollision(GameObject& other) {\n";
			cpp << "    (void)other;\n";
			cpp << "}\n\n";
		}
		cpp << "void " << className << "::OnDestroy() {\n";
		cpp << "}\n\n";
		cpp << "REGISTER_SCRIPT(" << className << ")\n";
		cpp.close();
	}

	// Register the new script files in GameScripts.vcxproj and GameScripts.vcxproj.filters under the Scripts filter
	{
		std::filesystem::path vcxprojPath = ResolveGameScriptsVcxproj();
		std::filesystem::path filtersPath = vcxprojPath.string() + ".filters";
		std::string relCpp = "Scripts\\" + className + ".cpp";
		std::string relH = "Scripts\\" + className + ".h";

		auto insertBeforeToken = [](const std::filesystem::path& file, const std::string& checkSubstr, const std::string& anchor, const std::string& insertion) {
			std::ifstream in(file);
			if (!in.is_open()) return;
			std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			in.close();
			if (content.find(checkSubstr) != std::string::npos) return;
			size_t pos = content.find(anchor);
			if (pos != std::string::npos) {
				content.insert(pos, insertion);
				std::ofstream out(file, std::ios::trunc);
				if (out.is_open()) {
					out << content;
				}
			}
		};

		insertBeforeToken(
			vcxprojPath,
			relCpp,
			"    <ClCompile Include=\"Scripts\\dllmain.cpp\" />",
			"    <ClCompile Include=\"" + relCpp + "\" />\n");
		insertBeforeToken(
			vcxprojPath,
			relH,
			"    <ClInclude Include=\"..\\RizealldayEngine\\base\\ScriptAPI.h\" />",
			"    <ClInclude Include=\"" + relH + "\" />\n");

		insertBeforeToken(
			filtersPath,
			relCpp,
			"    <ClCompile Include=\"Scripts\\dllmain.cpp\">",
			"    <ClCompile Include=\"" + relCpp + "\">\n      <Filter>Scripts</Filter>\n    </ClCompile>\n");
		insertBeforeToken(
			filtersPath,
			relH,
			"    <ClInclude Include=\"..\\RizealldayEngine\\base\\ScriptAPI.h\">",
			"    <ClInclude Include=\"" + relH + "\">\n      <Filter>Scripts</Filter>\n    </ClInclude>\n");
	}

	LogMessage(std::format("[Scripts] Created {}.h and {}.cpp in {}", className, className, targetDir.string()), 0);

	// 1. Attach <ClassName> to the target GameObject in a "Pending / Compiling..." state
	int attachIdx = (scriptGenTargetObjectIndex_ >= 0 && scriptGenTargetObjectIndex_ < static_cast<int>(gameObjects_.size()))
		? scriptGenTargetObjectIndex_
		: selectedObjectIndex_;
	if (attachIdx >= 0 && attachIdx < static_cast<int>(gameObjects_.size())) {
		auto& targetObj = gameObjects_[attachIdx];
		GameObject::ScriptComponent sc(className);
		sc.isCompiling = true;
		sc.isMissingScript = false;
		targetObj.scripts.push_back(std::move(sc));
		targetObj.hasScript = true;
		targetObj.scriptClassName = className;
		isSceneDirty_ = true;
		PushUndoSnapshot();
		LogMessage(std::format("[Scripts] Attached '{}' (Pending / Compiling...) to '{}'", className, targetObj.name), 0);
	}

	// 2. Trigger Build Scripts automatically
	TriggerScriptBuild();

	// 3. Open <ClassName>.cpp in Visual Studio
	OpenFileInVisualStudio(sourcePath);

	return true;
}

void EditorSuite::LogMessage(const std::string& msg, int logType)
{
	LogEntry entry;
	entry.message = msg;
	entry.type = logType;

	if (logType == 0) infoCount_++;
	else if (logType == 1) warningCount_++;
	else if (logType == 2) errorCount_++;

	auto now = std::time(nullptr);
	tm ltm{};
	localtime_s(&ltm, &now);
	std::ostringstream ss;
	ss << std::put_time(&ltm, "%H:%M:%S");
	entry.timestamp = ss.str();
	entry.message = std::format("[{}] {}", entry.timestamp, msg);

	consoleLogs_.push_back(entry);
	if (consoleLogs_.size() > 500) {
		consoleLogs_.erase(consoleLogs_.begin());
	}
}

void EditorSuite::RefreshAssetBrowser()
{
	if (!std::filesystem::exists(currentDirectory_)) {
		currentDirectory_ = "Resources";
	}
}

void EditorSuite::DrawDirectoryTreeNode(const std::filesystem::path& dirPath)
{
	if (!std::filesystem::exists(dirPath)) return;
}

bool EditorSuite::HasActiveParticleSystem(Vector3* outEmitterPos) const
{
	for (const auto& obj : gameObjects_) {
		if (obj.isActive && obj.hasParticleSystem) {
			if (outEmitterPos) {
				*outEmitterPos = obj.translation;
			}
			return true;
		}
	}
	return false;
}

void EditorSuite::StartPlayMode()
{
	if (sceneState_ != EditorSceneState::Edit) {
		sceneState_ = EditorSceneState::Play;
		return;
	}

	// Capture any Edit-mode Inspector tweaks into savedFields before taking the Play snapshot
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			CaptureScriptFieldsToSaved(sc);
		}
	}

	SavePlayModeSnapshot();
	DestroyAllScriptInstances();
	sceneState_ = EditorSceneState::Play;
	stepSingleFrame_ = false;

	RefreshAllScriptObjectViews();

	// 1. Instantiate all Script components for all GameObjects in the scene via GameScripts DLL
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		auto& obj = gameObjects_[i];
		for (auto& sc : obj.scripts) {
			sc.instance.reset();
			if (sc.className.empty()) continue;

			if (scriptModule_.IsLoaded() && scriptModule_.IsRegistered(sc.className)) {
				sc.instance = ScriptInstancePtr(scriptModule_.CreateScript(sc.className), ScriptInstanceDeleter{ &scriptModule_ });
				sc.isMissingScript = false;
				sc.isCompiling = false;
				if (sc.instance) {
					sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					ApplySavedFieldsToScript(sc);
				}
			} else {
				sc.isMissingScript = true;
				LogMessage(std::format("[Scripts] Warning: Script '{}' on GameObject '{}' is missing from GameScripts.dll and cannot run.", sc.className, obj.name), 1);
			}
		}
	}

	// 2. Call Initialize() on all created script instances
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (!gameObjects_[i].isActive) continue;

		for (size_t s = 0; s < gameObjects_[i].scripts.size(); ++s) {
			auto& sc = gameObjects_[i].scripts[s];
			if (sc.instance) {
				ScriptGameObjectView* view = GetOrUpdateScriptViewById(gameObjects_[i].id);
				Vector3 prevEuler = view ? view->rotation : gameObjects_[i].rotation;
				Quaternion prevQuat = view ? view->rotationQuat : gameObjects_[i].rotationQuat;

				sc.instance->SetOwner(view, gameObjects_[i].id);
				sc.instance->SetEngineAPI(&engineApi_);
				sc.instance->SetHost(this, this);
				sc.instance->Initialize();

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

	// 3. Start Audio Sources configured with Play On Awake (using cached LoadAudio / PlayAudio separation)
	Audio::GetInstance()->StopAllAudio();
	for (auto& obj : gameObjects_) {
		obj.runtimeVoiceHandle = 0;
		if (obj.isActive && (obj.hasAudioSource || obj.type == GameObjectType::AudioSource)) {
			if (obj.audioPlayOnAwake && !obj.audioClipName.empty() && obj.audioClipName != "None") {
				obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(obj.audioClipName);
				obj.runtimeVoiceHandle = Audio::GetInstance()->PlayAudio(obj.runtimeSoundHandle, obj.audioLoop, obj.audioVolume);
			}
		}
	}

	if (!pendingDeletes_.empty()) {
		ProcessPendingDeletes();
	}
}

void EditorSuite::StopPlayMode()
{
	// Stop all active runtime audio voices when exiting Play Mode
	Audio::GetInstance()->StopAllAudio();
	for (auto& obj : gameObjects_) {
		obj.runtimeVoiceHandle = 0;
	}

	RefreshAllScriptObjectViews();

	// 1. Call OnDestroy() and destroy all active script instances via GameScripts DLL
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		auto& obj = gameObjects_[i];
		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
				sc.instance->SetEngineAPI(&engineApi_);
				sc.instance->SetHost(this, this);
				sc.instance->OnDestroy();
				sc.instance.reset();
			}
		}
	}

	pendingDeletes_.clear();
	CollisionManager::GetInstance()->Clear();
	sceneState_ = EditorSceneState::Edit;
	stepSingleFrame_ = false;

	// 2. Restore scene snapshot (including any objects deleted during Play mode)
	RestorePlayModeSnapshot();

	// 3. Re-create Edit-mode script instances so Inspector can display exposed fields
	EnsureEditModeScriptInstances();
}

void EditorSuite::UpdateScripts(float deltaTime)
{
	if (sceneState_ == EditorSceneState::Edit && !IsStandaloneMode()) {
		return;
	}

	RefreshAllScriptObjectViews();

	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		auto& obj = gameObjects_[i];
		if (!obj.isActive || obj.scripts.empty()) continue;

		// Skip objects already queued for deferred deletion
		if (std::find(pendingDeletes_.begin(), pendingDeletes_.end(), static_cast<int>(obj.id)) != pendingDeletes_.end()) {
			continue;
		}

		for (size_t s = 0; s < obj.scripts.size(); ++s) {
			auto& sc = obj.scripts[s];
			if (sc.instance) {
				ScriptGameObjectView* view = GetOrUpdateScriptViewById(obj.id);
				Vector3 prevEuler = view ? view->rotation : obj.rotation;
				Quaternion prevQuat = view ? view->rotationQuat : obj.rotationQuat;

				sc.instance->SetOwner(view, obj.id);
				sc.instance->SetEngineAPI(&engineApi_);
				sc.instance->SetHost(this, this);
				sc.instance->Update(deltaTime);

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

void EditorSuite::SavePlayModeSnapshot()
{
	playModeSnapshots_.clear();
	playModeObjectSnapshots_.clear();
	playModeObjectSnapshots_.reserve(gameObjects_.size());

	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			CaptureScriptFieldsToSaved(sc);
		}

		TransformSnapshot s;
		s.id = obj.id;
		s.translation = obj.translation;
		s.rotation = obj.rotation;
		s.rotationQuat = obj.rotationQuat;
		s.scale = obj.scale;
		s.velocity = obj.velocity;
		s.isGrounded = obj.isGrounded;
		playModeSnapshots_.push_back(s);

		GameObject snapCopy = obj;
		for (auto& sc : snapCopy.scripts) {
			sc.instance.reset();
		}
		playModeObjectSnapshots_.push_back(std::move(snapCopy));
	}
	LogMessage(std::format("[Physics] Saved Play Mode snapshot for {} GameObjects.", playModeSnapshots_.size()), 0);
}

void EditorSuite::RestorePlayModeSnapshot()
{
	if (!playModeObjectSnapshots_.empty()) {
		bool structureChanged = (gameObjects_.size() != playModeObjectSnapshots_.size());
		if (!structureChanged) {
			for (size_t i = 0; i < gameObjects_.size(); ++i) {
				if (gameObjects_[i].id != playModeObjectSnapshots_[i].id) {
					structureChanged = true;
					break;
				}
			}
		}

		if (structureChanged) {
			if (dxCommon_ && !dxCommon_->IsDeviceLost()) {
				dxCommon_->SignalAndWaitFence();
			}

			std::vector<GameObject> restored;
			restored.reserve(playModeObjectSnapshots_.size());
			for (auto& snapObj : playModeObjectSnapshots_) {
				GameObject copyObj = snapObj;
				for (auto& sc : copyObj.scripts) {
					sc.instance.reset();
				}
				for (auto& existing : gameObjects_) {
					if (existing.id == copyObj.id) {
						copyObj.editObject3d = std::move(existing.editObject3d);
						copyObj.gameObject3d = std::move(existing.gameObject3d);
						copyObj.particleQuad = std::move(existing.particleQuad);
						break;
					}
				}
				if (copyObj.hasParticleSystem) {
					EnsureParticleQuadInitialized(copyObj);
				}
				restored.push_back(std::move(copyObj));
			}
			gameObjects_ = std::move(restored);
		} else {
			// Restore component lists (e.g. scripts modified during Play) while keeping GPU objects
			for (size_t i = 0; i < gameObjects_.size(); ++i) {
				gameObjects_[i].scripts = playModeObjectSnapshots_[i].scripts;
				for (auto& sc : gameObjects_[i].scripts) {
					sc.instance.reset();
				}
				gameObjects_[i].hasScript = playModeObjectSnapshots_[i].hasScript;
				gameObjects_[i].scriptClassName = playModeObjectSnapshots_[i].scriptClassName;
			}
		}
	}

	for (const auto& s : playModeSnapshots_) {
		for (auto& obj : gameObjects_) {
			if (obj.id == s.id) {
				obj.translation = s.translation;
				obj.rotation = s.rotation;
				obj.rotationQuat = s.rotationQuat;
				obj.scale = s.scale;
				obj.velocity = { 0.0f, 0.0f, 0.0f };
				obj.isGrounded = s.isGrounded;
				obj.isColliding = false;
				if (obj.hasParticleSystem && obj.particleQuad) {
					SyncGameObjectToParticleQuad(obj);
				}
				break;
			}
		}
	}

	// Rebuild hierarchy parent/child indices cleanly
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		gameObjects_[i].childrenIds.clear();
		gameObjects_[i].parentIndex = -1;
		gameObjects_[i].attachToIndex = -1;
	}
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (gameObjects_[i].parentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].parentId)) {
					gameObjects_[i].parentIndex = static_cast<int>(p);
					gameObjects_[p].childrenIds.push_back(gameObjects_[i].id);
					break;
				}
			}
		}
		if (gameObjects_[i].attachedParentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].attachedParentId)) {
					gameObjects_[i].attachToIndex = static_cast<int>(p);
					break;
				}
			}
		}
	}

	if (selectedObjectIndex_ >= static_cast<int>(gameObjects_.size())) {
		selectedObjectIndex_ = gameObjects_.empty() ? -1 : static_cast<int>(gameObjects_.size()) - 1;
	}

	LogMessage(std::format("[Physics] Restored initial transforms for {} GameObjects.", playModeSnapshots_.size()), 0);
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
		GameObject* found = self->FindGameObjectByName(name, warnIfMissing);
		return found ? self->GetOrUpdateScriptViewById(found->id) : nullptr;
	};
	engineApi_.findObjectById = [](void* ctx, uint32_t id, bool warnIfMissing) -> ScriptGameObjectView* {
		if (!ctx || id == 0) return nullptr;
		auto* self = static_cast<EditorSuite*>(ctx);
		GameObject* found = self->FindGameObjectById(id, warnIfMissing);
		return found ? self->GetOrUpdateScriptViewById(found->id) : nullptr;
	};
	engineApi_.getTransform = [](void* ctx, uint32_t id, ScriptTransformData* outTransform) -> bool {
		if (!ctx || id == 0 || !outTransform) return false;
		auto* self = static_cast<EditorSuite*>(ctx);
		GameObject* found = self->FindGameObjectById(id, false);
		if (!found) return false;
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
		if (!found) return false;
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
			static_cast<EditorSuite*>(ctx)->QueueDeleteGameObjectById(id);
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
	engineApi_.instantiate = [](void* ctx, const char* name, int primitiveType, Vector3 position) -> ScriptGameObjectView* {
		if (!ctx) return nullptr;
		auto* self = static_cast<EditorSuite*>(ctx);
		std::string objName = (name && name[0] != '\0') ? name : "InstantiatedObject";
		uint32_t createdId = 0;
		switch (primitiveType) {
		case 0: createdId = self->CreateCube(objName, position).id; break;
		case 1: createdId = self->CreateSphere(objName, position).id; break;
		case 2: createdId = self->CreatePlane(objName, position).id; break;
		case 3: createdId = self->CreateQuad(objName, position).id; break;
		default: {
			auto& emptyObj = self->CreateGameObject(objName, GameObjectType::Empty);
			emptyObj.translation = position;
			createdId = emptyObj.id;
			break;
		}
		}
		self->RefreshAllScriptObjectViews();
		return self->GetOrUpdateScriptViewById(createdId);
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
	scriptObjectViews_.resize(gameObjects_.size());
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		SyncGameObjectToView(gameObjects_[i], scriptObjectViews_[i]);
	}
}

ScriptGameObjectView* EditorSuite::GetOrUpdateScriptViewById(uint32_t id)
{
	if (scriptObjectViews_.size() != gameObjects_.size()) {
		RefreshAllScriptObjectViews();
	}
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (gameObjects_[i].id == id) {
			SyncGameObjectToView(gameObjects_[i], scriptObjectViews_[i]);
			return &scriptObjectViews_[i];
		}
	}
	return nullptr;
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

std::filesystem::path EditorSuite::ResolveGameScriptsDir() const
{
	std::error_code ec;
	std::filesystem::path dir = ScriptModule::ResolveWorkspaceRoot() / ScriptModule::kGameScriptsDirName;
	if (std::filesystem::exists(dir, ec)) {
		return std::filesystem::weakly_canonical(dir, ec);
	}
	return dir;
}

std::filesystem::path EditorSuite::ResolveGameScriptsVcxproj() const
{
	std::error_code ec;
	std::filesystem::path vcxproj = ScriptModule::ResolveWorkspaceRoot() / ScriptModule::kGameScriptsVcxprojSubpath;
	if (std::filesystem::exists(vcxproj, ec)) {
		return std::filesystem::weakly_canonical(vcxproj, ec);
	}
	return vcxproj;
}

std::filesystem::path EditorSuite::ResolveSolutionFilePath() const
{
	std::error_code ec;
	std::filesystem::path vcxproj = ResolveGameScriptsVcxproj();
	std::filesystem::path rootDir = vcxproj.parent_path().parent_path();
	if (std::filesystem::exists(rootDir, ec)) {
		for (const auto& entry : std::filesystem::directory_iterator(rootDir, ec)) {
			if (entry.is_regular_file()) {
				auto ext = entry.path().extension().string();
				if (ext == ".slnx" || ext == ".sln") {
					return std::filesystem::canonical(entry.path(), ec);
				}
			}
		}
	}
	return {};
}

std::string EditorSuite::ResolveMsBuildExe()
{
	if (!resolvedMsBuildPath_.empty() && std::filesystem::exists(resolvedMsBuildPath_)) {
		return resolvedMsBuildPath_;
	}

	std::error_code ec;
	// 1. Try vswhere.exe
	std::filesystem::path vswhere = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
	if (std::filesystem::exists(vswhere, ec)) {
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		HANDLE rPipe = NULL, wPipe = NULL;
		if (CreatePipe(&rPipe, &wPipe, &sa, 0)) {
			SetHandleInformation(rPipe, HANDLE_FLAG_INHERIT, 0);
			STARTUPINFOW si{};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			si.hStdOutput = wPipe;
			si.hStdError = wPipe;
			PROCESS_INFORMATION pi{};
			std::wstring cmd = L"\"" + vswhere.wstring() + L"\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe";
			std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
			cmdBuf.push_back(L'\0');
			if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
				CloseHandle(wPipe);
				wPipe = NULL;
				std::string output;
				char buf[512];
				DWORD bytesRead = 0;
				while (ReadFile(rPipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
					buf[bytesRead] = '\0';
					output.append(buf, bytesRead);
				}
				WaitForSingleObject(pi.hProcess, 3000);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				std::istringstream iss(output);
				std::string line;
				while (std::getline(iss, line)) {
					while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
						line.pop_back();
					}
					if (!line.empty() && std::filesystem::exists(line, ec)) {
						resolvedMsBuildPath_ = line;
						CloseHandle(rPipe);
						return resolvedMsBuildPath_;
					}
				}
			}
			if (wPipe) CloseHandle(wPipe);
			CloseHandle(rPipe);
		}
	}

	// 2. Check common Visual Studio 18 / 2022 paths and fallback buffer
	std::vector<std::string> fallbacks = {
		msbuildFallbackPathBuf_,
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe"
	};

	for (const auto& candidate : fallbacks) {
		if (!candidate.empty() && std::filesystem::exists(candidate, ec)) {
			resolvedMsBuildPath_ = candidate;
			return resolvedMsBuildPath_;
		}
	}

	return msbuildFallbackPathBuf_;
}

void EditorSuite::TriggerScriptBuild()
{
	if (isBuildingScripts_.load()) {
		queuedScriptRebuild_.store(true);
		LogMessage("[Scripts] Build already in progress; queued another rebuild when it finishes.", 0);
		return;
	}

	if (scriptBuildThread_.joinable()) {
		scriptBuildThread_.join();
	}

	std::string msbuildExe = ResolveMsBuildExe();
	std::filesystem::path vcxprojPath = ResolveGameScriptsVcxproj();
	std::error_code ec;
	if (!std::filesystem::exists(msbuildExe, ec)) {
		LogMessage(std::format("[Scripts] Error: MSBuild.exe not found at '{}'.", msbuildExe), 2);
		lastScriptBuildStatus_.store(2);
		return;
	}
	if (!std::filesystem::exists(vcxprojPath, ec)) {
		LogMessage(std::format("[Scripts] Error: GameScripts.vcxproj not found at '{}'.", vcxprojPath.string()), 2);
		lastScriptBuildStatus_.store(2);
		return;
	}

	isBuildingScripts_.store(true);
	queuedScriptRebuild_.store(false);
	LogMessage("[Scripts] Building GameScripts.vcxproj (Development|x64)...", 0);

	scriptBuildThread_ = std::thread([this, msbuildExe, vcxprojPath]() {
		bool runAgain = true;
		while (runAgain) {
			runAgain = false;
			auto startTime = std::chrono::steady_clock::now();

			SECURITY_ATTRIBUTES sa{};
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			HANDLE rPipe = NULL, wPipe = NULL;
			if (!CreatePipe(&rPipe, &wPipe, &sa, 0)) {
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[Scripts] Build failed: CreatePipe error.", 2 });
				lastScriptBuildStatus_.store(2);
				break;
			}
			SetHandleInformation(rPipe, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFOW si{};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			si.hStdOutput = wPipe;
			si.hStdError = wPipe;

			PROCESS_INFORMATION pi{};
			std::wstring cmdLine = L"\"" + std::filesystem::path(msbuildExe).wstring() + L"\" \""
				+ vcxprojPath.wstring() + L"\" /t:Build /p:Configuration=Development /p:Platform=x64 /v:minimal /nologo";
			std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
			cmdBuf.push_back(L'\0');

			std::wstring workDir = vcxprojPath.parent_path().wstring();
			BOOL started = CreateProcessW(
				NULL,
				cmdBuf.data(),
				NULL,
				NULL,
				TRUE,
				CREATE_NO_WINDOW,
				NULL,
				workDir.c_str(),
				&si,
				&pi);

			CloseHandle(wPipe);
			wPipe = NULL;

			if (!started) {
				CloseHandle(rPipe);
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[Scripts] Build failed: Could not start MSBuild.exe process.", 2 });
				lastScriptBuildStatus_.store(2);
				break;
			}

			std::string pendingText;
			char buffer[1024];
			DWORD bytesRead = 0;
			auto pushOutputLine = [this](std::string line) {
				while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
					line.pop_back();
				}
				if (line.empty()) return;
				int logType = 0;
				std::string lower = line;
				std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (lower.find(": error ") != std::string::npos || lower.find("fatal error") != std::string::npos) {
					logType = 2;
				} else if (lower.find(": warning ") != std::string::npos) {
					logType = 1;
				}
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[MSBuild] " + line, logType });
			};

			while (ReadFile(rPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
				buffer[bytesRead] = '\0';
				pendingText.append(buffer, bytesRead);
				size_t newlinePos = 0;
				while ((newlinePos = pendingText.find('\n')) != std::string::npos) {
					std::string line = pendingText.substr(0, newlinePos);
					pendingText.erase(0, newlinePos + 1);
					pushOutputLine(std::move(line));
				}
			}
			if (!pendingText.empty()) {
				pushOutputLine(std::move(pendingText));
			}

			WaitForSingleObject(pi.hProcess, INFINITE);
			DWORD exitCode = 1;
			GetExitCodeProcess(pi.hProcess, &exitCode);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			CloseHandle(rPipe);

			auto endTime = std::chrono::steady_clock::now();
			double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

			{
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				if (exitCode == 0) {
					pendingBuildLogs_.push_back({ std::format("[Scripts] Build succeeded ({:.2f}s)", elapsedSec), 0 });
					lastScriptBuildStatus_.store(1);
				} else {
					pendingBuildLogs_.push_back({ std::format("[Scripts] Build failed (exit code {})", exitCode), 2 });
					lastScriptBuildStatus_.store(2);
				}
			}

			if (queuedScriptRebuild_.exchange(false)) {
				runAgain = true;
			}
		}

		isBuildingScripts_.store(false);
	});
}

void EditorSuite::DrainScriptBuildLogs()
{
	std::vector<PendingBuildLog> logs;
	{
		std::lock_guard<std::mutex> lock(buildLogMutex_);
		if (pendingBuildLogs_.empty()) return;
		logs.swap(pendingBuildLogs_);
	}

	bool hadError = false;
	for (const auto& entry : logs) {
		LogMessage(entry.line, entry.type);
		if (entry.type == 2) {
			hadError = true;
		}
	}
	if (hadError) {
		bottomPanelTab_ = 1; // Focus Console tab on build error
	}
}

void EditorSuite::PollScriptSourceWatcher(float deltaTime)
{
	scriptSourcePollTimer_ += deltaTime;
	if (scriptSourcePollTimer_ >= 0.25f) {
		scriptSourcePollTimer_ = 0.0f;

		std::filesystem::path scriptsDir = ResolveGameScriptsDir() / "Scripts";
		std::error_code ec;
		if (std::filesystem::exists(scriptsDir, ec) && std::filesystem::is_directory(scriptsDir, ec)) {
			std::map<std::string, std::filesystem::file_time_type> currentStamps;
			for (const auto& entry : std::filesystem::directory_iterator(scriptsDir, ec)) {
				if (!entry.is_regular_file(ec)) continue;
				std::string ext = entry.path().extension().string();
				if (ext == ".cpp" || ext == ".h") {
					currentStamps[entry.path().filename().string()] = std::filesystem::last_write_time(entry.path(), ec);
				}
			}

			if (!scriptSourceWatcherInitialized_) {
				scriptSourceTimestamps_ = std::move(currentStamps);
				scriptSourceWatcherInitialized_ = true;
			} else if (currentStamps != scriptSourceTimestamps_) {
				scriptSourceTimestamps_ = std::move(currentStamps);
				if (autoBuildScripts_) {
					scriptSourceChangePending_ = true;
					scriptSourceDebounceTimer_ = 0.0f;
				}
			}
		}
	}

	if (scriptSourceChangePending_ && autoBuildScripts_) {
		scriptSourceDebounceTimer_ += deltaTime;
		if (scriptSourceDebounceTimer_ >= 1.0f) {
			scriptSourceChangePending_ = false;
			scriptSourceDebounceTimer_ = 0.0f;
			LogMessage("[Scripts] Detected source changes in GameScripts/Scripts; triggering Auto Build...", 0);
			TriggerScriptBuild();
		}
	}
}

void EditorSuite::OpenFileInVisualStudio(const std::filesystem::path& filePath)
{
	std::error_code ec;
	if (!std::filesystem::exists(filePath, ec)) {
		LogMessage(std::format("[Scripts] Cannot open file (does not exist): {}", filePath.string()), 1);
		return;
	}

	std::filesystem::path absFile = std::filesystem::canonical(filePath, ec);
	if (ec) absFile = std::filesystem::absolute(filePath, ec);

	std::thread([this, absFile]() {
		auto logAsync = [this](const std::string& text, int type = 0) {
			std::lock_guard<std::mutex> lock(buildLogMutex_);
			pendingBuildLogs_.push_back({ text, type });
		};

		// 1. Try attaching to a running Visual Studio DTE instance via Running Object Table (ROT)
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		bool comInitialized = SUCCEEDED(hr) || hr == S_FALSE;
		bool openedViaDte = false;

		if (comInitialized) {
			{
				Microsoft::WRL::ComPtr<IRunningObjectTable> pRot = nullptr;
				if (SUCCEEDED(GetRunningObjectTable(0, &pRot)) && pRot) {
					Microsoft::WRL::ComPtr<IEnumMoniker> pEnum = nullptr;
					if (SUCCEEDED(pRot->EnumRunning(&pEnum)) && pEnum) {
						Microsoft::WRL::ComPtr<IMoniker> pMoniker = nullptr;
						ULONG fetched = 0;
						while (!openedViaDte && pEnum->Next(1, &pMoniker, &fetched) == S_OK) {
							Microsoft::WRL::ComPtr<IBindCtx> pBindCtx = nullptr;
							if (SUCCEEDED(CreateBindCtx(0, &pBindCtx)) && pBindCtx) {
								LPOLESTR displayName = nullptr;
								if (SUCCEEDED(pMoniker->GetDisplayName(pBindCtx.Get(), nullptr, &displayName)) && displayName) {
									std::wstring disp(displayName);
									CoTaskMemFree(displayName);
									if (disp.find(L"!VisualStudio.DTE") == 0) {
										Microsoft::WRL::ComPtr<IUnknown> pUnk = nullptr;
										if (SUCCEEDED(pRot->GetObject(pMoniker.Get(), &pUnk)) && pUnk) {
											Microsoft::WRL::ComPtr<IDispatch> pDte = nullptr;
											if (SUCCEEDED(pUnk.As(&pDte)) && pDte) {
												// Get ItemOperations property on DTE
												OLECHAR itemOpsName[] = L"ItemOperations";
												LPOLESTR names[1] = { itemOpsName };
												DISPID dispidItemOps = DISPID_UNKNOWN;
												if (SUCCEEDED(pDte->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &dispidItemOps))) {
													DISPPARAMS noArgs = { nullptr, nullptr, 0, 0 };
													VARIANT vItemOps;
													VariantInit(&vItemOps);
													if (SUCCEEDED(pDte->Invoke(dispidItemOps, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &vItemOps, nullptr, nullptr))
														&& vItemOps.vt == VT_DISPATCH && vItemOps.pdispVal) {
														IDispatch* pItemOps = vItemOps.pdispVal;
														OLECHAR openFileName[] = L"OpenFile";
														LPOLESTR openNames[1] = { openFileName };
														DISPID dispidOpenFile = DISPID_UNKNOWN;
														if (SUCCEEDED(pItemOps->GetIDsOfNames(IID_NULL, openNames, 1, LOCALE_USER_DEFAULT, &dispidOpenFile))) {
															BSTR bstrFile = SysAllocString(absFile.wstring().c_str());
															VARIANT vArg;
															VariantInit(&vArg);
															vArg.vt = VT_BSTR;
															vArg.bstrVal = bstrFile;
															DISPPARAMS openParams = { &vArg, nullptr, 1, 0 };
															VARIANT vRes;
															VariantInit(&vRes);
															if (SUCCEEDED(pItemOps->Invoke(dispidOpenFile, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &openParams, &vRes, nullptr, nullptr))) {
																openedViaDte = true;
															}
															VariantClear(&vRes);
															SysFreeString(bstrFile);
														}

														// Also activate MainWindow if available
														OLECHAR mainWinName[] = L"MainWindow";
														LPOLESTR mwNames[1] = { mainWinName };
														DISPID dispidMw = DISPID_UNKNOWN;
														if (SUCCEEDED(pDte->GetIDsOfNames(IID_NULL, mwNames, 1, LOCALE_USER_DEFAULT, &dispidMw))) {
															VARIANT vMw;
															VariantInit(&vMw);
															if (SUCCEEDED(pDte->Invoke(dispidMw, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &vMw, nullptr, nullptr))
																&& vMw.vt == VT_DISPATCH && vMw.pdispVal) {
																OLECHAR actName[] = L"Activate";
																LPOLESTR actNames[1] = { actName };
																DISPID dispidAct = DISPID_UNKNOWN;
																if (SUCCEEDED(vMw.pdispVal->GetIDsOfNames(IID_NULL, actNames, 1, LOCALE_USER_DEFAULT, &dispidAct))) {
																	VARIANT vDummy;
																	VariantInit(&vDummy);
																	vMw.pdispVal->Invoke(dispidAct, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &noArgs, &vDummy, nullptr, nullptr);
																	VariantClear(&vDummy);
																}
															}
															VariantClear(&vMw);
														}
													}
													VariantClear(&vItemOps);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			CoUninitialize();
		}

		if (openedViaDte) {
			logAsync(std::format("[Scripts] Opened '{}' in running Visual Studio instance via DTE.", absFile.filename().string()), 0);
			return;
		}

		// 2. Fallback: Locate devenv.exe and launch with /edit "<filePath>"
		std::error_code localEc;
		std::vector<std::filesystem::path> devenvCandidates = {
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\devenv.exe"
		};

		for (const auto& devenv : devenvCandidates) {
			if (std::filesystem::exists(devenv, localEc)) {
				std::wstring cmd = L"\"" + devenv.wstring() + L"\" /edit \"" + absFile.wstring() + L"\"";
				std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
				cmdBuf.push_back(L'\0');
				STARTUPINFOW si{};
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi{};
				if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					logAsync(std::format("[Scripts] Opened '{}' via devenv.exe /edit.", absFile.filename().string()), 0);
					return;
				}
			}
		}

		// 3. Final fallback: ShellExecuteW default handler
		HINSTANCE res = ShellExecuteW(NULL, L"open", absFile.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
		if (reinterpret_cast<INT_PTR>(res) > 32) {
			logAsync(std::format("[Scripts] Opened '{}' with default system association (ShellExecuteW).", absFile.filename().string()), 0);
		} else {
			logAsync(std::format("[Scripts] Warning: Failed to open '{}' in Visual Studio or default editor.", absFile.string()), 1);
		}
	}).detach();
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

void EditorSuite::EnsureParticleQuadInitialized(GameObject& obj) {
	if (!obj.hasParticleSystem) return;
	if (!obj.particleQuad) {
		obj.particleQuad = std::make_shared<ParticleQuad>();
		obj.particleQuad->Initialize(dxCommon_, &dxc_, particleTextureHandle_, static_cast<uint32_t>(obj.particleMaxParticles));
	}
	SyncGameObjectToParticleQuad(obj);
}

void EditorSuite::SyncGameObjectToParticleQuad(GameObject& obj) {
	if (!obj.hasParticleSystem || !obj.particleQuad) return;
	auto& p = *obj.particleQuad;
	p.emitter.shape = obj.particleShape;
	p.emitter.spawnRadius = obj.particleSpawnRadius;
	p.emitter.boxExtents = obj.particleBoxExtents;
	p.emitter.coneAngle = obj.particleConeAngle;
	p.emitter.spawnRate = obj.particleSpawnRate;
	p.SetMaxParticles(static_cast<uint32_t>(obj.particleMaxParticles));
	p.emitter.direction = obj.particleDirection;
	p.emitter.spreadAngle = obj.particleSpreadAngle;
	p.emitter.speedMin = obj.particleSpeedMin;
	p.emitter.speedMax = obj.particleSpeedMax;
	p.emitter.gravity = obj.particleGravity;
	p.emitter.drag = obj.particleDrag;
	p.windField.enabled = obj.particleWindEnabled;
	p.windField.acceleration = obj.particleWindAcceleration;
	p.emitter.lifetimeMin = obj.particleLifetimeMin;
	p.emitter.lifetimeMax = obj.particleLifetimeMax;
	p.emitter.startScale = obj.particleStartScale;
	p.emitter.endScale = obj.particleEndScale;
	p.emitter.startColor = obj.particleStartColor;
	p.emitter.endColor = obj.particleEndColor;
	p.blendType = (obj.particleBlendMode == 1) ? ParticleQuad::BlendType::Add : ParticleQuad::BlendType::Normal;
	p.emitter.billboardMode = obj.particleBillboardMode;
	p.emitter.translate = obj.translation;
}

void EditorSuite::ApplyParticlePreset(GameObject& obj, int presetIndex) {
	if (presetIndex == 1) { // Smoke
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.3f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 20.0f;
		obj.particleSpeedMin = 0.5f;
		obj.particleSpeedMax = 1.5f;
		obj.particleGravity = 0.3f; // upward drift
		obj.particleDrag = 0.05f;
		obj.particleLifetimeMin = 2.0f;
		obj.particleLifetimeMax = 3.5f;
		obj.particleStartScale = 0.2f;
		obj.particleEndScale = 0.8f; // scale grows over lifetime
		obj.particleStartColor = { 0.8f, 0.8f, 0.8f, 0.6f };
		obj.particleEndColor = { 0.4f, 0.4f, 0.4f, 0.0f }; // alpha fades out
		obj.particleBlendMode = 0; // Normal Alpha
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 15.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 2) { // Water Fountain
		obj.particleShape = 3; // Cone
		obj.particleConeAngle = 15.0f;
		obj.particleSpawnRadius = 0.2f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 15.0f; // upward cone with spread
		obj.particleSpeedMin = 5.0f;
		obj.particleSpeedMax = 8.0f;
		obj.particleGravity = -9.8f; // gravity Y = -9.8
		obj.particleDrag = 0.02f;
		obj.particleLifetimeMin = 1.5f;
		obj.particleLifetimeMax = 2.2f;
		obj.particleStartScale = 0.2f;
		obj.particleEndScale = 0.15f;
		obj.particleStartColor = { 0.3f, 0.8f, 1.0f, 0.85f };
		obj.particleEndColor = { 0.85f, 0.95f, 1.0f, 0.1f };
		obj.particleBlendMode = 0; // Normal Alpha
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 25.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 3) { // Fireworks
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.1f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 180.0f; // 360-degree spherical velocity
		obj.particleSpeedMin = 4.0f;
		obj.particleSpeedMax = 9.0f;
		obj.particleGravity = -2.0f;
		obj.particleDrag = 1.2f; // drag deceleration
		obj.particleLifetimeMin = 1.5f;
		obj.particleLifetimeMax = 2.5f;
		obj.particleStartScale = 0.25f;
		obj.particleEndScale = 0.05f;
		obj.particleStartColor = { 1.0f, 0.9f, 0.2f, 1.0f };
		obj.particleEndColor = { 1.0f, 0.1f, 0.5f, 0.0f };
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 2; // Velocity Aligned
		obj.particleSpawnRate = 30.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 4) { // Explosion
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.4f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 180.0f; // fast radial burst
		obj.particleSpeedMin = 6.0f;
		obj.particleSpeedMax = 14.0f;
		obj.particleGravity = 0.0f;
		obj.particleDrag = 2.5f;
		obj.particleLifetimeMin = 0.8f;
		obj.particleLifetimeMax = 1.5f;
		obj.particleStartScale = 0.3f;
		obj.particleEndScale = 1.8f; // rapid expansion
		obj.particleStartColor = { 1.0f, 0.7f, 0.1f, 1.0f };
		obj.particleEndColor = { 0.2f, 0.2f, 0.2f, 0.0f }; // orange/yellow/red turning to dark gray
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 40.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 5) { // Torch / Campfire
		obj.particleShape = 3; // Cone
		obj.particleConeAngle = 20.0f;
		obj.particleSpawnRadius = 0.15f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 15.0f;
		obj.particleSpeedMin = 1.2f;
		obj.particleSpeedMax = 2.8f;
		obj.particleGravity = 0.6f; // continuous upward flame
		obj.particleDrag = 0.05f;
		obj.particleLifetimeMin = 0.8f;
		obj.particleLifetimeMax = 1.6f;
		obj.particleStartScale = 0.35f;
		obj.particleEndScale = 0.08f; // shrinking scale
		obj.particleStartColor = { 1.0f, 0.95f, 0.2f, 1.0f };
		obj.particleEndColor = { 0.9f, 0.15f, 0.0f, 0.0f }; // yellow -> orange -> red
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 20.0f;
		obj.particleWindEnabled = false;
	}
	EnsureParticleQuadInitialized(obj);
	if (obj.particleQuad) {
		obj.particleQuad->ClearParticles();
		obj.particleQuad->EmitBurst(15);
	}
}

void EditorSuite::RefreshAvailableCubemaps()
{
	availableCubemaps_.clear();
	std::vector<std::string> searchDirs = { "Resources", "Resources/Skybox", "Assets" };
	for (const auto& dir : searchDirs) {
		if (std::filesystem::exists(dir)) {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".dds") {
					availableCubemaps_.push_back(entry.path().string());
				}
			}
		}
	}
	if (availableCubemaps_.empty()) {
		availableCubemaps_.push_back("Resources/skybox.dds");
	}
}

void EditorSuite::DrawSkyboxSettingsUI()
{
#ifdef USE_IMGUI
	ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Skybox / Environment Settings");
	ImGui::Separator();

	const char* modeNames[] = { "Gradient", "Solid Color", "Cubemap" };
	int currentMode = static_cast<int>(skybox_.GetMode());
	if (ImGui::Combo("Background Mode", &currentMode, modeNames, 3)) {
		skybox_.SetMode(static_cast<SkyboxMode>(currentMode));
		if (skybox_.GetMode() == SkyboxMode::Cubemap) {
			if (!skybox_.LoadCubemap(skybox_.GetCubemapPath())) {
				LogMessage("[Skybox] Failed to load cubemap: " + skybox_.GetCubemapPath() + ". Falling back to Gradient mode.", 1);
				skybox_.SetMode(SkyboxMode::Gradient);
			}
		}
		isSceneDirty_ = true;
		PushUndoSnapshot();
	}
	ImGui::SetItemTooltip("Select sky background rendering mode: Gradient, Solid Color, or Cubemap");

	ImGui::Spacing();

	if (skybox_.GetMode() == SkyboxMode::Gradient) {
		Vector4 topCol = skybox_.GetTopColor();
		float topArr[3] = { topCol.x, topCol.y, topCol.z };
		if (ImGui::ColorEdit3("Top Color", topArr)) {
			skybox_.SetTopColor({ topArr[0], topArr[1], topArr[2], topCol.w });
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Sky apex / zenith gradient color");

		Vector4 horizCol = skybox_.GetHorizonColor();
		float horizArr[3] = { horizCol.x, horizCol.y, horizCol.z };
		if (ImGui::ColorEdit3("Horizon Color", horizArr)) {
			skybox_.SetHorizonColor({ horizArr[0], horizArr[1], horizArr[2], horizCol.w });
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Sky horizon midpoint gradient color");

		Vector4 botCol = skybox_.GetBottomColor();
		float botArr[3] = { botCol.x, botCol.y, botCol.z };
		if (ImGui::ColorEdit3("Ground Color", botArr)) {
			skybox_.SetBottomColor({ botArr[0], botArr[1], botArr[2], botCol.w });
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Nadir / ground hemisphere gradient color");
	} else if (skybox_.GetMode() == SkyboxMode::SolidColor) {
		Vector4 solidCol = skybox_.GetSolidColor();
		float solidArr[3] = { solidCol.x, solidCol.y, solidCol.z };
		if (ImGui::ColorEdit3("Solid Color", solidArr)) {
			skybox_.SetSolidColor({ solidArr[0], solidArr[1], solidArr[2], solidCol.w });
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Uniform solid clear color for the viewport background");
	} else if (skybox_.GetMode() == SkyboxMode::Cubemap) {
		if (availableCubemaps_.empty()) {
			RefreshAvailableCubemaps();
		}

		int currentIdx = -1;
		std::string currentPath = skybox_.GetCubemapPath();
		for (int i = 0; i < static_cast<int>(availableCubemaps_.size()); ++i) {
			if (availableCubemaps_[i] == currentPath) {
				currentIdx = i;
				break;
			}
		}

		const char* previewText = (currentIdx >= 0) ? availableCubemaps_[currentIdx].c_str() : "(Select Cubemap...)";
		if (ImGui::BeginCombo("Cubemap Asset", previewText)) {
			for (int i = 0; i < static_cast<int>(availableCubemaps_.size()); ++i) {
				bool isSelected = (currentIdx == i);
				if (ImGui::Selectable(availableCubemaps_[i].c_str(), isSelected)) {
					currentPath = availableCubemaps_[i];
					skybox_.SetCubemapPath(currentPath);
					if (!skybox_.LoadCubemap(currentPath)) {
						LogMessage("[Skybox] Failed to load cubemap: " + currentPath + ". Falling back to Gradient mode.", 1);
						skybox_.SetMode(SkyboxMode::Gradient);
					}
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SetItemTooltip("Choose a cubemap DDS file detected in Resources or Assets");

		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			RefreshAvailableCubemaps();
		}
		ImGui::SetItemTooltip("Rescan folders for .dds cubemap textures");

		char pathBuf[256];
		strncpy_s(pathBuf, currentPath.c_str(), sizeof(pathBuf) - 1);
		if (ImGui::InputText("File Path", pathBuf, sizeof(pathBuf))) {
			skybox_.SetCubemapPath(pathBuf);
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			if (!skybox_.LoadCubemap(pathBuf)) {
				LogMessage("[Skybox] Failed to load cubemap: " + std::string(pathBuf) + ". Falling back to Gradient mode.", 1);
				skybox_.SetMode(SkyboxMode::Gradient);
			}
			PushUndoSnapshot();
		}
		ImGui::SetItemTooltip("Manual file path to DDS cubemap (press Enter to apply)");

		float rot = skybox_.GetRotation();
		if (ImGui::SliderFloat("Rotation (Y)", &rot, 0.0f, 360.0f, "%.1f deg")) {
			skybox_.SetRotation(rot);
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Rotate cubemap around Y axis (in degrees)");

		float exp = skybox_.GetExposure();
		if (ImGui::SliderFloat("Exposure", &exp, 0.1f, 5.0f, "%.2f")) {
			skybox_.SetExposure(exp);
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
		ImGui::SetItemTooltip("Cubemap brightness and exposure multiplier");

		ImGui::Spacing();
		if (skybox_.HasValidCubemap()) {
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "Status: Cubemap Active & Loaded");
		} else {
			ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f), "Status: Fallback Gradient Active");
		}
	}
#endif
}

