#pragma once
#include <chrono>
#include <string>

/// Base application framework wrapping RizeEngine lifecycle and frame loop.
class Framework {
public:
	Framework() = default;
	virtual ~Framework() = default;

	Framework(const Framework&) = delete;
	Framework& operator=(const Framework&) = delete;

	/// Execute the complete application lifecycle: Initialize -> Main Loop -> Finalize.
	void Run();

	/// Initialize engine subsystems and application resources.
	virtual void Initialize();

	/// Per-frame logic update receiving clamped delta time in seconds.
	virtual void Update(float deltaTime);

	/// Per-frame rendering.
	virtual void Draw();

	/// Finalize application resources and engine subsystems.
	virtual void Finalize();

	float GetDeltaTime() const { return deltaTime_; }

protected:
	std::string windowTitle_ = "Rizeallday Engine";
	int windowWidth_ = 1280;
	int windowHeight_ = 720;

private:
	std::chrono::high_resolution_clock::time_point lastTime_{};
	float deltaTime_ = 0.016f;
};
