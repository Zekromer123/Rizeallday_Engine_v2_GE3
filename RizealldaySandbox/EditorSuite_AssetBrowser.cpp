#include "EditorSuiteInternal.h"

using EditorSuiteDetail::SceneNameFromPath;
using EditorSuiteDetail::IsSceneFile;

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
	float topY = ImGui::GetFrameHeight() + kTopBarHeight;
	float midH = (screenH - topY) * 0.65f;
	float bottomH = screenH - topY - midH;

	ImGui::SetNextWindowPos(ImVec2(0.0f, topY + midH));
	ImGui::SetNextWindowSize(ImVec2(screenW, bottomH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Project / Console", nullptr, flags)) {
		if (ImGui::BeginTabBar("BottomPanelTabs")) {
			// TAB 1: PROJECT (Asset Browser with Subfolder Navigation)
			if (ImGui::BeginTabItem("Project")) {
				bottomPanelTab_ = 0;

				if (ImGui::Button("< Back")) {
					std::filesystem::path parent = currentDirectory_.parent_path();
					if (!parent.empty() && parent != currentDirectory_) {
						currentDirectory_ = parent;
					} else if (currentDirectory_ != ".") {
						currentDirectory_ = ".";
					}
				}
				ImGui::SetItemTooltip("Navigate to parent directory");

				ImGui::SameLine();
				if (ImGui::Button("Root")) {
					currentDirectory_ = ".";
				}
				ImGui::SetItemTooltip("Browse workspace root (Resources, shaders, Assets)");

				ImGui::SameLine();
				if (ImGui::Button("Resources")) {
					if (std::filesystem::exists("Resources")) currentDirectory_ = "Resources";
				}
				ImGui::SameLine();
				if (ImGui::Button("Scripts")) {
					std::filesystem::path scriptsDir = ResolveGameScriptsDir() / "Scripts";
					if (std::filesystem::exists(scriptsDir)) {
						currentDirectory_ = scriptsDir;
					}
				}
				ImGui::SetItemTooltip("Browse GameScripts/Scripts (.cpp / .h)");

				ImGui::SameLine();
				if (ImGui::Button("shaders")) {
					if (std::filesystem::exists("shaders")) currentDirectory_ = "shaders";
					else if (std::filesystem::exists("../RizealldayEngine/shaders")) currentDirectory_ = "../RizealldayEngine/shaders";
				}
				if (!currentProjectRoot_.empty() && std::filesystem::exists(currentProjectRoot_)) {
					ImGui::SameLine();
					if (ImGui::Button("Project Root")) {
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
									if (IsSceneFile(entry.path()) && !IsPathInsideProject(entry.path())) {
										LogMessage(std::format("[Scene] '{}' is outside the current project and cannot be opened.", filename), 1);
									} else if (IsSceneFile(entry.path())) {
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

