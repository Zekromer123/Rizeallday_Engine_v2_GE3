# Rizeallday Engine v2

Selamat datang di **Rizeallday Engine v2**!
Project ini adalah evolusi dari Rizeallday Engine yang telah direstrukturisasi menggunakan arsitektur modular **Engine + Sandbox** (mengikuti pola arsitektur modern seperti [TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) dan template [KamataEngine](file:///D:/KamataEngine%20New%20Version)).

Engine ini dikembangkan khusus dengan **DirectX 12** murni, C++20, static library linkage, integrasi **Assimp** (mendukung model 3D FBX, OBJ, GLTF, DAE, dsb.), sistem **Multi-Lighting** (Directional, Point, Spot, dan Ambient), serta **ImGui** terintegrasi untuk debugging visual.

---

## Daftar Isi
1. [Arsitektur & Struktur Folder](#1-arsitektur--struktur-folder)
2. [Cara Membuka & Menjalankan Project](#2-cara-membuka--menjalankan-project)
3. [Panduan Penggunaan (Quick Start)](#3-panduan-penggunaan-quick-start)
4. [Referensi API & Perintah (Command Reference)](#4-referensi-api--perintah-command-reference)
   - [Core Lifecycle (`RizeEngine`)](#core-lifecycle-rizeengine)
   - [Grafik & Device (`DirectXCommon`)](#grafik--device-directxcommon)
   - [Model 3D & Assimp Loader (`Model`)](#model-3d--assimp-loader-model)
   - [Objek 3D Instance (`Object3d`)](#objek-3d-instance-object3d)
   - [Transformasi Dunia (`WorldTransform`)](#transformasi-dunia-worldtransform)
   - [Kamera 3D (`Camera` & `DebugCamera`)](#kamera-3d-camera--debugcamera)
   - [Sistem Pencahayaan (`LightGroup`)](#sistem-pencahayaan-lightgroup)
   - [Sprite 2D (`Sprite`)](#sprite-2d-sprite)
   - [Primitive Debug Drawer (`PrimitiveDrawer`)](#primitive-debug-drawer-primitivedrawer)
   - [Pengelola Tekstur (`TextureManager`)](#pengelola-tekstur-texturemanager)
   - [Sistem Input Keyboard, Mouse & Gamepad (`Input`)](#sistem-input-keyboard-mouse--gamepad-input)
   - [Sistem Audio WAV (`Audio`)](#sistem-audio-wav-audio)
   - [Fungsi Matematika (`Mathematics` / `MathUtil`)](#fungsi-matematika-mathematics--mathutil)
5. [Format 3D & Shader Pipeline](#5-format-3d--shader-pipeline)
6. [Catatan Keamanan & Isolasi Tugas Kuliah](#6-catatan-keamanan--isolasi-tugas-kuliah)

---

## 1. Arsitektur & Struktur Folder

Solusi terbagi menjadi dua proyek utama:
- **`RizealldayEngine`**: Dibuat sebagai **Static Library (`.lib`)**. Berisi seluruh core rendering DX12, math, shader, assimp loader, input, audio, dan lighting. Anda tidak perlu mengutak-atik folder ini lagi saat membuat game.
- **`RizealldaySandbox`**: Dibuat sebagai **Executable (`.exe`)**. Tempat Anda menulis logika game, scene, entitas, dan gameplay loop. Project ini diset sebagai **Startup Project**.

```
D:\Rizeallday_Engine_v2\
│
├── Rizeallday_Engine_v2.slnx              # File Solution Visual Studio
├── README.md                               # Dokumentasi lengkap engine ini
│
├── RizealldayEngine/                       # [LIBRARY] Core Engine
│   ├── RizealldayEngine.vcxproj            # Project Static Library
│   ├── base/                               # Core Subsystems
│   │   ├── RizeEngine.h / .cpp             # Master include & lifecycle engine
│   │   ├── DirectXCommon.h / .cpp          # Inisialisasi DX12, Device, SwapChain, Heap
│   │   ├── WinApp.h / .cpp                 # Pembuatan & manajemen window Windows
│   │   ├── TextureManager.h / .cpp         # Auto-allocating descriptor & texture cache
│   │   ├── GraphicPipeline.h / .cpp        # Pipeline State Object (PSO) & Root Signature
│   │   ├── DXCManager.h / .cpp             # Runtime HLSL Shader Compiler (DXC)
│   │   ├── Log.h / .cpp                    # Logger output ke debug console
│   │   ├── ConvertString.h / .cpp          # Utilitas konversi wstring <-> string
│   │   └── CrashHandler.h / .cpp           # Minidump crash handler
│   ├── 3d/                                 # 3D Graphics & Lighting
│   │   ├── Model.h / .cpp                  # Assimp loader (FBX, OBJ, GLTF) + GPU Buffers
│   │   ├── Object3d.h / .cpp               # Enkapsulasi Model + WorldTransform + Camera
│   │   ├── Camera.h / .cpp                 # ViewProjection matrix generator
│   │   ├── DebugCamera.h / .cpp            # Orbit/pan debug camera via mouse
│   │   ├── WorldTransform.h / .cpp         # Transform SRV/CBV buffer manager
│   │   ├── LightGroup.h / .cpp             # Multi-light buffer manager
│   │   ├── DirectionalLight.h              # Struct data Directional Light
│   │   ├── PointLight.h                    # Struct data Point Light (posisi, radius, decay)
│   │   ├── SpotLight.h                     # Struct data Spot Light (posisi, cone angle)
│   │   └── PrimitiveDrawer.h / .cpp        # Debug 3D line & grid drawer
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
│       ├── Object3d.VS.hlsl                # Vertex shader 3D (+ worldPosition output)
│       ├── Object3d.PS.hlsl                # Pixel shader multi-light (Dir+Point+Spot+Ambient)
│       ├── Sprite.hlsli                    # Header shader sprite 2D
│       ├── Sprite.VS.hlsl                  # Vertex shader sprite 2D
│       └── Sprite.PS.hlsl                  # Pixel shader sprite 2D
│
├── RizealldaySandbox/                      # [GAME APPLICATION] Ruang Kerja Pembuatan Game
│   ├── RizealldaySandbox.vcxproj           # Project Executable (Startup Project)
│   ├── main.cpp                            # Game loop & logika game Anda
│   ├── dxcompiler.dll, dxil.dll            # DXC binaries pendukung runtime
│   ├── assimp-vc143-mt.dll                 # Assimp runtime DLL
│   └── Resources/                          # Asset game
│       ├── Model/                          # Simpan file .fbx, .obj, .gltf di sini
│       ├── Texture/                        # Simpan gambar .png, .jpg, .dds di sini
│       └── Sound/                          # Simpan audio .wav di sini
│
└── externals/                              # Pustaka Pihak Ketiga
    ├── assimp/                             # Open Asset Import Library (Include, Lib, Bin)
    ├── DirectXTex/                         # Pustaka manipulasi tekstur Microsoft DX12
    └── imgui/                              # Dear ImGui library (Debug UI)
```

---

## 2. Cara Membuka & Menjalankan Project

1. Buka file **`D:\Rizeallday_Engine_v2\Rizeallday_Engine_v2.slnx`** menggunakan **Visual Studio 2022** (v145 atau v143).
2. Di panel **Solution Explorer**, pastikan **`RizealldaySandbox`** bertuliskan **tebal (Bold)** yang menandakan ia adalah **Startup Project**. Jika belum, klik kanan pada `RizealldaySandbox` lalu pilih **Set as Startup Project**.
3. Pilih konfigurasi:
   - **`Debug | x64`**: ImGui aktif, verifikasi GPU & debug layer aktif.
   - **`Release | x64`**: Optimasi penuh kecepatan tinggi, ImGui dinonaktifkan.
4. Tekan tombol **F5** atau menu **Debug -> Start Debugging**.
5. Project akan otomatis mengompilasi library `RizealldayEngine`, menyalin shader dan DLL pendukung, lalu membuka jendela game.

---

## 3. Panduan Penggunaan (Quick Start)

Untuk membuat game di `RizealldaySandbox/main.cpp`, Anda hanya perlu menyertakan satu header utama: `#include <RizeEngine.h>`.

Contoh template minimal siap pakai:
```cpp
#include <RizeEngine.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // 1. Inisialisasi Engine (Judul window, lebar, tinggi)
    RizeEngine::Initialize("Petualangan Rizeallday", 1280, 720);

    // 2. Load Model 3D (Bisa FBX atau OBJ!)
    // Letakkan file di RizealldaySandbox/Resources/Model/
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
    // Tambahkan Point Light merah di dekat karakter:
    lights->GetPointLight(0).active = 1;
    lights->GetPointLight(0).position = {0.0f, 1.0f, -2.0f};
    lights->GetPointLight(0).color = {1.0f, 0.2f, 0.2f, 1.0f};
    lights->GetPointLight(0).radius = 8.0f;

    // 6. Game Loop
    while (RizeEngine::ProcessMessage()) {
        RizeEngine::BeginFrame(); // Input update, clear screen, ImGui new frame

        // --- UPDATE GAMEPLAY ---
        Input* input = RizeEngine::GetInput();
        if (input->PushKey(DIK_D)) {
            player.worldTransform.translation.x += 0.05f;
        }
        if (input->PushKey(DIK_A)) {
            player.worldTransform.translation.x -= 0.05f;
        }

        // Rotasi perlahan
        player.worldTransform.rotation.y += 0.02f;

        camera.Update();
        player.Update();
        lights->Update();

        // --- DEBUG IMGUI ---
#ifdef USE_IMGUI
        ImGui::Begin("Debug Panel");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::DragFloat3("Posisi Player", &player.worldTransform.translation.x, 0.1f);
        ImGui::ColorEdit3("Warna Ambient", &lights->GetDirectionalLight().color.x);
        ImGui::End();
#endif

        // --- RENDER 3D ---
        auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
        lights->Draw(cmdList, 3); // Binding light group ke root parameter 3
        player.Draw(cmdList);     // Render model player

        RizeEngine::EndFrame();   // ImGui render, present swapchain
    }

    // 7. Cleanup
    RizeEngine::Finalize();
    return 0;
}
```

---

## 4. Referensi API & Perintah (Command Reference)

### Core Lifecycle (`RizeEngine`)
Namespace utama pengendali engine:
- `RizeEngine::Initialize(const char* title, int width, int height)`
  Menginisialisasi seluruh sistem: Window, DirectX 12, DXC compiler, Graphic Pipeline, Texture Manager, Input, Audio, dan crash handler.
- `bool RizeEngine::ProcessMessage()`
  Memproses event OS Windows. Mengembalikan `false` jika jendela ditutup (tombol silang atau Alt+F4 ditekan), otomatis keluar dari while loop.
- `void RizeEngine::BeginFrame()`
  Memulai frame baru: membaca input hardware terbaru, membersihkan Render Target & Depth Stencil buffer, serta memulai frame baru ImGui.
- `void RizeEngine::EndFrame()`
  Menyelesaikan frame: me-render data ImGui ke command list, mengeksekusi GPU queue, dan melakukan `Present` ke layar.
- `void RizeEngine::Finalize()`
  Menunggu GPU menyelesaikan seluruh instruksi, melepaskan audio, menutup device DirectX, dan menghancurkan window dengan aman tanpa memory leak.
- `RizeEngine::GetDirectXCommon()` -> Mengambil pointer ke subsystem DirectX 12.
- `RizeEngine::GetInput()` -> Mengambil pointer ke sistem input.
- `RizeEngine::GetAudio()` -> Mengambil pointer ke sistem suara.

---

### Grafik & Device (`DirectXCommon`)
Singleton pengelola DirectX 12 (`DirectXCommon::GetInstance()`):
- `ID3D12Device* GetDevice() const`: Akses GPU device.
- `ID3D12GraphicsCommandList* GetCommandList() const`: Akses command list aktif untuk menggambar.
- `ID3D12CommandQueue* GetCommandQueue() const`: Command queue GPU.
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: Helper membuat buffer GPU upload (ConstantBuffer / VertexBuffer).
- `void SignalAndWaitFence()`: Sinkronisasi CPU-GPU (menunggu GPU selesai).

---

### Model 3D & Assimp Loader (`Model`)
Memuat dan me-render model 3D kompleks:
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`
  **Perintah Pemuat Model**:
  Membaca file model 3D menggunakan Assimp. Otomatis mengenali ekstensi:
  - `.fbx` (Autodesk FBX - model bertekstur, animasi pose)
  - `.obj` (Wavefront OBJ)
  - `.gltf` / `.glb` (GL Transmission Format)
  - `.dae` (Collada)
  - `.3ds` (3D Studio)
  Otomatis membaca susunan vertex, normal, UV texcoord, face indices, dan men-generate Index Buffer serta Vertex Buffer di VRAM.
  *Model yang sama di-cache secara otomatis sehingga pemanggilan berulang tidak membuang memori.*
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Mengikat vertex & index buffer lalu menjalankan `DrawIndexedInstanced`.

---

### Objek 3D Instance (`Object3d`)
Merepresentasikan satu entitas nyata di dunia 3D:
- `void Initialize()`: Mengalokasikan memory material & transform di GPU.
- `void SetModel(Model* model)`: Mengaitkan model 3D yang akan digambar.
- `void SetCamera(Camera* camera)`: Mengaitkan kamera yang melihat objek ini.
- `worldTransform`: Variabel publik bertipe `WorldTransform` untuk mengatur posisi, skala, dan rotasi.
- `void Update()`: Menghitung matriks WVP (World-View-Projection) terbaru dan mentransfer ke GPU.
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: Mengirim ConstantBuffer material ke root slot 0, WVP ke slot 1, dan menggambar model.

---

### Transformasi Dunia (`WorldTransform`)
Mengatur transformasi objek di ruang 3D:
- `Vector3 scale`: Skala objek (default `{1.0f, 1.0f, 1.0f}`).
- `Vector3 rotation`: Rotasi Euler radian (Pitch, Yaw, Roll).
- `Vector3 translation`: Posisi di dunia `{x, y, z}`.
- `Matrix4x4 matWorld`: Matriks affine hasil kalkulasi Transformasi.
- `void UpdateMatrix(const Matrix4x4& viewProjectionMatrix)`: Menghitung matriks WVP dan menyalin ke mapped GPU memory.

---

### Kamera 3D (`Camera` & `DebugCamera`)
- **`Camera`**:
  - `Vector3 translation`: Posisi kamera `{x, y, z}`.
  - `Vector3 rotation`: Sudut pandang kamera radian `{pitch, yaw, roll}`.
  - `float fovY`: Field of View vertikal (default `0.45f` radian ~ 25 derajat).
  - `float aspectRatio`: Rasio aspek (default `1280.0f / 720.0f`).
  - `float nearClip`, `float farClip`: Jarak potong dekat dan jauh (`0.1f` - `1000.0f`).
  - `void Update()`: Menghitung matriks View (inversi affine) dan Projection.
  - `const Matrix4x4& GetViewProjectionMatrix() const`: Matriks siap pakai untuk shader.

- **`DebugCamera`**:
  - Kamera pembantu navigasi scene menggunakan mouse:
    - **Klik Kanan + Drag**: Rotasi kamera (Orbit / Free Look).
    - **Klik Tengah (Scroll wheel) + Drag** atau **Shift + Klik Kanan**: Pan kamera ke samping / atas.
    - **W / A / S / D / Q / E**: Bergerak maju, mundur, kiri, kanan, naik, turun.

---

### Sistem Pencahayaan (`LightGroup`)
Mendukung multi-light dalam 1 ConstantBuffer terpadu:
- `static LightGroup* Create()`: Membuat instance LightGroup.
- `void SetDefaultLighting()`: Mengatur pencahayaan awal (Directional Light putih dari atas, ambient lembut).
- `void SetAmbientColor(const Vector3& color)`: Mengatur warna cahaya global lingkungan.
- `void SetAmbientIntensity(float intensity)`: Mengatur kuat cahaya ambient (0.0 - 1.0).
- `DirectionalLightData& GetDirectionalLight()`:
  - `.color` (Vector4 RGBA)
  - `.direction` (Vector3 arah cahaya, misal `{0, -1, 0}` ke bawah)
  - `.intensity` (float kekuatan cahaya)
  - `.active` (1 = aktif, 0 = mati)
- `PointLightData& GetPointLight(int index)` (Index 0 sampai 2):
  - `.position` (Vector3 titik lampu)
  - `.color` (Vector4 RGBA)
  - `.radius` (float jarak jangkauan cahaya)
  - `.decay` (float kelengkungan atenuasi redup, default 1.0)
  - `.intensity` (float)
  - `.active` (1 = nyala, 0 = mati)
- `SpotLightData& GetSpotLight(int index)` (Index 0 sampai 2):
  - `.position` (Vector3 posisi sumber spot)
  - `.direction` (Vector3 arah sorot)
  - `.distance` (float jarak sorot)
  - `.cosAngle` (float batas luar kerucut spot)
  - `.cosFalloffStart` (float batas dalam kerucut spot)
  - `.active` (1 = nyala, 0 = mati)
- `void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex = 3)`:
  Mengirim data seluruh cahaya ke Pixel Shader pada root slot 3.

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
- `void DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color)`: Menggambar garis lurus dari `p1` ke `p2`.
- `void DrawGrid(float size, float subdivisions, const Vector4& color)`: Menggambar kisi-kisi lantai 3D.
- `void Render(ID3D12GraphicsCommandList* cmdList)`: Menggambar seluruh antrean garis yang telah didaftarkan pada frame tersebut.

---

### Pengelola Tekstur (`TextureManager`)
Singleton pengelola gambar (`TextureManager::GetInstance()`):
- `uint32_t LoadTexture(const std::string& filePath)`:
  Membaca file gambar (`.png`, `.jpg`, `.dds`, dll.), mengunggahnya ke VRAM GPU melalui intermediate buffer, membuat MipMaps otomatis, dan mengalokasikan SRV Descriptor Heap.
  Mengembalikan **`uint32_t textureHandle`**.
- `D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t textureHandle)`:
  Mengambil GPU handle dari ID tekstur untuk diikatkan ke shader atau Sprite.

---

### Sistem Input Keyboard, Mouse & Gamepad (`Input`)
Singleton pengelola input (`Input::GetInstance()` atau `RizeEngine::GetInput()`):

#### Keyboard:
- `bool PushKey(BYTE keyNumber)`: `true` selama tombol ditekan (misal `DIK_SPACE`, `DIK_W`, `DIK_ESCAPE`).
- `bool TriggerKey(BYTE keyNumber)`: `true` hanya pada **frame pertama** saat tombol baru ditekan.
- `bool ReleaseKey(BYTE keyNumber)`: `true` saat tombol baru saja dilepas.

#### Mouse:
- `bool IsPressMouse(int button)` / `PushMouseButton(int button)`: 0 = Klik Kiri, 1 = Klik Kanan, 2 = Klik Tengah.
- `bool IsTriggerMouse(int button)`: `true` saat baru ditekan.
- `int GetMouseX()` / `GetMouseMoveX()`: Gerakan mouse horizontal relatif.
- `int GetMouseY()` / `GetMouseMoveY()`: Gerakan mouse vertikal relatif.
- `int GetMouseWheel()`: Putaran roda scroll mouse.

#### Gamepad (Xbox / XInput controller):
- `bool IsGamepadConnected(int padIndex = 0)`: Mengecek stik controller terhubung.
- `bool PushGamepadButton(WORD button, int padIndex = 0)`: Misal `XINPUT_GAMEPAD_A`, `XINPUT_GAMEPAD_DPAD_UP`.
- `bool TriggerGamepadButton(WORD button, int padIndex = 0)`: Trigger tombol gamepad.
- `float GetLeftStickX(int padIndex = 0)`: Analog kiri (-1.0f sampai 1.0f, sudah include deadzone).
- `float GetLeftStickY(int padIndex = 0)`: Analog kiri (-1.0f sampai 1.0f).
- `float GetRightStickX(int padIndex = 0)`: Analog kanan (-1.0f sampai 1.0f).
- `float GetRightStickY(int padIndex = 0)`: Analog kanan (-1.0f sampai 1.0f).
- `void SetVibration(int padIndex, float leftMotor, float rightMotor)`: Mengaktifkan getaran controller (0.0f sampai 1.0f).

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
- `Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)`: Matriks Transformasi lengkap TRS.
- `Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip)`: Matriks proyeksi perspektif.
- `Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)`: Matriks proyeksi ortografis (2D).
- `Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)`: Perkalian matriks.
- `Matrix4x4 Inverse(const Matrix4x4& m)`: Inversi matriks (berguna untuk View Matrix).
- `Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix)`: Mengalikan vektor 3D dengan matriks.

---

## 5. Format 3D & Shader Pipeline

### Skema Input Layout Vertex (`Object3d.VS.hlsl`):
| Semantic | Format | Deskripsi |
|---|---|---|
| `POSITION0` | `R32G32B32A32_FLOAT` | Posisi koordinat lokal 3D (x, y, z, 1.0) |
| `TEXCOORD0` | `R32G32_FLOAT` | Koordinat tekstur UV (u, v) |
| `NORMAL0`   | `R32G32B32_FLOAT` | Vektor Normal permukaan (nx, ny, nz) |

### Skema Root Signature (`GraphicPipeline.cpp`):
| Slot | Register | Shader Stage | Tipe | Konten |
|---|---|---|---|---|
| **0** | `b0` | Pixel Shader | CBV | Material (Warna dasar, EnableLighting, LightingType) |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix (WVP, World Matrix) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | Tekstur 2D Diffuse |
| **3** | `b1` | Pixel Shader | CBV | **LightGroup** (Ambient, Directional, Point[3], Spot[3]) |

---

## 6. Catatan Keamanan & Isolasi Tugas Kuliah

> [!IMPORTANT]
> **Project Tugas Kuliah Anda (`D:\Rizeallday_Engine`) Tetap 100% Aman:**
> Seluruh kode, konfigurasi, dan pembaruan arsitektur ini dibangun sepenuhnya di dalam folder **`D:\Rizeallday_Engine_v2`**. Folder asli tugas kuliah Anda tidak pernah dimodifikasi sama sekali sehingga Anda tidak perlu khawatir melanggar kurikulum atau kriteria penilaian evaluasi tugas kampus Anda.
