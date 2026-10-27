#include "EditorSuiteInternal.h"

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
	if (showProjectHub_) DrawProjectHub();

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
	// The UI font is loaded in DirectXCommon::Initialize: the font atlas is locked during a frame,
	// and EditorSuite is created mid-frame by SceneManager.
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.ChildRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.FramePadding = ImVec2(8.0f, 4.0f);
	style.ItemSpacing = ImVec2(8.0f, 5.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
	style.IndentSpacing = 16.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 10.0f;
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.SeparatorTextBorderSize = 1.0f;

	// Neutral dark greys with a single blue accent
	const ImVec4 bgDark     = ImVec4(0.098f, 0.098f, 0.106f, 1.0f); // #19191B
	const ImVec4 bgPanel    = ImVec4(0.137f, 0.137f, 0.145f, 1.0f); // #232325
	const ImVec4 bgRaised   = ImVec4(0.180f, 0.180f, 0.192f, 1.0f); // #2E2E31
	const ImVec4 bgHover    = ImVec4(0.235f, 0.235f, 0.251f, 1.0f); // #3C3C40
	const ImVec4 bgActive   = ImVec4(0.290f, 0.290f, 0.310f, 1.0f);
	const ImVec4 accent     = ImVec4(0.231f, 0.510f, 0.965f, 1.0f); // #3B82F6
	const ImVec4 accentSoft = ImVec4(0.231f, 0.510f, 0.965f, 0.35f);
	const ImVec4 border     = ImVec4(0.067f, 0.067f, 0.075f, 1.0f);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text]                 = ImVec4(0.902f, 0.902f, 0.918f, 1.0f);
	colors[ImGuiCol_TextDisabled]         = ImVec4(0.502f, 0.502f, 0.533f, 1.0f);
	colors[ImGuiCol_WindowBg]             = bgPanel;
	colors[ImGuiCol_ChildBg]              = bgDark;
	colors[ImGuiCol_PopupBg]              = ImVec4(0.118f, 0.118f, 0.126f, 0.98f);
	colors[ImGuiCol_Border]               = border;
	colors[ImGuiCol_BorderShadow]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_FrameBg]              = bgDark;
	colors[ImGuiCol_FrameBgHovered]       = bgHover;
	colors[ImGuiCol_FrameBgActive]        = bgActive;
	colors[ImGuiCol_TitleBg]              = bgDark;
	colors[ImGuiCol_TitleBgActive]        = bgDark;
	colors[ImGuiCol_TitleBgCollapsed]     = bgDark;
	colors[ImGuiCol_MenuBarBg]            = bgDark;
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_ScrollbarGrab]        = bgHover;
	colors[ImGuiCol_ScrollbarGrabHovered] = bgActive;
	colors[ImGuiCol_ScrollbarGrabActive]  = accent;
	colors[ImGuiCol_CheckMark]            = accent;
	colors[ImGuiCol_SliderGrab]           = ImVec4(0.420f, 0.420f, 0.450f, 1.0f);
	colors[ImGuiCol_SliderGrabActive]     = accent;
	colors[ImGuiCol_Button]               = bgRaised;
	colors[ImGuiCol_ButtonHovered]        = bgHover;
	colors[ImGuiCol_ButtonActive]         = accentSoft;
	colors[ImGuiCol_Header]               = bgRaised;
	colors[ImGuiCol_HeaderHovered]        = bgHover;
	colors[ImGuiCol_HeaderActive]         = accentSoft;
	colors[ImGuiCol_Separator]            = border;
	colors[ImGuiCol_SeparatorHovered]     = accentSoft;
	colors[ImGuiCol_SeparatorActive]      = accent;
	colors[ImGuiCol_ResizeGrip]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_ResizeGripHovered]    = accentSoft;
	colors[ImGuiCol_ResizeGripActive]     = accent;
	colors[ImGuiCol_Tab]                  = bgDark;
	colors[ImGuiCol_TabHovered]           = bgHover;
	colors[ImGuiCol_TabSelected]          = bgRaised;
	colors[ImGuiCol_TabSelectedOverline]  = accent;
	colors[ImGuiCol_TabDimmed]            = bgDark;
	colors[ImGuiCol_TabDimmedSelected]    = bgPanel;
	colors[ImGuiCol_TableHeaderBg]        = bgRaised;
	colors[ImGuiCol_TableBorderStrong]    = border;
	colors[ImGuiCol_TableBorderLight]     = border;
	colors[ImGuiCol_TextSelectedBg]       = accentSoft;
	colors[ImGuiCol_NavCursor]            = accent;
	colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
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
	const float screenW = io.DisplaySize.x;
	const float barH = kTopBarHeight;
	const float menuH = ImGui::GetFrameHeight();
	const ImVec2 buttonPadding = ImGui::GetStyle().FramePadding;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;

	ImGui::SetNextWindowPos(ImVec2(0.0f, menuH));
	ImGui::SetNextWindowSize(ImVec2(screenW, barH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	auto buttonWidth = [&](const char* label) {
		return ImGui::CalcTextSize(label, nullptr, true).x + buttonPadding.x * 2.0f;
	};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
	if (ImGui::Begin("##TopPlayBar", nullptr, flags)) {
		const float rowH = ImGui::GetFrameHeight();
		ImGui::SetCursorPosY((barH - rowH) * 0.5f);

		// Left: script build status as a small colored dot + label
		const bool building = isBuildingScripts_.load();
		const bool buildFailed = !building && lastScriptBuildStatus_.load() == 2;
		const ImVec4 statusColor = building ? ImVec4(1.0f, 0.78f, 0.25f, 1.0f)
			: buildFailed ? ImVec4(0.95f, 0.35f, 0.35f, 1.0f)
			: ImVec4(0.35f, 0.85f, 0.45f, 1.0f);
		const char* statusText = building ? "Building scripts..." : buildFailed ? "Script build failed" : "Scripts ready";
		{
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 p = ImGui::GetCursorScreenPos();
			dl->AddCircleFilled(ImVec2(p.x + 5.0f, p.y + rowH * 0.5f), 4.0f, ImGui::GetColorU32(statusColor));
			ImGui::Dummy(ImVec2(14.0f, rowH));
			ImGui::SameLine(0.0f, 2.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled("%s", statusText);
			if (buildFailed) ImGui::SetItemTooltip("See the Console tab for MSBuild errors");
		}

		// Center: Play / Pause / Step / Stop
		const bool isPlay = (sceneState_ == EditorSceneState::Play);
		const bool isPause = (sceneState_ == EditorSceneState::Pause);
		const bool inPlaySession = (isPlay || isPause);
		const float playW = 76.0f;
		const float smallW = 64.0f;
		const float groupW = playW + smallW * 3.0f + spacing * 3.0f;
		ImGui::SameLine((std::max)((screenW - groupW) * 0.5f, ImGui::GetCursorPosX() + 16.0f));

		if (inPlaySession) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.50f, 0.25f, 1.0f));
		if (inPlaySession) ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.30f, 1.0f));
		if (ImGui::Button(inPlaySession ? "Playing" : "Play", ImVec2(playW, 0.0f))) {
			if (!inPlaySession) {
				StartPlayMode();
			} else {
				StopPlayMode();
			}
		}
		if (inPlaySession) ImGui::PopStyleColor(2);
		ImGui::SetItemTooltip(inPlaySession ? "Stop play mode" : "Enter play mode");

		ImGui::SameLine();
		ImGui::BeginDisabled(!inPlaySession);
		if (isPause) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.42f, 0.12f, 1.0f));
		if (ImGui::Button(isPause ? "Resume" : "Pause", ImVec2(smallW, 0.0f))) {
			sceneState_ = isPlay ? EditorSceneState::Pause : EditorSceneState::Play;
		}
		if (isPause) ImGui::PopStyleColor();
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Pause / resume the running game");

		ImGui::SameLine();
		if (ImGui::Button("Step", ImVec2(smallW, 0.0f))) {
			if (sceneState_ == EditorSceneState::Edit) {
				StartPlayMode();
			}
			sceneState_ = EditorSceneState::Pause;
			stepSingleFrame_ = true;
		}
		ImGui::SetItemTooltip("Advance a single frame (enters play mode paused)");

		ImGui::SameLine();
		ImGui::BeginDisabled(!inPlaySession);
		if (ImGui::Button("Stop", ImVec2(smallW, 0.0f))) {
			StopPlayMode();
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Stop play mode and restore the scene");

		// Right: Build Scripts + Tools menu
		const char* buildLabel = building ? "Building..." : "Build Scripts";
		const char* toolsLabel = "Tools";
		const float rightW = buttonWidth(buildLabel) + buttonWidth(toolsLabel) + spacing;
		ImGui::SameLine((std::max)(screenW - rightW - 10.0f, ImGui::GetCursorPosX() + 16.0f));

		ImGui::BeginDisabled(building);
		if (ImGui::Button(buildLabel)) {
			TriggerScriptBuild();
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Compile GameScripts.dll in the background (Ctrl+Shift+B)");

		ImGui::SameLine();
		if (ImGui::Button(toolsLabel)) {
			ImGui::OpenPopup("##TopBarTools");
		}
		if (ImGui::BeginPopup("##TopBarTools")) {
			ImGui::MenuItem("Auto Build Scripts", nullptr, &autoBuildScripts_);
			ImGui::SetItemTooltip("Rebuild and hot-reload GameScripts.dll ~1s after saving a .cpp/.h in GameScripts/Scripts");
			ImGui::MenuItem("Simulate Physics in Edit Mode", nullptr, &simulatePhysicsInEditMode_);
			ImGui::Separator();
			if (ImGui::MenuItem("New C++ Script...")) {
				scriptGenTargetObjectIndex_ = selectedObjectIndex_;
				scriptGenErrorMsg_.clear();
				showScriptGenModal_ = true;
			}
			if (ImGui::MenuItem("Build / Export...")) {
				showBuildSettingsModal_ = true;
			}
			ImGui::EndPopup();
		}
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
	float topY = ImGui::GetFrameHeight() + kTopBarHeight;
	float leftW = screenW * 0.18f;
	if (leftW < 220.0f) leftW = 220.0f;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(0.0f, topY));
	ImGui::SetNextWindowSize(ImVec2(leftW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Hierarchy", nullptr, flags)) {
		// Sub-bar: creation button and search filter
		if (ImGui::Button("+ Create")) {
			ImGui::OpenPopup("HierarchyCreationPopup");
		}
		ImGui::SetItemTooltip("Create a new GameObject (or right-click empty space)");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##SearchHierarchy", "Search...", hierarchySearchBuf_, sizeof(hierarchySearchBuf_));

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

					const char* typeName = "Empty";
					if (obj.type == GameObjectType::Cube) typeName = "Cube";
					else if (obj.type == GameObjectType::Sphere) typeName = "Sphere";
					else if (obj.type == GameObjectType::Plane) typeName = "Plane";
					else if (obj.type == GameObjectType::Quad) typeName = "Quad";
					else if (obj.type == GameObjectType::Camera) typeName = "Camera";
					else if (obj.type == GameObjectType::DirectionalLight) typeName = "Light";
					else if (obj.type == GameObjectType::PointLight) typeName = "Point Light";
					else if (obj.type == GameObjectType::SpotLight) typeName = "Spot Light";
					else if (obj.type == GameObjectType::ParticleSystem) typeName = "Particles";
					else if (obj.type == GameObjectType::MapchipGrid) typeName = "Mapchip";
					else if (obj.type == GameObjectType::Sprite) typeName = "Sprite";
					else if (obj.type == GameObjectType::AudioSource || obj.hasAudioSource) typeName = "Audio";

					// Row layout: [ name ............ type  (x) ] -- active toggle on the right, type tag dimmed
					const float rowStartX = ImGui::GetCursorPosX();
					const float rowW = ImGui::GetContentRegionAvail().x;
					const float toggleSize = ImGui::GetFontSize();
					const float typeW = ImGui::CalcTextSize(typeName).x;

					ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (static_cast<int>(i) == selectedObjectIndex_) nodeFlags |= ImGuiTreeNodeFlags_Selected;

					if (!obj.isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
					ImGui::SetNextItemAllowOverlap();
					ImGui::TreeNodeEx("##Row", nodeFlags, "%s", obj.name.c_str());
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

					const float toggleX = rowStartX + rowW - toggleSize - 2.0f;
					const float typeX = toggleX - typeW - ImGui::GetStyle().ItemSpacing.x;
					if (typeX > rowStartX + ImGui::CalcTextSize(obj.name.c_str()).x + 24.0f) {
						ImGui::SameLine(typeX);
						ImGui::TextDisabled("%s", typeName);
					}
					ImGui::SameLine(toggleX);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
					if (ImGui::Checkbox("##Active", &obj.isActive)) {
						isSceneDirty_ = true;
						PushUndoSnapshot();
					}
					ImGui::PopStyleVar();
					ImGui::SetItemTooltip(obj.isActive ? "Active (click to disable)" : "Inactive (click to enable)");

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
	float topY = ImGui::GetFrameHeight() + kTopBarHeight;
	float leftW = screenW * 0.18f;
	if (leftW < 220.0f) leftW = 220.0f;
	float rightW = screenW * 0.24f;
	if (rightW < 300.0f) rightW = 300.0f;
	float centerW = screenW - leftW - rightW;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(leftW, topY));
	ImGui::SetNextWindowSize(ImVec2(centerW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Viewport", nullptr, flags)) {
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

