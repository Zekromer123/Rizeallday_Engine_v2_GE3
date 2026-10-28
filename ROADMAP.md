# ROADMAP — Rizeallday Engine v2

> Satu-satunya roadmap aktif (menggantikan `ENGINE_ROADMAP.md` dan bagian rencana di `REVIEW_NOTES.md`, sejak 2026-10-28).
> Kesulitan: ⭐ mudah · ⭐⭐ sedang · ⭐⭐⭐ sulit · ⭐⭐⭐⭐ sangat sulit. Centang `[x]` saat item selesai.

## Selesai
- [x] **Tahap 1 — Perbaikan bug:** A1 (Instantiate di `Update()` → deferred spawn + view stabil), A2 (`IScriptHost` tidak lagi mengoper `GameObject*` editor)
- [x] **Tahap 2 — Fondasi:**
  - 2.1 pecah `EditorSuite.cpp`
  - 2.2 nlohmann/json (ID object tersimpan)
  - 2.3 transform hierarki
  - 2.4 SRV heap 1024 + free-list
  - 2.5 per-frame upload allocator + 2 frames in flight
  - 2.6 undo berbasis command
- [x] **Editor & render:**
  - fix cube inside-out
  - grid 3D ber-depth
  - gamma UI (UNORM RTV ImGui)
  - tema/font/toolbar baru
  - splash center
  - Project Hub
  - Open Scene per project
  - project-relative asset path
  - remove component
  - popup unsaved changes
  - fix lighting (directional basi, arah dari world matrix)
- [x] Dari roadmap lama: 1.1 (pecah EditorSuite), 1.2 (JSON library), 1.3 (hierarki), 1.6 (undo command), 4.2 (ring/linear allocator)

## Prinsip urutan
1. **Bisa diuji sebelum diperbesar.** Test runner dan profiler dibuat sebelum fitur besar.
2. **Data model dulu, fitur kemudian.** Component registry dan GUID dibuat sebelum Prefab dan UI Canvas.
3. **Engine yang bisa dipakai membuat game > efek visual.** Kecuali kalau targetnya demo portofolio (lihat jalur alternatif).

---

## Tahap 3 — Kualitas & Observability (baru, ±1 minggu) ⭐⭐
| # | Item | Kenapa | Selesai bila |
|---|---|---|---|
| 3.1 ✅ | **Test runner headless** (selesai 2026-10-28, 15 test) `RizealldaySandbox.exe --run-tests [filter]`. Hook uji sementara yang selama ini saya pakai dijadikan test permanen (JSON round-trip, undo/redo, hierarki, RemoveComponent, SRV allocator, spawn stress A1) | Mencegah regresi; setiap tahap berikutnya bisa diverifikasi otomatis | Exit code 0/1, laporan di log, semua test lama lolos |
| 3.2 ✅ | **CI GitHub Actions** (workflow siap 2026-10-28; diverifikasi lokal termasuk toolset v143 + WARP; run pertama di GitHub setelah push) (windows-latest): build Development + Release + `--run-tests` (WARP device, tanpa GPU) | Menjaga aturan "0 warning, 0 error" otomatis | Badge hijau di setiap push |
| 3.3 ✅ | **Profiler panel** (selesai 2026-10-28; Window > Profiler / Ctrl+P, diverifikasi di GPU dan WARP): CPU scope timer + GPU timestamp query per pass (shadow, scene, game, UI), draw call count, pemakaian upload allocator per frame, jumlah SRV bebas | Fitur grafis dan performa butuh angka | Panel "Stats" menampilkan ms per pass |
| 3.4 ✅ | **Quick wins tersisa** (selesai 2026-10-28): samakan `BuildConfig.json`/`game_build.cfg`; konstanta `KEY_*` di `ScriptAPI.h`; C26495 `NodeData::transform`; badge `.m4a/.wma` | Utang kecil yang sudah lama tercatat | Semua tertutup |
| 3.5 | **Satukan dokumen**: gabungkan `ENGINE_ROADMAP.md` + `REVIEW_NOTES.md` menjadi satu `ROADMAP.md` (berisi rencana ini, item selesai dicentang); perbarui `ENGINE_CONTEXT_HANDOVER.md` | Saat ini dua roadmap saling bertentangan | Satu sumber kebenaran |

## Tahap G — Siap untuk game tugas (survivors top-down 3D, target 2026-11-03) ⭐⭐
| # | Item | Status |
|---|---|---|
| G.1 ✅ | Lookup id O(1) (`FindGameObjectIndexById`, cache divalidasi saat hit) + collision layer/mask (Inspector "Layer" / "Collides With", disimpan di JSON, `ScriptLayer`/`LayerBit()` di `ScriptAPI.h`) + broadphase grid di `CollisionManager` (fallback brute force < 32 collider), bounds rigidbody di-cache | Selesai 2026-10-28, test `Collision.*`, `Objects.IdLookupSurvivesVectorChanges` |
| G.2 ✅ | Input script: gamepad (stik, trigger, tombol `PAD_*`, getaran), mouse (tombol, posisi di Game view, `GetMouseWorldPosition` ke lantai), `GetMoveAxis()`, time scale / pause, `GetTime`, unscaled dt | Selesai 2026-10-28, test `Scripting.MouseRayHitsGroundPlane`, `Scripting.TimeScaleScalesGameTime` |
| G.3 ✅ | Prefab (`Assets/Prefabs/*.prefab.json`, Hierarchy "Save as Prefab", double-click di Project), `InstantiatePrefab` (deferred, id di-remap), pool `Despawn` (reset ke nilai prefab + `Initialize()` lagi) | Selesai 2026-10-28, test `Prefabs.*` |
| G.4 | GPU instancing untuk mesh yang sama | Ditunda: ±500–1000 draw masih ringan di D3D12; ukur dulu dengan Profiler di game nyata |
| G.5 ✅ | UI game `UIRenderer` (kanvas 1280×720, rect/teks/gambar/bar/button, font atlas GDI, juga di Release) | Selesai 2026-10-28, test `UI.CanvasCommandsAndTextLayout`, dicek visual Development + Release |
| G.6 ✅ | `LoadScene` dari script (Stop mengembalikan scene asal di editor), save data `SaveData/save.json` | Selesai 2026-10-28, test `Runtime.*` |
| G.7 ✅ | `SCRIPTING_GUIDE.md`, GameObject > Samples > Survivors Demo + 5 script `Survivors*`, fingerprint API exe↔DLL, `--scene <file> --play` | Selesai 2026-10-28, test `Samples.SurvivorsDemoRuns` (Play 2,5 detik dengan DLL asli) |

## Tahap 4 — Data Model & Asset Pipeline (±2–3 minggu) ⭐⭐–⭐⭐⭐
| # | Item | Kenapa | Selesai bila |
|---|---|---|---|
| 4.1 | **Component registry (refactor bertahap, bukan ECS penuh)**: setiap komponen = struct sendiri + `ComponentInfo{name, draw, serialize, deserialize, remove}`. `GameObject` menyimpan `std::optional<T>` / `unique_ptr` per komponen. Inspector, serializer, RemoveComponent, dan menu Add Component berjalan lewat registry | Menambah komponen sekarang menyentuh 5 tempat; ini syarat Prefab/UI/Animator | Format scene tetap kompatibel (round-trip test lolos); Add/Remove/Inspector otomatis dari registry |
| 4.2 | **Asset Database + GUID** (`.meta` per aset, peta GUID↔path, dipindai saat project dibuka, rename di panel Project memperbarui peta) | Path putus saat rename; dibutuhkan Prefab dan export | Rename tekstur di editor tidak memutus scene |
| 4.3 | **Prefab** (`.prefab.json` berbasis GUID, instantiate dari editor + `Instantiate("Enemy")` dari script, override sederhana) | Fitur inti pembuatan game | Prefab ditaruh di scene, diedit, lalu instance-nya ikut berubah |
| 4.4 | **Per-project GameScripts**: Create Project menghasilkan `Scripts/<Project>Scripts.vcxproj` dari template; hot-reload memuat DLL milik project | Saat ini semua project berbagi satu DLL | Dua project punya script berbeda tanpa saling bentrok |
| 4.5 | **Exporter mandiri**: memakai runtime Release (bukan exe editor), hanya menyalin aset yang direferensikan (via GUID), dan menghasilkan folder game yang bersih | Hasil export sekarang = editor yang disamarkan | Game hasil export jalan di PC lain tanpa folder engine |

## Tahap 5 — Gameplay Runtime (±3–4 minggu) ⭐⭐–⭐⭐⭐
| # | Item | Kenapa | Selesai bila |
|---|---|---|---|
| 5.1 | **Animasi** (dari REVIEW_NOTES Tahap 3): `SampleLocalPose → BlendPoses → BuildSkinMatrices`, crossfade, Animator state machine dengan parameter dari script, animation events, skeleton overlay | Karakter FBX sudah ada tapi belum bisa transisi | Idle↔Walk crossfade mulus, dikendalikan dari script |
| 5.2 | **Physics: integrasi Jolt Physics** (rigid body, box/sphere/capsule/mesh, raycast, layer matrix) menggantikan AABB buatan sendiri | Collider anak tidak ikut rotasi, belum ada raycast; Jolt open-source dan dipakai di game AAA | Kubus bertumpuk stabil, raycast dari script, collider anak benar |
| 5.3 | **Input system**: action mapping (`"Jump"` → Space/A), gamepad XInput, mouse delta; diekspos ke script | Script masih memakai kode DIK mentah | Satu script jalan di keyboard dan gamepad |
| 5.4 | **Scene management runtime**: `LoadScene("Stage2")`, persistent object, save data API (JSON di `%APPDATA%`) | Alur Title → Game → Result | Game demo dengan 3 scene |
| 5.5 | **In-game UI (Canvas)**: Image, Text (DirectWrite → font atlas), Button, anchor; berjalan di Release tanpa ImGui | Game butuh HUD/menu | HUD skor + tombol Start di game hasil export |
| 5.6 | **Audio 3D & mixer** (X3DAudio, grup BGM/SE, fade) | Melengkapi Audio Source | Suara mengecil sesuai jarak |

## Tahap 6 — Grafis Modern (±4–6 minggu) ⭐⭐–⭐⭐⭐
Urutan wajib: **HDR → PBR → IBL**. PBR tanpa HDR tampak salah.
| # | Item | Selesai bila |
|---|---|---|
| 6.1 | **Shader hot-reload** (pantau `RizealldayEngine/shaders/`, buat ulang PSO, DeferRelease yang lama) | Edit .hlsl, simpan, langsung terlihat |
| 6.2 | **HDR render target (R16G16B16A16_FLOAT) + tonemapping ACES + exposure** | Viewport HDR, tidak ada banding |
| 6.3 | **PBR metallic/roughness** (tangent di `VertexData` + `aiProcess_CalcTangentSpace`, normal map, ORM) + **material asset `.mat.json` + preview** | Material sphere grid metal/rough terlihat benar |
| 6.4 | **IBL** (irradiance + prefiltered specular + BRDF LUT dari cubemap) | Objek PBR memantulkan skybox |
| 6.5 | **Post-processing**: bloom, color grading LUT, FXAA (TAA opsional) | Lampu terang ber-bloom |
| 6.6 | **Cascaded Shadow Maps** (3–4 cascade) + shadow untuk spot light | Bayangan tajam dekat dan jauh |
| 6.7 | **SSAO** | Sudut ruangan lebih gelap secara alami |

## Tahap 7 — Editor Pro (paralel dengan Tahap 5–6, item kecil) ⭐–⭐⭐
- **ImGui docking** (cabang docking imgui, layout yang bisa disimpan). Butuh ganti versi imgui di externals, jadi perlu persetujuan.
- **Gizmo:** ring rotasi, snap (Ctrl), mode local/world, multi-select, F = fokus kamera ke objek.
- **Copy/paste/duplicate** multi-object; thumbnail aset di panel Project; drag aset dari Project ke Viewport/Inspector.
- **Isolated play world** (salinan scene, bukan snapshot JSON); "Play from here".

## Tahap 8 — Performa & GPU-Driven (±1–2 bulan) ⭐⭐–⭐⭐⭐⭐
- Gabungkan `editObject3d`/`gameObject3d`/`shadowTransform` menjadi satu (sekarang aman berkat upload allocator).
- **GPU instancing** (mesh sama, satu draw call), lalu **VAT crowd** (layout 16k, presisi FP16).
- **Partikel GPU** (compute + `ExecuteIndirect`).
- **Job system** + perekaman command list multithread.
- Bindless (SM 6.6) lalu GPU culling (ExecuteIndirect) lalu Hi-Z occlusion.

## Tahap 9 — Next-Gen (opsional, portofolio) ⭐⭐⭐⭐
Mesh shader/meshlets, DXR (shadow/AO/reflection), volumetric fog, asset cooking (BC7, meshlet),
dan **MCP editor bridge** (AI agent bisa membaca/mengedit scene secara live).

---

## Urutan yang saya rekomendasikan
**3 (semua) → 4.1 → 4.2 → 4.3 → 5.1 → 5.2 → 5.3/5.4 → 6.1 → 6.2 → 6.3 → 5.5 → 4.4/4.5 → 6.4–6.7 → 8**,
dengan item kecil Tahap 7 diselipkan di sela-sela.

Jalur alternatif kalau target terdekat adalah **demo/presentasi visual**: 3.1–3.3 → 6.1 → 6.2 → 6.3 → 6.4 → 6.5 → 6.6 → baru kembali ke Tahap 4–5.

## Risiko
- **TINGGI** — 4.1 (component registry) menyentuh hampir semua file editor. Mitigasi: bertahap per komponen, dengan round-trip + undo test (3.1) sebagai jaring pengaman.
- **TINGGI** — 5.2 Jolt menambah library ke `externals/`, sehingga butuh persetujuan (aturan: externals hanya boleh ditambah, tidak diubah).
- **SEDANG** — 3.2 CI tanpa GPU: test render harus memakai WARP atau dilewati.
- **SEDANG** — 6.2/6.3 mengubah format RT dan root signature. Semua PSO (Sprite, Particle, Line, Skybox) perlu disesuaikan.
- **RENDAH** — 7 docking butuh ganti versi imgui.

## Verifikasi standar per item
- Kriteria "Selesai bila" di tabel tercapai.
- Build Development|x64 dan Release|x64: **0 Warning(s), 0 Error(s)**.
- `RizealldaySandbox.exe --run-tests` lolos (setelah 3.1 ada).
