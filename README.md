# Rizeallday Engine v2

[English](README_EN.md) | [日本語](README_JA.md) | **Bahasa Indonesia**

Selamat datang di **Rizeallday Engine v2**!

Project ini adalah evolusi dari Rizeallday Engine yang telah direstrukturisasi menggunakan arsitektur modular **Engine + Sandbox** (mengikuti pola arsitektur modern seperti [TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) dan template [KamataEngine](https://github.com)).

Engine ini dikembangkan khusus dengan **DirectX 12** murni, C++20, static library linkage, integrasi **Assimp** (mendukung model 3D FBX, OBJ, GLTF, DAE, dsb.), sistem **Multi-Lighting** (Directional, Point, Spot, dan Ambient), **GPU Instancing** dengan `StructuredBuffer + SV_InstanceID`, **Collision Detection System** (Sphere/AABB + Callback), serta **ImGui** terintegrasi untuk debugging visual.

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
   - [Collision Detection (`Collider` / `CollisionManager`)](#collision-detection-collider--collisionmanager)
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
- **GPU Instancing (Assignment 01_00)**:
  - `StructuredBuffer<TransformationMatrix>` + `SV_InstanceID` di vertex shader.
  - `DrawInstanced(6, N, 0, 0)` untuk render N quad sekali panggil.
  - `ParticleQuad` class mandiri (RootSignature + PSO + SRV sendiri, slot 20).
- **Collision Detection System** (Baru!):
  - `SphereCollider` dan `AABBCollider` siap pakai.
  - `CollisionManager` singleton dengan pola round-robin (総当たり).
  - `ICollidable` interface + callback `OnCollision()` sesuai kurikulum GE3.
  - Filter collisi via `attribute` + `mask` bitmask.
- **Dynamic Circle / Blob Shadow**: Bayangan melingkar di lantai tanpa tekstur eksternal.
- **Skeletal Mesh Animation**: Assimp FBX/GLTF/OBJ + GPU bone palette (128 matriks).
- **Unified Multi-Lighting**: 1 Directional + 3 Point + 3 Spot + Ambient.
- **Development | x64**: Konfigurasi hybrid, ImGui aktif, assert aktif, sandbox `/Od`, engine core `/O2`.
- **Input System**: DirectInput 8 (Keyboard & Mouse) + XInput (Gamepad + vibration).
- **Audio System**: XAudio2 low-latency PCM WAV.
- **Debug UI**: Dear ImGui terintegrasi penuh.

---

## 2. Arsitektur & Struktur Folder

Solusi terbagi menjadi dua proyek utama:
- **`RizealldayEngine`**: Dibuat sebagai **Static Library (`.lib`)**. Berisi seluruh core rendering DX12, math, shader, assimp loader, skeletal animation, input, audio, multi-lighting, blend mode, circle shadow, dan collision system. Anda tidak perlu mengutak-atik folder ini lagi saat membuat game.
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
│   │   ├── Collider.h / .cpp               # [NEW] SphereCollider, AABBCollider + free check functions
│   │   └── CollisionManager.h / .cpp       # [NEW] Singleton collision round-robin + ICollidable interface
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
│       ├── Particle.VS.hlsl                # [NEW] Vertex shader GPU Instancing (StructuredBuffer + SV_InstanceID)
│       ├── Particle.PS.hlsl                # [NEW] Pixel shader instancing (texture + alpha discard)
│       ├── Sprite.hlsli                    # Header shader sprite 2D
│       ├── Sprite.VS.hlsl                  # Vertex shader sprite 2D
│       └── Sprite.PS.hlsl                  # Pixel shader sprite 2D
│
├── RizealldaySandbox/                      # [GAME APPLICATION] Ruang Kerja Pembuatan Game
│   ├── RizealldaySandbox.vcxproj           # Project Executable (Startup Project)
│   ├── main.cpp                            # Game loop & logika game Anda
│   ├── ParticleQuad.h / .cpp               # [NEW] 10-quad GPU instancing (Assignment 01_00)
│   ├── dxcompiler.dll, dxil.dll            # DXC binaries pendukung runtime
│   ├── assimp-vc143-mt.dll                 # Assimp runtime DLL
│   └── Resources/                          # Asset game
│       ├── Model/                          # Simpan file .fbx, .obj, .gltf di sini
│       ├── circle.png                      # [NEW] Tekstur circle putih 256x256 (transparan bg)
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
5. Project akan otomatis mengompilasi library `RizealldayEngine`, menyalin shader dan DLL pendukung, lalu membuka jendela game.

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
    camera.translation = {0.0f, 2.0f, -10.0f};

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
Namespace utama pengendali engine:
- `RizeEngine::Initialize(const char* title, int width, int height)`
  Menginisialisasi seluruh sistem: Window, DirectX 12, DXC compiler, Graphic Pipeline, Texture Manager, Input, Audio, dan crash handler.
- `bool RizeEngine::ProcessMessage()`
  Memproses event OS Windows. Mengembalikan `false` jika jendela ditutup.
- `void RizeEngine::BeginFrame()`
  Membaca input terbaru, membersihkan Render Target & Depth Stencil, memulai frame baru ImGui.
- `void RizeEngine::EndFrame()`
  Render ImGui, eksekusi GPU queue, Present ke layar.
- `void RizeEngine::Finalize()`
  Menunggu GPU selesai, melepaskan semua resource, menutup window.
- `RizeEngine::GetDirectXCommon()` → Pointer ke subsystem DirectX 12.
- `RizeEngine::GetInput()` → Pointer ke sistem input.
- `RizeEngine::GetAudio()` → Pointer ke sistem suara.

---

### Grafik & Device (`DirectXCommon`)
Singleton pengelola DirectX 12 (`DirectXCommon::GetInstance()`):
- `ID3D12Device* GetDevice() const`: Akses GPU device.
- `ID3D12GraphicsCommandList* GetCommandList() const`: Akses command list aktif untuk menggambar.
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: Helper membuat buffer GPU upload.
- `void SignalAndWaitFence()`: Sinkronisasi CPU-GPU.

---

### Pipeline & Blend Mode (`GraphicPipeline` / `BlendMode`)
Engine menyediakan manajemen Pipeline State Object (PSO) terpisah untuk setiap mode blending:
```cpp
enum class BlendMode : int {
    Opaque     = 0, // Tidak transparan, Depth Write aktif (Default)
    AlphaBlend = 1, // Transparansi normal (SrcAlpha / InvSrcAlpha), Depth Write mati
    Add        = 2, // Additive blending (efek sihir, api, laser, kilau)
    Multiply   = 3, // Multiply blending (bayangan gelap, aura kutukan)
    Count      = 4
};
```
- Setiap `Object3d` memiliki properti `blendMode`.
- Mengubah mode blending:
  ```cpp
  character.SetBlendMode(BlendMode::AlphaBlend);
  character.GetMaterial()->color.w = 0.5f; // Transparansi 50%
  ```

---

### Model 3D & Assimp Loader (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`
  Membaca file model 3D menggunakan Assimp. Mendukung: `.fbx`, `.obj`, `.gltf`, `.glb`, `.dae`, `.3ds`.
  Model yang sama di-cache otomatis agar tidak membuang memori.
- `bool HasBones() const`: `true` jika model memiliki rig tulang untuk animasi.
- `size_t GetBoneCount() const`: Jumlah tulang pada model.
- `float GetAnimationDuration(int animIndex = 0) const`: Durasi animasi dalam detik.
- `void SetLockRootMotion(bool lock)`: Mengunci translasi akar karakter (in-place lock).
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Menjalankan `DrawIndexedInstanced`.

---

### Objek 3D Instance (`Object3d`)
- `void Initialize()`: Mengalokasikan memory material, transform, dan bone palette (128 matriks) di GPU.
- `void SetModel(Model* model, bool resetAnim = true)`: Mengaitkan model 3D.
- `void SetCamera(Camera* camera)`: Mengaitkan kamera.
- `worldTransform`: Variabel publik bertipe `WorldTransform` untuk mengatur posisi, skala, dan rotasi.
- `blendMode`: Mode blending objek.
- `Material* GetMaterial() const`: Akses data material GPU.
- **Kontrol Animasi Tulang (Skeletal Animation)**:
  - `void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f)`
  - `void SetAnimationSpeed(float speed)`
  - `float GetAnimationTime() const` / `float GetAnimationDuration() const`
- `void Update(float deltaTime = 1.0f / 60.0f)`: Menghitung matriks WVP terbaru dan mengevaluasi pose tulang.
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Mengikat PSO, material, WVP, bone palette, dan menggambar model.

---

### Bayangan Lingkaran 3D (`CircleShadow`)
- `void Initialize()`: Membuat vertex buffer quad, index buffer, constant buffer material & transform.
- `void Update(const Vector3& characterPos, float floorY, float radius, float opacity)`:
  - `characterPos`: Posisi karakter di dunia.
  - `floorY`: Ketinggian lantai.
  - `radius`: Ukuran radius bayangan.
  - `opacity`: Tingkat kegelapan bayangan.
- `void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera)`: Render quad bayangan.
- `CircleShadowMaterialGPU* GetMaterial()`: Mengatur `softness`, `color`, dan radius UV.

Contoh:
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f };
shadow.GetMaterial()->softness = 0.5f;

// Di dalam game loop:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera); // Render SEBELUM karakter
character.Draw(cmdList);
```

---

### Transformasi Dunia (`WorldTransform`)
- `Vector3 scale`: Skala objek (default `{1.0f, 1.0f, 1.0f}`).
- `Vector3 rotation`: Rotasi Euler radian (Pitch, Yaw, Roll).
- `Vector3 translation`: Posisi di dunia `{x, y, z}`.
- `Matrix4x4 matWorld`: Matriks affine hasil kalkulasi.
- `void UpdateMatrix(const Matrix4x4& viewProjectionMatrix)`: Menghitung WVP dan menyalin ke GPU.

---

### Kamera 3D (`Camera`, `DebugCamera`)
- **`Camera`**:
  - `Vector3 translation`: Posisi kamera.
  - `Vector3 rotation`: Sudut pandang kamera radian `{pitch, yaw, roll}`.
  - `float fovY`, `float aspectRatio`, `float nearClip`, `float farClip`.
  - `void Update()`: Menghitung matriks View (inversi affine) dan Projection.
  - `const Matrix4x4& GetViewProjectionMatrix() const`: Matriks siap pakai untuk shader.

- **`DebugCamera`**:
  - Kamera navigasi scene menggunakan mouse:
    - **Klik Kanan + Drag**: Rotasi kamera.
    - **Klik Tengah + Drag** / **Shift + Klik Kanan**: Pan kamera.
    - **W / A / S / D / Q / E**: Terbang bebas.

---

### Sistem Pencahayaan (`LightGroup`)
Mendukung multi-light dalam 1 ConstantBuffer terpadu:
- `static LightGroup* Create()`: Membuat instance LightGroup.
- `void SetDefaultLighting()`: Mengatur pencahayaan awal.
- `void SetAmbientColor(const Vector3& color)`: Mengatur warna cahaya global.
- `void SetAmbientIntensity(float intensity)`: Mengatur kuat cahaya ambient.
- `DirectionalLightData& GetDirectionalLight()`: `.color`, `.direction`, `.intensity`, `.active`.
- `PointLightData& GetPointLight(int index)` (Index 0-2): `.position`, `.color`, `.radius`, `.decay`, `.intensity`, `.active`.
- `SpotLightData& GetSpotLight(int index)` (Index 0-2): `.position`, `.direction`, `.distance`, `.cosAngle`, `.cosFalloffStart`, `.active`.
- `void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex = 3)`: Bind ke root slot 3.

---

### GPU Instancing / Particle Quad (`ParticleQuad`)

Sistem render banyak quad secara efisien menggunakan **GPU Instancing** (Assignment 01_00):

**Cara kerja:**
- Satu vertex buffer berisi 6 vertex (2 triangle) untuk bentuk quad.
- `StructuredBuffer<TransformationMatrix>` di GPU berisi transform untuk setiap instance.
- `Particle.VS.hlsl` membaca transformasi tiap instance via `SV_InstanceID`.
- `DrawInstanced(6, N, 0, 0)` merender N quad hanya dengan 1 draw call.

**Penggunaan:**
```cpp
uint32_t texHandle = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
ParticleQuad particles;
particles.Initialize(RizeEngine::GetDirectXCommon(), &dxc, texHandle, 10);

// Di game loop:
particles.Update(camera.GetViewProjectionMatrix());
particles.Draw(cmdList);
```

**Root Signature Particle (berbeda dari Object3d):**
| Slot | Register | Shader | Tipe | Konten |
|------|----------|--------|------|--------|
| 0 | `b0` | Pixel | CBV | Material (warna, alpha) |
| 1 | `t0` | Vertex | Descriptor Table (SRV) | StructuredBuffer (transform tiap instance, slot 20) |
| 2 | `t0` | Pixel | Descriptor Table (SRV) | Tekstur 2D (circle.png) |

---

### Collision Detection (`Collider` / `CollisionManager`)

Sistem deteksi tabrakan berdasarkan kurikulum GE3 — **Sphere vs Sphere** dengan algoritma kuadrat jarak (tanpa `sqrt`) dan pola callback `OnCollision()`.

**Tersedia dua tipe collider:**
```cpp
// Sphere collider (paling umum)
SphereCollider sphere;
sphere.center = { 0.0f, 0.0f, 0.0f };
sphere.radius = 1.0f;
sphere.attribute = CollisionAttribute::Player;
sphere.mask      = CollisionAttribute::EnemyBullet;

// AABB collider (kotak siku-siku)
AABBCollider box;
box.SetCenterSize({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }); // center, halfExtents
box.attribute = CollisionAttribute::Ground;
box.mask      = CollisionAttribute::Player;
```

**Attribute bitmask yang tersedia:**
| Attribute | Nilai | Keterangan |
|-----------|-------|------------|
| `CollisionAttribute::Player` | bit 0 | Objek player |
| `CollisionAttribute::Enemy` | bit 1 | Objek enemy |
| `CollisionAttribute::PlayerBullet` | bit 2 | Peluru player |
| `CollisionAttribute::EnemyBullet` | bit 3 | Peluru enemy |
| `CollisionAttribute::Ground` | bit 4 | Lantai/terrain |
| `CollisionAttribute::All` | 0xFFFFFFFF | Semua objek |

**Membuat class yang bisa bertabrakan (implement `ICollidable`):**
```cpp
// Player.h
#include <RizeEngine.h>

class Player : public ICollidable {
public:
    SphereCollider collider_;

    const SphereCollider& GetSphereCollider() const override { return collider_; }

    void OnCollision(ICollidable* other) override {
        // Reaksi saat terkena tabrakan
        hp_ -= 10;
    }

    void Initialize() {
        collider_.radius    = 1.0f;
        collider_.attribute = CollisionAttribute::Player;
        collider_.mask      = CollisionAttribute::EnemyBullet | CollisionAttribute::Enemy;
    }

    void Update(float dt) {
        // Update posisi karakter...
        // Update center collider sesuai posisi dunia:
        collider_.center = worldTransform.translation;
    }

private:
    int hp_ = 100;
    WorldTransform worldTransform;
};
```

**Pola penggunaan tiap frame (di `GameScene::Update()`):**
```cpp
// 1. Update semua objek terlebih dahulu
player.Update(deltaTime);
enemy.Update(deltaTime);
for (auto* bullet : enemyBullets) bullet->Update(deltaTime);

// 2. Register semua collidable ke CollisionManager
CollisionManager::GetInstance()->RegisterCollider(&player);
CollisionManager::GetInstance()->RegisterCollider(&enemy);
for (auto* bullet : enemyBullets) {
    CollisionManager::GetInstance()->RegisterCollider(bullet);
}

// 3. Jalankan pengecekan semua pasangan (round-robin / 総当たり)
// Otomatis memanggil OnCollision() pada pasangan yang bertabrakan
CollisionManager::GetInstance()->CheckAllCollisions();
```

**Fungsi deteksi collision mandiri (tanpa manager):**
```cpp
// Cek langsung antara dua sphere
bool hit = CheckSphereVsSphere(sphereA, sphereB);

// Cek sphere vs AABB
bool hit = CheckSphereVsAABB(sphere, aabb);

// Cek AABB vs AABB
bool hit = CheckAABBvsAABB(aabbA, aabbB);
```

**Algoritma inti (Sphere vs Sphere — tanpa sqrt untuk efisiensi):**
```cpp
// Syarat bertabrakan:
// distSq <= (radiusA + radiusB)^2
float dx = B.center.x - A.center.x;
float dy = B.center.y - A.center.y;
float dz = B.center.z - A.center.z;
float distSq = dx*dx + dy*dy + dz*dz;
float radiusSum = A.radius + B.radius;
bool hit = (distSq <= radiusSum * radiusSum);
```

---

### Sprite 2D (`Sprite`)
Untuk me-render elemen 2D (gambar UI, background, hud):
- `void Initialize(DirectXCommon* dxCommon)`
- `void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle)`: Menentukan tekstur yang ditampilkan.
- `transformSprite`: Posisi, rotasi, skala di layar piksel (0 - 1280, 0 - 720).
- `void Update()`: Menghitung transformasi matriks ortografis 2D.
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Menggambar quad 2D ke layar.

---

### Primitive Debug Drawer (`PrimitiveDrawer`)
Singleton untuk menggambar garis bantuan 3D (`PrimitiveDrawer::GetInstance()`):
- `void DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color)`: Garis lurus dari `p1` ke `p2`.
- `void DrawGrid(float size, float subdivisions, const Vector4& color)`: Kisi-kisi lantai 3D.
- `void Render(ID3D12GraphicsCommandList* cmdList)`: Menggambar seluruh antrean garis.

---

### Pengelola Tekstur (`TextureManager`)
Singleton pengelola gambar (`TextureManager::GetInstance()`):
- `uint32_t LoadTexture(const std::string& filePath)`:
  Membaca file gambar (`.png`, `.jpg`, `.dds`, dll.), mengunggah ke VRAM, membuat MipMaps otomatis, dan mengalokasikan SRV Descriptor Heap.
  Mengembalikan **`uint32_t textureHandle`**.
- `D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t textureHandle)`:
  Mengambil GPU handle dari ID tekstur untuk diikatkan ke shader atau Sprite.

---

### Sistem Input Keyboard, Mouse & Gamepad (`Input`)
Singleton pengelola input (`Input::GetInstance()` atau `RizeEngine::GetInput()`):

#### Keyboard:
- `bool PushKey(BYTE keyNumber)`: `true` selama tombol ditekan (misal `DIK_SPACE`, `DIK_W`).
- `bool TriggerKey(BYTE keyNumber)`: `true` hanya pada **frame pertama** saat tombol baru ditekan.
- `bool ReleaseKey(BYTE keyNumber)`: `true` saat tombol baru saja dilepas.

#### Mouse:
- `bool PushMouseButton(int button)`: 0 = Klik Kiri, 1 = Klik Kanan, 2 = Klik Tengah.
- `bool IsTriggerMouse(int button)`: `true` saat baru ditekan.
- `int GetMouseMoveX()` / `GetMouseMoveY()`: Gerakan mouse relatif.
- `int GetMouseWheel()`: Putaran roda scroll.

#### Gamepad (Xbox / XInput):
- `bool IsGamepadConnected(int padIndex = 0)`: Mengecek stik terhubung.
- `bool PushGamepadButton(WORD button, int padIndex = 0)`: Misal `XINPUT_GAMEPAD_A`.
- `bool TriggerGamepadButton(WORD button, int padIndex = 0)`: Trigger tombol.
- `float GetLeftStickX(int padIndex = 0)` / `GetLeftStickY()`: Analog kiri (-1.0f ~ 1.0f).
- `float GetRightStickX(int padIndex = 0)` / `GetRightStickY()`: Analog kanan.
- `void SetVibration(int padIndex, float leftMotor, float rightMotor)`: Getaran controller.

---

### Sistem Audio WAV (`Audio`)
Singleton suara (`Audio::GetInstance()` atau `RizeEngine::GetAudio()`):
- `SoundData SoundLoadWave(const std::string& filename)`: Membaca file `.wav` ke memori.
- `void SoundPlayWave(const SoundData& soundData)`: Memutar suara melalui XAudio2.
- `void SoundUnload(SoundData* soundData)`: Menghapus data suara dari RAM.

---

### Fungsi Matematika (`Mathematics` / `MathUtil`)
Kelas statis untuk kalkulasi matriks 3D:
- `Matrix4x4 MakeIdentity4x4()`: Matriks identitas.
- `Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)`: Matriks TRS.
- `Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip)`: Matriks proyeksi perspektif.
- `Matrix4x4 MakeOrthographicMatrix(...)`: Matriks proyeksi ortografis (2D).
- `Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)`: Perkalian matriks.
- `Matrix4x4 Inverse(const Matrix4x4& m)`: Inversi matriks (untuk View Matrix).
- `Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix)`: Mengalikan vektor 3D dengan matriks.

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

### Skema Root Signature (`GraphicPipeline.cpp`) — Object3d:
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

### Pipeline Shader Circle Shadow:
- **Vertex Shader**: Mentransformasi posisi vertex quad datar di sumbu XZ (tanpa skinning).
- **Pixel Shader**: Menghitung jarak radial elips dari pusat UV `(0.5, 0.5)` dengan `smoothstep` untuk menghasilkan tepi bayangan lembut tanpa tekstur bitmap.

### Pipeline Shader Particle Instancing:
- **Vertex Shader** (`Particle.VS.hlsl`): Membaca `StructuredBuffer<TransformationMatrix>` menggunakan `SV_InstanceID` untuk mentransformasi setiap quad ke posisi dunianya.
- **Pixel Shader** (`Particle.PS.hlsl`): Sampling tekstur, mengalikan dengan material color, dan men-discard fragment dengan alpha < 0.01 agar lingkaran circle.png terlihat bersih.
