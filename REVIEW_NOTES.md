# Code Review & Roadmap Notes — 2026-10-27

Review dilakukan pada mirror `D:\GE3test\Rizeallday_Engine_v2`. Nomor baris bisa sedikit bergeser di workspace utama `D:\GE3`.

---

## A. Bug & risiko di kode (kerjakan dulu)

### A1. [HIGH] Crash/heap corruption saat script memanggil Instantiate di `Update()`
- **Lokasi:** `RizealldaySandbox/EditorSuite.cpp` ±baris 6490–6520 (loop update script) dan ±6769 (`engineApi_.instantiate`).
- **Masalah:** loop menyimpan `auto& obj = gameObjects_[i]`, `auto& sc = obj.scripts[s]`, dan `ScriptGameObjectView* view`, lalu memanggil `sc.instance->Update()`. Kalau script memanggil `instantiate`:
  - `CreateCube()` dan sejenisnya melakukan `push_back` ke `std::vector<GameObject> gameObjects_` → vector bisa realokasi → `obj` dan `sc` jadi dangling.
  - `RefreshAllScriptObjectViews()` me-resize `scriptObjectViews_` → `view` dan pointer `gameObject_` milik **semua** script jadi dangling.
  - Setelah `Update()`, `SyncViewToGameObject(*view, ...)` menulis ke memori yang sudah dibebaskan.
- **Perbaikan:**
  - (a) Antrekan spawn ke `pendingSpawns_` dan proses di akhir frame (seperti `pendingDeletes_`). Kembalikan ID/handle, bukan pointer.
  - (b) Atau ubah storage menjadi alamat stabil (`std::vector<std::unique_ptr<GameObject>>` + `std::deque`/stable pool untuk views).
  - Idealnya lakukan (a) **dan** (b).
- **Tes:** buat script yang men-spawn 50 kubus per frame selama beberapa detik dalam Play mode. Tes ini harus lolos tanpa crash di Development maupun Release.

### A2. [HIGH] Salah tafsir tipe di jalur fallback `IScriptHost`
- **Lokasi:** `RizealldayEngine/base/ScriptAPI.h` (`IScriptHost`, `ScriptBase::FindGameObjectById/ByName`), `EditorSuite::FindGameObjectById` (±baris 636).
- **Masalah:** `IScriptHost::FindGameObjectById` mengembalikan `GameObject*` editor yang asli (berisi `std::string` dll.). Di sisi DLL, `GameObject` adalah alias `ScriptGameObjectView`. Kalau `engineApi_` null, script membaca struct yang salah.
- **Perbaikan:** hapus fallback `host_` dari `ScriptBase` (cukup tabel `EngineAPI`), atau ubah `IScriptHost` agar mengembalikan `ScriptGameObjectView*`. Periksa juga `OnCollision(*reinterpret_cast<GameObject*>(otherView))` (±baris 6556) agar selalu memberikan view.

### A3. [MEDIUM] Sinkronisasi CPU/GPU dan constant buffer per objek
- **Lokasi:** `DirectXCommon::PostDraw` (±baris 249–256), `Object3d::Initialize`.
- **Masalah:**
  - Hanya ada satu command allocator dan CPU menunggu fence setiap frame, jadi CPU dan GPU tidak pernah paralel.
  - Tiap `Object3d` punya CB sendiri, sehingga tiap GameObject butuh `editObject3d` + `gameObject3d` + `shadowTransform`. Bone palette 8 KB juga dimiliki objek statis.
- **Perbaikan (fondasi untuk instancing/VAT/preview):**
  - Per-frame **linear upload allocator** (ring buffer di upload heap): tiap draw meminta potongan CB untuk WVP/material/bone sesuai kamera yang sedang dirender.
  - 2 frame in flight: command allocator per frame, tunggu fence frame N-2.
  - Setelah itu `editObject3d`/`gameObject3d`/`shadowTransform` bisa digabung menjadi satu.

### A4. [MEDIUM] Dokumentasi tidak cocok dengan kode
- `ENGINE_CONTEXT_HANDOVER.md` salah soal ukuran heap (256, bukan 1024), slot preview (133–136, bukan 127), root param 0 (b3/ALL) dan root param 5 (CameraForGPU), serta PCF yang sebenarnya sudah ada.
- Ada dua roadmap yang bertabrakan: `ENGINE_ROADMAP.md`/handover (Phase 0–5) dan `engine road map.docx` (Stage 0–5).
- **Perbaikan:** perbarui handover sesuai `CLAUDE.md`, lalu gabungkan kedua roadmap menjadi satu file.

### A5. [LOW] Salinan shader
- `RizealldaySandbox/shaders/` adalah salinan post-build dari `RizealldayEngine/shaders/`. Hasil edit di Sandbox akan tertimpa.
- Keluarkan folder itu dari source control (.gitignore), dan pastikan nanti shader hot-reload memantau `RizealldayEngine/shaders/`.

### Quick wins (dari Phase 0 lama, masih berlaku)
- Samakan `BuildConfig.json` dan `game_build.cfg` di `EditorApp.cpp` / exporter.
- Konstanta `KEY_*` di `ScriptAPI.h` agar script tidak perlu `<dinput.h>`.
- Inisialisasi `NodeData::transform` di `Model.h` (C26495).
- Badge `[AUDIO]` untuk `.m4a` / `.wma`.

---

## B. Catatan per Stage dari roadmap .docx

| Stage | Pendapat |
|---|---|
| 0 — Heap & modularisasi | Arahnya benar (heap 1024 + free-list, pecah `EditorSuite.cpp`). Untuk bone palette, linear allocator (A3) lebih tepat daripada buffer identitas global. **Tambahkan yang hilang:** nlohmann/json, transform hierarki, undo berbasis command. |
| 1 — Isolated Play World | Snapshot JSON sudah ada dan berfungsi, jadi kurang mendesak. Yang mendesak adalah A1. Catatan: `DeferRelease` khusus untuk resource GPU, bukan untuk menghancurkan "world". Hot-reload shader bagus, tapi butuh PSO yang bisa dibuat ulang. |
| 2 — PBR + preview material | Wajib digabung dengan **HDR RT (R16G16B16A16_FLOAT) + tonemapping ACES**, karena PBR tanpa HDR terlihat salah. Butuh tangent di `VertexData` + `aiProcess_CalcTangentSpace`. `.mat.json` butuh nlohmann/json lebih dulu. |
| 3 — Animasi | Crossfade harus mem-blend **pose lokal** (T: lerp, R: slerp, S: lerp per node) **sebelum** dikalikan ke hierarki, bukan matriks final. `Model::CalculateBoneTransforms` perlu dipecah menjadi `SampleLocalPose()` → `BlendPoses()` → `BuildSkinMatrices()`. Skeleton overlay dan animation events relatif mudah. |
| 4 — VAT | Lebar texture maksimal **16384**, jadi model dengan >16k vertex perlu layout yang dibungkus ke beberapa baris (simpan `rowsPerFrame` di `.vat.json`). Normalisasi posisi ke AABB agar presisi FP16 cukup. Bergantung pada A3 + structured buffer instancing. |
| 5 — Partikel multi-emitter | Partikel sekarang disimulasikan di CPU (`ParticleQuad`). Langkah "Hikari" yang sebenarnya adalah partikel compute shader (UAV + `ExecuteIndirect`). Slot SRV 140–249 yang hardcode perlu dipindah ke free-list. |

---

## C. Urutan kerja yang disarankan

1. **Perbaikan:** A1, A2, quick wins, perbarui handover dan gabungkan roadmap (A4).
2. **Fondasi:** pecah `EditorSuite.cpp` → nlohmann/json → transform hierarki → heap 1024 + free-list → linear CB allocator + 2 frames in flight (A3) → undo berbasis command.
3. **Animasi:** pemisahan pose lokal, crossfade, Animator params dari script, events, skeleton overlay.
4. **Grafis:** tangent + PBR + HDR + ACES + preview material (+ IBL).
5. **Workflow:** shader hot-reload, isolated play world, prefab.
6. **Performa:** GPU instancing → VAT crowd → partikel GPU.

Setiap langkah harus diakhiri dengan build Development|x64 dan Release|x64 yang **0 warning, 0 error**.
