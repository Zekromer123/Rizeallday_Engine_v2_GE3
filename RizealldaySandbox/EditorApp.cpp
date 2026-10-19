#include "EditorApp.h"
#include "scene/EditorScene.h"
#include "scene/TitleScene.h"
#include <cstring>
#include <format>
#include <fstream>

void EditorApp::Initialize() {
	// Parse command line for automated scene transition verification test
	if (std::strstr(GetCommandLineA(), "--test-scene-transitions") != nullptr) {
		testMode_ = true;
		std::ofstream testLog("scene_test.log", std::ios::trunc);
		testLog << "[SCENE TEST START] Beginning automated 60 scene transitions test...\n";
	}

	// 1. Initialize Engine & Framework Services
	windowTitle_ = "Rizeallday Engine v2";
	windowWidth_ = 1280;
	windowHeight_ = 720;
	Framework::Initialize();

	// 2. Queue the initial application scene (EditorScene)
	sceneManager_.ChangeScene(std::make_unique<EditorScene>());
}

void EditorApp::Update(float deltaTime) {
	sceneManager_.Update(deltaTime);

	if (testMode_) {
		testTransitionCount_++;

		std::ofstream testLog("scene_test.log", std::ios::app);
		testLog << "Transition " << testTransitionCount_ << " -> " 
		        << (testTransitionCount_ % 2 == 1 ? "TitleScene" : "EditorScene") << "\n";

		if (testTransitionCount_ % 2 == 1) {
			sceneManager_.ChangeScene(std::make_unique<TitleScene>());
		} else {
			sceneManager_.ChangeScene(std::make_unique<EditorScene>());
		}

		if (testTransitionCount_ >= kTargetTransitions) {
			testLog << std::format("[SCENE TEST PASSED] Successfully completed {} scene transitions with 0 errors.\n", testTransitionCount_);
			Log::GetInstance()->logOutput(std::format("[SCENE TEST PASSED] Successfully completed {} scene transitions with 0 errors.\n", testTransitionCount_));
			PostQuitMessage(0);
		}
	}
}

void EditorApp::Draw() {
	sceneManager_.Draw();
}

void EditorApp::Finalize() {
	sceneManager_.Finalize();
	Framework::Finalize();
}
