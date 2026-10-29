#include "EditorApp.h"
#include "scene/EditorScene.h"
#include "scene/TitleScene.h"
#include "EditorSuite.h"
#include "base/EngineVersion.h"
#include <cstring>
#include <filesystem>
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
	windowTitle_ = std::string("Rizeallday Engine ") + kEngineVersion;
	windowWidth_ = 1280;
	windowHeight_ = 720;
	Framework::Initialize();

	// 2. Queue the initial application scene:
	//    - If standalone exported game config exists or running automated scene transition test, open EditorScene directly.
	//    - Otherwise start with TitleScene (3.2s Rizeallday Engine splash screen -> transitions to EditorScene).
	// The exporter writes game_build.cfg next to the exe (this used to look for "BuildConfig.json", which no
	// exporter ever wrote, so exported games still showed the editor splash)
	bool isExportedGame = EditorSuite::IsExportedGameLaunch() ||
	                      (std::strstr(GetCommandLineA(), "--standalone") != nullptr);
	// Headless self-tests (EditorSuite_Tests.cpp) are only compiled into editor builds
	bool runSelfTests = false;
#ifdef USE_IMGUI
	runSelfTests = std::strstr(GetCommandLineA(), "--run-tests") != nullptr;
#endif
	if (testMode_ || runSelfTests || isExportedGame) {
		sceneManager_.ChangeScene(std::make_unique<EditorScene>());
	} else {
		sceneManager_.ChangeScene(std::make_unique<TitleScene>());
	}
}

int EditorApp::GetExitCode() {
	return EditorSuite::sSelfTestExitCode;
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
