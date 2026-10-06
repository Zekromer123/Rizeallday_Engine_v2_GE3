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

	// ===== 4.5. Assignment 01_00: 3D Particle System (Instancing + Billboard + Wind) =====
	DXCManager dxc;
	dxc.Initialize();
	uint32_t circleTextureHandle = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
	ParticleQuad particleQuads;
	particleQuads.Initialize(RizeEngine::GetDirectXCommon(), &dxc, circleTextureHandle, 100);
	bool showParticleQuads = true;

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

		particleQuads.Update(deltaTime, camera);

		lights->Update();

#ifdef USE_IMGUI
		// --- ImGui Debug Panel ---
		ImGui::Begin("Rizeallday Engine v2 - Control Panel");
		ImGui::Text("FPS: %.1f (dt: %.4f s)", ImGui::GetIO().Framerate, deltaTime);
		ImGui::Separator();

		// ===== Particle System Settings =====
		ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Particle System Settings");
		ImGui::Checkbox("Show Particles", &showParticleQuads);
		if (showParticleQuads) {
			ImGui::Checkbox("Update Particles", &particleQuads.isUpdate);
			ImGui::Checkbox("Use Billboard", &particleQuads.useBillboard);

			// Blend Mode Selector
			const char* blendModes[] = { "Normal (AlphaBlend)", "Add (Additive Glow)" };
			int currentBlend = (particleQuads.blendType == ParticleQuad::BlendType::Add) ? 1 : 0;
			if (ImGui::Combo("Blend Mode", &currentBlend, blendModes, 2)) {
				particleQuads.blendType = (currentBlend == 1) ? ParticleQuad::BlendType::Add : ParticleQuad::BlendType::Normal;
			}

			ImGui::ColorEdit4("Global Color", &particleQuads.globalColor.x);
			ImGui::Text("Active Particles: %u / %u", particleQuads.GetActiveCount(), particleQuads.GetMaxParticles());

			// Emitter Controls
			if (ImGui::CollapsingHeader("Emitter Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Auto Emit", &particleQuads.emitter.emit);
				ImGui::DragFloat3("Emitter Pos", &particleQuads.emitter.translate.x, 0.1f);
				ImGui::DragFloat3("Emitter Radius", &particleQuads.emitter.radius.x, 0.1f, 0.0f, 10.0f);
				int emitCount = static_cast<int>(particleQuads.emitter.count);
				if (ImGui::DragInt("Spawn Count", &emitCount, 1.0f, 1, 20)) {
					particleQuads.emitter.count = static_cast<uint32_t>(emitCount);
				}
				ImGui::DragFloat("Spawn Frequency", &particleQuads.emitter.frequency, 0.01f, 0.01f, 2.0f);
				if (ImGui::Button("Burst (10 Particles)")) {
					particleQuads.EmitBurst(10);
				}
			}

			// Wind / Acceleration Field Controls
			if (ImGui::CollapsingHeader("Wind Field", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Enable Wind Field", &particleQuads.windField.enabled);
				ImGui::DragFloat3("Wind Acceleration", &particleQuads.windField.acceleration.x, 0.5f);
				ImGui::DragFloat3("Field Area Min", &particleQuads.windField.area.min.x, 0.5f);
				ImGui::DragFloat3("Field Area Max", &particleQuads.windField.area.max.x, 0.5f);
			}
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
