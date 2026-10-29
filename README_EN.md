# Rizeallday Engine v2

**English** | [日本語](README_JA.md) | [Bahasa Indonesia](README.md)

A 3D game engine built on **DirectX 12** and **C++20**, with a Unity-style editor, hot-reloadable C++ scripting,
and a game build/export pipeline. Designed for making 3D games directly, from prototype to release build.

![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![API](https://img.shields.io/badge/graphics-DirectX%2012-green)
![Language](https://img.shields.io/badge/C%2B%2B-20-orange)

---

## Features

**Rendering**
- Pure DirectX 12: 2 frames in flight, per-frame upload allocator, free-list descriptor heap
- Shadow mapping (3×3 PCF), cubemap skybox, multi-lighting (directional, point, spot, ambient)
- FBX / OBJ / glTF models via Assimp, GPU skeletal animation (up to 128 bones)
- GPU-instanced particles, 2D sprites, and an in-game UI (text, images, bars, buttons) that also works in Release builds

**Editor**
- Project Hub, parent/child Hierarchy, Inspector, gizmos (move/rotate/scale), Asset Browser
- Undo/redo, JSON scenes, prefabs, Play / Pause / Step, full-window game preview (F11)
- GUID asset database (`.meta` files): renaming or moving assets in the Project panel keeps scenes and prefabs intact
- Japanese text (editor, game UI, file names) and an editor language option: English / 日本語 / Bahasa Indonesia (Options > Language)
- Animator: animation state machine with a node editor (Window > Animator), crossfades, parameters and events from scripts
- Asset import from inside the editor (dialog or drag from Explorer) and a recent-projects list (File > Open Recent)
- Profiler (CPU & GPU per pass, draw calls, upload memory), game Build & Export

**Gameplay & scripting**
- C++ scripts in `GameScripts.dll` with hot reload and Inspector-editable fields (`EXPOSE_FIELD`)
- Keyboard, mouse (world-space aiming) and XInput gamepad input
- Sphere / AABB / OBB collision with layers & masks and a grid broadphase, simple rigidbodies
- Prefabs with object pooling, scene switching, save data, time scale (pause / slow motion)
- `OnCollisionEnter/Exit` events, raycast / overlap-sphere / tag queries, and script control of audio, particles, materials, lights, rigidbodies and animation
- XAudio2 audio

**Quality**
- Headless self-tests (`--run-tests`), also on the WARP software adapter
- GitHub Actions CI: warning-free Development + Release builds and test run

---

## Requirements

- Windows 10/11 with a Direct3D 12 capable GPU
- Visual Studio 2022 (v143 toolset) or Visual Studio 2026 (v145), *Desktop development with C++* workload
- Windows 10/11 SDK

## Getting started

```bash
git clone --recursive https://github.com/Zekromer123/Rizeallday_Engine_v2_GE3.git
```

1. Open `Rizeallday_Engine_v2_GE3.slnx`.
2. Select **Development | x64**. The startup project is `RizealldaySandbox`.
3. Press **F5**, then create or open a project in the Project Hub.
4. Try the sample: **GameObject > Samples > Survivors Demo**, then press **Play**.

| Configuration | Purpose |
|---|---|
| `Debug` | Editor + D3D12 debug layer |
| `Development` | Editor (ImGui) with an optimized engine, for everyday work |
| `Release` | Game runtime without the editor, for player builds |

## Script example

```cpp
#include "base/ScriptAPI.h"

class Mover : public ScriptBase {
public:
    EXPOSE_FIELD(float, speed, 6.0f);

    void Update(float deltaTime) override {
        GameObject* self = GetOwner();
        const Vector2 move = GetMoveAxis();          // WASD / left stick
        self->translation.x += move.x * speed * deltaTime;
        self->translation.z += move.y * speed * deltaTime;
        if (IsKeyTriggered(KEY_SPACE)) InstantiatePrefab("Bullet", self->translation);
        UIText("Hello!", 24.0f, 24.0f);              // game UI, 1280x720 canvas
    }
};
REGISTER_SCRIPT(Mover)
```

Build scripts with **Ctrl+Shift+B** in the editor; the DLL reloads automatically.

## Repository layout

```
RizealldayEngine/     Static library: rendering, input, audio, collision, scripting runtime, HLSL shaders
RizealldaySandbox/    Editor + game runtime (.exe), self-tests, sample assets
GameScripts/          Gameplay scripts (.dll, hot reload)
externals/            Third-party libraries
.github/workflows/    CI
```

## Tests

```bash
bin\Development\RizealldaySandbox.exe --run-tests [filter] [--warp]
```

Run from the `RizealldaySandbox` folder. Exit code `0` means every test passed; the report is written to `test_results.txt`.

## Third-party libraries

- [DirectXTex](https://github.com/microsoft/DirectXTex) — MIT
- [Dear ImGui](https://github.com/ocornut/imgui) — MIT
- [Assimp](https://github.com/assimp/assimp) — BSD-3-Clause
- [nlohmann/json](https://github.com/nlohmann/json) — MIT
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) — University of Illinois/NCSA
