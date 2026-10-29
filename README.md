# Rizeallday Engine v2

[English](README_EN.md) | [日本語](README_JA.md) | **Bahasa Indonesia**

Game engine 3D berbasis **DirectX 12** dan **C++20** dengan editor bergaya Unity, scripting C++ yang bisa di-*hot reload*,
dan pipeline build/export game. Dibuat untuk membuat game 3D secara langsung, dari prototipe sampai build rilis.

![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![API](https://img.shields.io/badge/graphics-DirectX%2012-green)
![Language](https://img.shields.io/badge/C%2B%2B-20-orange)

---

## Fitur

**Rendering**
- DirectX 12 murni: 2 frame in flight, upload allocator per frame, descriptor heap dengan free-list
- Shadow map (PCF 3×3), skybox cubemap, multi-lighting (directional, point, spot, ambient)
- Model FBX / OBJ / glTF via Assimp, animasi skeletal di GPU (hingga 128 bone)
- Partikel GPU instancing, sprite 2D, dan UI game (teks, gambar, bar, tombol) yang juga tampil di build Release

**Editor**
- Project Hub, Hierarchy dengan parent/child, Inspector, gizmo (move/rotate/scale), Asset Browser
- Undo/redo, scene JSON, prefab, Play / Pause / Step, preview game layar penuh (F11)
- Asset database GUID (`.meta`): rename / pindah aset di panel Project tidak memutus scene dan prefab
- Teks Jepang (editor, UI game, nama file) dan pilihan bahasa editor: English / 日本語 / Bahasa Indonesia (Options > Language)
- Animator: state machine animasi dengan editor node (Window > Animator), crossfade, parameter dan event dari script
- Import aset langsung dari editor (dialog atau drag dari Explorer) dan daftar project terakhir (File > Open Recent)
- Profiler (CPU & GPU per pass, draw call, memori upload), Build & Export game

**Gameplay & scripting**
- Script C++ di `GameScripts.dll` dengan hot reload dan field yang bisa diedit di Inspector (`EXPOSE_FIELD`)
- Input keyboard, mouse (posisi di dunia untuk membidik), dan gamepad XInput
- Collision Sphere / AABB / OBB dengan layer & mask serta broadphase grid, rigidbody sederhana
- Prefab dengan object pooling, pergantian scene, save data, time scale (pause / slow motion)
- Event `OnCollisionEnter/Exit`, raycast / overlap sphere / pencarian tag, kontrol audio, partikel, material, lampu, rigidbody, dan animasi dari script
- Audio XAudio2

**Kualitas**
- Self-test headless (`--run-tests`), termasuk di adapter software WARP
- CI GitHub Actions: build Development + Release (tanpa warning) dan menjalankan test

---

## Persyaratan

- Windows 10/11 dengan GPU yang mendukung Direct3D 12
- Visual Studio 2022 (toolset v143) atau Visual Studio 2026 (v145), workload *Desktop development with C++*
- Windows 10/11 SDK

## Memulai

```bash
git clone --recursive https://github.com/Zekromer123/Rizeallday_Engine_v2_GE3.git
```

1. Buka `Rizeallday_Engine_v2_GE3.slnx`.
2. Pilih konfigurasi **Development | x64**. Startup project: `RizealldaySandbox`.
3. Tekan **F5**, lalu buat atau buka project di Project Hub.
4. Coba contoh: **GameObject > Samples > Survivors Demo**, lalu tekan **Play**.

| Konfigurasi | Kegunaan |
|---|---|
| `Debug` | Editor + D3D12 debug layer |
| `Development` | Editor (ImGui) dengan engine teroptimasi, untuk pengembangan sehari-hari |
| `Release` | Runtime game tanpa editor, untuk build pemain |

## Contoh script

```cpp
#include "base/ScriptAPI.h"

class Mover : public ScriptBase {
public:
    EXPOSE_FIELD(float, speed, 6.0f);

    void Update(float deltaTime) override {
        GameObject* self = GetOwner();
        const Vector2 move = GetMoveAxis();          // WASD / stik kiri
        self->translation.x += move.x * speed * deltaTime;
        self->translation.z += move.y * speed * deltaTime;
        if (IsKeyTriggered(KEY_SPACE)) InstantiatePrefab("Bullet", self->translation);
        UIText("Hello!", 24.0f, 24.0f);              // UI game, kanvas 1280x720
    }
};
REGISTER_SCRIPT(Mover)
```

Build script dengan **Ctrl+Shift+B** di editor. DLL akan di-reload otomatis.

## Struktur repository

```
RizealldayEngine/     Static library: rendering, input, audio, collision, scripting runtime, shader HLSL
RizealldaySandbox/    Editor + runtime game (.exe), self-test, asset contoh
GameScripts/          Script gameplay (.dll, hot reload)
externals/            Library pihak ketiga
.github/workflows/    CI
```

## Test

```bash
bin\Development\RizealldaySandbox.exe --run-tests [filter] [--warp]
```

Jalankan dari folder `RizealldaySandbox`. Exit code `0` berarti semua test lolos, dan laporannya ada di `test_results.txt`.

## Library pihak ketiga

- [DirectXTex](https://github.com/microsoft/DirectXTex) — MIT
- [Dear ImGui](https://github.com/ocornut/imgui) — MIT
- [Assimp](https://github.com/assimp/assimp) — BSD-3-Clause
- [nlohmann/json](https://github.com/nlohmann/json) — MIT
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) — University of Illinois/NCSA
