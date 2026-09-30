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

	// ===== 4. Load FBX Models =====
	Model* modelIdle = Model::CreateFromFile("Resources/Model", "Idle.fbx");
	Model* modelRun = Model::CreateFromFile("Resources/Model", "Running.fbx");
	Model* modelCombo = Model::CreateFromFile("Resources/Model", "Two Hand Sword Combo.fbx");

	// Enable Lock Root Motion (In-Place) to prevent run & combo animation from jumping
	if (modelRun) modelRun->SetLockRootMotion(true);
	if (modelCombo) modelCombo->SetLockRootMotion(true);

	// Setup Character Object
	Object3d character;
	character.Initialize();
	character.SetModel(modelIdle); // Default: Idle
	character.SetCamera(&camera);
	character.worldTransform.translation = { 0.0f, -1.5f, 0.0f };
	character.worldTransform.rotation.y = 3.14159f; // Face camera at start
	character.worldTransform.scale = { 0.006f, 0.006f, 0.006f }; // Mixamo scale

	// Verify bone animation setup
	if (modelIdle && modelIdle->HasBones()) {
		Log::GetInstance()->logOutput(std::format("Skeletal Animation Ready: Duration = {:.2f}s, Bones = {}\n",
			modelIdle->GetAnimationDuration(0), modelIdle->GetBoneCount()));
	}

	// ===== Circle Shadow =====
	CircleShadow shadow;
	shadow.Initialize();
	shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f }; // Black, 45% opacity
	shadow.GetMaterial()->softness = 0.5f;
	float shadowRadius = 0.3f;

	// ===== 4.5. Assignment 01_00: 10 Quad Instancing (circle.png) =====
	DXCManager dxc;
	dxc.Initialize();
	uint32_t circleTextureHandle = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
	ParticleQuad particleQuads;
	particleQuads.Initialize(RizeEngine::GetDirectXCommon(), &dxc, circleTextureHandle, 10);
	bool showParticleQuads = true;
	bool showCharacter = true;

	// Character State Variables
	CharacterState currentState = CharacterState::Idle;
	float attackTimer = 0.0f;
	bool manualMode = false;
	int selectedModelIndex = 0; // 0 = Idle, 1 = Running, 2 = Combo

	auto lastTime = std::chrono::high_resolution_clock::now();

	// ===== 5. Game Loop =====
	while (RizeEngine::ProcessMessage()) {
		RizeEngine::BeginFrame();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
		deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

		Input* input = RizeEngine::GetInput();

		if (!manualMode) {
			// --- AUTOMATIC GAMEPLAY MODE ---
			// Attack timer countdown
			if (attackTimer > 0.0f) {
				attackTimer -= deltaTime;
				currentState = CharacterState::Attacking;
				character.SetModel(modelCombo);
			}
			else {
				// Movement Input (WASD)
				Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
				Vector3 rawInput = { 0.0f, 0.0f, 0.0f };
				if (input->PushKey(DIK_W)) { rawInput.z += 1.0f; }
				if (input->PushKey(DIK_S)) { rawInput.z -= 1.0f; }
				if (input->PushKey(DIK_A)) { rawInput.x -= 1.0f; }
				if (input->PushKey(DIK_D)) { rawInput.x += 1.0f; }

				// Attack Input: Press J or SPACE
				if (input->TriggerKey(DIK_J) || input->TriggerKey(DIK_SPACE)) {
					attackTimer = modelCombo ? modelCombo->GetAnimationDuration(0) : 1.2f;
					currentState = CharacterState::Attacking;
					character.SetModel(modelCombo);
				}
				else if (rawInput.Length() > 0.001f) {
					// Character is running
					currentState = CharacterState::Running;
					character.SetModel(modelRun);

					rawInput = rawInput.Normalize();
					float yaw = camera.rotation.y;
					moveDir = {
						rawInput.x * std::cos(yaw) + rawInput.z * std::sin(yaw),
						0.0f,
						-rawInput.x * std::sin(yaw) + rawInput.z * std::cos(yaw)
					};
					float runSpeed = 4.5f * deltaTime;
					character.worldTransform.translation += moveDir * runSpeed;

					// Face character in moving direction
					character.worldTransform.rotation.y = std::atan2(moveDir.x, moveDir.z);
				}
				else {
					// Character is idle
					currentState = CharacterState::Idle;
					character.SetModel(modelIdle);
				}
			}
		}
		else {
			// --- MANUAL INSPECTOR MODE (Select via ImGui) ---
			if (selectedModelIndex == 0) character.SetModel(modelIdle);
			else if (selectedModelIndex == 1) character.SetModel(modelRun);
			else if (selectedModelIndex == 2) character.SetModel(modelCombo);
		}

		// Camera rotation controls
		if (input->PushKey(DIK_Q)) {
			camera.rotation.y -= cameraRotateSpeed * deltaTime;
		}
		if (input->PushKey(DIK_E)) {
			camera.rotation.y += cameraRotateSpeed * deltaTime;
		}

		// Update Camera
		camera.Update();

		if (showCharacter) {
			character.Update(deltaTime);
		}
		if (showParticleQuads) {
			particleQuads.Update(camera.GetViewProjectionMatrix());
		}
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

		if (showCharacter) {
			ImGui::Separator();
			const char* stateStr = "Idle";
			if (currentState == CharacterState::Running) stateStr = "Running (WASD)";
			if (currentState == CharacterState::Attacking) stateStr = "Attacking (J / Space)";
			ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Current State: %s", stateStr);

			ImGui::Separator();
			ImGui::Text("FBX Animation:");
			if (character.GetModel()) {
				ImGui::Text("Bone Count: %zu", character.GetModel()->GetBoneCount());
				ImGui::Text("Duration: %.2f sec", character.GetAnimationDuration());
				ImGui::Text("Current Time: %.2f sec", character.GetAnimationTime());
			}
			float animSpeed = character.GetAnimationSpeed();
			if (ImGui::SliderFloat("Animation Speed", &animSpeed, 0.0f, 3.0f, "%.2fx")) {
				character.SetAnimationSpeed(animSpeed);
			}

			bool lockRoot = modelRun ? modelRun->GetLockRootMotion() : true;
			if (ImGui::Checkbox("Lock Root Motion (In-Place)", &lockRoot)) {
				if (modelRun) modelRun->SetLockRootMotion(lockRoot);
				if (modelCombo) modelCombo->SetLockRootMotion(lockRoot);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Enable to lock root motion in-place without jumping or resetting position.");
			}

			ImGui::Separator();
			ImGui::Checkbox("Manual Model Selector Mode", &manualMode);
			if (manualMode) {
				ImGui::RadioButton("1. Idle.fbx", &selectedModelIndex, 0);
				ImGui::RadioButton("2. Running.fbx", &selectedModelIndex, 1);
				ImGui::RadioButton("3. Two Hand Sword Combo.fbx", &selectedModelIndex, 2);
			}
			else {
				ImGui::BulletText("Press [W, A, S, D] to Run");
				ImGui::BulletText("Press [J] or [SPACE] to Attack (Combo)");
			}

			ImGui::Separator();
			ImGui::DragFloat3("Character Position", &character.worldTransform.translation.x, 0.05f);
			ImGui::DragFloat3("Character Rotation", &character.worldTransform.rotation.x, 0.05f);
			ImGui::DragFloat3("Character Scale", &character.worldTransform.scale.x, 0.005f, 0.001f, 1.0f);

			// ===== Blend Mode =====
			ImGui::Separator();
			ImGui::Text("Character Blend Mode:");
			const char* blendNames[] = { "Opaque", "Alpha Blend", "Additive", "Multiply" };
			int blendIdx = static_cast<int>(character.blendMode);
			if (ImGui::Combo("Blend Mode", &blendIdx, blendNames, 4)) {
				character.SetBlendMode(static_cast<BlendMode>(blendIdx));
			}
			if (character.blendMode == BlendMode::AlphaBlend || character.blendMode == BlendMode::Add) {
				float alpha = character.GetMaterial() ? character.GetMaterial()->color.w : 1.0f;
				if (ImGui::SliderFloat("Character Alpha", &alpha, 0.0f, 1.0f)) {
					if (character.GetMaterial()) character.GetMaterial()->color.w = alpha;
				}
			}

			// ===== Circle Shadow =====
			ImGui::Separator();
			ImGui::Text("Circle Shadow:");
			ImGui::DragFloat("Shadow Radius", &shadowRadius, 0.01f, 0.05f, 2.0f);
			float shadowAlpha = shadow.GetMaterial()->color.w;
			if (ImGui::SliderFloat("Shadow Opacity", &shadowAlpha, 0.0f, 1.0f)) {
				shadow.GetMaterial()->color.w = shadowAlpha;
			}
			ImGui::SliderFloat("Edge Softness", &shadow.GetMaterial()->softness, 0.0f, 1.0f);
		}

		ImGui::Separator();
		ImGui::ColorEdit4("Light Color", &lights->GetDirectionalLight().color.x);
		ImGui::DragFloat3("Light Direction", &lights->GetDirectionalLight().direction.x, 0.05f);

		ImGui::End();
#endif

		// --- Render ---
		auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
		lights->Draw(cmdList, 3);

		if (showCharacter) {
			// Update and render shadow BEFORE character so character renders on top
			shadow.Update(character.worldTransform.translation, -1.5f, shadowRadius, shadow.GetMaterial()->color.w);
			shadow.Draw(cmdList, &camera);
			character.Draw(cmdList); // Render active character model
		}

		if (showParticleQuads) {
			particleQuads.Draw(cmdList);
		}

		RizeEngine::EndFrame();
	}

	// ===== 6. Finalize =====
	RizeEngine::Finalize();
	return 0;
}
