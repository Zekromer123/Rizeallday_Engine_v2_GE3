#include "EditorSuiteInternal.h"
#include "base/GraphicPipeline.h"

// Shader hot reload (editor builds): save an .hlsl and the Object3d pipeline is rebuilt within half a second.
#ifdef USE_IMGUI

namespace fs = std::filesystem;

namespace {
	/// Where shaders are edited: RizealldayEngine/shaders in the source tree (the run folder's shaders/ is a build copy), else ./shaders
	fs::path DefaultShaderSourceDir()
	{
		std::error_code ec;
		const fs::path source = ScriptModule::ResolveWorkspaceRoot() / "RizealldayEngine" / "shaders";
		return fs::is_directory(source, ec) ? source : fs::path("shaders");
	}
}

bool EditorSuite::ReloadShaders(const fs::path& dir)
{
	GraphicPipeline* pipeline = RizeEngine::GetGraphicPipeline();
	if (!pipeline) return false;
	std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> retired;
	std::string error;
	if (!pipeline->Reload(&dxc_, dir.wstring(), retired, &error)) {
		LogMessage("[Shaders] Reload failed, keeping the previous shaders:\n" + error, 2);
		return false;
	}
	for (auto& pso : retired) dxCommon_->DeferRelease(pso); // frames in flight may still use them
	++shaderReloadCount_;
	LogMessage("[Shaders] Object3d shaders reloaded.", 0);
	return true;
}

void EditorSuite::PollShaderWatcher(float deltaTime)
{
	shaderPollTimer_ += deltaTime;
	if (shaderPollTimer_ < 0.5f) return;
	shaderPollTimer_ = 0.0f;

	const fs::path dir = shaderWatchDirOverride_.empty() ? DefaultShaderSourceDir() : shaderWatchDirOverride_;
	const fs::path deploy = shaderDeployDirOverride_.empty() ? fs::path("shaders") : shaderDeployDirOverride_;
	std::error_code ec;
	std::map<std::string, fs::file_time_type> stamps;
	for (const auto& entry : fs::directory_iterator(dir, ec)) {
		const std::string ext = entry.path().extension().string();
		if (entry.is_regular_file(ec) && (ext == ".hlsl" || ext == ".hlsli")) stamps[entry.path().filename().string()] = fs::last_write_time(entry.path(), ec);
	}
	if (!shaderWatcherInitialized_) {
		shaderStamps_ = std::move(stamps);
		shaderWatcherInitialized_ = true;
		return;
	}
	if (stamps == shaderStamps_) return;

	std::vector<std::string> changed;
	for (const auto& [name, time] : stamps) {
		auto it = shaderStamps_.find(name);
		if (it == shaderStamps_.end() || it->second != time) changed.push_back(name);
	}
	shaderStamps_ = std::move(stamps);

	// Keep the run folder (and so builds / exports) in step with the edited sources
	const bool separateDeploy = !fs::equivalent(dir, deploy, ec);
	if (separateDeploy) {
		fs::create_directories(deploy, ec);
		for (const std::string& name : changed) fs::copy_file(dir / name, deploy / name, fs::copy_options::overwrite_existing, ec);
	}
	bool affectsObject3d = false;
	for (const std::string& name : changed) {
		if (name.starts_with("Object3d")) affectsObject3d = true;
		else LogMessage("[Shaders] " + name + " changed: only the Object3d shaders reload live; restart the editor for this one.", 1);
	}
	if (affectsObject3d) ReloadShaders(dir);
}

#endif
