# Rizeallday Engine v2

[Bahasa Indonesia](README.md) | [日本語](README_JA.md) | **English**

Welcome to **Rizeallday Engine v2**!  
This project is a modern 3D game engine built from the ground up using a clean modular **Engine + Sandbox** architecture (following modern C++ game engine paradigms such as [TheCherno Hazel Engine](https://www.youtube.com/@TheCherno)).

Built with pure **DirectX 12**, **C++20**, static library linkage, **Assimp** integration (supporting animated 3D FBX, OBJ, GLTF, DAE, etc.), GPU-accelerated **Skeletal Animation** (up to 128 bones palette), a unified **Multi-Lighting** pipeline (Directional, Point, Spot, and Ambient lights), **GPU Particle Instancing** via `StructuredBuffer + SV_InstanceID`, a comprehensive **3D Collision Detection System** (Sphere, AABB, OBB/Rotated Box via 15-axis SAT, Plane, Triangle, Segment + Callback pattern), and runtime **Dear ImGui** debugging.

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
   - [Camera (`Camera`, `DebugCamera`)](#camera-camera-debugcamera)
   - [Lighting System (`LightGroup`)](#lighting-system-lightgroup)
   - [GPU Instancing & Particles (`ParticleQuad`)](#gpu-instancing--particles-particlequad)
   - [3D Collision Detection System (`Collider` / `CollisionManager`)](#3d-collision-detection-system-collider--collisionmanager)
   - [2D Sprite Renderer (`Sprite`)](#2d-sprite-renderer-sprite)
   - [Primitive Debug Drawer (`PrimitiveDrawer`)](#primitive-debug-drawer-primitivedrawer)
   - [Texture Manager (`TextureManager`)](#texture-manager-texturemanager)
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
- **GPU Instancing**:
  - High-performance multi-quad instancing using `StructuredBuffer<TransformationMatrix>` and `SV_InstanceID`.
  - Single-draw-call rendering with `DrawInstanced(6, N, 0, 0)`.
  - Self-contained `ParticleQuad` component with dedicated Root Signature and PSO (descriptor table binding at SRV slot 20).
- **3D Collision Detection System**:
  - Comprehensive collision primitives: `SphereCollider`, `AABBCollider`, `OBBCollider` (freely rotated 3D box), `PlaneCollider`, `TriangleCollider`, `Segment`, and `Ray`.
  - **15-Axis Separating Axis Theorem (SAT)** implementation for high-precision OBB vs OBB collisions.
  - **Slab Method** for segment/ray intersections against AABB and OBB.
  - `CollisionManager` singleton performing round-robin pairwise checks each frame.
  - `ICollidable` interface supporting polymorphic Sphere, AABB, and OBB colliders with `OnCollision()` callbacks.
  - Fast bitmask collision filtering via `attribute` and detection `mask`.
- **Dynamic Circle / Blob Shadows**:
  - Ground-projected circular drop shadow with smooth radial `smoothstep` falloff.
  - Real-time controls for shadow radius, opacity, softness, and tint color without external textures.
- **Skeletal Mesh Animation**:
  - Full Assimp integration supporting FBX (Mixamo rigs, etc.), OBJ, GLTF, GLB, and Collada DAE.
  - Skinned vertex shader computing matrix blending for up to 4 bone influences per vertex (`BLENDINDICES` + `BLENDWEIGHT`).
  - ConstantBuffer bone palette supporting up to 128 bones per mesh.
  - Slerp quaternion & Lerp vector keyframe interpolation with in-place root motion locking.
- **Unified Multi-Lighting**:
  - Directional Light (Lambertian / Half-Lambert diffuse with color & intensity control).
  - 3 Point Lights with radius and exponential distance decay attenuation.
  - 3 Spot Lights with distance attenuation and smoothstep cone falloff.
  - Ambient Lighting with customizable color and intensity.
- **Development | x64 Configuration**:
  - Optimized hybrid configuration with `assert()` enabled (no `NDEBUG`), Dear ImGui active (`USE_IMGUI`), `/Od` unoptimized sandbox for accurate debugging, and `/O2` engine core for top frame rates.
- **Input System**: DirectInput 8 (Keyboard & Mouse) + XInput (Xbox Gamepad with analog deadzone filtering & dual-motor vibration).
- **Audio System**: Low-latency PCM WAV audio playback powered by XAudio2.
- **Debug UI**: Seamlessly integrated Dear ImGui for live inspection and tweaking of transforms, lighting, animation playback, blend modes, shadows, and particles.

---

## 2. Architecture & Directory Structure

The solution (`Rizeallday_Engine_v2_GE3.slnx`) is separated into two clean projects:
- **`RizealldayEngine`**: **Static Library (`.lib`)**. Contains all DX12 core rendering, shaders, mathematics, Assimp loaders, skeletal animation, input, lighting, audio, and collision detection.
- **`RizealldaySandbox`**: **Executable (`.exe`)**. The active gameplay application and startup project where game logic, scenes, entities, and controllers are implemented.

```
D:\GE3\Rizeallday_Engine_v2_GE3\
│
├── Rizeallday_Engine_v2_GE3.slnx          # Visual Studio Solution
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
│   │   ├── ConvertString.h / .cpp          # String <-> wstring conversion utility
│   │   └── CrashHandler.h / .cpp           # Minidump crash reporter
│   ├── 3d/                                 # 3D Graphics, Lighting & Collision
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
│   │   ├── PrimitiveDrawer.h / .cpp        # Debug 3D line & grid renderer
│   │   ├── Collider.h / .cpp               # Sphere, AABB, OBB, Plane, Triangle, Segment & intersection math
│   │   └── CollisionManager.h / .cpp       # CollisionManager singleton & ICollidable interface
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
│       ├── Particle.VS.hlsl                # GPU Instancing vertex shader (StructuredBuffer + SV_InstanceID)
│       ├── Particle.PS.hlsl                # Instancing pixel shader (textured + alpha discard)
│       ├── Sprite.hlsli                    # 2D sprite shader header
│       ├── Sprite.VS.hlsl                  # 2D sprite vertex shader
│       └── Sprite.PS.hlsl                  # 2D sprite pixel shader
│
├── RizealldaySandbox/                      # [APPLICATION] Game Sandbox
│   ├── main.cpp                            # Gameplay loop & character controller
│   ├── ParticleQuad.h / .cpp               # GPU instancing quad manager
│   ├── dxcompiler.dll, dxil.dll            # DirectX Shader Compiler binaries
│   ├── assimp-vc143-mt.dll                 # Assimp runtime DLL
│   └── Resources/                          # Game Assets
│       ├── Model/                          # .fbx, .obj, .gltf 3D models & textures
│       ├── circle.png                      # 256x256 white circle texture
│       └── Sound/                          # .wav audio files
│
└── externals/                              # Third-Party Dependencies
    ├── assimp/                             # Open Asset Import Library
    ├── DirectXTex/                         # Microsoft DirectXTex texture processing
    └── imgui/                              # Dear ImGui library
```

---

## 3. Quick Start Guide

### Building and Running
1. Open **`Rizeallday_Engine_v2_GE3.slnx`** in Visual Studio 2022 (v143 / v145 toolset).
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

---

## 4. API & Subsystem Reference

### Engine Lifecycle (`RizeEngine`)
- `RizeEngine::Initialize(const char* title, int width, int height)`: Initializes Win32 window, DirectX 12 device, DXC compiler, PSOs, texture managers, input, and audio subsystems.
- `bool RizeEngine::ProcessMessage()`: Pumps Windows OS messages. Returns `false` on exit request (`WM_QUIT`).
- `void RizeEngine::BeginFrame()`: Clears backbuffer & depth stencil, polls input devices, starts ImGui frame.
- `void RizeEngine::EndFrame()`: Renders ImGui draw data, executes command lists, presents swapchain.
- `void RizeEngine::Finalize()`: Waits for GPU idle via fences and safely releases all COM resources.

---

### DirectX 12 Subsystem (`DirectXCommon`)
- `ID3D12Device* GetDevice() const`: Direct GPU device access.
- `ID3D12GraphicsCommandList* GetCommandList() const`: Active command list for drawing.
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: Allocates GPU upload buffer.
- `void SignalAndWaitFence()`: CPU-GPU synchronization barrier.

---

### Pipeline & Blend Modes (`GraphicPipeline` / `BlendMode`)
```cpp
enum class BlendMode : int {
    Opaque     = 0, // Solid opaque mesh, Depth Write ON (Default)
    AlphaBlend = 1, // Standard alpha blending, Depth Write OFF
    Add        = 2, // Additive blending (particles, fire, glow, magic effects)
    Multiply   = 3, // Multiplicative darkening (shadows, curse effects)
    Count      = 4
};
```

---

### 3D Model & Skeletal Mesh (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`: Loads `.fbx`, `.obj`, `.gltf`, `.glb`, `.dae` with automated caching.
- `bool HasBones() const`: Returns `true` if model contains rigged skeletal bone data.
- `size_t GetBoneCount() const`: Returns total bound bones.
- `void SetLockRootMotion(bool lock)`: Locks horizontal root motion for clean in-place locomotion.

---

### 3D Entity Instance (`Object3d`)
- `void Initialize()`, `SetModel()`, `SetCamera()`, `PlayAnimation()`.
- `WorldTransform worldTransform`: Contains `scale`, `rotation`, and `translation`.
- `void Update(float deltaTime)`, `void Draw(ID3D12GraphicsCommandList* cmdList)`.

---

### Dynamic Circle Shadow (`CircleShadow`)
Ground-projected procedural circular drop shadow without bitmap textures:
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f };
shadow.GetMaterial()->softness = 0.5f;

// Inside game loop:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera);
```

---

### Camera (`Camera`, `DebugCamera`)
- **`Camera`**: Perspective 3D camera (`translation`, `rotation`, `fovY`, `Update()`, `GetViewProjectionMatrix()`).
- **`DebugCamera`**: Mouse-driven development camera with right-click orbit, middle-click pan, and WASDQE navigation.

---

### Lighting System (`LightGroup`)
Unified multi-light ConstantBuffer manager supporting 1 Directional Light, 3 Point Lights, 3 Spot Lights, and Ambient lighting.

---

### GPU Instancing & Particles (`ParticleQuad`)
A high-performance batch quad renderer using GPU Instancing:
- Single vertex buffer with 6 vertices forming a quad.
- `StructuredBuffer<TransformationMatrix>` holding world transforms for each instance.
- Dynamic matrix indexing in vertex shader via `SV_InstanceID`.
- Single draw call: `cmdList->DrawInstanced(6, instanceCount, 0, 0)`.

---

### 3D Collision Detection System (`Collider` / `CollisionManager`)

A complete, high-performance 3D collision detection suite:

#### 1. Collision Primitives:
- **`SphereCollider`**: Center and radius.
- **`AABBCollider`**: Axis-aligned box defined by `min` and `max`.
- **`OBBCollider`**: Freely rotated 3D box defined by `center`, `orientations[3]` (normalized local axes), and `size` (half-extents). Supports `SetRotation(euler)`.
- **`PlaneCollider`**: Infinite plane defined by `normal` and `distance`.
- **`TriangleCollider`**: 3D triangle defined by 3 vertices.
- **`Segment`**: Line segment defined by `origin` and `different` (direction/displacement).
- **`Ray` & `Line`**: Directed ray and infinite line.

#### 2. Intersection Routines:
| Detection Function | Algorithm / Technique |
|---|---|
| `CheckSphereVsSphere(Sphere, Sphere)` | Squared distance comparison (no square root) |
| `CheckSphereVsAABB(Sphere, AABB)` | Clamped closest point on AABB to sphere center |
| `CheckSphereVsOBB(Sphere, OBB)` | Transforms sphere into OBB local space |
| `CheckSphereVsPlane(Sphere, Plane)` | Dot product perpendicular distance |
| `CheckAABBvsAABB(AABB, AABB)` | Axis interval overlap on X, Y, Z |
| `CheckAABBvsSegment(AABB, Segment)` | Slab Method for parametric $t$ intervals |
| `CheckOBBvsOBB(OBB, OBB)` | **Separating Axis Theorem (SAT) on 15 axes** |
| `CheckOBBvsAABB(OBB, AABB)` | 15-axis SAT with aligned bounding box |
| `CheckOBBvsSegment(OBB, Segment)` | Local space Slab Method |
| `CheckPlaneVsSegment(Plane, Segment)` | Parametric plane penetration test |
| `CheckTriangleVsSegment(Triangle, Segment)` | Point-in-Triangle test via edge cross products |

#### 3. Geometric Math Helpers:
- `ClosestPoint(point, aabb)`: Nearest point on AABB.
- `ClosestPoint(point, obb)`: Nearest point on OBB.
- `ClosestPoint(point, segment)`: Nearest point on line segment.
- `Project(v1, v2)`: Vector projection.
- `Reflect(v, normal)`: Vector reflection against surface normal.

#### 4. CollisionManager & `ICollidable`:
```cpp
class Player : public ICollidable {
public:
    OBBCollider obb_;

    ColliderShape GetColliderShape() const override { return ColliderShape::OBB; }
    const OBBCollider& GetOBBCollider() const override { return obb_; }

    void Initialize() {
        obb_.size = { 0.4f, 0.9f, 0.4f };
        obb_.attribute = CollisionAttribute::Player;
        obb_.mask      = CollisionAttribute::Obstacle | CollisionAttribute::EnemyBullet;
    }

    void Update(const Vector3& pos, const Vector3& rot) {
        obb_.center = pos;
        obb_.SetRotation(rot); // Updates local orientation axes
    }

    void OnCollision(ICollidable* other) override {
        // Automatic callback when hit
    }
};
```

---

### 2D Sprite Renderer (`Sprite`)
For 2D UI, backgrounds, and HUD elements:
- `Initialize()`, `SetTexture()`, `transformSprite`, `Update()`, `Draw()`.

---

### Input System (`Input`)
- **Keyboard (DirectInput 8)**: `PushKey()`, `TriggerKey()`, `ReleaseKey()`.
- **Mouse**: `PushMouseButton()`, `IsTriggerMouse()`, `GetMouseMoveX()`, `GetMouseMoveY()`, `GetMouseWheel()`.
- **Gamepad (XInput)**: `IsGamepadConnected()`, `PushGamepadButton()`, `TriggerGamepadButton()`, `GetLeftStickX()`, `GetLeftStickY()`, `SetVibration()`.

---

### Audio System (`Audio`)
- `SoundLoadWave(filename)`, `SoundPlayWave(soundData)`, `SoundUnload(soundData)`.

---

### Math Library (`Mathematics` / `MathUtil`)
- `MakeIdentity4x4()`, `MakeAffineMatrix()`, `MakePerspectiveFovMatrix()`, `MakeOrthographicMatrix()`, `Multiply()`, `Inverse()`, `Transform()`.

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

### Root Signature Bindings (`GraphicPipeline.cpp` — Object3d):
| Root Slot | Register | Visibility | Type | Buffer Content |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material (Color, EnableLighting, LightingType) |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix (WVP Matrix, World Matrix) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2D Diffuse Texture |
| **3** | `b1` | Pixel Shader | CBV | LightGroup (Ambient, Directional, Point[3], Spot[3]) |
| **4** | `b1` | Vertex Shader | CBV | **BonePalette** (Array of 128 Skinning Matrices) |

### Particle Instancing Root Signature (`ParticleQuad`):
| Root Slot | Register | Visibility | Type | Buffer Content |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material (Tint Color, Alpha) |
| **1** | `t0` | Vertex Shader | Descriptor Table (SRV) | `StructuredBuffer<TransformationMatrix>` (SRV Slot 20) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2D Diffuse Texture (`circle.png`) |
