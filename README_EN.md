# Rizeallday Engine v2

[Bahasa Indonesia](README.md) | [日本語](README_JA.md) | **English**

Welcome to **Rizeallday Engine v2**!  
This project is an advanced evolution of the custom DirectX 12 game engine developed as part of game programming coursework at Nihon Kougakuin College (日本工学院専門学校). It has been restructured using a modern modular **Engine + Sandbox** architecture (inspired by [TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) and the Japanese industry template [KamataEngine](https://github.com)).

Built with pure **DirectX 12**, **C++20**, static library linkage, **Assimp** integration (supporting animated 3D FBX, OBJ, GLTF, DAE, etc.), a GPU-accelerated **Skeletal Animation** system (up to 128 bones palette), a unified **Multi-Lighting** pipeline (Directional, Point, Spot, and Ambient lights), an **Isometric Action-RPG Camera**, and runtime **Dear ImGui** debugging.

---

## Table of Contents
1. [Key Features](#1-key-features)
2. [Architecture & Directory Structure](#2-architecture--directory-structure)
3. [Quick Start Guide](#3-quick-start-guide)
4. [API & Subsystem Reference](#4-api--subsystem-reference)
   - [Engine Lifecycle (`RizeEngine`)](#engine-lifecycle-rizeengine)
   - [DirectX 12 Subsystem (`DirectXCommon`)](#directx-12-subsystem-directxcommon)
   - [Pipeline & Blend Modes (`GraphicPipeline` / `BlendMode`)](#pipeline--blend-modes-graphicpipeline--blendmode)
   - [3D Model & Skeletal Mesh (`Model`)](#3d-model--skeletal-mesh-model)
   - [3D Entity Instance (`Object3d`)](#3d-entity-instance-object3d)
   - [Dynamic Circle Shadow (`CircleShadow`)](#dynamic-circle-shadow-circleshadow)
   - [World Transform (`WorldTransform`)](#world-transform-worldtransform)
   - [Camera & Isometric Camera (`Camera`, `IsometricCamera`, `DebugCamera`)](#camera--isometric-camera)
   - [Lighting System (`LightGroup`)](#lighting-system-lightgroup)
   - [2D Sprite Renderer (`Sprite`)](#2d-sprite-renderer-sprite)
   - [Input System (`Input`)](#input-system-input)
   - [Audio System (`Audio`)](#audio-system-audio)
   - [Math Library (`Mathematics` / `MathUtil`)](#math-library-mathematics--mathutil)
5. [Shader & Root Signature Pipeline](#5-shader--root-signature-pipeline)

---

## 1. Key Features

- **Pure DirectX 12 Rendering**: Command lists, descriptor heaps, root signatures, pipeline state objects (PSO), HLSL Shader Model 6.0 (`dxc`).
- **Multi-Blend Mode Rendering**:
  - Independent PSOs for `Opaque`, `AlphaBlend`, `Add` (additive glow/spells/fire), and `Multiply` (darkening/curse effects).
  - Automatic `DepthWriteMask = ZERO` handling for transparent objects to prevent depth occlusion artifacts.
- **Dynamic Circle / Blob Shadows**:
  - Ground-projected circular drop shadow with smooth radial `smoothstep` falloff.
  - Perspective ellipse adjustment for isometric top-down projection.
  - Real-time controls for shadow radius, opacity, softness, and tint color.
- **Skeletal Mesh Animation**:
  - Full Assimp integration supporting FBX (Mixamo rigs, etc.), OBJ, GLTF, GLB, and Collada DAE.
  - Skinned vertex shader computing dual quaternion / linear matrix blending for up to 4 bone influences per vertex (`BLENDINDICES` + `BLENDWEIGHT`).
  - ConstantBuffer bone palette supporting up to 128 bones per mesh.
  - Slerp quaternion & Lerp vector keyframe interpolation.
  - **In-Place / Root Motion Lock**: Built-in toggle to prevent character rubber-banding during animation cycles.
- **Unified Multi-Lighting**:
  - Directional Light (Lambertian / Half-Lambert diffuse with color & intensity control).
  - 3 Point Lights with radius and exponential distance decay attenuation.
  - 3 Spot Lights with distance attenuation and smoothstep cone falloff.
  - Ambient Lighting with customizable color and intensity.
- **Isometric ARPG Camera**:
  - Top-down diagonal isometric view ($45^\circ$ pitch, $45^\circ$ yaw) styled after modern action-RPGs (*Lunarium*, *Hades*, *Diablo*).
  - Smooth target damping (cinematic follow).
  - Real-time Q/E orbit camera rotation with camera-relative 8-way movement translation.
- **Development | x64 Configuration**:
  - Optimized hybrid configuration with `assert()` enabled (no `NDEBUG`), Dear ImGui active (`USE_IMGUI`), `/Od` unoptimized sandbox for accurate debugging, and `/O2` engine core for top frame rates.
- **Input System**: DirectInput 8 (Keyboard & Mouse) + XInput (Xbox Gamepad with analog deadzone filtering & dual-motor vibration).
- **Audio System**: Low-latency PCM WAV audio playback powered by XAudio2.
- **Debug UI**: Seamlessly integrated Dear ImGui for live inspection and tweaking of transforms, lighting, animation playback, blend modes, shadows, and camera properties.

---

## 2. Architecture & Directory Structure

The solution (`Rizeallday_Engine_v2.slnx`) is separated into two clean projects:
- **`RizealldayEngine`**: **Static Library (`.lib`)**. Contains all DX12 core rendering, shaders, mathematics, Assimp loaders, skeletal animation, input, lighting, and audio.
- **`RizealldaySandbox`**: **Executable (`.exe`)**. The active gameplay application and startup project where game logic, scenes, entities, and controllers are implemented.

```
D:\Rizeallday_Engine_v2\
│
├── Rizeallday_Engine_v2.slnx              # Visual Studio Solution
├── README.md                               # Indonesian Documentation
├── README_EN.md                            # English Documentation
├── README_JA.md                            # Japanese Documentation
│
├── RizealldayEngine/                       # [STATIC LIBRARY] Core Engine
│   ├── base/                               # Core Subsystems
│   │   ├── RizeEngine.h / .cpp             # Master lifecycle & engine singleton access
│   │   ├── DirectXCommon.h / .cpp          # DX12 Device, CommandQueue, SwapChain, Heaps
│   │   ├── WinApp.h / .cpp                 # Win32 Window management & message pump
│   │   ├── BlendMode.h                     # BlendMode enum (Opaque, AlphaBlend, Add, Multiply)
│   │   ├── TextureManager.h / .cpp         # Texture cache & SRV descriptor allocator
│   │   ├── GraphicPipeline.h / .cpp        # Pipeline State Objects (PSO 4 Blend Modes) & Root Signature
│   │   ├── DXCManager.h / .cpp             # Runtime HLSL DXC shader compiler
│   │   ├── Log.h / .cpp                    # Thread-safe logging subsystem
│   │   └── CrashHandler.h / .cpp           # Minidump crash reporter
│   ├── 3d/                                 # 3D Graphics & Animation
│   │   ├── Model.h / .cpp                  # Assimp loader, skeletal animation & GPU buffers
│   │   ├── Object3d.h / .cpp               # 3D Entity instance (Model + Transform + Material + BlendMode)
│   │   ├── CircleShadow.h / .cpp           # Procedural 3D blob drop shadow on floor quad
│   │   ├── Camera.h / .cpp                 # Perspective & ViewProjection matrix generator
│   │   ├── DebugCamera.h / .cpp            # Orbit/pan debug camera with mouse & WASD
│   │   ├── WorldTransform.h / .cpp         # World matrix & WVP ConstantBuffer manager
│   │   ├── LightGroup.h / .cpp             # Multi-light GPU buffer manager
│   │   ├── DirectionalLight.h              # Directional light data definition
│   │   ├── PointLight.h                    # Point light data definition
│   │   ├── SpotLight.h                     # Spot light data definition
│   │   └── PrimitiveDrawer.h / .cpp        # Debug 3D line & grid renderer
│   ├── 2d/                                 # 2D Graphics
│   │   └── Sprite.h / .cpp                 # 2D Sprite & UI quad renderer
│   ├── input/                              # Input Subsystem
│   │   └── Input.h / .cpp                  # DirectInput8 (Keyboard/Mouse) + XInput (Gamepad)
│   ├── audio/                              # Audio Subsystem
│   │   └── Audio.h / .cpp                  # XAudio2 loader & playback
│   ├── math/                               # Mathematics
│   │   ├── Vector2.h, Vector3.h, Vector4.h # SIMD-friendly vector structs & operators
│   │   ├── Matrix4x4.h                     # Row-major 4x4 matrix and Transform structs
│   │   ├── MathUtil.h / .cpp               # Affine, Perspective, Orthographic & Inverse math
│   │   └── VertexData.h                    # VertexData with Bone indices & weights
│   └── shaders/                            # HLSL Shaders
│       ├── Object3d.hlsli                  # Shared VS/PS I/O struct
│       ├── Object3d.VS.hlsl                # Skinned skeletal vertex shader
│       ├── Object3d.PS.hlsl                # Multi-light pixel shader (Lambert / Half-Lambert)
│       ├── CircleShadow.VS.hlsl            # Unskinned quad vertex shader for drop shadow
│       ├── CircleShadow.PS.hlsl            # Radial smoothstep gradient pixel shader
│       ├── Sprite.hlsli                    # 2D sprite shader header
│       ├── Sprite.VS.hlsl                  # 2D sprite vertex shader
│       └── Sprite.PS.hlsl                  # 2D sprite pixel shader
│
├── RizealldaySandbox/                      # [APPLICATION] Game Sandbox
│   ├── main.cpp                            # Gameplay loop & character controller
│   ├── IsometricCamera.h / .cpp            # Smooth action-RPG isometric follow camera
│   ├── dxcompiler.dll, dxil.dll            # DirectX Shader Compiler binaries
│   ├── assimp-vc143-mt.dll                 # Assimp runtime DLL
│   └── Resources/                          # Game Assets
│       ├── Model/                          # .fbx, .obj, .gltf 3D models & textures
│       ├── Texture/                        # .png, .dds images
│       └── Sound/                          # .wav audio files
│
└── externals/                              # Third-Party Dependencies
    ├── assimp/                             # Open Asset Import Library
    ├── DirectXTex/                         # Microsoft DirectXTex texture processing
    └── imgui/                              # Dear ImGui (Docking / Viewport branch)
```

---

## 3. Quick Start Guide

### Building and Running
1. Open **`D:\Rizeallday_Engine_v2\Rizeallday_Engine_v2.slnx`** in Visual Studio 2022 (v143 / v145 toolset).
2. Set **`RizealldaySandbox`** as the **Startup Project** in Solution Explorer.
3. Select your target configuration:
   - **`Development | x64` (Recommended for Day-to-Day Development)**:
     - Dear ImGui enabled (`USE_IMGUI`).
     - Assertions enabled (`assert()` active, `NDEBUG` is omitted for the sandbox).
     - Sandbox project compiled with `/Od` for accurate debugger stepping, while the engine core remains at `/O2` for peak rendering performance.
   - **`Debug | x64`**:
     - Full runtime diagnostics, D3D12 GPU debug layer enabled, ImGui active.
   - **`Release | x64`**:
     - Maximum compiler optimizations (`/O2`), ImGui and debug logging stripped for retail release.
4. Press **F5** to build and launch.

### Minimal Game Code Template
```cpp
#include <RizeEngine.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // 1. Initialize Engine
    RizeEngine::Initialize("Rizeallday Game Project", 1280, 720);

    // 2. Load Animated 3D Model
    Model* heroModel = Model::CreateFromFile("Resources/Model", "Idle.fbx");
    heroModel->SetLockRootMotion(true); // Lock horizontal root motion for clean in-place movement

    // 3. Setup Camera
    Camera camera;
    camera.Initialize();
    camera.translation = { 0.0f, 1.5f, -8.0f };
    camera.rotation = { 0.1f, 0.0f, 0.0f };

    // 4. Create 3D Object Entity
    Object3d hero;
    hero.Initialize();
    hero.SetModel(heroModel);
    hero.SetCamera(&camera);
    hero.worldTransform.scale = { 0.018f, 0.018f, 0.018f };

    // 5. Setup Lighting
    LightGroup* lights = LightGroup::Create();
    lights->SetDefaultLighting();

    // 6. Game Loop
    while (RizeEngine::ProcessMessage()) {
        RizeEngine::BeginFrame();

        float deltaTime = 0.016f;
        Input* input = RizeEngine::GetInput();

        // Gameplay Input
        if (input->PushKey(DIK_W)) {
            hero.worldTransform.translation.z += 4.5f * deltaTime;
        }

        camera.Update();
        hero.Update(deltaTime); // Ticks skeletal animation
        lights->Update();

        // Render
        auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
        lights->Draw(cmdList, 3);
        hero.Draw(cmdList);

        RizeEngine::EndFrame();
    }

    RizeEngine::Finalize();
    return 0;
}
```

---

## 4. API & Subsystem Reference

### Engine Lifecycle (`RizeEngine`)
- `RizeEngine::Initialize(const char* title, int width, int height)`: Initializes Win32 window, DirectX 12 device, DXC compiler, PSOs, texture managers, input, and audio subsystems.
- `bool RizeEngine::ProcessMessage()`: Pumps Windows OS messages. Returns `false` on exit request (`WM_QUIT`).
- `void RizeEngine::BeginFrame()`: Clears backbuffer & depth stencil, polls input devices, starts ImGui frame.
- `void RizeEngine::EndFrame()`: Renders ImGui draw data, executes command lists, presents swapchain.
- `void RizeEngine::Finalize()`: Waits for GPU idle via fences and safely releases all COM resources.
- `RizeEngine::GetDirectXCommon()`: Accesses the DX12 wrapper subsystem.
- `RizeEngine::GetInput()`: Accesses the unified input manager.
- `RizeEngine::GetAudio()`: Accesses the XAudio2 sound engine.

---

### DirectX 12 Subsystem (`DirectXCommon`)
- `ID3D12Device* GetDevice() const`: Direct GPU device access.
- `ID3D12GraphicsCommandList* GetCommandList() const`: Active command list for drawing.
- `ID3D12CommandQueue* GetCommandQueue() const`: Command queue.
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: Allocates GPU upload buffer.
- `void SignalAndWaitFence()`: CPU-GPU synchronization barrier.

---

### Pipeline & Blend Modes (`GraphicPipeline` / `BlendMode`)
The engine caches independent Pipeline State Objects (PSO) for each blend mode:
```cpp
enum class BlendMode : int {
    Opaque     = 0, // Solid opaque mesh, Depth Write ON (Default)
    AlphaBlend = 1, // Standard alpha blending (SrcAlpha / InvSrcAlpha), Depth Write OFF
    Add        = 2, // Additive blending (particles, fire, glow, magic effects)
    Multiply   = 3, // Multiplicative darkening (shadows, curse effects)
    Count      = 4
};
```
- Each `Object3d` instance owns a `blendMode` property.
- When `AlphaBlend` or `Add` is active, depth-writing is automatically disabled (`DepthWriteMask = ZERO`) to eliminate clipping artifacts with background geometry.
- Usage:
  ```cpp
  character.SetBlendMode(BlendMode::AlphaBlend);
  character.GetMaterial()->color.w = 0.5f; // 50% opacity
  ```

---

### 3D Model & Skeletal Mesh (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`:
  Loads and parses 3D formats (`.fbx`, `.obj`, `.gltf`, `.glb`, `.dae`). Auto-caches models in RAM/VRAM.
- `bool HasBones() const`: Returns `true` if model contains rigged skeletal bone data.
- `size_t GetBoneCount() const`: Returns the total number of bones bound to the mesh.
- `float GetAnimationDuration(int animIndex = 0) const`: Returns total duration in seconds.
- `void CalculateBoneTransforms(float timeInSeconds, int animIndex, std::vector<Matrix4x4>& outMatrices)`:
  Traverses the bone hierarchy, computes interpolated Slerp/Lerp transforms, and evaluates final skinning matrices (`offsetMatrix * globalTransform`).
- `void SetLockRootMotion(bool lock)`:
  Locks horizontal root bone ($X, Z$) translations for smooth in-place locomotion while preserving natural vertical ($Y$) footstep bobbing.

---

### 3D Entity Instance (`Object3d`)
- `void Initialize()`: Allocates GPU upload constant buffers for material, transform, and bone palette (128 matrices).
- `void SetModel(Model* model, bool resetAnim = true)`: Binds a `Model` asset.
- `void SetCamera(Camera* camera)`: Binds the viewing `Camera`.
- `WorldTransform worldTransform`: Public transform containing `scale`, `rotation` (Euler), and `translation`.
- `BlendMode blendMode`: Object blend mode (`BlendMode::Opaque`, `AlphaBlend`, `Add`, `Multiply`).
- `void SetBlendMode(BlendMode mode)` / `BlendMode GetBlendMode() const`: Blend mode accessors.
- `Material* GetMaterial() const`: Direct access to GPU material buffer (RGBA color, lighting flags).
- **Skeletal Animation Controls**:
  - `void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f)`
  - `void SetAnimationSpeed(float speed)`
  - `float GetAnimationTime() const` / `float GetAnimationDuration() const`
- `void Update(float deltaTime = 1.0f / 60.0f)`: Advances animation time, evaluates bone transforms, and uploads WVP and bone palette matrices to GPU constant buffers.
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Automatically binds the corresponding blend PSO, material (Slot 0), WVP (Slot 1), texture SRV (Slot 2), bone palette (Slot 4), and draws all meshes.

---

### Dynamic Circle Shadow (`CircleShadow`)
Procedural ground-projected circular drop shadow (blob shadow) beneath 3D characters:
- **Technique**: Renders a floor-aligned horizontal quad using custom unskinned shaders (`CircleShadow.VS.hlsl` & `CircleShadow.PS.hlsl`) with a procedural radial `smoothstep` falloff—requiring zero external bitmap textures.
- `static void StaticInitialize(DirectXCommon* dxCommon, DXCManager* dxcManager)`: Initialized automatically in `RizeEngine::Initialize()`.
- `void Initialize()`: Allocates quad vertex/index buffers and GPU constant buffers.
- `void Update(const Vector3& characterPos, float floorY, float radius, float opacity)`:
  - `characterPos`: Character position (shadow tracks X and Z).
  - `floorY`: Elevation of the ground plane (e.g. `-1.5f`).
  - `radius`: World-space shadow radius.
  - `opacity`: Shadow opacity (0.0 = transparent, 1.0 = solid dark).
- `void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera)`: Renders the shadow with alpha blending and depth writing disabled.
- `CircleShadowMaterialGPU* GetMaterial()`: Provides real-time control over `softness`, `color`, and UV parameters.

Example usage:
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color = { 0.0f, 0.0f, 0.0f, 0.45f }; // 45% black
shadow.GetMaterial()->softness = 0.5f;

// Inside game loop:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera); // Render BEFORE character
character.Draw(cmdList);       // Render character on top
```

---

### Camera & Isometric Camera
- **`Camera`**:
  - `Vector3 translation`, `Vector3 rotation` (Euler pitch/yaw/roll in radians).
  - `float fovY`, `float aspectRatio`, `float nearClip`, `float farClip`.
  - `void Update()`: Recomputes view matrix (affine inverse) and perspective projection.
- **`IsometricCamera`**:
  - High-level isometric camera with $45^\circ$ pitch and $45^\circ$ yaw.
  - Smooth target follow damping without desynchronization.
  - `RotateYaw(float deltaYaw)`: Q/E 360-degree orbit rotation around the player.
  - `float GetYaw() const`: Returns current yaw for camera-aligned player locomotion.
- **`DebugCamera`**:
  - Free-look mouse-driven development camera. Right-click drag to rotate, middle-click drag to pan, WASDQE to fly.

---

### Lighting System (`LightGroup`)
Unified multi-light ConstantBuffer manager:
- `static LightGroup* Create()`: Instantiates the lighting manager.
- `void SetDefaultLighting()`: Sets standard ambient and white directional light.
- `void SetAmbientColor(const Vector3& color)` & `SetAmbientIntensity(float intensity)`: Controls global scene ambience.
- `DirectionalLightData& GetDirectionalLight()`: Directional light data (`color`, `direction`, `intensity`, `active`).
- `PointLightData& GetPointLight(int index)` (Index 0 to 2): Point light data (`position`, `color`, `radius`, `decay`, `intensity`, `active`).
- `SpotLightData& GetSpotLight(int index)` (Index 0 to 2): Spot light data (`position`, `direction`, `distance`, `cosAngle`, `cosFalloffStart`, `active`).
- `void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex = 3)`: Binds lighting buffer to root parameter 3.

---

### Input System (`Input`)
- **Keyboard (DirectInput 8)**:
  - `bool PushKey(BYTE keyNumber)`: `true` continuously while held (e.g. `DIK_W`, `DIK_SPACE`).
  - `bool TriggerKey(BYTE keyNumber)`: `true` only on the initial frame pressed.
  - `bool ReleaseKey(BYTE keyNumber)`: `true` on key release.
- **Mouse**:
  - `bool PushMouseButton(int button)`: 0 = Left, 1 = Right, 2 = Middle.
  - `bool IsTriggerMouse(int button)`: Trigger click detection.
  - `int GetMouseMoveX()`, `int GetMouseMoveY()`, `int GetMouseWheel()`.
- **Gamepad (XInput)**:
  - `bool IsGamepadConnected(int padIndex = 0)`.
  - `bool PushGamepadButton(WORD button)`, `bool TriggerGamepadButton(WORD button)`.
  - `float GetLeftStickX()`, `float GetLeftStickY()` (normalized with deadzone).
  - `void SetVibration(int padIndex, float leftMotor, float rightMotor)`.

---

## 5. Shader & Root Signature Pipeline

### Vertex Input Layout (`Object3d.VS.hlsl`):
| Semantic | Format | Description |
|---|---|---|
| `POSITION0` | `R32G32B32A32_FLOAT` | Local 3D vertex position (x, y, z, 1.0) |
| `TEXCOORD0` | `R32G32_FLOAT` | Texture coordinates (u, v) |
| `NORMAL0`   | `R32G32B32_FLOAT` | Surface normal vector (nx, ny, nz) |
| `BLENDINDICES0` | `R32G32B32A32_UINT` | 4 bone indices for GPU skinning |
| `BLENDWEIGHT0`  | `R32G32B32A32_FLOAT` | 4 bone blending weights (sum = 1.0) |

### Root Signature Bindings (`GraphicPipeline.cpp`):
| Root Slot | Register | Visibility | Type | Buffer Content |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material (Color, EnableLighting, LightingType) |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix (WVP Matrix, World Matrix) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2D Diffuse Texture |
| **3** | `b1` | Pixel Shader | CBV | LightGroup (Ambient, Directional, Point[3], Spot[3]) |
| **4** | `b1` | Vertex Shader | CBV | **BonePalette** (Array of 128 Skinning Matrices) |

### Circle Shadow Shaders (`CircleShadow.VS.hlsl` & `CircleShadow.PS.hlsl`):
- **Vertex Shader**: Transforms flat XZ quad vertices with position, texcoord, and normal without bone skinning overhead.
- **Pixel Shader**: Calculates an analytical radial ellipse distance from the center UV `(0.5, 0.5)` with `smoothstep` edge falloff, generating soft circular shadows with zero texture lookups.

