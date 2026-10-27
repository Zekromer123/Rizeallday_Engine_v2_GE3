# Rizeallday Engine v2 — AI Handover & Architecture Context

> [!IMPORTANT]
> **Instructions for AI Assistant (Gemini / Antigravity):**
> Read this document first to understand the complete architecture, conventions, build pipeline, and current state of **Rizeallday Engine v2** before making any code changes.

---

## 1. Workspaces & Build Environment

- **Primary Workspace**: `D:\GE3\Rizeallday_Engine_v2_GE3`
- **Mirror Workspace**: `D:\GE3test\Rizeallday_Engine_v2` *(Keep synced with primary workspace when requested)*
- **Solution File**: `Rizeallday_Engine_v2_GE3.slnx`
- **Compiler / Toolchain**: Visual Studio 2026 / v145 (`C++20`, `/utf-8`, DirectX 12 Shader Model 6.0 via `dxcompiler.dll`)
- **MSBuild Path**:
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\GE3\Rizeallday_Engine_v2_GE3\Rizeallday_Engine_v2_GE3.slnx" /p:Configuration=Development /p:Platform=x64 /m /nologo
  ```

### Coding & Build Rules (Mandatory)
1. **Zero Warnings, Zero Errors**: Both `Development|x64` and `Release|x64` must compile with `0 Warning(s)` and `0 Error(s)` in `RizealldayEngine`, `RizealldaySandbox`, and `GameScripts`.
2. **UTF-8 with BOM**: Save all `.h` and `.cpp` source files as **UTF-8 with BOM**.
3. **Language Convention**:
   - All C++ code comments, log messages, and ImGui UI strings must be in **English**.
   - Communicate with the user in **Indonesian** (unless requested otherwise, e.g., Japanese translations for commit messages or school submissions).
4. **Do Not Modify `externals/`**: Never edit third-party headers/sources inside `externals/` (`DirectXTex`, `imgui`, `assimp`).
5. **Windows Min/Max Macros**: Always use `(std::min)(a, b)` and `(std::max)(a, b)` or `std::clamp` to avoid macro collisions with `<windows.h>`.

---

## 2. Project Structure

```text
Rizeallday_Engine_v2_GE3/
├── BuildPaths.props                  # Unified OutDir (bin/<Config>/) & IntDir (bin/Intermediate/)
├── Rizeallday_Engine_v2_GE3.slnx     # Solution file (4 projects)
├── externals/                        # DirectXTex, Dear ImGui, Assimp
├── RizealldayEngine/                 # Static Library (.lib) — Core Engine
│   ├── 2d/
│   │   ├── Sprite.h / .cpp           # 2D Sprite with UV cutout rectangle & dynamic vertex buffer
│   ├── 3d/
│   │   ├── Camera.h / .cpp           # Perspective/ViewProjection camera
│   │   ├── DebugCamera.h / .cpp      # Orbit/free debug camera
│   │   ├── LightGroup.h / .cpp       # Directional, Point (max 4), and Spot (max 4) lights + shadow params
│   │   ├── Model.h / .cpp            # Assimp (.fbx, .obj, .gltf) mesh, bone skinning, node anims, local AABB
│   │   ├── Object3d.h / .cpp         # 3D instance renderer, material CBV, bone palette CBV (128 bones)
│   │   ├── ShadowMap.h / .cpp        # 2048x2048 D32_FLOAT shadow map pass (DSV #3, SRV #130)
│   │   ├── Skybox.h / .cpp           # Gradient, Solid Color, and DDS Cubemap skybox
│   │   ├── WorldTransform.h / .cpp   # WVP / World / WorldInverseTranspose constant buffer
│   │   ├── Collider.h / .cpp         # AABB & Sphere colliders
│   │   └── CollisionManager.h / .cpp # Broad/narrow collision detection & callbacks
│   ├── audio/
│   │   └── Audio.h / .cpp            # XAudio2 + Windows Media Foundation (.wav, .mp3, .m4a, .wma)
│   ├── base/
│   │   ├── DirectXCommon.h / .cpp    # D3D12 Device, SwapChain, CommandList, RTV/DSV/SRV Heaps, Fence, DeferRelease
│   │   ├── DXCManager.h / .cpp       # Runtime HLSL SM 6.0 compiler (IDxcCompiler3)
│   │   ├── GraphicPipeline.h / .cpp  # RootSignature & PSOs (Opaque/Alpha/Add/Subtract + Geometry Shader modes)
│   │   ├── RenderTexture.h / .cpp    # Offscreen render targets for Editor Scene/Game/Preview viewports
│   │   ├── RizeEngine.h / .cpp       # Top-level engine facade
│   │   ├── ScriptAPI.h               # ABI-safe C++ scripting interface across DLL boundary
│   │   ├── ScriptBase.h              # ScriptBase header wrapper
│   │   ├── ScriptModule.h / .cpp     # Shadow-copy DLL loader & hot-reload watcher for GameScripts.dll
│   │   ├── TextureManager.h / .cpp   # Texture cache, DirectXTex loader, SRV allocator, metadata queries
│   │   └── WinApp.h / .cpp           # Win32 window management (1280x720 default)
│   ├── framework/
│   │   └── Framework.h / .cpp        # Template Method pattern application lifecycle
│   ├── input/
│   │   └── Input.h / .cpp            # DirectInput8 keyboard & mouse state
│   ├── math/
│   │   ├── Vector2.h, Vector3.h, Vector4.h, Matrix4x4.h, Quaternion.h, MathUtil.h, VertexData.h
│   │   └── Frustum.h                 # Gribb-Hartmann 6-plane frustum extraction & AABB culling
│   ├── scene/
│   │   ├── BaseScene.h               # Abstract scene state interface
│   │   └── SceneManager.h / .cpp     # State Pattern scene transition manager
│   └── shaders/                      # HLSL shaders (Object3d, Sprite, Particle, Shadow, Skybox, CircleShadow)
├── RizealldaySandbox/                # Application Executable (.exe) — Editor Suite & Standalone Runner
│   ├── main.cpp                      # 10-line entry point running EditorApp
│   ├── EditorApp.h / .cpp            # Framework subclass; starts TitleScene (splash) -> EditorScene
│   ├── EditorSuite.h / .cpp          # Unity 6 Dark Theme Editor, Scene JSON, Gizmos, Physics, Exporter
│   ├── ParticleQuad.h / .cpp         # Instanced 3D particle system (Smoke, Fountain, Fireworks, Explosion, Torch)
│   └── scene/
│       ├── TitleScene.h / .cpp       # 3.2s Rizeallday Engine splash animation -> transitions to EditorScene
│       └── EditorScene.h / .cpp      # Hosts EditorSuite, LightGroup, Camera, and ParticleQuad
└── GameScripts/                      # Dynamic Library (.dll) — Hot-Reloadable C++ Gameplay Scripts
    └── Scripts/
        ├── dllmain.cpp               # Script factory registry & reflection exports
        ├── RotateScript.h / .cpp     # Sample rotation script
        └── OscillateScript.h / .cpp  # Sample oscillation script
```

---

## 3. Core Subsystems & Completed Features

### 3.1. DirectX 12 & Descriptor Heap Layout (`DirectXCommon` & `TextureManager`)
- Single shared shader-visible **SRV Descriptor Heap** (`srvDescriptorHeap`, capacity `1024`):
  - Index `0`: Reserved for **Dear ImGui** font/UI texture.
  - Indices `1..126`: Managed by `TextureManager` for 2D/3D textures.
  - Index `127`: Preview texture slot (`LoadPreviewTexture`).
  - Index `128`: `editViewRT_` (Editor `# Scene` viewport texture).
  - Index `129`: `gameViewRT_` (Editor `Game` viewport texture).
  - Index `130`: `shadowMap_` depth SRV (`R32_FLOAT`).
  - Index `132`: `modelPreviewRT_` (3D model Inspector preview).
- **GPU Resource Lifetime Safety**: Always use `dxCommon_->DeferRelease(resource)` when releasing `ID3D12Resource` / `ID3D12PipelineState` while frames may be in flight.

### 3.2. 2D Sprite System (`Sprite` & `TextureManager` — Chapter 05 Requirement)
- `Sprite` supports dynamic UV cutout rectangles and size adjustment:
  - `SetTextureLeftTop(const Vector2&)`, `SetTextureSize(const Vector2&)`, `SetTextureRect(leftTop, size)`
  - `AdjustTextureSize()` queries `TextureManager::GetInstance()->GetMetaData(textureHandle_)`
  - `UpdateVertexBuffer()` dynamically computes normalized UV coordinates (`0.0f..1.0f`) from pixel coordinates and uploads to the persistent mapped `vertexDataSprite` pointer on every `Update()`.

### 3.3. Audio System (`Audio` — XAudio2 + Windows Media Foundation)
- Satisfies all school assignment requirements:
  1. **Load/Play Separation & RAM Caching**: `Audio::LoadAudio(filename)` decodes once and caches PCM data in `loadedFileCache_`. `Audio::PlayAudio(soundHandle, loop, volume)` creates an `IXAudio2SourceVoice` reading from the cached RAM buffer without touching disk.
  2. **Windows Media Foundation (`IMFSourceReader`)**: Decodes `.mp3`, `.m4a`, `.wma` (and `.wav` fallback) into 16-bit PCM.
  3. **Voice Management**: `Audio::Update()` cleans up finished non-looping voices every frame; supports `StopAudio(voiceHandle)`, `StopAllAudio()`, `SetVolume()`, and `IsPlaying()`.
- Integrated into **EditorSuite Inspector** as the **Audio Source** component (`audioClipName`, `audioVolume`, `audioPlayOnAwake`, `audioLoop`, Play Preview / Stop buttons) and serialized in `.scene.json`.

### 3.4. ImGui Build Configuration (`Development|x64` vs `Release|x64`)
- **`Debug|x64` & `Development|x64`**:
  - `USE_IMGUI` is defined.
  - Runs the full **Unity 6 Dark Theme Editor Suite**.
- **`Release|x64`**:
  - `USE_IMGUI` is **NOT** defined in `RizealldayEngine` or `RizealldaySandbox`.
  - All 7 `externals/imgui/*.cpp` files have `<ExcludedFromBuild Condition="'$(Configuration)|$(Platform)'=='Release|x64'">true</ExcludedFromBuild>` in `RizealldayEngine.vcxproj`.
  - In `Release|x64`, `EditorSuite::Update` still executes all camera, light, particle, script, and physics updates, while `EditorSuite::RenderViewports` renders `gameCamera_` directly to the main swapchain backbuffer.

### 3.5. Startup Splash Screen (`TitleScene` $\rightarrow$ `EditorScene`)
- Unless launched with `BuildConfig.json` / `--standalone` / `--test-scene-transitions`, `EditorApp::Initialize()` starts in `TitleScene`.
- `TitleScene` renders a 3.2-second intro animation using `Sprite` and `Sprite::SetTextureRect`:
  1. Center vertical white line grows (`0.10s..0.45s`).
  2. Rizeallday icon slides out left & text slides out right from behind the center slit (`0.35s..1.00s`).
  3. Holds (`1.00s..2.00s`).
  4. Slides back in & line shrinks (`2.00s..3.00s`).
  5. Automatically transitions to `EditorScene` at `3.20s` (or immediately on `Space` / `Enter` / `Esc`).

### 3.6. 3D Rendering, Lighting, Shadow Mapping & Frustum Culling
- **Root Signature (`GraphicPipeline` & `ShadowMap`)**:
  - Root Param `0`: CBV `b0` (Pixel) — `Material`
  - Root Param `1`: CBV `b0` (Vertex) — `TransformationMatrix` (`WorldTransform`)
  - Root Param `2`: SRV Table `t0` (Pixel) — Albedo Texture
  - Root Param `3`: CBV `b1` (Pixel) — `LightGroup` (Directional + 4 Point + 4 Spot + `lightViewProjection` + shadow bias/strength)
  - Root Param `4`: CBV `b1` (Vertex) — `BonePalette` (`float4x4 gBones[128]`)
  - Root Param `5`: CBV `b2` (Pixel) — `CircleShadow`
  - Root Param `6`: SRV Table `t1` (Pixel) — `ShadowMap` (`Texture2D<float>`)
- **Directional & Spot Light Rotation Sync**:
  - Rotating a `Directional Light` or `Spot Light` via Transform Rotation or the Viewport Gizmo (`E`) computes `lightDirection = Transform({0, 0, 1}, rotMat)`.
  - Editing `Direction` in the Light Inspector inversely updates `rotation` and `rotationQuat`.
- **Per-Object Shadow Transform**:
  - Each `GameObject` owns `std::unique_ptr<WorldTransform> shadowTransform` and binds its `BonePalette` during `RenderShadowPass` so multiple shadow casters never overwrite a shared constant buffer.
  - `Ground Plane` defaults to `castShadow = false` and `receiveShadow = true`.
- **FBX Skeletal & Node Hierarchy Animation**:
  - `Model::LoadAssimpModel` supports both skinned meshes (`mNumBones > 0`) and unskinned animated node hierarchies (`mNumBones == 0 && mNumAnimations > 0` by creating virtual node bones).
  - `EditorSuite::RenderScene3D` advances animation on `editObject3d` using `currentDeltaTime_` and copies the bone palette to `gameObject3d` via `CopyBonePaletteFrom`.
  - Inspector includes an **Animator (FBX / glTF Animation)** collapsible section with Clip selector, Play/Pause, Loop, Restart, and Playback Speed.
- **Frustum Culling (`RizealldayEngine/math/Frustum.h`)**:
  - Extracts 6 frustum planes from `ViewProjection` matrices (`Frustum::ExtractFromViewProjection`).
  - Transforms each model's cached local AABB to world space (`Frustum::TransformAABB`) and skips off-screen objects in both `RenderScene3D` and `RenderShadowPass`.
  - Bottom viewport bar has a `Cull` toggle and live `Vis:<N> Cul:<M>` counter.

---

## 4. C++ Hot-Reload Scripting System (`GameScripts.dll`)

### How It Works
1. Scripts live in `GameScripts/Scripts/<ClassName>.h` and `.cpp`, inheriting from `ScriptBase` (`RizealldayEngine/base/ScriptAPI.h`).
2. `EditorSuite` loads a shadow copy (`GameScripts_shadow_<N>.dll`) so `MSBuild` can recompile `bin/Scripts/GameScripts.dll` while the Editor is running.
3. Saving any `.cpp`/`.h` in `GameScripts/Scripts/` triggers `PollScriptSourceWatcher` (when `Auto Build Scripts` is checked) $\rightarrow$ runs MSBuild in a background thread $\rightarrow$ serializes all `EXPOSE_FIELD` values $\rightarrow$ unloads old shadow DLL $\rightarrow$ loads new shadow DLL $\rightarrow$ restores `EXPOSE_FIELD` values without restarting Play Mode.

### Script Template Example
```cpp
// PlayerMovement.h
#pragma once
#include "base/ScriptAPI.h"

class PlayerMovement : public ScriptBase {
public:
    EXPOSE_FIELD(float, moveSpeed, 6.0f);
    EXPOSE_FIELD(float, jumpForce, 7.5f);
    EXPOSE_FIELD(bool, faceMoveDirection, true);

    void Initialize() override;
    void Update(float deltaTime) override;
    void OnCollision(GameObject& other) override;
    void OnDestroy() override;
};
```

```cpp
// PlayerMovement.cpp
#include "PlayerMovement.h"
#include <dinput.h>
#include <cmath>

void PlayerMovement::Initialize() {
    Log("[PlayerMovement] Initialized!");
}

void PlayerMovement::Update(float deltaTime) {
    GameObject* self = GetOwner();
    if (!self) return;

    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(DIK_W) || IsKeyDown(DIK_UP))    moveDir.z += 1.0f;
    if (IsKeyDown(DIK_S) || IsKeyDown(DIK_DOWN))  moveDir.z -= 1.0f;
    if (IsKeyDown(DIK_D) || IsKeyDown(DIK_RIGHT)) moveDir.x += 1.0f;
    if (IsKeyDown(DIK_A) || IsKeyDown(DIK_LEFT))  moveDir.x -= 1.0f;

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
    if (len > 0.0001f) {
        moveDir.x /= len;
        moveDir.z /= len;
        self->translation.x += moveDir.x * moveSpeed * deltaTime;
        self->translation.z += moveDir.z * moveSpeed * deltaTime;
        if (faceMoveDirection) {
            self->rotation.y = std::atan2(moveDir.x, moveDir.z);
            self->rotationQuat = Quaternion::FromEuler(self->rotation);
        }
    }

    if (IsKeyTriggered(DIK_SPACE) && self->isGrounded) {
        self->velocity.y = jumpForce;
        self->isGrounded = false;
    }
}

void PlayerMovement::OnCollision(GameObject& other) {
    (void)other;
}

void PlayerMovement::OnDestroy() {
}

REGISTER_SCRIPT(PlayerMovement)
```

---

## 5. Next Development Roadmap (6 Phases)

### Phase 0 — Quick Wins (Immediate)
- [ ] **0.1**: Align exported config check in `EditorApp.cpp` (`game_build.cfg` vs `BuildConfig.json`) so exported games skip the splash screen or follow build settings cleanly.
- [ ] **0.2**: Define `DIK_*` / `KEY_*` constants directly in `ScriptAPI.h` so user scripts do not need `#include <dinput.h>`.
- [ ] **0.3**: Expose mouse delta/position, gamepad buttons, and audio playback helpers (`PlayAudio`, `StopAudio`) in `EngineAPI` / `ScriptBase`.
- [ ] **0.4**: Initialize `NodeData::transform = Mathematics::MakeIdentity4x4();` in `Model.h` to silence IntelliSense warning `C26495`.
- [ ] **0.5**: Add `.m4a` and `.wma` extensions to the `[AUDIO]` badge check in `EditorSuite::DrawProjectAndConsolePanel`.

### Phase 1 — Codebase Architecture Foundation
- [ ] **1.1**: Split `EditorSuite.cpp` (~8,000 lines) into focused translation units (`EditorPanels.cpp`, `EditorSceneSerializer.cpp`, `EditorGizmos.cpp`, `EditorScripting.cpp`, `EditorPhysics.cpp`).
- [ ] **1.2**: Replace manual string-search JSON serialization with `nlohmann/json`.
- [ ] **1.3**: Compute hierarchical world matrices (`parentWorldMatrix * localMatrix`) so child `GameObject` transforms follow their parent in 3D space.
- [ ] **1.4**: Command-based Undo/Redo for transform/property edits instead of full-scene JSON snapshots on every tweak.

### Phase 2 — Gameplay & Editor Tools
- [ ] **2.1**: **Prefab System** (`.prefab.json` drag-and-drop & `InstantiatePrefab("Bullet.prefab.json")` from scripts).
- [ ] **2.2**: **In-Game 2D UI / Canvas System** (`Sprite` + Font rendering that works in `Release|x64` without ImGui).
- [ ] **2.3**: **Physics Upgrades** (`Raycast` API for scripts, OBB colliders, collision layer matrix).
- [ ] **2.4**: **Animator Controller / Crossfade Blending** (smoothly blend between `Idle.fbx`, `walk.fbx`, `Running.fbx`, and `Two Hand Sword Combo.fbx` from C++ scripts).
- [ ] **2.5**: **Scene Switching from Script** (`LoadScene("Stage02.scene.json")`).

### Phase 3 — Modern Graphics Pipeline
- [ ] **3.1**: **PBR Shading** (Cook-Torrance BRDF: Metallic, Roughness, Normal Map, AO).
- [ ] **3.2**: **Image-Based Lighting (IBL)** from the active Skybox cubemap.
- [ ] **3.3**: **HDR RenderTarget (`R16G16B16A16_FLOAT`) + ACES Tonemapping**.
- [ ] **3.4**: **Post-Processing Pass** (Bloom, Vignette, FXAA).
- [ ] **3.5**: **Cascaded Shadow Maps (CSM) + PCF Soft Shadows**.

### Phase 4 & 5 — Performance & Next-Gen (Hikari Engine Inspiration)
- [ ] **4.1**: GPU Instancing for repeated meshes (Mapchip blocks, Tetris blocks, foliage).
- [ ] **4.2**: Bindless Descriptors (SM 6.6 `ResourceDescriptorHeap`).
- [ ] **4.3**: Compute Shader Frustum / Hi-Z Occlusion Culling + `ExecuteIndirect`.
- [ ] **5.1**: Hardware Ray Tracing (DXR) & Froxel Volumetric Fog.
- [ ] **5.2**: Live MCP Server inside `EditorSuite` so AI agents can inspect/manipulate the scene graph directly.
