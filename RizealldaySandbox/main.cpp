#include <RizeEngine.h>
#include <cassert>

/// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ===== エンジン初期化 =====
	RizeEngine::Initialize("Rizeallday Engine v2", 1280, 720);

	// ===== リソース読み込み =====
	// Model* model = Model::CreateFromFile("Resources/Model", "player.fbx");

	Camera camera;
	camera.Initialize();
	camera.translation = {0.0f, 2.0f, -10.0f};

	LightGroup* lights = LightGroup::Create();
	lights->SetDefaultLighting();

	// ===== ゲームループ =====
	while (RizeEngine::ProcessMessage()) {
		RizeEngine::BeginFrame();

		// --- 更新処理ここから ---
		Input* input = RizeEngine::GetInput();

		camera.Update();
		lights->Update();

		assert(false && "assert Test");
		// --- 更新処理ここまで ---

#ifdef USE_IMGUI
		// --- ImGui デバッグ ---
		ImGui::Begin("Debug");
		ImGui::Text("Rizeallday Engine v2");
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::End();
#endif

		// --- 描画処理ここから ---
		auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
		lights->Draw(cmdList, 3);

		// object.Draw(cmdList);
		// --- 描画処理ここまで ---

		RizeEngine::EndFrame();
	}

	// ===== 終了処理 =====
	RizeEngine::Finalize();
	return 0;
}
