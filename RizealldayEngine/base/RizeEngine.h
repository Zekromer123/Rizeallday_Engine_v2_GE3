#pragma once

// === Rizeallday Engine v2 ===
// Include this single header to access the entire engine.

// Base
#include "base/DirectXCommon.h"
#include "base/WinApp.h"
#include "base/TextureManager.h"
#include "base/DXCManager.h"
#include "base/GraphicPipeline.h"
#include "base/Log.h"

// 3D
#include "3d/Model.h"
#include "3d/Object3d.h"
#include "3d/Camera.h"
#include "3d/DebugCamera.h"
#include "3d/WorldTransform.h"
#include "3d/LightGroup.h"
#include "3d/DirectionalLight.h"
#include "3d/PointLight.h"
#include "3d/SpotLight.h"
#include "3d/PrimitiveDrawer.h"
#include "3d/CircleShadow.h"
#include "3d/Collider.h"
#include "3d/CollisionManager.h"

// 2D
#include "2d/Sprite.h"

// Input
#include "input/Input.h"

// Audio
#include "audio/Audio.h"

// Math
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/MathUtil.h"
#include "math/VertexData.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace RizeEngine {

/// エンジン初期化
void Initialize(const char* title = "Rizeallday Engine", int width = 1280, int height = 720);

/// ウィンドウメッセージ処理 (falseでループ終了)
bool ProcessMessage();

/// フレーム開始処理 (PreDraw + ImGui NewFrame)
void BeginFrame();

/// フレーム終了処理 (ImGui Render + PostDraw + Present)
void EndFrame();

/// エンジン終了処理
void Finalize();

/// サブシステムアクセス
DirectXCommon* GetDirectXCommon();
Input* GetInput();
Audio* GetAudio();

} // namespace RizeEngine
