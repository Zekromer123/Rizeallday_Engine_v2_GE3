#include <RizeEngine.h>
#include "ParticleQuad.h"
#include "EditorSuite.h"
#include <chrono>
#include <algorithm>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ===== 1. Initialize Engine =====
	RizeEngine::Initialize("Rizeallday Engine v2", 1280, 720);

	// ===== 2. Setup Game Camera =====
	Camera gameCamera;
	gameCamera.Initialize();
	gameCamera.translation = { 0.0f, 1.5f, -8.0f };
	gameCamera.rotation = { 0.15f, 0.0f, 0.0f };

	// ===== 3. Lighting Setup =====
	LightGroup* lights = LightGroup::Create();
	lights->SetDefaultLighting();
	lights->SetAmbientColor({ 0.25f, 0.25f, 0.3f });
	lights->SetAmbientIntensity(0.6f);
	lights->GetDirectionalLight().direction = { 0.5f, -1.0f, 0.5f };
	lights->GetDirectionalLight().color = { 1.0f, 0.95f, 0.9f, 1.0f };

	// ===== 4. 3D Particle System (Instancing + Billboard + Wind) =====
	DXCManager dxc;
	dxc.Initialize();
	uint32_t circleTextureHandle = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
	ParticleQuad particleQuads;
	particleQuads.Initialize(RizeEngine::GetDirectXCommon(), &dxc, circleTextureHandle, 100);
	bool showParticleQuads = true;

	// ===== 5. Initialize Hybrid Hazel/Kamata Editor Suite =====
	EditorSuite editor;
	editor.Initialize(RizeEngine::GetDirectXCommon(), &gameCamera, lights, &particleQuads);

	auto lastTime = std::chrono::high_resolution_clock::now();

	// ===== 6. Main Loop =====
	while (RizeEngine::ProcessMessage()) {
		if (RizeEngine::GetDirectXCommon()->IsDeviceLost()) {
			Sleep(16);
			continue;
		}

		RizeEngine::BeginFrame();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
		deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

		Input* input = RizeEngine::GetInput();

		// Update Editor & Cameras
		editor.Update(deltaTime, input);
		lights->Update();

		// Particle Physics Simulation
		if (editor.HasActiveParticleSystem()) {
			particleQuads.Simulate(deltaTime);
		}

		// Render Offscreen Viewports (Edit View & Game View) then restore to Main BackBuffer
		auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
		editor.RenderViewports(cmdList, showParticleQuads);

		// Draw ImGui Editor Workspace
		editor.DrawEditorUI(deltaTime, showParticleQuads);

		RizeEngine::EndFrame();
	}

	// ===== 7. Finalize =====
	RizeEngine::Finalize();
	return 0;
}
