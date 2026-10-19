#include "scene/TitleScene.h"
#include "scene/EditorScene.h"
#include "scene/SceneManager.h"
#include <imgui.h>

void TitleScene::Initialize() {
}

void TitleScene::Finalize() {
}

void TitleScene::Update(float deltaTime) {
	(void)deltaTime;
}

void TitleScene::Draw() {
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(480.0f, 240.0f), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
	                         ImGuiWindowFlags_NoCollapse |
	                         ImGuiWindowFlags_NoMove;

	if (ImGui::Begin("Rizeallday Engine - Application State Demo", nullptr, flags)) {
		ImGui::Dummy(ImVec2(0.0f, 15.0f));
		ImGui::SetWindowFontScale(1.4f);
		const char* titleText = "Rizeallday Engine v2";
		float titleWidth = ImGui::CalcTextSize(titleText).x;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s", titleText);
		ImGui::SetWindowFontScale(1.0f);

		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		const char* subText = "State Pattern Scene Management Demo (TitleScene)";
		float subWidth = ImGui::CalcTextSize(subText).x;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - subWidth) * 0.5f);
		ImGui::TextDisabled("%s", subText);

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		const char* descText = "Click below to transition to EditorScene";
		float descWidth = ImGui::CalcTextSize(descText).x;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - descWidth) * 0.5f);
		ImGui::Text("%s", descText);

		ImGui::Dummy(ImVec2(0.0f, 25.0f));
		float buttonWidth = 180.0f;
		float buttonHeight = 42.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - buttonWidth) * 0.5f);
		if (ImGui::Button("[ Open Editor ]", ImVec2(buttonWidth, buttonHeight))) {
			if (sceneManager_) {
				sceneManager_->ChangeScene(std::make_unique<EditorScene>());
			}
		}
	}
	ImGui::End();
#endif
}
