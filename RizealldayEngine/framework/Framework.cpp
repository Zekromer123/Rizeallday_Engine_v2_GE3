#include "framework/Framework.h"
#include "base/RizeEngine.h"
#include "base/Profiler.h"
#include <Windows.h>
#include <algorithm>

void Framework::Run() {
	Initialize();

	lastTime_ = std::chrono::high_resolution_clock::now();

	while (RizeEngine::ProcessMessage()) {
		if (RizeEngine::GetDirectXCommon()->IsDeviceLost()) {
			Sleep(16);
			continue;
		}

		RizeEngine::BeginFrame();

		auto currentTime = std::chrono::high_resolution_clock::now();
		deltaTime_ = std::chrono::duration<float>(currentTime - lastTime_).count();
		lastTime_ = currentTime;
		deltaTime_ = std::clamp(deltaTime_, 0.0001f, 0.1f);

		{
			RIZE_PROFILE_CPU("Update");
			Update(deltaTime_);
		}
		{
			RIZE_PROFILE_CPU("Draw");
			Draw();
		}

		RizeEngine::EndFrame();
	}

	Finalize();
}

void Framework::Initialize() {
	RizeEngine::Initialize(windowTitle_.c_str(), windowWidth_, windowHeight_);
}

void Framework::Update(float deltaTime) {
	(void)deltaTime;
}

void Framework::Draw() {
}

void Framework::Finalize() {
	RizeEngine::Finalize();
}
