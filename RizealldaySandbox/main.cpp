#include <RizeEngine.h>
#include "IsometricCamera.h"
#include <chrono>
#include <algorithm>

enum class CharacterState {
	Idle,
	Running,
	Attacking
};

/// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ===== 1. Inisialisasi Engine =====
	RizeEngine::Initialize("Rizeallday Engine v2 - FBX Character Test", 1280, 720);


	// ===== 2. Setup Kamera =====
	Camera camera;
	camera.Initialize();
	IsometricCamera isoCamera;
	isoCamera.Initialize(&camera);
	float cameraRotateSpeed = 1.5f;
	// ===== 3. Pencahayaan (Lighting) =====
	LightGroup* lights = LightGroup::Create();
	lights->SetDefaultLighting();
	lights->SetAmbientColor({ 0.25f, 0.25f, 0.3f });
	lights->SetAmbientIntensity(0.6f);
	lights->GetDirectionalLight().direction = { 0.5f, -1.0f, 0.5f };
	lights->GetDirectionalLight().color = { 1.0f, 0.95f, 0.9f, 1.0f };

	// ===== 4. Load Ketiga Model FBX =====
	Model* modelIdle = Model::CreateFromFile("Resources/Model", "Idle.fbx");
	Model* modelRun = Model::CreateFromFile("Resources/Model", "Running.fbx");
	Model* modelCombo = Model::CreateFromFile("Resources/Model", "Two Hand Sword Combo.fbx");

	// Aktifkan Lock Root Motion (In-Place) agar animasi lari & combo tidak melompat/reset posisi
	if (modelRun) modelRun->SetLockRootMotion(true);
	if (modelCombo) modelCombo->SetLockRootMotion(true);


	// Setup Objek Karakter
	Object3d character;
	character.Initialize();
	character.SetModel(modelIdle); // Default awal: Idle
	character.SetCamera(&camera);
	character.worldTransform.translation = { 0.0f, -1.5f, 0.0f };
	character.worldTransform.rotation.y = 3.14159f; // Hadap ke kamera saat mulai
	character.worldTransform.scale = { 0.006f, 0.006f, 0.006f }; // Skala Mixamo FBX


	// Verifikasi evaluasi animasi tulang
	if (modelIdle && modelIdle->HasBones()) {
		Log::GetInstance()->logOutput(std::format("Skeletal Animation Ready: Duration = {:.2f}s, Bones = {}\n",
			modelIdle->GetAnimationDuration(0), modelIdle->GetBoneCount()));
	}

	// ===== Circle Shadow =====
	CircleShadow shadow;
	shadow.Initialize();
	shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f }; // Hitam, 45% transparan
	shadow.GetMaterial()->softness = 0.5f;
	float shadowRadius = 0.3f; // Radius bayangan di dunia 3D


	// State Karakter
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
			// --- MODE GAMEPLAY OTOMATIS ---
			// Timer serangan
			if (attackTimer > 0.0f) {
				attackTimer -= deltaTime;
				currentState = CharacterState::Attacking;
				character.SetModel(modelCombo);
			}
			else {
				// Cek input gerak (WASD)
				Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
				Vector3 rawInput = { 0.0f, 0.0f, 0.0f };
				if (input->PushKey(DIK_W)) { rawInput.z += 1.0f; }
				if (input->PushKey(DIK_S)) { rawInput.z -= 1.0f; }
				if (input->PushKey(DIK_A)) { rawInput.x -= 1.0f; }
				if (input->PushKey(DIK_D)) { rawInput.x += 1.0f; }

				// Input serang: Tekan J atau SPACE
				if (input->TriggerKey(DIK_J) || input->TriggerKey(DIK_SPACE)) {
					attackTimer = modelCombo ? modelCombo->GetAnimationDuration(0) : 1.2f;
					currentState = CharacterState::Attacking;
					character.SetModel(modelCombo);
				}
				else if (rawInput.Length() > 0.001f) {
					// Karakter sedang lari
					currentState = CharacterState::Running;
					character.SetModel(modelRun);

					rawInput = rawInput.Normalize();
					float yaw = isoCamera.GetYaw();
					moveDir = {
						rawInput.x * std::cos(yaw) + rawInput.z * std::sin(yaw),
						 0.0f,
						-rawInput.x * std::sin(yaw) + rawInput.z * std::cos(yaw)
					};
					float runSpeed = 4.5f * deltaTime;
					character.worldTransform.translation += moveDir * runSpeed;

					// Hadapkan karakter ke arah lari
					character.worldTransform.rotation.y = std::atan2(moveDir.x, moveDir.z);
				}
				else {
					// Karakter sedang diam
					currentState = CharacterState::Idle;
					character.SetModel(modelIdle);
				}
			}
		}
		else {
			// --- MODE INSPEKTOR MANUAL (Pilih lewat ImGui) ---
			if (selectedModelIndex == 0) character.SetModel(modelIdle);
			else if (selectedModelIndex == 1) character.SetModel(modelRun);
			else if (selectedModelIndex == 2) character.SetModel(modelCombo);
		}

		// Update Kamera agar mengikuti karakter
		Vector3 targetPos = character.worldTransform.translation;
		targetPos.y += 1.0f;
		isoCamera.Update(targetPos, deltaTime);
		//camera.translation.x = character.worldTransform.translation.x;
		//camera.translation.y = character.worldTransform.translation.y + 1.5f;
		//camera.translation.z = character.worldTransform.translation.z - 9.5f;

		
		if (input->PushKey(DIK_Q)) {
			// Putar kamera ke kiri
			isoCamera.RotateYaw(-cameraRotateSpeed * deltaTime);
		}
		if (input->PushKey(DIK_E)) {
			// Putar kamera ke kanan
			isoCamera.RotateYaw(cameraRotateSpeed * deltaTime);
		}


		//camera.Update();
		character.Update(deltaTime);
		lights->Update();

#ifdef USE_IMGUI
		// --- Panel Debug ImGui ---
		ImGui::Begin("Karakter FBX Controller");
		ImGui::Text("Rizeallday Engine v2");
		ImGui::Text("FPS: %.1f (dt: %.4f s)", ImGui::GetIO().Framerate, deltaTime);
		ImGui::Separator();

		const char* stateStr = "Idle";
		if (currentState == CharacterState::Running) stateStr = "Running (WASD)";
		if (currentState == CharacterState::Attacking) stateStr = "Attacking (J / Space)";
		ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "State Saat Ini: %s", stateStr);

		ImGui::Separator();
		ImGui::Text("Animasi FBX:");
		if (character.GetModel()) {
			ImGui::Text("Jumlah Bones: %zu", character.GetModel()->GetBoneCount());
			ImGui::Text("Durasi: %.2f detik", character.GetAnimationDuration());
			ImGui::Text("Waktu Animasi: %.2f detik", character.GetAnimationTime());
		}
		float animSpeed = character.GetAnimationSpeed();
		if (ImGui::SliderFloat("Kecepatan Animasi", &animSpeed, 0.0f, 3.0f, "%.2fx")) {
			character.SetAnimationSpeed(animSpeed);
		}

		bool lockRoot = modelRun ? modelRun->GetLockRootMotion() : true;
		if (ImGui::Checkbox("Kunci Root Motion (In-Place)", &lockRoot)) {
			if (modelRun) modelRun->SetLockRootMotion(lockRoot);
			if (modelCombo) modelCombo->SetLockRootMotion(lockRoot);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Aktifkan agar animasi lari di tempat (In-Place) tanpa melompat/reset posisi.");
		}

		ImGui::Separator();

		ImGui::Checkbox("Mode Manual Pilih Model", &manualMode);
		if (manualMode) {
			ImGui::RadioButton("1. Idle.fbx", &selectedModelIndex, 0);
			ImGui::RadioButton("2. Running.fbx", &selectedModelIndex, 1);
			ImGui::RadioButton("3. Two Hand Sword Combo.fbx", &selectedModelIndex, 2);
		}
		else {
			ImGui::BulletText("Tekan [W, A, S, D] untuk Lari");
			ImGui::BulletText("Tekan [J] atau [SPACE] untuk Serang (Combo)");
		}

		ImGui::Separator();
		ImGui::DragFloat3("Posisi Karakter", &character.worldTransform.translation.x, 0.05f);
		ImGui::DragFloat3("Rotasi Karakter", &character.worldTransform.rotation.x, 0.05f);
		ImGui::DragFloat3("Skala Karakter", &character.worldTransform.scale.x, 0.005f, 0.001f, 1.0f);

		ImGui::Separator();
		ImGui::ColorEdit4("Warna Cahaya", &lights->GetDirectionalLight().color.x);
		ImGui::DragFloat3("Arah Cahaya", &lights->GetDirectionalLight().direction.x, 0.05f);

		// ===== Blend Mode =====
		ImGui::Separator();
		ImGui::Text("Blend Mode Karakter:");
		const char* blendNames[] = { "Opaque", "Alpha Blend", "Additive", "Multiply" };
		int blendIdx = static_cast<int>(character.blendMode);
		if (ImGui::Combo("Blend Mode", &blendIdx, blendNames, 4)) {
			character.SetBlendMode(static_cast<BlendMode>(blendIdx));
		}
		if (character.blendMode == BlendMode::AlphaBlend || character.blendMode == BlendMode::Add) {
			float alpha = character.GetMaterial() ? character.GetMaterial()->color.w : 1.0f;
			if (ImGui::SliderFloat("Alpha Karakter", &alpha, 0.0f, 1.0f)) {
				if (character.GetMaterial()) character.GetMaterial()->color.w = alpha;
			}
		}

		// ===== Circle Shadow =====
		ImGui::Separator();
		ImGui::Text("Circle Shadow:");
		ImGui::DragFloat("Radius Bayangan", &shadowRadius, 0.01f, 0.05f, 2.0f);
		float shadowAlpha = shadow.GetMaterial()->color.w;
		if (ImGui::SliderFloat("Transparansi Bayangan", &shadowAlpha, 0.0f, 1.0f)) {
			shadow.GetMaterial()->color.w = shadowAlpha;
		}
		ImGui::SliderFloat("Kelembutan Tepi", &shadow.GetMaterial()->softness, 0.0f, 1.0f);

		ImGui::End();
#endif

		// --- Render ---
		auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
		lights->Draw(cmdList, 3);

		// Update dan render bayangan SEBELUM karakter (agar karakter menutupi bayangan)
		shadow.Update(character.worldTransform.translation, -1.5f, shadowRadius, shadow.GetMaterial()->color.w);
		shadow.Draw(cmdList, &camera);

		character.Draw(cmdList); // Render model yang sedang aktif (pipeline di-bind ulang otomatis)

		RizeEngine::EndFrame();
	}

	// ===== 6. Finalisasi =====
	RizeEngine::Finalize();
	return 0;
}
