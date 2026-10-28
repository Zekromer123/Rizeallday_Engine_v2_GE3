// Headless self-tests, run with `RizealldaySandbox.exe --run-tests [filter]` (editor builds only).
// They run inside the real editor (real D3D12 device, real EditorSuite) right after initialisation.
// Rules: every file a test writes lives under <temp>/RizealldayTests; the user's project, scenes and
// recent-project list are never touched (EditorSuite::isRunningTests_ blocks AddRecentProject).
#ifdef USE_IMGUI

#include "EditorSuiteInternal.h"
#include "SceneJson.h"
#include "Tests/TestFramework.h"
#include "base/DirectXCommon.h"
#include <random>
#include <set>

/// Friend of EditorSuite: the only door the tests use to reach private state.
struct EditorSuiteTestAccess {
	using Kind = EditorSuite::ComponentKind;

	static std::vector<GameObject>& Objects(EditorSuite& s) { return s.gameObjects_; }
	static EditorSceneState& State(EditorSuite& s) { return s.sceneState_; }
	static EngineAPI& Api(EditorSuite& s) { return s.engineApi_; }
	static double& GameTime(EditorSuite& s) { return s.gameTime_; }
	static std::string& ScenePath(EditorSuite& s) { return s.currentScenePath_; }
	static std::string& SceneName(EditorSuite& s) { return s.currentSceneName_; }
	static bool ScriptsLoaded(EditorSuite& s) { return s.scriptModule_.IsLoaded() && s.scriptModule_.IsRegistered("SurvivorsGameManager"); }
	static bool& MergeAllowed(EditorSuite& s) { return s.undoMergeAllowed_; }
	static size_t UndoCount(const EditorSuite& s) { return s.undoCommands_.size(); }
	static size_t RedoCount(const EditorSuite& s) { return s.redoCommands_.size(); }
	static DirectXCommon* Dx(EditorSuite& s) { return s.dxCommon_; }
	static LightGroup* Lights(EditorSuite& s) { return s.lights_; }
	static Skybox& Sky(EditorSuite& s) { return s.skybox_; }
	static std::filesystem::path& ProjectRoot(EditorSuite& s) { return s.currentProjectRoot_; }
	static std::filesystem::path& CurrentDirectory(EditorSuite& s) { return s.currentDirectory_; }
	static ScriptGameObjectView* ViewFor(EditorSuite& s, uint32_t id)
	{
		auto it = s.scriptObjectViews_.find(id);
		return (it != s.scriptObjectViews_.end()) ? it->second.get() : nullptr;
	}
	static void RemoveComponent(EditorSuite& s, uint32_t objectId, Kind kind)
	{
		if (GameObject* obj = s.FindGameObjectById(objectId, false)) s.RemoveComponent(*obj, kind);
	}
	static std::filesystem::path TempRoot() { return std::filesystem::temp_directory_path() / "RizealldayTests"; }
	static void Run(EditorSuite& s, bool running) { s.isRunningTests_ = running; }
	static void Log(EditorSuite& s, const std::string& text, int type) { s.LogMessage(text, type); }
	static void PrepareForTest(EditorSuite& s)
	{
		s.sceneState_ = EditorSceneState::Edit; // a failed test must not leave Play mode behind
		s.NewScene();
		s.ResetUndoHistory();
	}
};

namespace {

using A = EditorSuiteTestAccess;

std::string Fmt(const Vector3& v)
{
	return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

// ===========================================================================
// JSON
// ===========================================================================

} // namespace

TEST_CASE(Json, RoundTripIdempotent)
{
	const size_t baseCount = A::Objects(s).size();
	const uint32_t cubeId = s.CreateCube("RoundTripCube", { 1.0f, 2.0f, 3.0f }).id;
	const uint32_t sphereId = s.CreateSphere("RoundTripSphere", { -4.0f, 0.5f, 2.0f }).id;
	GameObject& child = s.CreateGameObject("RoundTripChild", GameObjectType::Empty);
	const uint32_t childId = child.id;
	child.parentId = static_cast<int>(cubeId);
	child.translation = { 0.25f, 0.5f, 0.75f };
	s.RebuildHierarchyLinks();
	s.UpdateWorldMatrices();

	const std::string first = s.SerializeSceneToString();
	REQUIRE(s.DeserializeSceneFromString(first));
	const std::string second = s.SerializeSceneToString();
	CHECK(first == second);

	CHECK(A::Objects(s).size() == baseCount + 3);
	CHECK(s.FindGameObjectById(cubeId, false) != nullptr);
	CHECK(s.FindGameObjectById(sphereId, false) != nullptr);
	const GameObject* reloadedChild = s.FindGameObjectById(childId, false);
	REQUIRE(reloadedChild != nullptr);
	CHECK(reloadedChild->parentId == static_cast<int>(cubeId));
	CHECK(reloadedChild->parentIndex >= 0);
	const GameObject* reloadedCube = s.FindGameObjectById(cubeId, false);
	REQUIRE(reloadedCube != nullptr);
	CHECK(reloadedCube->hasMesh);
	CHECK(std::find(reloadedCube->childrenIds.begin(), reloadedCube->childrenIds.end(), childId) != reloadedCube->childrenIds.end());
}

TEST_CASE(Json, LegacyBackslashRepair)
{
	// Raw string: the JSON text really contains single backslashes (an invalid JSON escape, as in old scene files)
	const std::string legacy =
		R"({"sceneName":"LegacyScene","gameObjects":[)"
		R"({"id":1,"name":"Parent","type":0},)"
		R"({"id":2,"name":"Child","type":0,"parentId":1,"modelPath":".\Resources\Model\Idle.fbx"}]})";
	REQUIRE(s.DeserializeSceneFromString(legacy));
	CHECK(A::Objects(s).size() == 2);

	const GameObject* parent = s.FindGameObjectById(1, false);
	const GameObject* child = s.FindGameObjectById(2, false);
	REQUIRE(parent != nullptr);
	REQUIRE(child != nullptr);
	CHECK_MSG(child->modelPath == "./Resources/Model/Idle.fbx", child->modelPath);
	CHECK(child->parentId == 1);
	CHECK(child->parentIndex >= 0);
	CHECK(std::find(parent->childrenIds.begin(), parent->childrenIds.end(), 2u) != parent->childrenIds.end());
}

TEST_CASE(Json, InvalidInputKeepsScene)
{
	const size_t count = A::Objects(s).size();
	const std::string before = s.SerializeSceneToString();
	CHECK(!s.DeserializeSceneFromString("{ broken"));
	CHECK(!s.DeserializeSceneFromString(""));
	CHECK(!s.DeserializeSceneFromString("[1, 2, 3]"));
	CHECK(A::Objects(s).size() == count);
	CHECK(s.SerializeSceneToString() == before);
}

// ===========================================================================
// Undo / Redo
// ===========================================================================

TEST_CASE(Undo, UndoRedoSequence)
{
	std::vector<std::string> states;
	states.push_back(s.SerializeSceneToString()); // state 0: fresh scene

	// Records one undo command and its resulting scene string
	auto commit = [&]() {
		const size_t before = A::UndoCount(s);
		A::MergeAllowed(s) = false;
		s.PushUndoSnapshot();
		CHECK(A::UndoCount(s) == before + 1);
		states.push_back(s.SerializeSceneToString());
	};

	// 1. modify an object
	A::Objects(s)[0].translation.x += 5.0f;
	commit();

	// 2. create an object
	const uint32_t cubeId = s.CreateCube("UndoCube", { 1.0f, 2.0f, 3.0f }).id;
	commit();

	// 3. create a child of it
	GameObject& childRef = s.CreateSphere("UndoChild", { 0.0f, 1.0f, 0.0f });
	const uint32_t childId = childRef.id;
	childRef.parentId = static_cast<int>(cubeId);
	s.RebuildHierarchyLinks();
	s.UpdateWorldMatrices();
	commit();

	// 4. delete the child
	s.QueueDeleteGameObjectById(childId);
	s.ProcessPendingDeletes();
	CHECK(s.FindGameObjectById(childId, false) == nullptr);
	A::MergeAllowed(s) = false;
	s.PushUndoSnapshot(); // no-op when ProcessPendingDeletes already recorded the deletion
	states.push_back(s.SerializeSceneToString());
	CHECK(A::UndoCount(s) == 4);

	// 5. scene setting (skybox)
	A::Sky(s).SetExposure(1.75f);
	commit();
	CHECK(states[5] != states[4]);

	// 6. two quick edits of the same object merge into one command
	GameObject* cube = s.FindGameObjectById(cubeId, false);
	REQUIRE(cube != nullptr);
	cube->translation.x = 10.0f;
	commit();
	CHECK(A::UndoCount(s) == 6);
	cube = s.FindGameObjectById(cubeId, false);
	REQUIRE(cube != nullptr);
	cube->translation.x = 11.0f;
	s.PushUndoSnapshot(); // undoMergeAllowed_ is true after the previous push: merged
	CHECK(A::UndoCount(s) == 6);
	states.back() = s.SerializeSceneToString();
	CHECK(states[6] != states[5]);

	const size_t steps = states.size() - 1;
	REQUIRE(steps == 6);

	// Undo back through every recorded state
	for (size_t i = steps; i >= 1; --i) {
		s.Undo();
		CHECK_MSG(s.SerializeSceneToString() == states[i - 1], "undo to state " + std::to_string(i - 1));
	}
	CHECK(A::UndoCount(s) == 0);
	s.Undo(); // nothing left: must be a no-op
	CHECK(s.SerializeSceneToString() == states[0]);

	// Redo forward again
	for (size_t i = 1; i <= steps; ++i) {
		s.Redo();
		CHECK_MSG(s.SerializeSceneToString() == states[i], "redo to state " + std::to_string(i));
	}
	CHECK(A::RedoCount(s) == 0);
	s.Redo(); // nothing left: must be a no-op
	CHECK(s.SerializeSceneToString() == states[steps]);
}

TEST_CASE(Undo, CreateRecordsCompleteObject)
{
	const size_t baseCount = A::Objects(s).size();
	const uint32_t id = s.CreateSphere("UndoSphere", { 2.0f, 0.0f, -1.0f }).id;
	s.PushUndoSnapshot();
	CHECK(A::UndoCount(s) == 1);

	s.Undo();
	CHECK(s.FindGameObjectById(id, false) == nullptr);
	CHECK(A::Objects(s).size() == baseCount);

	s.Redo();
	const GameObject* restored = s.FindGameObjectById(id, false);
	REQUIRE(restored != nullptr);
	CHECK(restored->hasMesh);
	CHECK(restored->meshType == GameObject::MeshType::Sphere);
	CHECK(restored->hasBoxCollider || restored->hasSphereCollider);
	CHECK(restored->name == "UndoSphere");
	CHECK(A::Objects(s).size() == baseCount + 1);
}

TEST_CASE(Undo, PlayModeNotRecorded)
{
	const size_t commands = A::UndoCount(s);
	A::State(s) = EditorSceneState::Play;
	A::Objects(s)[0].translation.x += 3.0f;
	A::MergeAllowed(s) = false;
	s.PushUndoSnapshot();
	const size_t afterPlayEdit = A::UndoCount(s);
	A::State(s) = EditorSceneState::Edit;
	CHECK(afterPlayEdit == commands);
}

// ===========================================================================
// Hierarchy
// ===========================================================================

TEST_CASE(Hierarchy, ChildFollowsParent)
{
	const uint32_t parentId = s.CreateGameObject("HParent", GameObjectType::Empty).id;
	const uint32_t childId = s.CreateGameObject("HChild", GameObjectType::Empty).id;

	GameObject* parent = s.FindGameObjectById(parentId, false);
	GameObject* child = s.FindGameObjectById(childId, false);
	REQUIRE(parent != nullptr);
	REQUIRE(child != nullptr);

	parent->translation = { 0.0f, 0.5f, 0.0f };
	parent->rotation = { 0.0f, 3.14159265f / 4.0f, 0.0f };
	parent->rotationQuat = Quaternion::FromEuler(parent->rotation);
	child->translation = { 2.0f, 0.0f, 0.0f };
	child->parentId = static_cast<int>(parentId);

	s.RebuildHierarchyLinks();
	s.UpdateWorldMatrices();

	child = s.FindGameObjectById(childId, false);
	REQUIRE(child != nullptr);
	Vector3 t{};
	Quaternion q = Quaternion::Identity();
	Vector3 sc{};
	s.GetWorldTRS(*child, t, q, sc);
	// Row-vector RotY(45 deg) maps +X to (cos, 0, -sin)
	CHECK_MSG(std::fabs(t.x - 1.4142f) < 1e-3f && std::fabs(t.y - 0.5f) < 1e-3f && std::fabs(t.z + 1.4142f) < 1e-3f, Fmt(t));
	CHECK_NEAR(sc.x, 1.0f, 1e-3f);
	CHECK_NEAR(sc.y, 1.0f, 1e-3f);
	CHECK_NEAR(sc.z, 1.0f, 1e-3f);
}

TEST_CASE(Hierarchy, ReparentKeepsWorldTransform)
{
	const uint32_t parentId = s.CreateGameObject("KeepParent", GameObjectType::Empty).id;
	const uint32_t childId = s.CreateGameObject("KeepChild", GameObjectType::Empty).id;

	GameObject* parent = s.FindGameObjectById(parentId, false);
	GameObject* child = s.FindGameObjectById(childId, false);
	REQUIRE(parent != nullptr);
	REQUIRE(child != nullptr);
	parent->translation = { 3.0f, 0.0f, 0.0f };
	parent->rotation = { 0.0f, 3.14159265f * 0.5f, 0.0f };
	parent->rotationQuat = Quaternion::FromEuler(parent->rotation);
	parent->scale = { 2.0f, 2.0f, 2.0f };
	child->translation = { 5.0f, 1.0f, 2.0f };
	s.UpdateWorldMatrices();

	REQUIRE(s.ReparentGameObject(childId, static_cast<int>(parentId), true));
	child = s.FindGameObjectById(childId, false);
	REQUIRE(child != nullptr);
	CHECK(child->parentId == static_cast<int>(parentId));

	Vector3 t{};
	Quaternion q = Quaternion::Identity();
	Vector3 sc{};
	s.GetWorldTRS(*child, t, q, sc);
	CHECK_MSG(std::fabs(t.x - 5.0f) < 1e-3f && std::fabs(t.y - 1.0f) < 1e-3f && std::fabs(t.z - 2.0f) < 1e-3f, Fmt(t));
	CHECK_NEAR(sc.x, 1.0f, 1e-3f);
	CHECK_NEAR(sc.y, 1.0f, 1e-3f);
	CHECK_NEAR(sc.z, 1.0f, 1e-3f);
	CHECK_NEAR(std::fabs(q.w), 1.0f, 1e-3f); // world rotation still identity (q or -q)

	// Back to the root: the local transform becomes the world transform again
	REQUIRE(s.ReparentGameObject(childId, -1, true));
	child = s.FindGameObjectById(childId, false);
	REQUIRE(child != nullptr);
	CHECK(child->parentId == -1);
	CHECK_MSG(std::fabs(child->translation.x - 5.0f) < 1e-3f && std::fabs(child->translation.y - 1.0f) < 1e-3f &&
		std::fabs(child->translation.z - 2.0f) < 1e-3f, Fmt(child->translation));
}

TEST_CASE(Hierarchy, RejectsCycle)
{
	const uint32_t aId = s.CreateGameObject("CycleA", GameObjectType::Empty).id;
	const uint32_t bId = s.CreateGameObject("CycleB", GameObjectType::Empty).id;
	const uint32_t cId = s.CreateGameObject("CycleC", GameObjectType::Empty).id;

	REQUIRE(s.ReparentGameObject(bId, static_cast<int>(aId), true));
	REQUIRE(s.ReparentGameObject(cId, static_cast<int>(bId), true));
	CHECK(s.IsDescendantOf(cId, aId));

	// A cannot go under its own child or grandchild, nor under itself
	CHECK(!s.ReparentGameObject(aId, static_cast<int>(bId), true));
	CHECK(!s.ReparentGameObject(aId, static_cast<int>(cId), true));
	CHECK(!s.ReparentGameObject(aId, static_cast<int>(aId), true));
	const GameObject* a = s.FindGameObjectById(aId, false);
	REQUIRE(a != nullptr);
	CHECK(a->parentId == -1);

	// After detaching B the same move is legal
	REQUIRE(s.ReparentGameObject(bId, -1, true));
	CHECK(s.ReparentGameObject(aId, static_cast<int>(bId), true));
}

// ===========================================================================
// Components
// ===========================================================================

TEST_CASE(Components, RemoveAndUndo)
{
	const uint32_t lightId = s.CreateGameObject("CompLight", GameObjectType::PointLight).id;
	{
		GameObject* light = s.FindGameObjectById(lightId, false);
		REQUIRE(light != nullptr);
		light->hasLight = true;
		light->lightType = GameObject::LightType::Point;
	}
	const uint32_t cubeId = s.CreateCube("CompCube", { 0.0f, 1.0f, 0.0f }).id;

	s.PushUndoSnapshot();
	const size_t commandsBefore = A::UndoCount(s);
	const std::string original = s.SerializeSceneToString();

	// Light
	A::MergeAllowed(s) = false;
	A::RemoveComponent(s, lightId, A::Kind::Light);
	{
		const GameObject* light = s.FindGameObjectById(lightId, false);
		REQUIRE(light != nullptr);
		CHECK(!light->hasLight);
		CHECK(light->type == GameObjectType::Empty);
	}

	// Mesh
	A::MergeAllowed(s) = false;
	A::RemoveComponent(s, cubeId, A::Kind::Mesh);
	{
		const GameObject* cube = s.FindGameObjectById(cubeId, false);
		REQUIRE(cube != nullptr);
		CHECK(!cube->hasMesh);
		CHECK(cube->editObject3d == nullptr);
		CHECK(cube->gameObject3d == nullptr);
		CHECK(cube->shadowTransform == nullptr);
		CHECK(cube->type == GameObjectType::Empty);
	}

	// Box collider
	A::MergeAllowed(s) = false;
	A::RemoveComponent(s, cubeId, A::Kind::BoxCollider);
	{
		const GameObject* cube = s.FindGameObjectById(cubeId, false);
		REQUIRE(cube != nullptr);
		CHECK(!cube->hasBoxCollider);
	}

	CHECK(A::UndoCount(s) == commandsBefore + 3);
	CHECK(s.SerializeSceneToString() != original);

	s.Undo();
	s.Undo();
	s.Undo();
	CHECK(s.SerializeSceneToString() == original);
	const GameObject* cube = s.FindGameObjectById(cubeId, false);
	REQUIRE(cube != nullptr);
	CHECK(cube->hasMesh);
	CHECK(cube->hasBoxCollider);
}

// ===========================================================================
// Scripts
// ===========================================================================

TEST_CASE(Scripts, DeferredSpawnKeepsViewStable)
{
	const size_t baseCount = A::Objects(s).size();
	constexpr int kSpawnCount = 200;

	std::vector<ScriptGameObjectView*> views;
	std::vector<uint32_t> ids;
	for (int i = 0; i < kSpawnCount; ++i) {
		ScriptGameObjectView* view = s.QueueScriptSpawn("Spawned", i % 5, { static_cast<float>(i), 0.0f, 0.0f });
		REQUIRE(view != nullptr);
		views.push_back(view);
		ids.push_back(view->id);
	}
	CHECK(std::set<uint32_t>(ids.begin(), ids.end()).size() == static_cast<size_t>(kSpawnCount));

	// Nothing exists yet, but every pending view can be found and edited
	CHECK(A::Objects(s).size() == baseCount);
	for (int i = 0; i < kSpawnCount; ++i) {
		CHECK(s.FindPendingSpawnView(ids[i]) == views[i]);
	}

	s.ProcessPendingSpawns();

	CHECK(A::Objects(s).size() == baseCount + kSpawnCount);
	int stable = 0;
	int exist = 0;
	int movedCorrectly = 0;
	for (int i = 0; i < kSpawnCount; ++i) {
		if (A::ViewFor(s, ids[i]) == views[i]) ++stable;
		const GameObject* obj = s.FindGameObjectById(ids[i], false);
		if (obj) {
			++exist;
			if (std::fabs(obj->translation.x - static_cast<float>(i)) < 1e-4f) ++movedCorrectly;
		}
		CHECK(s.FindPendingSpawnView(ids[i]) == nullptr); // no longer pending
	}
	CHECK_MSG(stable == kSpawnCount, "stable views: " + std::to_string(stable));
	CHECK_MSG(exist == kSpawnCount, "created objects: " + std::to_string(exist));
	CHECK_MSG(movedCorrectly == kSpawnCount, "objects at the queued position: " + std::to_string(movedCorrectly));
	for (int i = 0; i < kSpawnCount; ++i) {
		CHECK(views[i]->id == ids[i]); // the old pointers still read valid data
	}

	// Cancelling a pending spawn prevents its creation
	ScriptGameObjectView* keepA = s.QueueScriptSpawn("KeepA", 0, { 0.0f, 0.0f, 0.0f });
	ScriptGameObjectView* dropB = s.QueueScriptSpawn("DropB", 1, { 1.0f, 0.0f, 0.0f });
	ScriptGameObjectView* keepC = s.QueueScriptSpawn("KeepC", 2, { 2.0f, 0.0f, 0.0f });
	REQUIRE(keepA != nullptr);
	REQUIRE(dropB != nullptr);
	REQUIRE(keepC != nullptr);
	const uint32_t idA = keepA->id;
	const uint32_t idB = dropB->id;
	const uint32_t idC = keepC->id;

	CHECK(s.CancelPendingSpawn(idB));
	CHECK(!s.CancelPendingSpawn(idB));            // already cancelled
	CHECK(!s.CancelPendingSpawn(ids[0]));         // already created, no longer pending
	CHECK(s.FindPendingSpawnView(idB) == nullptr);
	CHECK(!dropB->isActive);                      // retired, not freed: the script's pointer is still readable

	s.ProcessPendingSpawns();
	CHECK(s.FindGameObjectById(idA, false) != nullptr);
	CHECK(s.FindGameObjectById(idC, false) != nullptr);
	CHECK(s.FindGameObjectById(idB, false) == nullptr);
	CHECK(A::Objects(s).size() == baseCount + kSpawnCount + 2);
}

// ===========================================================================
// Rendering
// ===========================================================================

TEST_CASE(Rendering, SrvFreeListDeferredReuse)
{
	DirectXCommon* dx = A::Dx(s);
	REQUIRE(dx != nullptr);

	auto isFixed = [](uint32_t index) {
		return index == 0 || (index >= DirectXCommon::kFixedSrvBegin && index < DirectXCommon::kFixedSrvEnd);
	};

	// Allocate 10 indices: valid, never a fixed slot, all distinct
	std::vector<uint32_t> first;
	for (int i = 0; i < 10; ++i) {
		const uint32_t index = dx->AllocateSrvIndex();
		if (index == DirectXCommon::kInvalidSrvIndex) {
			ctx.Fail(__FILE__, __LINE__, "AllocateSrvIndex returned kInvalidSrvIndex");
			continue;
		}
		CHECK(!isFixed(index));
		CHECK(index < DirectXCommon::kMaxSrvDescriptors);
		first.push_back(index);
	}
	CHECK(std::set<uint32_t>(first.begin(), first.end()).size() == first.size());

	// Free them: they must not come back while the GPU may still read them
	for (uint32_t index : first) dx->FreeSrvIndex(index);
	std::vector<uint32_t> second;
	for (int i = 0; i < 10; ++i) {
		const uint32_t index = dx->AllocateSrvIndex();
		if (index == DirectXCommon::kInvalidSrvIndex) continue;
		second.push_back(index);
		CHECK_MSG(std::find(first.begin(), first.end(), index) == first.end(),
			"freed index " + std::to_string(index) + " was handed out again immediately");
		CHECK(!isFixed(index));
	}
	for (uint32_t index : second) dx->FreeSrvIndex(index);

	// After the GPU caught up the freed indices are allocatable again
	for (int i = 0; i < 3; ++i) dx->SignalAndWaitFence();
	std::vector<uint32_t> third;
	for (size_t i = 0; i < first.size() + second.size(); ++i) {
		const uint32_t index = dx->AllocateSrvIndex();
		if (index == DirectXCommon::kInvalidSrvIndex) break;
		third.push_back(index);
		CHECK(!isFixed(index));
	}
	for (uint32_t index : first) {
		CHECK_MSG(std::find(third.begin(), third.end(), index) != third.end(),
			"freed index " + std::to_string(index) + " was not reusable after the fences completed");
	}
	for (uint32_t index : second) {
		CHECK_MSG(std::find(third.begin(), third.end(), index) != third.end(),
			"freed index " + std::to_string(index) + " was not reusable after the fences completed");
	}

	// Leave the heap exactly as we found it
	for (uint32_t index : third) dx->FreeSrvIndex(index);
	for (int i = 0; i < 3; ++i) dx->SignalAndWaitFence();
	(void)dx->PeekSrvIndex(); // reclaims the retired indices
}

// ===========================================================================
// Math
// ===========================================================================

TEST_CASE(Math, DecomposeComposeRoundTrip)
{
	struct Sample {
		Vector3 t;
		Vector3 euler;
		Vector3 scale;
	};
	std::vector<Sample> samples = {
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 1.0f, 2.0f, 3.0f }, { 0.3f, 0.7f, -0.2f }, { 1.0f, 2.0f, 0.5f } },
		{ { -5.0f, 0.25f, 9.0f }, { 1.2f, -2.4f, 0.9f }, { 3.0f, 3.0f, 3.0f } },
		{ { 10.0f, -10.0f, 0.0f }, { 3.0f, 0.1f, -1.5f }, { 0.4f, 1.7f, 2.2f } },
	};
	std::mt19937 rng(12345);
	std::uniform_real_distribution<float> pos(-20.0f, 20.0f);
	std::uniform_real_distribution<float> ang(-3.0f, 3.0f);
	std::uniform_real_distribution<float> scl(0.3f, 3.0f);
	for (int i = 0; i < 24; ++i) {
		samples.push_back({ { pos(rng), pos(rng), pos(rng) }, { ang(rng), ang(rng), ang(rng) }, { scl(rng), scl(rng), scl(rng) } });
	}

	int index = 0;
	for (const Sample& sample : samples) {
		const Matrix4x4 original = TransformUtil::ComposeTRS(sample.t, Quaternion::FromEuler(sample.euler), sample.scale);
		Vector3 t{};
		Quaternion q = Quaternion::Identity();
		Vector3 sc{};
		TransformUtil::Decompose(original, t, q, sc);
		const Matrix4x4 rebuilt = TransformUtil::ComposeTRS(t, q, sc);

		float maxError = 0.0f;
		for (int r = 0; r < 4; ++r) {
			for (int c = 0; c < 4; ++c) {
				maxError = (std::max)(maxError, std::fabs(original.m[r][c] - rebuilt.m[r][c]));
			}
		}
		CHECK_MSG(maxError <= 1e-4f, "sample " + std::to_string(index) + " max error " + std::to_string(maxError));
		CHECK_NEAR(sc.x, sample.scale.x, 1e-4f);
		CHECK_NEAR(sc.y, sample.scale.y, 1e-4f);
		CHECK_NEAR(sc.z, sample.scale.z, 1e-4f);
		++index;
	}
}

// ===========================================================================
// Project
// ===========================================================================

TEST_CASE(Project, CreateProjectRules)
{
	namespace fs = std::filesystem;
	const fs::path parent = A::TempRoot() / "Projects";
	std::error_code ec;
	fs::create_directories(parent, ec);

	const fs::path savedRoot = A::ProjectRoot(s);
	const fs::path savedDirectory = A::CurrentDirectory(s);

	const bool created = s.CreateNewProject(parent.string(), "TestGame");
	CHECK(created);
	const fs::path projectRoot = parent / "TestGame";
	CHECK(fs::exists(projectRoot / "Assets" / "Scenes" / "SampleScene.scene.json"));
	CHECK(fs::exists(projectRoot / "ProjectSettings" / "project.json"));

	if (created) {
		std::ifstream in(projectRoot / "ProjectSettings" / "project.json");
		std::stringstream buffer;
		buffer << in.rdbuf();
		const auto settings = SceneJson::Parse(buffer.str());
		CHECK(settings.has_value());
		if (settings) {
			std::string projectName;
			SceneJson::Read(*settings, "projectName", projectName);
			CHECK_MSG(projectName == "TestGame", projectName);
		}

		const std::string level = s.NormalizeScenePath("Level2");
		CHECK_MSG(level.ends_with("Assets\\Scenes\\Level2.scene.json"), level);
		CHECK_MSG(s.IsPathInsideProject(level), level);
		CHECK(s.NormalizeScenePath("Level3.json").ends_with("Level3.scene.json"));
		CHECK_MSG(s.ToProjectRelativePath(projectRoot / "Assets" / "Textures" / "circle.png") == "Assets/Textures/circle.png",
			s.ToProjectRelativePath(projectRoot / "Assets" / "Textures" / "circle.png"));
	}

	CHECK(!s.CreateNewProject(parent.string(), "TestGame")); // folder is not empty any more
	CHECK(!s.CreateNewProject(parent.string(), "Bad:Name"));
	CHECK(!fs::exists(parent / "Bad:Name"));
	CHECK(!s.CreateNewProject(parent.string(), ""));

	A::ProjectRoot(s) = savedRoot;
	A::CurrentDirectory(s) = savedDirectory;
}

// ===========================================================================
// Lighting
// ===========================================================================

TEST_CASE(Lighting, SwitchingTypeDisablesDirectional)
{
	LightGroup* lights = A::Lights(s);
	REQUIRE(lights != nullptr);

	GameObject* sun = nullptr;
	for (GameObject& obj : A::Objects(s)) {
		if (obj.hasLight && obj.lightType == GameObject::LightType::Directional) {
			sun = &obj;
			break;
		}
	}
	REQUIRE(sun != nullptr);

	// Sanity: as a directional light it drives the sun
	s.Update(0.0f, RizeEngine::GetInput());
	CHECK(lights->GetDirectionalLight().active == 1);

	sun->lightType = GameObject::LightType::Point;
	s.Update(0.0f, RizeEngine::GetInput());
	CHECK(lights->GetDirectionalLight().active == 0);
	CHECK(lights->GetPointLight(0).active == 1);
}

// ===========================================================================
// Profiler
// ===========================================================================

TEST_CASE(Profiler, CpuScopesAndCounters)
{
	Profiler* profiler = Profiler::GetInstance();
	profiler->FinishCpuFrame(); // flush whatever earlier tests recorded

	{
		RIZE_PROFILE_CPU("TestParent");
		volatile double sink = 0.0;
		for (int i = 0; i < 200000; ++i) sink = sink + std::sqrt(static_cast<double>(i));
		{
			RIZE_PROFILE_CPU("TestChild");
			for (int i = 0; i < 200000; ++i) sink = sink + std::sqrt(static_cast<double>(i));
		}
	}
	profiler->CountDrawCall();
	profiler->CountDrawCall(10);
	profiler->FinishCpuFrame();

	const ProfilerSnapshot& snap = profiler->GetLastFrame();
	const ProfilerScopeResult* parent = nullptr;
	const ProfilerScopeResult* child = nullptr;
	for (const ProfilerScopeResult& r : snap.cpu) {
		if (std::string(r.name) == "TestParent") parent = &r;
		if (std::string(r.name) == "TestChild") child = &r;
	}
	REQUIRE(parent != nullptr);
	REQUIRE(child != nullptr);
	CHECK(child->depth == parent->depth + 1);
	CHECK(parent->ms >= 0.0f);
	CHECK(child->ms >= 0.0f);
	CHECK_MSG(parent->ms >= child->ms, "parent " + std::to_string(parent->ms) + " ms, child " + std::to_string(child->ms) + " ms");
	CHECK(parent->avgMs > 0.0f || parent->ms == 0.0f);

	CHECK(snap.drawCalls == 2);
	CHECK(snap.instances == 11);

	// Counters are per frame: the next publish starts from zero
	profiler->FinishCpuFrame();
	CHECK(profiler->GetLastFrame().drawCalls == 0);
	CHECK(profiler->GetLastFrame().cpu.empty());
}

// ===========================================================================
// Scripting API / export (ROADMAP 3.4)
// ===========================================================================

TEST_CASE(Scripting, KeyConstantsMatchDirectInput)
{
	// Scripts use KEY_* without <dinput.h>; each value must be the real DirectInput scan code
	CHECK(KEY_ESCAPE == DIK_ESCAPE);
	CHECK(KEY_1 == DIK_1);
	CHECK(KEY_0 == DIK_0);
	CHECK(KEY_W == DIK_W);
	CHECK(KEY_A == DIK_A);
	CHECK(KEY_S == DIK_S);
	CHECK(KEY_D == DIK_D);
	CHECK(KEY_Q == DIK_Q);
	CHECK(KEY_E == DIK_E);
	CHECK(KEY_Z == DIK_Z);
	CHECK(KEY_M == DIK_M);
	CHECK(KEY_P == DIK_P);
	CHECK(KEY_L == DIK_L);
	CHECK(KEY_SPACE == DIK_SPACE);
	CHECK(KEY_ENTER == DIK_RETURN);
	CHECK(KEY_TAB == DIK_TAB);
	CHECK(KEY_BACKSPACE == DIK_BACK);
	CHECK(KEY_LSHIFT == DIK_LSHIFT);
	CHECK(KEY_RSHIFT == DIK_RSHIFT);
	CHECK(KEY_LCTRL == DIK_LCONTROL);
	CHECK(KEY_RCTRL == DIK_RCONTROL);
	CHECK(KEY_LALT == DIK_LMENU);
	CHECK(KEY_RALT == DIK_RMENU);
	CHECK(KEY_F1 == DIK_F1);
	CHECK(KEY_F10 == DIK_F10);
	CHECK(KEY_F11 == DIK_F11);
	CHECK(KEY_F12 == DIK_F12);
	CHECK(KEY_UP == DIK_UP);
	CHECK(KEY_DOWN == DIK_DOWN);
	CHECK(KEY_LEFT == DIK_LEFT);
	CHECK(KEY_RIGHT == DIK_RIGHT);
}

TEST_CASE(Export, GameConfigDetection)
{
	namespace fs = std::filesystem;
	const fs::path dir = EditorSuiteTestAccess::TempRoot() / "ExportDetect";
	std::error_code ec;
	fs::create_directories(dir, ec);

	CHECK(!EditorSuite::IsExportedGameFolder(dir));
	CHECK(!EditorSuite::IsExportedGameFolder(fs::path()));
	{
		std::ofstream(dir / EditorSuite::kExportedGameConfigFile) << "standaloneGameMode = true\n";
	}
	CHECK(EditorSuite::IsExportedGameFolder(dir));
	// The editor's own folder must never be mistaken for an exported game
	CHECK(!EditorSuite::IsExportedGameFolder("."));
	CHECK(std::string(EditorSuite::kExportedGameConfigFile) == "game_build.cfg");
}

// ===========================================================================
// Collision / object lookup
// ===========================================================================

namespace {

// Test collider that records every pair it is told about
struct RecordingCollider : ICollidable {
	ColliderShape shape = ColliderShape::Sphere;
	SphereCollider sphere{};
	AABBCollider box{};
	OBBCollider obb{};
	uint32_t index = 0;
	std::set<std::pair<uint32_t, uint32_t>>* hits = nullptr;

	ColliderShape GetColliderShape() const override { return shape; }
	const SphereCollider& GetSphereCollider() const override { return sphere; }
	const AABBCollider& GetAABBCollider() const override { return box; }
	const OBBCollider& GetOBBCollider() const override { return obb; }
	void OnCollision(ICollidable* other) override
	{
		const uint32_t o = static_cast<RecordingCollider*>(other)->index;
		if (hits) hits->insert({ (std::min)(index, o), (std::max)(index, o) });
	}
};

void SetFilter(RecordingCollider& c, uint32_t attribute, uint32_t mask)
{
	c.sphere.attribute = c.box.attribute = c.obb.attribute = attribute;
	c.sphere.mask = c.box.mask = c.obb.mask = mask;
}

std::set<std::pair<uint32_t, uint32_t>> RunCollisions(std::vector<RecordingCollider>& colliders, bool broadphase)
{
	std::set<std::pair<uint32_t, uint32_t>> hits;
	CollisionManager* mgr = CollisionManager::GetInstance();
	const bool previous = mgr->IsBroadphaseEnabled();
	mgr->SetBroadphaseEnabled(broadphase);
	mgr->Clear();
	for (auto& c : colliders) {
		c.hits = &hits;
		mgr->RegisterCollider(&c);
	}
	mgr->CheckAllCollisions();
	mgr->SetBroadphaseEnabled(previous);
	return hits;
}

} // namespace

TEST_CASE(Collision, LayerMaskFilter)
{
	std::vector<RecordingCollider> pair(2);
	for (uint32_t i = 0; i < 2; ++i) {
		pair[i].index = i;
		pair[i].sphere.center = { 0.5f * static_cast<float>(i), 0.0f, 0.0f };
		pair[i].sphere.radius = 1.0f;
	}
	// Player bullet vs enemy: both accept each other -> hit
	SetFilter(pair[0], LayerBit(LAYER_PLAYER_BULLET), LayerBit(LAYER_ENEMY));
	SetFilter(pair[1], LayerBit(LAYER_ENEMY), LayerBit(LAYER_PLAYER) | LayerBit(LAYER_PLAYER_BULLET));
	CHECK(RunCollisions(pair, false).size() == 1);

	// One side does not accept the other -> no hit
	SetFilter(pair[1], LayerBit(LAYER_ENEMY), LayerBit(LAYER_PLAYER));
	CHECK(RunCollisions(pair, false).empty());

	// Same layer, mask excludes own layer (enemies ignore enemies) -> no hit
	SetFilter(pair[0], LayerBit(LAYER_ENEMY), kCollideWithAll & ~LayerBit(LAYER_ENEMY));
	SetFilter(pair[1], LayerBit(LAYER_ENEMY), kCollideWithAll & ~LayerBit(LAYER_ENEMY));
	CHECK(RunCollisions(pair, false).empty());
	CHECK(RunCollisions(pair, true).empty());
}

TEST_CASE(Collision, BroadphaseMatchesBruteForce)
{
	std::mt19937 rng(1234);
	std::uniform_real_distribution<float> pos(-60.0f, 60.0f);
	std::uniform_real_distribution<float> size(0.2f, 1.5f);
	std::uniform_real_distribution<float> angle(-3.14f, 3.14f);
	std::uniform_int_distribution<int> shapePick(0, 2);
	std::uniform_int_distribution<int> layerPick(0, 3);

	constexpr uint32_t kCount = 600;
	std::vector<RecordingCollider> colliders(kCount);
	for (uint32_t i = 0; i < kCount; ++i) {
		RecordingCollider& c = colliders[i];
		c.index = i;
		const Vector3 center = { pos(rng), pos(rng) * 0.05f, pos(rng) }; // mostly flat, like a top-down arena
		const float s = size(rng);
		switch (shapePick(rng)) {
		case 0:
			c.shape = ColliderShape::Sphere;
			c.sphere.center = center;
			c.sphere.radius = s;
			break;
		case 1:
			c.shape = ColliderShape::AABB;
			c.box.SetCenterSize(center, { s, s * 0.7f, s * 1.3f });
			break;
		default:
			c.shape = ColliderShape::OBB;
			c.obb.center = center;
			c.obb.size = { s, s * 0.5f, s * 1.2f };
			c.obb.SetRotation({ 0.0f, angle(rng), 0.0f });
			break;
		}
		const uint32_t layer = static_cast<uint32_t>(layerPick(rng));
		// Layer 2 (enemies) ignore each other, everyone else collides with all
		SetFilter(c, LayerBit(layer), (layer == 2) ? (kCollideWithAll & ~LayerBit(2)) : kCollideWithAll);
	}
	// A huge ground box and a long wall exercise the "large proxy" path
	colliders[0].shape = ColliderShape::AABB;
	colliders[0].box.SetCenterSize({ 0.0f, -1.0f, 0.0f }, { 80.0f, 1.0f, 80.0f });
	colliders[1].shape = ColliderShape::AABB;
	colliders[1].box.SetCenterSize({ 0.0f, 0.0f, 30.0f }, { 70.0f, 3.0f, 0.5f });

	const auto brute = RunCollisions(colliders, false);
	const CollisionManager::Stats bruteStats = CollisionManager::GetInstance()->GetLastStats();
	const auto fast = RunCollisions(colliders, true);
	const CollisionManager::Stats fastStats = CollisionManager::GetInstance()->GetLastStats();

	CHECK(!bruteStats.usedBroadphase);
	CHECK(fastStats.usedBroadphase);
	CHECK_MSG(brute == fast, std::format("brute-force hits {}, broadphase hits {}", brute.size(), fast.size()));
	CHECK(brute.size() > 50); // the scene must actually produce contacts
	CHECK(bruteStats.collisions == fastStats.collisions);
	// The broadphase must prune most pairs (brute force tests every layer-compatible pair)
	CHECK_MSG(fastStats.narrowphaseTests * 10 < bruteStats.narrowphaseTests,
		std::format("narrowphase tests: broadphase {}, brute force {}", fastStats.narrowphaseTests, bruteStats.narrowphaseTests));
}

TEST_CASE(Objects, IdLookupSurvivesVectorChanges)
{
	std::vector<uint32_t> ids;
	for (int i = 0; i < 50; ++i) {
		ids.push_back(s.CreateCube("Cube" + std::to_string(i), { static_cast<float>(i), 0.0f, 0.0f }).id);
	}
	for (uint32_t id : ids) {
		const GameObject* obj = s.FindGameObjectById(id, false);
		REQUIRE(obj != nullptr);
		CHECK(obj->id == id);
	}

	// Delete every third object, then reorder the vector behind the cache's back
	for (size_t i = 0; i < ids.size(); i += 3) s.QueueDeleteGameObjectById(ids[i]);
	s.ProcessPendingDeletes();
	auto& objects = A::Objects(s);
	std::reverse(objects.begin(), objects.end());

	int found = 0;
	for (size_t i = 0; i < ids.size(); ++i) {
		const GameObject* obj = s.FindGameObjectById(ids[i], false);
		if (i % 3 == 0) {
			CHECK(obj == nullptr);
		} else if (obj && obj->id == ids[i] && obj->name == "Cube" + std::to_string(i)) {
			++found;
		}
	}
	CHECK_MSG(found == 33, "objects found after delete + reorder: " + std::to_string(found));
	CHECK(s.FindGameObjectIndexById(0xFFFFFFF0u) == -1);

	// Layer + mask survive a JSON round trip
	GameObject* obj = s.FindGameObjectById(ids[1], false);
	REQUIRE(obj != nullptr);
	obj->layer = LAYER_ENEMY;
	obj->collisionMask = LayerBit(LAYER_PLAYER) | LayerBit(LAYER_PLAYER_BULLET);
	const std::string json = s.SerializeSceneToString();
	REQUIRE(s.DeserializeSceneFromString(json));
	obj = s.FindGameObjectById(ids[1], false);
	REQUIRE(obj != nullptr);
	CHECK(obj->layer == LAYER_ENEMY);
	CHECK(obj->collisionMask == (LayerBit(LAYER_PLAYER) | LayerBit(LAYER_PLAYER_BULLET)));
	CHECK(EditorSuite::GetCollisionLayerName(LAYER_PLAYER_BULLET) == "PlayerBullet");
	CHECK(EditorSuite::GetCollisionLayerName(20) == "Layer 20");
}

TEST_CASE(Scripting, MouseRayHitsGroundPlane)
{
	// Top-down camera: 20 units up, tilted 60 degrees down, looking along +Z
	const Matrix4x4 cameraWorld = Mathematics::MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 1.0472f, 0.0f, 0.0f }, { 0.0f, 20.0f, -10.0f });
	const Matrix4x4 view = Mathematics::Inverse(cameraWorld);
	const Matrix4x4 proj = Mathematics::MakePerspectiveFovMatrix(0.8f, 16.0f / 9.0f, 0.1f, 500.0f);
	const Matrix4x4 viewProj = Mathematics::Multiply(view, proj);

	const Vector3 targets[] = { { 0.0f, 0.0f, 2.0f }, { 5.0f, 0.0f, 4.0f }, { -3.0f, 0.0f, 0.5f }, { 2.0f, 1.5f, 3.0f } };
	for (const Vector3& target : targets) {
		const Vector3 ndc = Mathematics::Transform(target, viewProj);
		REQUIRE(std::fabs(ndc.x) <= 1.0f && std::fabs(ndc.y) <= 1.0f); // on screen
		Vector3 hit{};
		REQUIRE(EditorSuite::ScreenRayToPlaneY(viewProj, ndc.x, ndc.y, target.y, hit));
		CHECK_NEAR(hit.x, target.x, 1e-3f);
		CHECK_NEAR(hit.y, target.y, 1e-6f);
		CHECK_NEAR(hit.z, target.z, 1e-3f);
	}
	// Looking at the sky: a plane far below the horizon line is behind the ray
	Vector3 hit{};
	CHECK(!EditorSuite::ScreenRayToPlaneY(viewProj, 0.0f, 0.0f, 100.0f, hit));
}

TEST_CASE(Scripting, TimeScaleScalesGameTime)
{
	EngineAPI& api = A::Api(s);
	REQUIRE(api.setTimeScale != nullptr);
	REQUIRE(api.getTime != nullptr);
	A::State(s) = EditorSceneState::Play;
	A::GameTime(s) = 0.0;

	api.setTimeScale(api.hostContext, 0.5f);
	CHECK_NEAR(api.getTimeScale(api.hostContext), 0.5f, 1e-6f);
	s.Update(0.1f, RizeEngine::GetInput());
	CHECK_NEAR(api.getTime(api.hostContext), 0.05f, 1e-5f);
	CHECK_NEAR(api.getDeltaTime(api.hostContext), 0.05f, 1e-5f);
	CHECK_NEAR(api.getUnscaledDeltaTime(api.hostContext), 0.1f, 1e-5f);

	api.setTimeScale(api.hostContext, 0.0f); // paused
	s.Update(0.1f, RizeEngine::GetInput());
	CHECK_NEAR(api.getTime(api.hostContext), 0.05f, 1e-5f);
	CHECK_NEAR(api.getDeltaTime(api.hostContext), 0.0f, 1e-6f);

	api.setTimeScale(api.hostContext, -3.0f); // clamped
	CHECK_NEAR(api.getTimeScale(api.hostContext), 0.0f, 1e-6f);
	api.setTimeScale(api.hostContext, 1.0f);
	A::State(s) = EditorSceneState::Edit;

	// Out-of-range queries are safe; the mouse is never "in" a Game view that was not drawn
	CHECK(!api.isMouseDown(api.hostContext, 0));
	CHECK(!api.isPadConnected(api.hostContext, 7));
	CHECK_NEAR(api.getPadTrigger(api.hostContext, 1, 9), 0.0f, 1e-6f);
}

// ===========================================================================
// Prefabs & pooling
// ===========================================================================

TEST_CASE(Prefabs, SaveAndInstantiateRemapsIds)
{
	A::ProjectRoot(s) = A::TempRoot() / "PrefabProject";

	GameObject& root = s.CreateCube("Enemy", { 4.0f, 0.0f, 4.0f });
	root.layer = LAYER_ENEMY;
	root.collisionMask = LayerBit(LAYER_PLAYER) | LayerBit(LAYER_PLAYER_BULLET);
	root.scale = { 0.5f, 2.0f, 0.5f };
	const uint32_t rootId = root.id;
	const uint32_t childId = s.CreateSphere("EnemyEye", { 0.0f, 1.0f, 0.0f }).id;
	REQUIRE(s.ReparentGameObject(childId, static_cast<int>(rootId), false));

	REQUIRE(s.SavePrefab(rootId, "Enemy"));
	CHECK(std::filesystem::exists(A::TempRoot() / "PrefabProject" / "Assets" / "Prefabs" / "Enemy.prefab.json"));
	CHECK(s.HasPrefab("Enemy"));
	CHECK(!s.HasPrefab("DoesNotExist"));

	const size_t before = A::Objects(s).size();
	const uint32_t newRoot = s.InstantiatePrefabNow("Enemy", { -7.0f, 0.0f, 3.0f });
	REQUIRE(newRoot != 0);
	CHECK(newRoot != rootId);
	CHECK(A::Objects(s).size() == before + 2);

	const GameObject* inst = s.FindGameObjectById(newRoot, false);
	REQUIRE(inst != nullptr);
	CHECK(inst->parentId == -1);
	CHECK_NEAR(inst->translation.x, -7.0f, 1e-5f);
	CHECK_NEAR(inst->scale.y, 2.0f, 1e-5f);
	CHECK(inst->layer == LAYER_ENEMY);
	CHECK(inst->collisionMask == (LayerBit(LAYER_PLAYER) | LayerBit(LAYER_PLAYER_BULLET)));
	CHECK(inst->prefabSource == "Enemy");
	REQUIRE(inst->childrenIds.size() == 1);
	const GameObject* eye = s.FindGameObjectById(inst->childrenIds[0], false);
	REQUIRE(eye != nullptr);
	CHECK(eye->id != childId);
	CHECK(eye->name == "EnemyEye");
	CHECK(eye->parentId == static_cast<int>(newRoot));
}

TEST_CASE(Prefabs, PoolReusesDespawnedInstances)
{
	A::ProjectRoot(s) = A::TempRoot() / "PoolProject";
	const uint32_t templateId = s.CreateSphere("Bullet", { 0.0f, 0.0f, 0.0f }).id;
	REQUIRE(s.SavePrefab(templateId, "Bullet"));
	A::State(s) = EditorSceneState::Play;

	// Missing prefab: no view, nothing queued
	CHECK(s.QueuePrefabSpawn("NoSuchPrefab", { 0.0f, 0.0f, 0.0f }) == nullptr);

	std::vector<uint32_t> ids;
	for (int i = 0; i < 20; ++i) {
		ScriptGameObjectView* view = s.QueuePrefabSpawn("Bullet", { static_cast<float>(i), 0.5f, 0.0f });
		REQUIRE(view != nullptr);
		ids.push_back(view->id);
	}
	s.ProcessPendingSpawns();
	const size_t afterSpawn = A::Objects(s).size();
	for (size_t i = 0; i < ids.size(); ++i) {
		const GameObject* obj = s.FindGameObjectById(ids[i], false);
		REQUIRE(obj != nullptr);
		CHECK(obj->isActive);
		CHECK_NEAR(obj->translation.x, static_cast<float>(i), 1e-5f);
	}

	// Despawn half: they stay in the scene, hidden, and go to the pool
	for (size_t i = 0; i < 10; ++i) s.QueueDespawn(ids[i]);
	s.QueueDespawn(ids[0]); // duplicate request is ignored
	s.ProcessPendingSpawns();
	CHECK(A::Objects(s).size() == afterSpawn);
	CHECK(s.GetPooledCount("Bullet") == 10);
	CHECK(!s.FindGameObjectById(ids[3], false)->isActive);

	// Spawning again reuses pooled objects instead of creating new ones
	std::set<uint32_t> reused;
	for (int i = 0; i < 10; ++i) {
		ScriptGameObjectView* view = s.QueuePrefabSpawn("Bullet", { 100.0f, 0.0f, static_cast<float>(i) });
		REQUIRE(view != nullptr);
		reused.insert(view->id);
	}
	s.ProcessPendingSpawns();
	CHECK(A::Objects(s).size() == afterSpawn);
	CHECK(s.GetPooledCount("Bullet") == 0);
	CHECK(reused == std::set<uint32_t>(ids.begin(), ids.begin() + 10));
	for (uint32_t id : reused) {
		const GameObject* obj = s.FindGameObjectById(id, false);
		REQUIRE(obj != nullptr);
		CHECK(obj->isActive);
		CHECK_NEAR(obj->translation.x, 100.0f, 1e-5f);
	}

	// Cancelling a pending reuse puts the object back into the pool
	s.QueueDespawn(ids[15]);
	s.ProcessPendingSpawns();
	ScriptGameObjectView* pending = s.QueuePrefabSpawn("Bullet", { 0.0f, 0.0f, 0.0f });
	REQUIRE(pending != nullptr);
	CHECK(pending->id == ids[15]);
	s.QueueDespawn(pending->id);
	CHECK(s.GetPooledCount("Bullet") == 1);
	s.ProcessPendingSpawns();
	CHECK(!s.FindGameObjectById(ids[15], false)->isActive);

	// Despawning a non-prefab object deletes it
	const uint32_t plain = s.CreateCube("Plain", { 0.0f, 0.0f, 0.0f }).id;
	s.QueueDespawn(plain);
	s.ProcessPendingSpawns();
	s.ProcessPendingDeletes();
	CHECK(s.FindGameObjectById(plain, false) == nullptr);

	A::State(s) = EditorSceneState::Edit;
	s.ResetPrefabPools();
}

// ===========================================================================
// Game UI
// ===========================================================================

TEST_CASE(UI, CanvasCommandsAndTextLayout)
{
	UIRenderer* ui = UIRenderer::GetInstance();
	ui->Clear();
	CHECK(!ui->HasCommands());

	EngineAPI& api = A::Api(s);
	REQUIRE(api.uiRect != nullptr);
	api.uiRect(api.hostContext, 10.0f, 10.0f, 200.0f, 20.0f, { 1.0f, 0.0f, 0.0f, 1.0f });
	CHECK(ui->GetQuadCount() == 1);

	// Text: one quad per visible glyph, spaces and newlines produce none
	api.uiText(api.hostContext, "HP 100\nLv 3", 20.0f, 40.0f, 32.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 0);
	CHECK_MSG(ui->GetQuadCount() == 1 + 8, "quads: " + std::to_string(ui->GetQuadCount()));

	// Measuring: proportional, scales linearly with size, widest line wins
	const float w32 = api.uiMeasureText(api.hostContext, "SCORE", 32.0f);
	const float w64 = api.uiMeasureText(api.hostContext, "SCORE", 64.0f);
	CHECK(w32 > 30.0f && w32 < 200.0f);
	CHECK_NEAR(w64, w32 * 2.0f, 0.01f);
	CHECK(api.uiMeasureText(api.hostContext, "iii", 32.0f) < api.uiMeasureText(api.hostContext, "WWW", 32.0f));
	CHECK_NEAR(api.uiMeasureText(api.hostContext, "AB\nA", 32.0f), api.uiMeasureText(api.hostContext, "AB", 32.0f), 1e-4f);

	// A missing image falls back to the white texture instead of failing
	api.uiImage(api.hostContext, "Assets/Textures/does_not_exist.png", 0.0f, 0.0f, 64.0f, 64.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
	CHECK(ui->GetQuadCount() == 1 + 8 + 1);

	ui->Clear();
	CHECK(!ui->HasCommands());
}

// ===========================================================================
// Runtime: scene switching + save data
// ===========================================================================

TEST_CASE(Runtime, SaveDataPersists)
{
	A::ProjectRoot(s) = A::TempRoot() / "SaveProject";
	EngineAPI& api = A::Api(s);
	REQUIRE(api.saveSetNumber != nullptr);

	CHECK(!api.saveHasKey(api.hostContext, "HighScore"));
	CHECK_NEAR(api.saveGetNumber(api.hostContext, "HighScore", -1.0), -1.0, 1e-9);
	api.saveSetNumber(api.hostContext, "HighScore", 12345.0);
	api.saveSetString(api.hostContext, "PlayerName", "Rize");
	CHECK(std::filesystem::exists(A::TempRoot() / "SaveProject" / "SaveData" / "save.json"));

	// Switching project root forces a reload from disk: values must come back from the file
	A::ProjectRoot(s) = A::TempRoot() / "OtherProject";
	CHECK(!api.saveHasKey(api.hostContext, "HighScore"));
	A::ProjectRoot(s) = A::TempRoot() / "SaveProject";
	CHECK_NEAR(api.saveGetNumber(api.hostContext, "HighScore", 0.0), 12345.0, 1e-9);
	char name[8]{};
	CHECK(api.saveGetString(api.hostContext, "PlayerName", name, static_cast<int>(sizeof(name))));
	CHECK(std::string(name) == "Rize");
	char tiny[3]{};
	CHECK(api.saveGetString(api.hostContext, "PlayerName", tiny, static_cast<int>(sizeof(tiny))));
	CHECK(std::string(tiny) == "Ri"); // truncated, still terminated
	CHECK(!api.saveGetString(api.hostContext, "HighScore", name, static_cast<int>(sizeof(name)))); // a number, not a string

	api.saveDeleteAll(api.hostContext);
	CHECK(!api.saveHasKey(api.hostContext, "PlayerName"));
}

TEST_CASE(Runtime, LoadSceneDuringPlayAndRestoreOnStop)
{
	namespace fs = std::filesystem;
	A::ProjectRoot(s) = A::TempRoot() / "SceneSwitchProject";
	const fs::path scenes = s.GetScenesDirectory();
	std::error_code ec;
	fs::create_directories(scenes, ec);

	// Stage2 on disk contains only "Stage2Marker"
	s.CreateCube("Stage2Marker", { 1.0f, 0.0f, 0.0f });
	{
		std::ofstream(scenes / "Stage2.scene.json") << s.SerializeSceneToString();
	}

	A::PrepareForTest(s);
	A::ProjectRoot(s) = A::TempRoot() / "SceneSwitchProject";
	s.CreateCube("Stage1Marker", { 0.0f, 0.0f, 0.0f });
	A::ScenePath(s) = (scenes / "Stage1.scene.json").string();
	A::SceneName(s) = "Stage1";

	s.StartPlayMode();
	EngineAPI& api = A::Api(s);
	api.loadScene(api.hostContext, "Stage2");
	CHECK(s.FindGameObjectByName("Stage1Marker", false) != nullptr); // deferred until the end of the frame
	s.Update(0.016f, RizeEngine::GetInput());
	CHECK(s.FindGameObjectByName("Stage2Marker", false) != nullptr);
	CHECK(s.FindGameObjectByName("Stage1Marker", false) == nullptr);
	char sceneName[64]{};
	api.getSceneName(api.hostContext, sceneName, static_cast<int>(sizeof(sceneName)));
	CHECK(std::string(sceneName) == "Stage2");
	CHECK(A::State(s) == EditorSceneState::Play);

	// A missing scene keeps the current one
	api.loadScene(api.hostContext, "NoSuchStage");
	s.Update(0.016f, RizeEngine::GetInput());
	CHECK(s.FindGameObjectByName("Stage2Marker", false) != nullptr);

	s.StopPlayMode();
	CHECK(s.FindGameObjectByName("Stage1Marker", false) != nullptr);
	CHECK(s.FindGameObjectByName("Stage2Marker", false) == nullptr);
	CHECK(A::SceneName(s) == "Stage1");
	CHECK(A::ScenePath(s) == (scenes / "Stage1.scene.json").string());
}

// ===========================================================================
// Samples
// ===========================================================================

TEST_CASE(Samples, SurvivorsDemoRuns)
{
	namespace fs = std::filesystem;
	A::ProjectRoot(s) = A::TempRoot() / "SurvivorsProject";
	REQUIRE(s.CreateSurvivorsDemo());
	CHECK(fs::exists(A::TempRoot() / "SurvivorsProject" / "Assets" / "Prefabs" / "Enemy.prefab.json"));
	CHECK(fs::exists(A::TempRoot() / "SurvivorsProject" / "Assets" / "Prefabs" / "Bullet.prefab.json"));
	CHECK(fs::exists(A::TempRoot() / "SurvivorsProject" / "Assets" / "Scenes" / "SurvivorsDemo.scene.json"));
	CHECK(s.FindGameObjectByName("Enemy", false) == nullptr); // templates are not left in the scene
	const GameObject* player = s.FindGameObjectByName("Player", false);
	REQUIRE(player != nullptr);
	CHECK(player->layer == LAYER_PLAYER);
	REQUIRE(s.FindGameObjectByName("GameManager", false) != nullptr);

	// GameScripts.dll is built with the solution, so the sample scripts must be available here
	REQUIRE(A::ScriptsLoaded(s));

	const size_t sceneObjects = A::Objects(s).size();
	s.StartPlayMode();
	for (int frame = 0; frame < 150; ++frame) {
		s.Update(1.0f / 60.0f, RizeEngine::GetInput());
	}
	int enemies = 0;
	int bullets = 0;
	for (const GameObject& obj : A::Objects(s)) {
		if (obj.prefabSource == "Enemy" && obj.isActive) ++enemies;
		if (obj.prefabSource == "Bullet") ++bullets;
	}
	CHECK_MSG(enemies >= 1, "active enemies after 2.5 s: " + std::to_string(enemies));
	CHECK_MSG(bullets >= 3, "bullets after 2.5 s: " + std::to_string(bullets));
	CHECK_MSG(UIRenderer::GetInstance()->GetQuadCount() > 20, "HUD quads: " + std::to_string(UIRenderer::GetInstance()->GetQuadCount()));

	s.StopPlayMode();
	CHECK(A::Objects(s).size() == sceneObjects);
	CHECK(!UIRenderer::GetInstance()->HasCommands());
}

// ===========================================================================
// Runner
// ===========================================================================

int EditorSuite::RunSelfTests(const std::string& filter)
{
	namespace fs = std::filesystem;

	// Everything the tests write goes below this folder, which starts empty
	const fs::path tempRoot = EditorSuiteTestAccess::TempRoot();
	std::error_code ec;
	fs::remove_all(tempRoot, ec);
	fs::create_directories(tempRoot, ec);

	const fs::path savedProjectRoot = currentProjectRoot_;
	const fs::path savedDirectory = currentDirectory_;
	const fs::path savedSessionRoot = sSessionProjectRoot_; // CreateNewProject() changes it; OpenProject() would add it to the recent list
	isRunningTests_ = true;

	std::string report;
	int passed = 0;
	int failed = 0;
	int ran = 0;

	LogMessage(std::format("[Tests] Running self-tests (filter: '{}', temp folder: {})", filter, tempRoot.string()), 0);

	for (const TestCase& test : GetTestRegistry()) {
		if (!filter.empty() && std::string_view(test.name).find(filter) == std::string_view::npos) continue;
		++ran;

		TestContext ctx;
		ctx.name = test.name;
		try {
			EditorSuiteTestAccess::PrepareForTest(*this);
			test.fn(*this, ctx);
		} catch (const std::exception& e) {
			ctx.Fail(__FILE__, __LINE__, std::string("unhandled exception: ") + e.what());
		} catch (...) {
			ctx.Fail(__FILE__, __LINE__, "unhandled unknown exception");
		}

		// A failed test must not leave Play mode or a foreign project behind
		sceneState_ = EditorSceneState::Edit;
		currentProjectRoot_ = savedProjectRoot;
		currentDirectory_ = savedDirectory;
		sSessionProjectRoot_ = savedSessionRoot;

		if (ctx.failures == 0) {
			++passed;
			report += std::format("[PASS] {}\n", ctx.name);
			LogMessage(std::format("[Tests] [PASS] {}", ctx.name), 0);
		} else {
			++failed;
			report += std::format("[FAIL] {}\n", ctx.name);
			LogMessage(std::format("[Tests] [FAIL] {}", ctx.name), 2);
			for (const std::string& message : ctx.messages) {
				report += std::format("    {}\n", message);
				LogMessage(std::format("[Tests]     {}", message), 2);
			}
		}
	}

	if (ran == 0) {
		// A filter that matches nothing is a mistake, not a pass
		++failed;
		report += "[FAIL] No tests matched the filter.\n";
		LogMessage("[Tests] No tests matched the filter.", 2);
	}
	report += std::format("Summary: {} passed, {} failed\n", passed, failed);
	LogMessage(std::format("[Tests] Summary: {} passed, {} failed", passed, failed), failed == 0 ? 0 : 2);

	{
		std::ofstream out("test_results.txt", std::ios::trunc);
		out << report;
	}

	// Leave a clean default scene behind and make sure the user's data was never the target
	sceneState_ = EditorSceneState::Edit;
	currentProjectRoot_ = savedProjectRoot;
	currentDirectory_ = savedDirectory;
	sSessionProjectRoot_ = savedSessionRoot;
	NewScene();
	isRunningTests_ = false;

	return failed;
}

#endif // USE_IMGUI
