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
#include "3d/Camera.h"
#include "3d/LightGroup.h"
#include "3d/PrimitiveDrawer.h"
#include "3d/CircleShadow.h"

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
	// Logger
	Log::GetInstance()->saveLogs();

	// Crash Handler
	CrashHandler::GetInstance()->Initialize();

	// Window
	sWinApp = WinApp::GetInstance();
	sWinApp->CreateGameWindow(title, width, height);

	int clientW = sWinApp->GetClientWidth();
	int clientH = sWinApp->GetClientHeight();

	// DirectX
	sDxCommon = DirectXCommon::GetInstance();
	sDxCommon->Initialize(sWinApp->GetHWND(), clientW, clientH);

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
	Camera::StaticInitialize(sDxCommon);
	LightGroup::StaticInitialize(sDxCommon);
	PrimitiveDrawer::GetInstance()->Initialize(sDxCommon);

	// Daftarkan pipeline ke Object3d agar Draw() bisa bind blend mode
	Object3d::StaticSetPipeline(sGraphicPipeline);

	// Inisialisasi Circle Shadow system
	CircleShadow::StaticInitialize(sDxCommon, sDxcManager);

	Log::GetInstance()->logOutput("Rizeallday Engine v2 initialized.\n");
}

bool ProcessMessage() {
	return !sWinApp->ProcessMessage();
}

static bool sFrameActive = false;

void BeginFrame() {
	// Input update
	sInput->Update();

	if (!sDxCommon || sDxCommon->IsDeviceLost()) {
		sFrameActive = false;
		return;
	}

	sFrameActive = true;

	// PreDraw (clears screen, sets render targets, starts ImGui frame)
	sDxCommon->PreDraw();

	// Bind the graphics pipeline
	sGraphicPipeline->Bind(sDxCommon->GetCommandList());

	// Set Descriptor Heaps for textures
	TextureManager::GetInstance()->SetDescriptorHeaps(sDxCommon->GetCommandList());
}

void EndFrame() {
	if (!sDxCommon || !sFrameActive) {
		return;
	}
	sFrameActive = false;
	// PostDraw (renders ImGui, presents)
	sDxCommon->PostDraw();
}

void Finalize() {
	// Wait for GPU to finish
	if (sDxCommon && !sDxCommon->IsDeviceLost()) {
		sDxCommon->SignalAndWaitFence();
	}

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
