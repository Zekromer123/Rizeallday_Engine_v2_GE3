# Rizeallday Engine v2 — Claude Code Project Context

> Read this file first. Also read `REVIEW_NOTES.md` (code review from 2026-10-27) before starting work.
> `ENGINE_CONTEXT_HANDOVER.md` is partially OUTDATED — where it conflicts with this file or the code, trust the code.

## Communication
- Talk to the user in **Indonesian** (unless asked otherwise, e.g. Japanese for commit messages / school submissions).
- All C++ comments, log messages, and ImGui UI strings must be in **English**.

## Workspaces
- Primary: `D:\GE3\Rizeallday_Engine_v2_GE3`
- Mirror: `D:\GE3test\Rizeallday_Engine_v2` (keep synced only when the user asks)
- Solution: `Rizeallday_Engine_v2_GE3.slnx` (projects: `RizealldayEngine` .lib, `RizealldaySandbox` .exe, `GameScripts` .dll)

## Build (must verify after every change)
```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "Rizeallday_Engine_v2_GE3.slnx" /p:Configuration=Development /p:Platform=x64 /m /nologo
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "Rizeallday_Engine_v2_GE3.slnx" /p:Configuration=Release /p:Platform=x64 /m /nologo
```
Both `Development|x64` and `Release|x64` must end with **0 Warning(s), 0 Error(s)**.

## Mandatory coding rules
1. Save every `.h` / `.cpp` as **UTF-8 with BOM**.
2. Never modify anything in `externals/` (DirectXTex, imgui, assimp).
3. Use `(std::min)(a, b)`, `(std::max)(a, b)` or `std::clamp` (Windows macro collision).
4. Release GPU objects (`ID3D12Resource`, `ID3D12PipelineState`, ...) that may be in flight via `dxCommon_->DeferRelease(...)`.
5. `USE_IMGUI` is defined in Debug/Development only. Release builds have no ImGui — guard all ImGui code.
6. Edit shaders ONLY in `RizealldayEngine/shaders/`. `RizealldaySandbox/shaders/` is a post-build copy and gets overwritten.
7. Script DLL boundary: never pass `std::string`, `std::vector`, or the editor `GameObject` across it. Scripts only see `ScriptGameObjectView` (see `base/ScriptAPI.h`).

## Actual state of the code (verified 2026-10-27, mirror workspace)
These correct the handover document:

### SRV descriptor heap (`DirectXCommon.h`)
- `kMaxSrvDescriptors = 256` (NOT 1024)
- `[0]` ImGui · `[1..127]` TextureManager (`kMaxTextures = 128`, linear `nextIndex_`, no free-list)
- `[128]` Scene RT · `[129]` Game RT · `[130]` ShadowMap SRV · `[131]` Cubemap · `[132]` Asset/Model Preview RT
- `[133..136]` Preview texture ring (`kPreviewSrvBaseIndex = 133`, 4 slots) · `[140..249]` Particles
- RTV heap 8, DSV heap 8 (ShadowMap DSV = 3, Preview depth = 4)

### Object3d root signature (`GraphicPipeline.cpp`)
| Param | Type | Register | Visibility | Content |
|---|---|---|---|---|
| 0 | CBV | b3 | ALL | Material (PS + Explode GS) |
| 1 | CBV | b0 | Vertex | TransformationMatrix (WVP / World / WorldInverseTranspose) |
| 2 | Table | t0 | Pixel | Albedo texture |
| 3 | CBV | b1 | Pixel | LightGroup |
| 4 | CBV | b1 | Vertex | BonePalette (`float4x4 gBones[128]`) |
| 5 | CBV | b2 | Pixel | CameraForGPU |
| 6 | Table | t1 | Pixel | ShadowMap |

### Other facts
- Shadow map already uses **3x3 PCF** (`SampleCmpLevelZero`) in `Object3d.PS.hlsl`.
- Frame sync: single command allocator; `PostDraw()` waits on the fence every frame (no frames in flight).
- Each `Object3d` owns its own Material CB + 8 KB BonePalette CB (even static meshes). Every `GameObject` has `editObject3d`, `gameObject3d`, and `shadowTransform`.
- `GameObject` is a "fat struct" (all component fields inline) stored in `std::vector<GameObject> gameObjects_` — addresses are NOT stable.
- Play mode uses a JSON snapshot (`SavePlayModeSnapshot` / `RestorePlayModeSnapshot`). Undo pushes a full-scene JSON string per edit.
- JSON is parsed manually (string search) — no JSON library yet.
- Child transforms do NOT follow parents yet (`parentId` exists, no world-matrix propagation).
- Exported-game check in `EditorApp.cpp` looks for `BuildConfig.json`; exporter writes `game_build.cfg` (mismatch).

## Work plan
See `REVIEW_NOTES.md` for known bugs and the recommended order. Start with Step 1 (bug fixes) unless the user says otherwise.
