#pragma once
#include "SurvivorsShared.h"

// Put on an empty "GameManager" object: spawns "Enemy" prefabs around the player (faster over time),
// draws the HUD, handles pause (Esc / P / Start) and game over (saves the best score, restart by LoadScene).
class SurvivorsGameManager : public ScriptBase {
public:
	EXPOSE_FIELD(float, spawnRadius, 16.0f);
	EXPOSE_FIELD(float, startSpawnInterval, 1.0f);
	EXPOSE_FIELD(float, minSpawnInterval, 0.12f);
	EXPOSE_FIELD(int, maxAliveEnemies, 300);

	void Initialize() override;
	void Update(float deltaTime) override;

private:
	void SpawnEnemy();
	void DrawHud();
	// Returns the index of the clicked button, or -1. Up/Down/D-pad move `menuIndex_`.
	int DrawMenu(const char* title, const char* const* labels, int count);
	float Random01();

	float spawnTimer_ = 0.0f;
	float elapsed_ = 0.0f;
	int spawned_ = 0;
	int killsAtStart_ = 0;
	int bestKills_ = 0;
	bool paused_ = false;
	bool savedResult_ = false;
	int menuIndex_ = 0;
	uint32_t rng_ = 0x9E3779B9u;
};
