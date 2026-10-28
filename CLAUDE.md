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

## Actual state of the code (updated 2026-10-28 after Stage 2, mirror workspace)
These correct the handover document:

### SRV descriptor heap (`DirectXCommon.h`)
- `kMaxSrvDescriptors = 1024`. Fixed slots: `[0]` ImGui, `[128..139]` (`kFixedSrvBegin/End`).
- `[128]` Scene RT · `[129]` Game RT · `[130]` ShadowMap SRV · `[131]` Cubemap · `[132]` Asset/Model Preview RT · `[133..136]` Preview texture ring
- Every other slot: free-list via `DirectXCommon::AllocateSrvIndex()` / `FreeSrvIndex()` (fence-deferred reuse).
  TextureManager handle == SRV index. Particles no longer use SRV descriptors (root SRV).
- RTV heap 8: `[0..1]` back buffers (sRGB, 3D), `[2..4]` Scene/Game/Preview RT, `[6..7]` back buffers (UNORM, ImGui only). DSV heap 8 (ShadowMap DSV = 3, Preview depth = 4)

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
- Frame sync: **2 frames in flight** (`DirectXCommon::kFramesInFlight`), one command allocator + fence value per slot; `PreDraw()` waits only for the frame that last used the slot.
- **Per-frame upload allocator** (`DirectXCommon::AllocateUpload / UploadData / UploadConstants`, 32 MB per frame). Any buffer the CPU rewrites every frame MUST go through it — never write a persistently mapped upload buffer that the GPU may still read. Object3d / WorldTransform / Camera / LightGroup / Sprite / Skybox / CircleShadow / PrimitiveDrawer / ParticleQuad keep CPU-side data and upload at bind time. Static meshes share one identity bone palette per frame.
- Every `GameObject` still has `editObject3d`, `gameObject3d`, and `shadowTransform` (could now be merged — optional).
- `GameObject` is a "fat struct" (all component fields inline) stored in `std::vector<GameObject> gameObjects_` — addresses are NOT stable. Object ids are preserved by save/load/undo.
- Transform hierarchy: `translation/rotation/scale` are LOCAL; `worldMatrix` is derived by `UpdateWorldMatrices()` (EditorSuite_Hierarchy.cpp). Use `GetWorldTRS()` for world values; `ReparentGameObject()` keeps the world transform. Math helpers: `RizealldaySandbox/TransformUtil.h` (row-vector convention).
- JSON: nlohmann/json v3.12.0 (`externals/nlohmann/`), helpers in `RizealldaySandbox/SceneJson.h` (`Parse`, `Read`, `DumpCompact`, `Num`, `Arr`). Per-object read/write: `WriteGameObjectJson / ReadGameObjectJson`.
- Undo is command-based (EditorSuite_SceneIO.cpp): `PushUndoSnapshot()` records only changed objects/settings vs. a baseline; continuous edits of the same objects within 0.6 s merge; Play-mode edits are not recorded. Use `ResetUndoHistory()` after loading/creating a scene.
- `EditorSuite.cpp` is split into `EditorSuite_*.cpp` files by responsibility (shared includes in `EditorSuiteInternal.h`).
- Play mode uses a JSON snapshot (`SavePlayModeSnapshot` / `RestorePlayModeSnapshot`).
- Exported-game check in `EditorApp.cpp` looks for `BuildConfig.json`; exporter writes `game_build.cfg` (mismatch).

### Game-ready systems (Tahap G, 2026-10-28; user-facing docs in `SCRIPTING_GUIDE.md`)
- `FindGameObjectById` is O(1) via `FindGameObjectIndexById` (cache validated on hit, rebuilt on miss — no invalidation needed).
- Collision: `layer`/`collisionMask` per object (saved in JSON, Inspector "Layer"/"Collides With", `ScriptLayer` + `LayerBit()` in ScriptAPI.h).
  `CollisionManager` uses a sorted-cell grid broadphase from 32 colliders (`SetBroadphaseEnabled`, `GetLastStats`). Rigidbody resolution caches bounds.
- Prefabs: `EditorSuite_Prefabs.cpp` (`SavePrefab`, `InstantiatePrefabNow`, `QueuePrefabSpawn`, `QueueDespawn` + pools). `GameObject::prefabSource/prefabNodeIndex` are runtime-only.
- Game UI: `RizealldayEngine/2d/UIRenderer` (1280x720 canvas, GDI font atlas via `TextureManager::CreateTextureFromRGBA`, shaders `UI.VS/PS.hlsl`).
  Cleared at the start of `UpdateScripts`, rendered after the 3D game view in both the editor RT and the Release backbuffer.
- Runtime: `EditorSuite_Runtime.cpp` (deferred `LoadScene` from scripts, origin restored on Stop; save data `SaveData/save.json`).
- Time: `timeScale_` / `gameTime_` scale script dt, physics and animation in Play. `gameViewRect_` maps the mouse to the Game view.
- `EngineAPI` grew (mouse, pad, time, prefab, UI, scene, save). The DLL exports `GameScripts_GetApiFingerprint`; `ScriptModule` refuses a mismatch
  (bump `kScriptApiVersion` when semantics change). `bin/Scripts` is shared by Development and Release — always rebuild both sides.
- Samples: GameObject > Samples > Survivors Demo (`EditorSuite_Samples.cpp`, scripts `GameScripts/Scripts/Survivors*`).
  Dev flag: `RizealldaySandbox.exe --scene <file> --play` (infers the project root from the scene's `Assets` folder).
- Windows macros: avoid method names `DrawText`, `LoadString` etc. (`UIRenderer::DrawString`, `SaveText/LoadText`).

## Work plan
`ROADMAP.md` is the single active roadmap (ENGINE_ROADMAP.md is archived; REVIEW_NOTES.md section B keeps technical notes).
Tahap 1–3 and Tahap G (game-ready systems for the survivors assignment) are done. Next: support the user while they build the game; G.4 GPU instancing only if the Profiler shows draw calls are the bottleneck.
After every change: both builds 0/0 and the self-tests must pass. Run them from the Sandbox folder:
`bin\Development\RizealldaySandbox.exe --run-tests [filter] [--warp]` (exit code 0 = pass, report in `RizealldaySandbox/test_results.txt`;
`--warp` = D3D12 software adapter, as used by CI). New features should add a `TEST_CASE` in `RizealldaySandbox/EditorSuite_Tests.cpp`.
CI: `.github/workflows/build.yml` builds Development + Release (fails on warnings outside `externals/`), runs the tests on WARP,
and uploads `test_results.txt` + logs. The runner has VS 2022, so it builds with `PlatformToolset=v143` (code must stay v143-compatible).
