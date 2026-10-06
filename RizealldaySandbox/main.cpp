#include <RizeEngine.h>
#include "ParticleQuad.h"
#include <chrono>
#include <algorithm>

enum class CharacterState {
	Idle,
	Running,
	Attacking
};

/// Entry point for Windows application (main function)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ===== 1. Initialize Engine =====
	RizeEngine::Initialize("Rizeallday Engine v2 - 3D Particle Instancing Test", 1280, 720);

	// ===== 2. Setup Camera =====
	Camera camera;
	camera.Initialize();
	camera.translation = { 0.0f, 1.5f, -8.0f };
	camera.rotation = { 0.15f, 0.0f, 0.0f };
	float cameraRotateSpeed = 1.5f;

	// ===== 3. Lighting Setup =====
	LightGroup* lights = LightGroup::Create();
	lights->SetDefaultLighting();
	lights->SetAmbientColor({ 0.25f, 0.25f, 0.3f });
	lights->SetAmbientIntensity(0.6f);
	lights->GetDirectionalLight().direction = { 0.5f, -1.0f, 0.5f };
	lights->GetDirectionalLight().color = { 1.0f, 0.95f, 0.9f, 1.0f };

	// ===== 4.5. Assignment 01_00: 10 Quad Instancing (circle.png) =====
	DXCManager dxc;
	dxc.Initialize();
	uint32_t circleTextureHandle = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
	ParticleQuad particleQuads;
	particleQuads.Initialize(RizeEngine::GetDirectXCommon(), &dxc, circleTextureHandle, 10);
	bool showParticleQuads = true;
	bool showCharacter = true;


	auto lastTime = std::chrono::high_resolution_clock::now();

	// ===== 5. Game Loop =====
	while (RizeEngine::ProcessMessage()) {
		RizeEngine::BeginFrame();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
		deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

		Input* input = RizeEngine::GetInput();

		// Camera rotation controls
		if (input->PushKey(DIK_Q)) {
			camera.rotation.y -= cameraRotateSpeed * deltaTime;
		}
		if (input->PushKey(DIK_E)) {
			camera.rotation.y += cameraRotateSpeed * deltaTime;
		}

		// Update Camera
		camera.Update();

		particleQuads.Update(camera.GetViewProjectionMatrix());

		lights->Update();

#ifdef USE_IMGUI
		// --- ImGui Debug Panel ---
		ImGui::Begin("Rizeallday Engine v2 - Control Panel");
		ImGui::Text("FPS: %.1f (dt: %.4f s)", ImGui::GetIO().Framerate, deltaTime);
		ImGui::Separator();

		// ===== Assignment 01_00: 10 Quad Instancing =====
		ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Assignment 01_00: 10 Quad Instancing");
		ImGui::Checkbox("Show 10 Quad Instancing (circle.png)", &showParticleQuads);
		ImGui::Checkbox("Show FBX Character", &showCharacter);
		if (showParticleQuads) {
			ImGui::ColorEdit4("Particle Color", &particleQuads.color.x);
		}

		ImGui::Separator();
		ImGui::Text("Camera Controls:");
		ImGui::DragFloat3("Camera Position", &camera.translation.x, 0.1f);
		ImGui::DragFloat3("Camera Rotation", &camera.rotation.x, 0.02f);
		ImGui::BulletText("Hold [Q] / [E] to Rotate Camera Yaw");





		ImGui::Separator();
		ImGui::ColorEdit4("Light Color", &lights->GetDirectionalLight().color.x);
		ImGui::DragFloat3("Light Direction", &lights->GetDirectionalLight().direction.x, 0.05f);

		ImGui::End();
#endif

		// --- Render ---
		auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
		lights->Draw(cmdList, 3);

		if (showParticleQuads) {
			particleQuads.Draw(cmdList);
		}

		RizeEngine::EndFrame();
	}

	// ===== 6. Finalize =====
	RizeEngine::Finalize();
	return 0;
}
