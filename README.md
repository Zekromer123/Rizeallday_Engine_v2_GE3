# Rizeallday Engine v2

[English](README_EN.md) | [日本語](README_JA.md) | **Bahasa Indonesia**

Selamat datang di **Rizeallday Engine v2**!

Project ini adalah engine game 3D modern yang dirancang menggunakan arsitektur modular **Engine + Sandbox** (mengikuti pola arsitektur modern seperti [TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) dan arsitektur game engine C++ profesional).

Engine ini dikembangkan khusus dengan **DirectX 12** murni, C++20, static library linkage, integrasi **Assimp** (mendukung model 3D FBX, OBJ, GLTF, DAE, dsb.), sistem **Multi-Lighting** (Directional, Point, Spot, dan Ambient), **GPU Instancing** dengan `StructuredBuffer + SV_InstanceID`, **Collision Detection System 3D Lengkap** (Sphere, AABB, OBB/Kotak Berotasi via SAT 15 sumbu, Plane, Triangle, Segment + Callback OnCollision), serta **ImGui** terintegrasi untuk debugging visual.

---

## Daftar Isi
1. [Fitur Utama](#1-fitur-utama)
2. [Arsitektur & Struktur Folder](#2-arsitektur--struktur-folder)
3. [Cara Membuka & Menjalankan Project](#3-cara-membuka--menjalankan-project)
4. [Panduan Penggunaan (Quick Start)](#4-panduan-penggunaan-quick-start)
5. [Referensi API & Perintah (Command Reference)](#5-referensi-api--perintah-command-reference)
   - [Core Lifecycle (`RizeEngine`)](#core-lifecycle-rizeengine)
   - [Grafik & Device (`DirectXCommon`)](#grafik--device-directxcommon)
   - [Pipeline & Blend Mode (`GraphicPipeline` / `BlendMode`)](#pipeline--blend-mode-graphicpipeline--blendmode)
   - [Model 3D & Assimp Loader (`Model`)](#model-3d--assimp-loader-model)
   - [Objek 3D Instance (`Object3d`)](#objek-3d-instance-object3d)
   - [Bayangan Lingkaran 3D (`CircleShadow`)](#bayangan-lingkaran-3d-circleshadow)
   - [Transformasi Dunia (`WorldTransform`)](#transformasi-dunia-worldtransform)
   - [Kamera 3D (`Camera`, `DebugCamera`)](#kamera-3d-camera-debugcamera)
   - [Sistem Pencahayaan (`LightGroup`)](#sistem-pencahayaan-lightgroup)
   - [GPU Instancing / Particle Quad (`ParticleQuad`)](#gpu-instancing--particle-quad-particlequad)
   - [Collision Detection 3D (`Collider` / `CollisionManager`)](#collision-detection-3d-collider--collisionmanager)
   - [Sprite 2D (`Sprite`)](#sprite-2d-sprite)
   - [Primitive Debug Drawer (`PrimitiveDrawer`)](#primitive-debug-drawer-primitivedrawer)
   - [Pengelola Tekstur (`TextureManager`)](#pengelola-tekstur-texturemanager)
   - [Sistem Input Keyboard, Mouse & Gamepad (`Input`)](#sistem-input-keyboard-mouse--gamepad-input)
   - [Sistem Audio WAV (`Audio`)](#sistem-audio-wav-audio)
   - [Fungsi Matematika (`Mathematics` / `MathUtil`)](#fungsi-matematika-mathematics--mathutil)
6. [Format 3D & Shader Pipeline](#6-format-3d--shader-pipeline)

---

## 1. Fitur Utama

- **Pure DirectX 12 Rendering**: Command list, descriptor heap, root signature, PSO, HLSL Shader Model 6.0 (`dxc`).
- **Multi-Blend Mode Rendering**: PSO terpisah untuk `Opaque`, `AlphaBlend`, `Add`, dan `Multiply`.
- **GPU Instancing**:
  - `StructuredBuffer<TransformationMatrix>` + `SV_InstanceID` di vertex shader.
  - `DrawInstanced(6, N, 0, 0)` untuk render N quad secara instan dalam 1 draw call.
  - Komponen `ParticleQuad` mandiri (RootSignature + PSO + SRV tersendiri di slot 20).
- **Collision Detection System 3D**:
  - Ragam bentuk: `SphereCollider`, `AABBCollider`, `OBBCollider` (kotak berotasi bebas), `PlaneCollider`, `TriangleCollider`, `Segment`, dan `Ray`.
  - Algoritma **Separating Axis Theorem (SAT) 15 sumbu** untuk tabrakan OBB vs OBB yang sangat presisi.
  - Algoritma **Slab Method** untuk raycasting/segment terhadap AABB dan OBB.
  - `CollisionManager` singleton dengan pola round-robin (総当たり).
  - `ICollidable` interface dengan callback `OnCollision()` polimorfik untuk Sphere, AABB, dan OBB.
  - Filter tabrakan menggunakan bitmask `attribute` dan `mask`.
- **Dynamic Circle / Blob Shadow**: Bayangan melingkar di lantai dengan radial falloff real-time tanpa tekstur eksternal.
- **Skeletal Mesh Animation**: Assimp FBX/GLTF/OBJ + GPU bone palette (128 matriks) dengan in-place root motion lock.
- **Unified Multi-Lighting**: 1 Directional + 3 Point + 3 Spot + Ambient.
- **Development | x64**: Konfigurasi hybrid, ImGui aktif, assert aktif, sandbox `/Od`, engine core `/O2`.
- **Input System**: DirectInput 8 (Keyboard & Mouse) + XInput (Gamepad + vibration).
- **Audio System**: XAudio2 low-latency PCM WAV.
- **Debug UI**: Dear ImGui terintegrasi penuh.

---

## 2. Arsitektur & Struktur Folder

Solusi terbagi menjadi dua proyek utama:
- **`RizealldayEngine`**: Dibuat sebagai **Static Library (`.lib`)**. Berisi seluruh core rendering DX12, math, shader, assimp loader, skeletal animation, input, audio, multi-lighting, blend mode, circle shadow, dan collision system.
- **`RizealldaySandbox`**: Dibuat sebagai **Executable (`.exe`)**. Tempat Anda menulis logika game, scene, entitas, dan gameplay loop. Project ini diset sebagai **Startup Project**.

```
D:\GE3\Rizeallday_Engine_v2_GE3\
│
├── Rizeallday_Engine_v2_GE3.slnx          # File Solution Visual Studio
├── README.md                               # Dokumentasi lengkap engine ini (Bahasa Indonesia)
├── README_EN.md                            # Documentation in English
├── README_JA.md                            # 日本語版ドキュメント
│
├── RizealldayEngine/                       # [LIBRARY] Core Engine
│   ├── RizealldayEngine.vcxproj            # Project Static Library
│   ├── base/                               # Core Subsystems
│   │   ├── RizeEngine.h / .cpp             # Master include & lifecycle engine
│   │   ├── DirectXCommon.h / .cpp          # Inisialisasi DX12, Device, SwapChain, Heap
│   │   ├── WinApp.h / .cpp                 # Pembuatan & manajemen window Windows
│   │   ├── BlendMode.h                     # Enum BlendMode (Opaque, AlphaBlend, Add, Multiply)
│   │   ├── TextureManager.h / .cpp         # Auto-allocating descriptor & texture cache
│   │   ├── GraphicPipeline.h / .cpp        # Pipeline State Object (PSO 4 Blend Mode) & Root Signature
│   │   ├── DXCManager.h / .cpp             # Runtime HLSL Shader Compiler (DXC)
│   │   ├── Log.h / .cpp                    # Logger output ke debug console
│   │   ├── ConvertString.h / .cpp          # Utilitas konversi wstring <-> string
│   │   └── CrashHandler.h / .cpp           # Minidump crash handler
│   ├── 3d/                                 # 3D Graphics, Lighting & Collision
│   │   ├── Model.h / .cpp                  # Assimp loader (FBX, OBJ, GLTF) + Bone Transforms
│   │   ├── Object3d.h / .cpp               # Enkapsulasi Model + WorldTransform + Camera + BlendMode
│   │   ├── CircleShadow.h / .cpp           # Blob shadow lingkaran 3D (soft-falloff quad di lantai)
│   │   ├── Camera.h / .cpp                 # ViewProjection matrix generator
│   │   ├── DebugCamera.h / .cpp            # Orbit/pan debug camera via mouse
│   │   ├── WorldTransform.h / .cpp         # Transform SRV/CBV buffer manager
│   │   ├── LightGroup.h / .cpp             # Multi-light buffer manager
│   │   ├── DirectionalLight.h              # Struct data Directional Light
│   │   ├── PointLight.h                    # Struct data Point Light (posisi, radius, decay)
│   │   ├── SpotLight.h                     # Struct data Spot Light (posisi, cone angle)
│   │   ├── PrimitiveDrawer.h / .cpp        # Debug 3D line & grid drawer
│   │   ├── Collider.h / .cpp               # Sphere, AABB, OBB, Plane, Triangle, Segment & intersection math
│   │   └── CollisionManager.h / .cpp       # CollisionManager singleton + ICollidable interface
│   ├── 2d/                                 # 2D Graphics
│   │   └── Sprite.h / .cpp                 # 2D Sprite renderer (UI & gambar 2D)
│   ├── input/                              # Input Handler
│   │   └── Input.h / .cpp                  # DirectInput (Keyboard/Mouse) + XInput (Gamepad)
│   ├── audio/                              # Audio System
│   │   └── Audio.h / .cpp                  # XAudio2 loader & playback WAV
│   ├── math/                               # Math Library
│   │   ├── Vector2.h, Vector3.h, Vector4.h # Struktur vector + operator overloads
│   │   ├── Matrix4x4.h                     # Matrix 4x4, Transform, TransformationMatrix
│   │   ├── MathUtil.h / .cpp               # Fungsi matematika (Affine, Perspective, dll)
│   │   └── VertexData.h                    # Definisi Vertex, Material, ModelData
│   └── shaders/                            # HLSL Shaders
│       ├── Object3d.hlsli                  # Header shader 3D (VertexShaderOutput)
│       ├── Object3d.VS.hlsl                # Vertex shader 3D (+ GPU Skinning & bone palette)
│       ├── Object3d.PS.hlsl                # Pixel shader multi-light (Dir+Point+Spot+Ambient)
│       ├── CircleShadow.VS.hlsl            # Vertex shader khusus Circle Shadow 3D
│       ├── CircleShadow.PS.hlsl            # Pixel shader Circle Shadow (radial gradient falloff)
│       ├── Particle.VS.hlsl                # Vertex shader GPU Instancing (StructuredBuffer + SV_InstanceID)
│       ├── Particle.PS.hlsl                # Pixel shader instancing (texture + alpha discard)
│       ├── Sprite.hlsli                    # Header shader sprite 2D
│       ├── Sprite.VS.hlsl                  # Vertex shader sprite 2D
│       └── Sprite.PS.hlsl                  # Pixel shader sprite 2D
│
├── RizealldaySandbox/                      # [GAME APPLICATION] Ruang Kerja Pembuatan Game
│   ├── RizealldaySandbox.vcxproj           # Project Executable (Startup Project)
│   ├── main.cpp                            # Game loop & logika game Anda
│   ├── ParticleQuad.h / .cpp               # Komponen GPU instancing quad
│   ├── dxcompiler.dll, dxil.dll            # DXC binaries pendukung runtime
│   ├── assimp-vc143-mt.dll                 # Assimp runtime DLL
│   └── Resources/                          # Asset game
│       ├── Model/                          # Simpan file .fbx, .obj, .gltf di sini
│       ├── circle.png                      # Tekstur circle putih 256x256
│       └── Sound/                          # Simpan audio .wav di sini
│
└── externals/                              # Pustaka Pihak Ketiga
    ├── assimp/                             # Open Asset Import Library (Include, Lib, Bin)
    ├── DirectXTex/                         # Pustaka manipulasi tekstur Microsoft DX12
    └── imgui/                              # Dear ImGui library (Debug UI)
```

---

## 3. Cara Membuka & Menjalankan Project

1. Buka file **`Rizeallday_Engine_v2_GE3.slnx`** menggunakan **Visual Studio 2022** (v145 atau v143).
2. Di panel **Solution Explorer**, pastikan **`RizealldaySandbox`** bertuliskan **tebal (Bold)** sebagai **Startup Project**. Jika belum, klik kanan → **Set as Startup Project**.
3. Pilih konfigurasi target:
   - **`Development | x64` (Direkomendasikan untuk pengembangan)**:
     - ImGui aktif (`USE_IMGUI`).
     - Assertion aktif (`assert()` aktif, `NDEBUG` tidak didefinisikan pada sandbox).
     - Kode sandbox dioptimasi `/Od` agar breakpoint dan debugging variabel akurat, sementara engine core tetap berjalan di `/O2` untuk performa maksimal.
   - **`Debug | x64`**:
     - ImGui aktif, Direct3D 12 Debug Layer aktif, verifikasi GPU penuh.
   - **`Release | x64`**:
     - Optimasi penuh kecepatan tinggi (`/O2`), ImGui dan debug logging dinonaktifkan untuk rilis pemain akhir.
4. Tekan tombol **F5** atau menu **Debug → Start Debugging**.

---

## 4. Panduan Penggunaan (Quick Start)

Untuk membuat game di `RizealldaySandbox/main.cpp`, Anda hanya perlu menyertakan satu header utama: `#include <RizeEngine.h>`.

Contoh template minimal siap pakai:
```cpp
#include <RizeEngine.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // 1. Inisialisasi Engine (Judul window, lebar, tinggi)
    RizeEngine::Initialize("Petualangan Rizeallday", 1280, 720);

    // 2. Load Model 3D (Bisa FBX atau OBJ!)
    Model* playerModel = Model::CreateFromFile("Resources/Model", "player.fbx");

    // 3. Siapkan Kamera
    Camera camera;
    camera.Initialize();
    camera.translation = { 0.0f, 2.0f, -10.0f };

    // 4. Siapkan Object3d (Instance dunia)
    Object3d player;
    player.Initialize();
    player.SetModel(playerModel);
    player.SetCamera(&camera);

    // 5. Konfigurasi Pencahayaan
    LightGroup* lights = LightGroup::Create();
    lights->SetDefaultLighting();

    // 6. Game Loop
    while (RizeEngine::ProcessMessage()) {
        RizeEngine::BeginFrame();

        Input* input = RizeEngine::GetInput();
        if (input->PushKey(DIK_D)) {
            player.worldTransform.translation.x += 0.05f;
        }

        camera.Update();
        player.Update(0.016f);
        lights->Update();

        auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
        lights->Draw(cmdList, 3);
        player.Draw(cmdList);

        RizeEngine::EndFrame();
    }

    RizeEngine::Finalize();
    return 0;
}
```

---

## 5. Referensi API & Perintah (Command Reference)

### Core Lifecycle (`RizeEngine`)
- `RizeEngine::Initialize(const char* title, int width, int height)`: Inisialisasi seluruh subsistem engine.
- `bool RizeEngine::ProcessMessage()`: Memproses event OS Windows. Mengembalikan `false` jika jendela ditutup.
- `void RizeEngine::BeginFrame()`: Membaca input terbaru, membersihkan Render Target & Depth Stencil, memulai frame ImGui.
- `void RizeEngine::EndFrame()`: Render ImGui, eksekusi GPU queue, Present ke layar.
- `void RizeEngine::Finalize()`: Melepaskan seluruh resource dengan aman.

---

### Grafik & Device (`DirectXCommon`)
- `ID3D12Device* GetDevice() const`: Akses GPU device.
- `ID3D12GraphicsCommandList* GetCommandList() const`: Command list aktif untuk menggambar.
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: Helper membuat buffer GPU.
- `void SignalAndWaitFence()`: Sinkronisasi CPU-GPU.

---

### Pipeline & Blend Mode (`GraphicPipeline` / `BlendMode`)
Engine menyediakan manajemen PSO terpisah untuk setiap mode blending:
```cpp
enum class BlendMode : int {
    Opaque     = 0, // Solid tidak transparan, Depth Write aktif
    AlphaBlend = 1, // Transparansi normal, Depth Write mati
    Add        = 2, // Additive blending (efek cahaya, api, sihir)
    Multiply   = 3, // Multiply blending (bayangan gelap, kutukan)
    Count      = 4
};
```

---

### Model 3D & Assimp Loader (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`: Membaca file `.fbx`, `.obj`, `.gltf`, `.glb`, `.dae` dengan auto-cache.
- `bool HasBones() const`: Mengetahui apakah model memiliki rig tulang.
- `size_t GetBoneCount() const`: Jumlah tulang pada model.
- `void SetLockRootMotion(bool lock)`: Mengunci translasi akar karakter (in-place) untuk animasi looping.

---

### Objek 3D Instance (`Object3d`)
- `void Initialize()`: Alokasi memory material, transform, dan bone palette (128 matriks).
- `void SetModel(Model* model, bool resetAnim = true)`: Mengaitkan model 3D.
- `void SetCamera(Camera* camera)`: Mengaitkan kamera.
- `WorldTransform worldTransform`: Variabel transform objek (scale, rotation, translation).
- `void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f)`.
- `void Update(float deltaTime)` / `void Draw(ID3D12GraphicsCommandList* cmdList)`.

---

### Bayangan Lingkaran 3D (`CircleShadow`)
- Menggambar drop shadow melingkar di lantai tanpa memerlukan file gambar tekstur eksternal:
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f };
shadow.GetMaterial()->softness = 0.5f;

// Di game loop:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera);
```

---

### Kamera 3D (`Camera`, `DebugCamera`)
- **`Camera`**: Kamera perspektif standar 3D (`translation`, `rotation`, `fovY`, `Update()`, `GetViewProjectionMatrix()`).
- **`DebugCamera`**: Kamera terbang bebas pengembang menggunakan mouse dan tombol WASDQE.

---

### Sistem Pencahayaan (`LightGroup`)
- `static LightGroup* Create()`, `SetDefaultLighting()`, `SetAmbientColor()`, `SetAmbientIntensity()`.
- Menyediakan 1 Directional Light, 3 Point Lights, dan 3 Spot Lights dalam satu ConstantBuffer GPU.

---

### GPU Instancing / Particle Quad (`ParticleQuad`)
Sistem rendering instancing batch berkecepatan tinggi:
- Satu vertex buffer 6 vertex untuk quad.
- `StructuredBuffer<TransformationMatrix>` untuk data matriks transformasi setiap instance.
- Vertex shader membaca matriks via `SV_InstanceID`.
- Memanggil `DrawInstanced(6, instanceCount, 0, 0)` untuk menggambar seluruh quad dalam 1 draw call.

---

### Collision Detection 3D (`Collider` / `CollisionManager`)

Engine menyediakan pustaka tabrakan 3D lengkap yang modular dan siap pakai:

#### 1. Ragam Bentuk Geometri (Collider Primitives):
- **`SphereCollider`**: Bola (`center`, `radius`)
- **`AABBCollider`**: Kotak sejajar sumbu (`min`, `max`, `SetCenterSize`)
- **`OBBCollider`**: **Kotak berotasi 3D** (`center`, `orientations[3]`, `size`, `SetRotation()`)
- **`PlaneCollider`**: Bidang datar (`normal`, `distance`)
- **`TriangleCollider`**: Segitiga 3D (`vertices[3]`, `Normal()`)
- **`Segment`**: Garis berarah (`origin`, `different`)
- **`Ray` & `Line`**: Sinar dan garis lurus 3D

#### 2. Tabel Kombinasi Deteksi Tabrakan:
| Fungsi Deteksi | Algoritma / Metode |
|---|---|
| `CheckSphereVsSphere(Sphere, Sphere)` | Perbandingan kuadrat jarak (tanpa `sqrt`) |
| `CheckSphereVsAABB(Sphere, AABB)` | Closest Point clamp pada AABB |
| `CheckSphereVsOBB(Sphere, OBB)` | Transformasi Sphere ke Local Space OBB |
| `CheckSphereVsPlane(Sphere, Plane)` | Dot product jarak ke bidang normal |
| `CheckAABBvsAABB(AABB, AABB)` | Tumpang tindih interval sumbu X, Y, Z |
| `CheckAABBvsSegment(AABB, Segment)` | Slab Method interval $t$ parametrik |
| `CheckOBBvsOBB(OBB, OBB)` | **Separating Axis Theorem (SAT) 15 sumbu** |
| `CheckOBBvsAABB(OBB, AABB)` | SAT dengan AABB sebagai OBB tereksitasi |
| `CheckOBBvsSegment(OBB, Segment)` | Local space Slab Method |
| `CheckPlaneVsSegment(Plane, Segment)` | Perhitungan titik tembus parameter $t$ |
| `CheckTriangleVsSegment(Triangle, Segment)` | Point-in-Triangle via Cross Product sisi segitiga |

#### 3. Fungsi Utilitas Geometri & Vektor:
- `ClosestPoint(point, aabb)`: Titik terdekat pada AABB
- `ClosestPoint(point, obb)`: Titik terdekat pada OBB
- `ClosestPoint(point, segment)`: Titik terdekat pada garis
- `Project(v1, v2)`: Proyeksi vektor $v_1$ ke $v_2$
- `Reflect(v, normal)`: Pantulan vektor terhadap normal bidang

#### 4. Integrasi ke Game Loop via `CollisionManager`:
```cpp
// Contoh: Entitas dengan OBB (Kotak berotasi mengikuti hadap karakter)
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
        obb_.SetRotation(rot); // Sinkronkan rotasi kotak dengan karakter
    }

    void OnCollision(ICollidable* other) override {
        // Callback otomatis saat mendeteksi tabrakan
    }
};
```

**Di dalam game loop:**
```cpp
// 1. Daftarkan collidable aktif setiap frame
CollisionManager::GetInstance()->RegisterCollider(&player);
CollisionManager::GetInstance()->RegisterCollider(&enemy);
CollisionManager::GetInstance()->RegisterCollider(&obstacle);

// 2. Jalankan pemeriksaan tabrakan
CollisionManager::GetInstance()->CheckAllCollisions();
```

---

### Sprite 2D (`Sprite`)
Untuk me-render elemen 2D (gambar UI, background, hud):
- `void Initialize(DirectXCommon* dxCommon)`, `SetTexture()`, `Update()`, `Draw()`.

---

### Primitive Debug Drawer (`PrimitiveDrawer`)
- `DrawLine3d(p1, p2, color)`: Garis lurus 3D.
- `DrawGrid(size, subdivisions, color)`: Kisi-kisi lantai 3D.
- `Render(cmdList)`: Menggambar seluruh antrean garis.

---

### Pengelola Tekstur (`TextureManager`)
- `uint32_t LoadTexture(filePath)`: Memuat gambar, membuat MipMaps, alokasi SRV descriptor, mengembalikan `textureHandle`.
- `D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(textureHandle)`: Mengambil GPU descriptor handle.

---

### Sistem Input Keyboard, Mouse & Gamepad (`Input`)
- **Keyboard**: `PushKey()`, `TriggerKey()`, `ReleaseKey()`.
- **Mouse**: `PushMouseButton()`, `IsTriggerMouse()`, `GetMouseMoveX()`, `GetMouseMoveY()`, `GetMouseWheel()`.
- **Gamepad**: `IsGamepadConnected()`, `PushGamepadButton()`, `TriggerGamepadButton()`, `GetLeftStickX()`, `GetLeftStickY()`, `SetVibration()`.

---

### Sistem Audio WAV (`Audio`)
- `SoundLoadWave(filename)`, `SoundPlayWave(soundData)`, `SoundUnload(soundData)`.

---

### Fungsi Matematika (`Mathematics` / `MathUtil`)
- `MakeIdentity4x4()`, `MakeAffineMatrix()`, `MakePerspectiveFovMatrix()`, `MakeOrthographicMatrix()`, `Multiply()`, `Inverse()`, `Transform()`.

---

## 6. Format 3D & Shader Pipeline

### Skema Input Layout Vertex (`Object3d.VS.hlsl`):
| Semantic | Format | Deskripsi |
|---|---|---|
| `POSITION0` | `R32G32B32A32_FLOAT` | Posisi koordinat lokal 3D (x, y, z, 1.0) |
| `TEXCOORD0` | `R32G32_FLOAT` | Koordinat tekstur UV (u, v) |
| `NORMAL0`   | `R32G32B32_FLOAT` | Vektor Normal permukaan (nx, ny, nz) |
| `BLENDINDICES0` | `R32G32B32A32_UINT` | 4 indeks tulang untuk GPU Skinning |
| `BLENDWEIGHT0`  | `R32G32B32A32_FLOAT` | 4 bobot pengaruh tulang (total bobot = 1.0) |

### Skema Root Signature (`GraphicPipeline.cpp` — Object3d):
| Slot | Register | Shader Stage | Tipe | Konten |
|---|---|---|---|---|
| **0** | `b0` | Pixel Shader | CBV | Material (Warna dasar, EnableLighting, LightingType) |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix (WVP, World Matrix) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | Tekstur 2D Diffuse |
| **3** | `b1` | Pixel Shader | CBV | **LightGroup** (Ambient, Directional, Point[3], Spot[3]) |
| **4** | `b1` | Vertex Shader | CBV | **BonePalette** (Array 128 Matriks Transformasi Tulang) |

### Root Signature Particle Instancing (`ParticleQuad`):
| Slot | Register | Shader Stage | Tipe | Konten |
|---|---|---|---|---|
| **0** | `b0` | Pixel Shader | CBV | Material (warna tint, alpha) |
| **1** | `t0` | Vertex Shader | Descriptor Table (SRV) | `StructuredBuffer<TransformationMatrix>` (slot 20) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | Tekstur 2D (circle.png, dll.) |
