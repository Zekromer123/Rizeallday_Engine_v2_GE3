#include "SurvivorsGameManager.h"
#include <cstdio>

void SurvivorsGameManager::Initialize() {
	Survivors::gKills = 0;
	Survivors::gEnemySpeedScale = 1.0f;
	spawnTimer_ = 0.5f;
	elapsed_ = 0.0f;
	spawned_ = 0;
	paused_ = false;
	savedResult_ = false;
	menuIndex_ = 0;
	bestKills_ = LoadInt("BestKills", 0);
	SetTimeScale(1.0f);
}

float SurvivorsGameManager::Random01() {
	// xorshift32: deterministic and good enough for spawn positions
	rng_ ^= rng_ << 13;
	rng_ ^= rng_ >> 17;
	rng_ ^= rng_ << 5;
	return static_cast<float>(rng_ & 0xFFFFFF) / static_cast<float>(0x1000000);
}

void SurvivorsGameManager::SpawnEnemy() {
	const float angle = Random01() * 6.2831853f;
	const Vector3 pos = {
		Survivors::gPlayerPos.x + std::cos(angle) * spawnRadius,
		0.5f,
		Survivors::gPlayerPos.z + std::sin(angle) * spawnRadius,
	};
	if (InstantiatePrefab("Enemy", pos)) {
		++spawned_;
	}
}

void SurvivorsGameManager::Update(float deltaTime) {
	const bool pausePressed = IsKeyTriggered(KEY_ESCAPE) || IsKeyTriggered(KEY_P) || IsPadButtonTriggered(PAD_START);

	if (Survivors::gGameOver) {
		if (!savedResult_) {
			savedResult_ = true;
			if (Survivors::gKills > bestKills_) {
				bestKills_ = Survivors::gKills;
				SaveInt("BestKills", bestKills_);
			}
			SetTimeScale(0.0f);
			menuIndex_ = 0;
		}
		DrawHud();
		static const char* const kLabels[] = { "Retry", "Reset Best Score" };
		const int clicked = DrawMenu("GAME OVER", kLabels, 2);
		if (clicked == 0) {
			LoadScene(GetSceneName()); // reload this scene from disk
		} else if (clicked == 1) {
			DeleteAllSaveData();
			bestKills_ = 0;
		}
		return;
	}

	if (pausePressed) {
		paused_ = !paused_;
		SetTimeScale(paused_ ? 0.0f : 1.0f);
		menuIndex_ = 0;
	}
	if (paused_) {
		DrawHud();
		static const char* const kLabels[] = { "Resume", "Restart" };
		const int clicked = DrawMenu("PAUSED", kLabels, 2);
		if (clicked == 0) {
			paused_ = false;
			SetTimeScale(1.0f);
		} else if (clicked == 1) {
			LoadScene(GetSceneName());
		}
		return;
	}

	// Difficulty ramps up over three minutes
	elapsed_ += deltaTime;
	const float ramp = (std::min)(elapsed_ / 180.0f, 1.0f);
	const float interval = startSpawnInterval + (minSpawnInterval - startSpawnInterval) * ramp;
	Survivors::gEnemySpeedScale = 1.0f + ramp * 0.6f;

	spawnTimer_ -= deltaTime;
	const int alive = spawned_ - Survivors::gKills;
	if (spawnTimer_ <= 0.0f && alive < maxAliveEnemies) {
		spawnTimer_ = interval;
		SpawnEnemy();
	}

	DrawHud();
}

void SurvivorsGameManager::DrawHud() {
	char text[96];

	// HP bar (top-left)
	UIText("HP", 24.0f, 20.0f, 26.0f);
	const float hp01 = static_cast<float>(Survivors::gPlayerHp) / static_cast<float>(Survivors::gPlayerMaxHp);
	UIProgressBar(70.0f, 24.0f, 260.0f, 20.0f, hp01, { 0.9f, 0.2f, 0.25f, 1.0f });

	// Kills + best (top-left, below the bar)
	std::snprintf(text, sizeof(text), "Kills: %d    Best: %d", Survivors::gKills, bestKills_);
	UIText(text, 24.0f, 56.0f, 26.0f);

	// Survival time (top-center)
	const int seconds = static_cast<int>(elapsed_);
	std::snprintf(text, sizeof(text), "%02d:%02d", seconds / 60, seconds % 60);
	UIText(text, kUICanvasWidth * 0.5f, 16.0f, 40.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1);

	// Controls hint (bottom-left)
	UIText("WASD / Left stick: move    Mouse / Right stick: aim    P / Esc / Start: pause",
		24.0f, kUICanvasHeight - 40.0f, 20.0f, { 1.0f, 1.0f, 1.0f, 0.6f });
}

int SurvivorsGameManager::DrawMenu(const char* title, const char* const* labels, int count) {
	// Dim the game behind the menu
	UIRect(0.0f, 0.0f, kUICanvasWidth, kUICanvasHeight, { 0.0f, 0.0f, 0.0f, 0.55f });
	UIText(title, kUICanvasWidth * 0.5f, 200.0f, 72.0f, { 1.0f, 0.85f, 0.3f, 1.0f }, 1);

	// Keyboard / gamepad navigation
	if (IsKeyTriggered(KEY_UP) || IsKeyTriggered(KEY_W) || IsPadButtonTriggered(PAD_DPAD_UP)) menuIndex_ = (menuIndex_ + count - 1) % count;
	if (IsKeyTriggered(KEY_DOWN) || IsKeyTriggered(KEY_S) || IsPadButtonTriggered(PAD_DPAD_DOWN)) menuIndex_ = (menuIndex_ + 1) % count;

	int clicked = -1;
	const float w = 320.0f;
	const float h = 56.0f;
	for (int i = 0; i < count; ++i) {
		const float x = (kUICanvasWidth - w) * 0.5f;
		const float y = 330.0f + static_cast<float>(i) * (h + 16.0f);
		if (UIButton(labels[i], x, y, w, h, menuIndex_ == i)) clicked = i;
	}
	return clicked;
}

REGISTER_SCRIPT(SurvivorsGameManager)
