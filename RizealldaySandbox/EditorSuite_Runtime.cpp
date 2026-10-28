#include "EditorSuiteInternal.h"
#include "SceneJson.h"
#include "audio/Audio.h"

// Runtime services for game scripts: scene switching and persistent save data.

// ---------------------------------------------------------------------------
// Scene switching
// ---------------------------------------------------------------------------

std::filesystem::path EditorSuite::ResolveSceneRequest(const std::string& nameOrPath) const
{
	std::filesystem::path p(nameOrPath);
	if (!nameOrPath.ends_with(".json")) {
		// Bare name: "Stage2" -> <project>/Assets/Scenes/Stage2.scene.json
		return GetScenesDirectory() / (nameOrPath + ".scene.json");
	}
	if (p.is_absolute()) return p;
	return std::filesystem::path(ResolveAssetPath(nameOrPath));
}

void EditorSuite::RequestSceneLoad(const std::string& nameOrPath)
{
	if (nameOrPath.empty()) return;
	pendingSceneLoad_ = nameOrPath; // the last request in a frame wins
}

bool EditorSuite::ProcessPendingSceneLoad()
{
	if (pendingSceneLoad_.empty()) return false;
	const std::string request = std::move(pendingSceneLoad_);
	pendingSceneLoad_.clear();

	const std::filesystem::path path = ResolveSceneRequest(request);
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		LogMessage(std::format("[Scene] LoadScene('{}'): file not found ({}).", request, path.string()), 2);
		return false;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	const bool gameRunning = (sceneState_ != EditorSceneState::Edit) || IsStandaloneMode();
	if (!gameRunning) {
		return LoadScene(path.string());
	}

	// In the editor, Stop must bring back the scene that Play started from
	if (!IsStandaloneMode() && !runtimeSceneSwitched_) {
		SceneJson::Json settings = SceneJson::Json::object();
		WriteSceneSettingsJson(&settings);
		runtimeOriginSettings_ = SceneJson::DumpCompact(settings);
		runtimeOriginScenePath_ = currentScenePath_;
		runtimeOriginSceneName_ = currentSceneName_;
		runtimeOriginDirty_ = isSceneDirty_;
		runtimeSceneSwitched_ = true;
	}

	Audio::GetInstance()->StopAllAudio();
	pendingDeletes_.clear();
	CollisionManager::GetInstance()->Clear();

	// Calls OnDestroy() on the old scene's scripts, clears spawns/pools/UI
	if (!DeserializeSceneFromString(buffer.str())) {
		LogMessage(std::format("[Scene] LoadScene('{}'): invalid scene file.", request), 2);
		return false;
	}
	currentScenePath_ = path.string();
	currentSceneName_ = EditorSuiteDetail::SceneNameFromPath(path);
	timeScale_ = 1.0f;
	gameTime_ = 0.0;

	// Same order as StartPlayMode: every object exists before any Initialize() runs
	std::vector<uint32_t> ids;
	ids.reserve(gameObjects_.size());
	for (const auto& obj : gameObjects_) ids.push_back(obj.id);
	for (uint32_t id : ids) StartScriptsOnSpawnedObject(id, true);
	RefreshAllScriptObjectViews();

	LogMessage(std::format("[Scene] Switched to '{}' ({} objects).", currentSceneName_, gameObjects_.size()), 0);
	return true;
}

void EditorSuite::RestoreRuntimeSceneOrigin()
{
	if (!runtimeSceneSwitched_) return;
	runtimeSceneSwitched_ = false;
	currentScenePath_ = runtimeOriginScenePath_;
	currentSceneName_ = runtimeOriginSceneName_;
	isSceneDirty_ = runtimeOriginDirty_;
	if (std::optional<SceneJson::Json> settings = SceneJson::Parse(runtimeOriginSettings_)) {
		ReadSceneSettingsJson(&*settings);
	}
	runtimeOriginSettings_.clear();
}

// ---------------------------------------------------------------------------
// Save data: one JSON object of numbers and strings in <project>/SaveData/save.json
// (next to the executable in an exported game). Written on every change.
// ---------------------------------------------------------------------------

std::filesystem::path EditorSuite::GetSaveDataPath() const
{
	const std::filesystem::path root = currentProjectRoot_.empty() ? std::filesystem::path(".") : currentProjectRoot_;
	return root / "SaveData" / "save.json";
}

void EditorSuite::EnsureSaveDataLoaded()
{
	const std::string path = GetSaveDataPath().string();
	if (saveDataLoadedFrom_ == path) return;
	saveDataLoadedFrom_ = path;
	saveNumbers_.clear();
	saveStrings_.clear();

	std::ifstream in(path, std::ios::binary);
	if (!in) return;
	std::stringstream buffer;
	buffer << in.rdbuf();
	std::optional<SceneJson::Json> parsed = SceneJson::Parse(buffer.str());
	if (!parsed || !parsed->is_object()) {
		LogMessage(std::format("[SaveData] '{}' is not valid JSON; starting with empty save data.", path), 1);
		return;
	}
	for (auto it = parsed->begin(); it != parsed->end(); ++it) {
		if (it.value().is_number()) saveNumbers_[it.key()] = it.value().get<double>();
		else if (it.value().is_string()) saveStrings_[it.key()] = it.value().get<std::string>();
	}
}

void EditorSuite::WriteSaveData()
{
	SceneJson::Json j = SceneJson::Json::object();
	for (const auto& [key, value] : saveNumbers_) j[key] = value;
	for (const auto& [key, value] : saveStrings_) j[key] = value;

	const std::filesystem::path path = GetSaveDataPath();
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	// Write a temp file first so a crash mid-write never leaves a truncated save
	const std::filesystem::path tmp = path.string() + ".tmp";
	{
		std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
		if (!out) {
			LogMessage(std::format("[SaveData] Cannot write '{}'.", tmp.string()), 2);
			return;
		}
		out << j.dump(1, '\t');
	}
	std::filesystem::rename(tmp, path, ec);
	if (ec) {
		std::filesystem::remove(path, ec);
		std::filesystem::rename(tmp, path, ec);
	}
}

void EditorSuite::SetSaveNumber(const std::string& key, double value)
{
	if (key.empty() || !std::isfinite(value)) return;
	EnsureSaveDataLoaded();
	saveStrings_.erase(key);
	saveNumbers_[key] = value;
	WriteSaveData();
}

double EditorSuite::GetSaveNumber(const std::string& key, double fallback)
{
	EnsureSaveDataLoaded();
	auto it = saveNumbers_.find(key);
	return (it != saveNumbers_.end()) ? it->second : fallback;
}

void EditorSuite::SetSaveString(const std::string& key, const std::string& value)
{
	if (key.empty()) return;
	EnsureSaveDataLoaded();
	saveNumbers_.erase(key);
	saveStrings_[key] = value;
	WriteSaveData();
}

bool EditorSuite::GetSaveString(const std::string& key, std::string& out)
{
	EnsureSaveDataLoaded();
	auto it = saveStrings_.find(key);
	if (it == saveStrings_.end()) return false;
	out = it->second;
	return true;
}

bool EditorSuite::HasSaveKey(const std::string& key)
{
	EnsureSaveDataLoaded();
	return saveNumbers_.contains(key) || saveStrings_.contains(key);
}

void EditorSuite::DeleteAllSaveData()
{
	EnsureSaveDataLoaded();
	saveNumbers_.clear();
	saveStrings_.clear();
	WriteSaveData();
}
