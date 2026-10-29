// Asset import: copy files / folders into <project>/Assets from a file dialog or by dropping them from Explorer,
// together with the files a model needs (.mtl + textures, .bin + images, <name>.fbm). Imported files get a GUID.
#include "EditorSuiteInternal.h"
#include "SceneJson.h"
#include "base/ConvertString.h"
#include "EngineUpdate.h"
#include <wrl.h>
#include <shobjidl.h>

namespace fs = std::filesystem;

namespace {
	std::string Lower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	/// ".png", or a compound extension such as ".prefab.json"
	std::string FullExtension(const fs::path& file)
	{
		const std::string name = Lower(file.filename().string());
		for (const char* compound : { ".scene.json", ".prefab.json", ".animator.json" }) {
			if (name.ends_with(compound)) return compound;
		}
		return Lower(file.extension().string());
	}

	/// Assets/<folder> for a file type ("" = the Assets root)
	const char* CategoryFolder(const std::string& ext)
	{
		static const std::pair<const char*, std::vector<std::string>> kCategories[] = {
			{ "Textures", { ".png", ".jpg", ".jpeg", ".dds", ".tga", ".bmp", ".hdr" } },
			{ "Models", { ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds" } },
			{ "Audio", { ".wav", ".mp3", ".ogg", ".m4a", ".wma" } },
			{ "Scenes", { ".scene.json" } },
			{ "Prefabs", { ".prefab.json" } },
			{ "Animators", { ".animator.json" } },
			{ "Levels", { ".csv" } },
		};
		for (const auto& [folder, exts] : kCategories) {
			if (std::find(exts.begin(), exts.end(), ext) != exts.end()) return folder;
		}
		return "";
	}

	/// `dir/name` or, when taken, `dir/name 1.ext`, `dir/name 2.ext`, ...
	fs::path UniqueTarget(const fs::path& dir, const fs::path& name)
	{
		std::error_code ec;
		fs::path target = dir / name;
		if (!fs::exists(target, ec)) return target;
		const std::string ext = fs::is_directory(target, ec) ? std::string() : FullExtension(name);
		const std::string file = name.filename().string();
		const std::string stem = file.substr(0, file.size() - ext.size());
		for (int i = 1; i < 10000; ++i) {
			target = dir / (stem + " " + std::to_string(i) + ext);
			if (!fs::exists(target, ec)) return target;
		}
		return dir / name;
	}

	/// Files referenced by a model next to it (relative paths, as written in the model)
	std::vector<std::string> CompanionFiles(const fs::path& model)
	{
		std::vector<std::string> out;
		const std::string ext = Lower(model.extension().string());
		std::error_code ec;
		auto addIfLocal = [&](std::string rel) {
			rel.erase(0, rel.find_first_not_of(" \t\""));
			while (!rel.empty() && (rel.back() == '\r' || rel.back() == ' ' || rel.back() == '\t' || rel.back() == '"')) rel.pop_back();
			if (rel.empty() || rel.starts_with("data:") || fs::path(rel).is_absolute() || rel.find("..") != std::string::npos) return;
			if (fs::is_regular_file(model.parent_path() / rel, ec)) out.push_back(rel);
		};
		if (ext == ".obj") {
			// mtllib lines, then the texture maps of each material file
			std::ifstream obj(model);
			std::string line;
			std::vector<std::string> mtls;
			while (std::getline(obj, line)) {
				if (line.starts_with("mtllib ")) mtls.push_back(line.substr(7));
			}
			const fs::path sameName = model.stem().string() + ".mtl";
			if (mtls.empty() && fs::is_regular_file(model.parent_path() / sameName, ec)) mtls.push_back(sameName.string());
			for (std::string mtl : mtls) {
				addIfLocal(mtl);
				if (out.empty() || out.back().find(".mtl") == std::string::npos) continue;
				std::ifstream in(model.parent_path() / out.back());
				while (std::getline(in, line)) {
					const size_t key = line.find("map_");
					if (key == std::string::npos && !line.starts_with("bump ") && !line.starts_with("norm ")) continue;
					// The file name is the last token (options such as -bm 1.0 come first)
					const size_t lastSpace = line.find_last_of(" \t");
					if (lastSpace != std::string::npos) addIfLocal(line.substr(lastSpace + 1));
				}
			}
		} else if (ext == ".gltf") {
			std::ifstream in(model, std::ios::binary);
			std::stringstream buffer;
			buffer << in.rdbuf();
			if (auto json = SceneJson::Parse(buffer.str())) {
				for (const char* list : { "buffers", "images" }) {
					const SceneJson::Json* items = SceneJson::Find(*json, list);
					if (!items || !items->is_array()) continue;
					for (const auto& item : *items) {
						std::string uri;
						if (SceneJson::Read(item, "uri", uri)) addIfLocal(uri);
					}
				}
			}
		}
		return out;
	}
}

std::vector<std::string> EditorSuite::OpenNativeFilesDialog()
{
	std::vector<std::string> files;
	HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	const bool shouldUninit = SUCCEEDED(hrInit);
	{
		Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&dialog))) && dialog) {
			DWORD options = 0;
			if (SUCCEEDED(dialog->GetOptions(&options))) dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
			const COMDLG_FILTERSPEC filters[] = {
				{ L"Supported assets", L"*.png;*.jpg;*.jpeg;*.dds;*.tga;*.bmp;*.fbx;*.obj;*.gltf;*.glb;*.dae;*.wav;*.mp3;*.ogg;*.m4a;*.wma;*.json;*.csv" },
				{ L"Textures", L"*.png;*.jpg;*.jpeg;*.dds;*.tga;*.bmp" },
				{ L"3D models", L"*.fbx;*.obj;*.gltf;*.glb;*.dae" },
				{ L"Audio", L"*.wav;*.mp3;*.ogg;*.m4a;*.wma" },
				{ L"All files", L"*.*" },
			};
			dialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters);
			dialog->SetTitle(L"Import Assets");
			if (SUCCEEDED(dialog->Show(NULL))) {
				Microsoft::WRL::ComPtr<IShellItemArray> items;
				DWORD count = 0;
				if (SUCCEEDED(dialog->GetResults(&items)) && items && SUCCEEDED(items->GetCount(&count))) {
					for (DWORD i = 0; i < count; ++i) {
						Microsoft::WRL::ComPtr<IShellItem> item;
						PWSTR path = nullptr;
						if (SUCCEEDED(items->GetItemAt(i, &item)) && item && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
							files.push_back(ConvertStrings{}.ConvertString(std::wstring(path)));
							CoTaskMemFree(path);
						}
					}
				}
			}
		}
	}
	if (shouldUninit) CoUninitialize();
	return files;
}

fs::path EditorSuite::DefaultImportFolder(const fs::path& source) const
{
	if (currentProjectRoot_.empty()) return {};
	std::error_code ec;
	// The folder open in the Project panel wins when it is inside the project's Assets
	const std::string browsing = AssetDb().ToAssetPath(fs::absolute(currentDirectory_, ec).string());
	if (!browsing.empty() && browsing != "Assets") return currentProjectRoot_ / browsing;
	const bool isFolder = fs::is_directory(source, ec);
	const char* category = isFolder ? "" : CategoryFolder(FullExtension(source));
	return category[0] ? currentProjectRoot_ / "Assets" / category : currentProjectRoot_ / "Assets";
}

std::vector<std::string> EditorSuite::ImportAssets(const std::vector<fs::path>& sources, const fs::path& targetDir)
{
	std::vector<std::string> imported;
	if (currentProjectRoot_.empty()) {
		LogMessage("[Import] Open or create a project first.", 1);
		return imported;
	}
	if (sceneState_ != EditorSceneState::Edit) {
		LogMessage("[Import] Stop Play mode before importing assets.", 1);
		return imported;
	}
	AssetDatabase& db = AssetDb();
	std::error_code ec;
	for (const fs::path& raw : sources) {
		const fs::path source = fs::absolute(raw, ec);
		if (!fs::exists(source, ec)) {
			LogMessage(std::format("[Import] Not found: {}", source.string()), 1);
			continue;
		}
		if (AssetDatabase::IsMetaFile(source)) continue;
		if (!db.ToAssetPath(source.string()).empty()) {
			LogMessage(std::format("[Import] '{}' is already in the project's Assets folder.", source.filename().string()), 1);
			continue;
		}
		const fs::path dir = targetDir.empty() ? DefaultImportFolder(source) : targetDir;
		fs::create_directories(dir, ec);
		const fs::path target = UniqueTarget(dir, source.filename());

		if (fs::is_directory(source, ec)) {
			fs::copy(source, target, fs::copy_options::recursive, ec);
			// .meta files of the copy would duplicate GUIDs of another project: drop them, the database makes new ones
			for (auto it = fs::recursive_directory_iterator(target, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
				if (it->is_regular_file() && AssetDatabase::IsMetaFile(it->path())) fs::remove(it->path(), ec);
			}
		} else {
			fs::copy_file(source, target, ec);
			// Files the model loads from next to itself (kept at the same relative path, existing ones are not replaced)
			for (const std::string& rel : CompanionFiles(source)) {
				const fs::path companion = target.parent_path() / rel;
				if (fs::exists(companion, ec)) continue;
				fs::create_directories(companion.parent_path(), ec);
				fs::copy_file(source.parent_path() / rel, companion, ec);
			}
			// FBX exporters put external textures in <name>.fbm
			const fs::path fbm = source.parent_path() / (source.stem().string() + ".fbm");
			if (Lower(source.extension().string()) == ".fbx" && fs::is_directory(fbm, ec)) {
				const fs::path fbmTarget = target.parent_path() / (target.stem().string() + ".fbm");
				if (!fs::exists(fbmTarget, ec)) fs::copy(fbm, fbmTarget, fs::copy_options::recursive, ec);
			}
		}
		if (ec || !fs::exists(target)) {
			LogMessage(std::format("[Import] Could not copy '{}': {}", source.filename().string(), ec.message()), 2);
			ec.clear();
			continue;
		}
		imported.push_back(db.ToAssetPath(target.string()));
	}

	if (!imported.empty()) {
		db.Refresh(); // GUIDs (.meta) for everything that was copied, companions and folder contents included
		resolvedAssetCache_.clear();
		// Show the result in the Project panel
		currentDirectory_ = (currentProjectRoot_ / imported.back()).parent_path();
		selectedAssetPath_ = currentProjectRoot_ / imported.back();
		LogMessage(std::format("[Import] Imported {} item(s) into {}", imported.size(),
			db.ToAssetPath(currentDirectory_.string()).empty() ? std::string("Assets") : db.ToAssetPath(currentDirectory_.string())), 0);
	}
	return imported;
}

void EditorSuite::ProcessPendingImports()
{
	if (pendingImports_.empty()) return;
	std::vector<fs::path> sources;
	for (const std::string& dropped : pendingImports_) {
		const fs::path path(dropped);
		// A release zip dropped on the window is an engine update, not an asset
		if (_wcsicmp(path.extension().c_str(), L".zip") == 0 && EngineUpdate::ZipIsEnginePackage(path)) {
			BeginEngineUpdate(path);
		} else {
			sources.push_back(path);
		}
	}
	pendingImports_.clear();
	if (!sources.empty()) ImportAssets(sources);
}
