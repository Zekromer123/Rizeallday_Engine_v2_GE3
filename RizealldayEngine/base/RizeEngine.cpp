#include "base/RizeEngine.h"
#include "base/DirectXCommon.h"
#include "base/WinApp.h"
#include "base/DXCManager.h"
#include "base/GraphicPipeline.h"
#include "base/TextureManager.h"
#include "base/Log.h"
#include "base/CrashHandler.h"
#include "input/Input.h"
#include "audio/Audio.h"
#include "3d/Model.h"
#include "3d/Object3d.h"
#include "3d/WorldTransform.h"
#include "3d/LightGroup.h"
#include "3d/PrimitiveDrawer.h"

namespace {
	WinApp* sWinApp = nullptr;
	DirectXCommon* sDxCommon = nullptr;
	DXCManager* sDxcManager = nullptr;
	GraphicPipeline* sGraphicPipeline = nullptr;
	Input* sInput = nullptr;
	Audio* sAudio = nullptr;
}

namespace RizeEngine {

void Initialize(const char* title, int width, int height) {
	// Crash Handler
	CrashHandler::GetInstance()->Initialize();

	// Window
	sWinApp = WinApp::GetInstance();
	sWinApp->CreateGameWindow(title, width, height);

	// DirectX
	sDxCommon = DirectXCommon::GetInstance();
	sDxCommon->Initialize(sWinApp->GetHWND(), width, height);

	// DXC Shader Compiler
	sDxcManager = new DXCManager();
	sDxcManager->Initialize();

	// Graphics Pipeline
	sGraphicPipeline = new GraphicPipeline();
	sGraphicPipeline->Initialize(sDxCommon->GetDevice(), sDxcManager);

	// Texture Manager
	TextureManager::GetInstance()->Initialize(sDxCommon);

	// Input
	sInput = Input::GetInstance();
	sInput->Initialize(sWinApp->GetHInstance(), sWinApp->GetHWND());

	// Audio
	sAudio = Audio::GetInstance();
	sAudio->Initialize();

	// Static Initialization for 3D systems
	Model::StaticInitialize(sDxCommon);
	Object3d::StaticInitialize(sDxCommon);
	WorldTransform::StaticInitialize(sDxCommon);
	LightGroup::StaticInitialize(sDxCommon);
	PrimitiveDrawer::GetInstance()->Initialize(sDxCommon);

	Log::GetInstance()->logOutput("Rizeallday Engine v2 initialized.\n");
}

bool ProcessMessage() {
	return !sWinApp->ProcessMessage();
}

void BeginFrame() {
	// Input update
	sInput->Update();

	// PreDraw (clears screen, sets render targets, starts ImGui frame)
	sDxCommon->PreDraw();

	// Bind the graphics pipeline
	sGraphicPipeline->Bind(sDxCommon->GetCommandList());
}

void EndFrame() {
	// PostDraw (renders ImGui, presents)
	sDxCommon->PostDraw();
}

void Finalize() {
	// Wait for GPU to finish
	sDxCommon->SignalAndWaitFence();

	// Cleanup
	sAudio->Finalize();

	delete sGraphicPipeline;
	sGraphicPipeline = nullptr;
	delete sDxcManager;
	sDxcManager = nullptr;

	sDxCommon->Finalize(sWinApp->GetHWND());
	sWinApp->TerminateGameWindow();

	Log::GetInstance()->logOutput("Rizeallday Engine v2 finalized.\n");
}

DirectXCommon* GetDirectXCommon() {
	return sDxCommon;
}

Input* GetInput() {
	return sInput;
}

Audio* GetAudio() {
	return sAudio;
}

} // namespace RizeEngine
