# Changelog — Rizeallday Engine v2

## [2.2.0] — 2026-10-30

### Fitur Baru
- **Character Controller**: Fungsi `MoveCharacter(motion, stepHeight)` untuk pergerakan karakter yang halus tanpa menembus dinding (AABB anti-tunneling), dilengkapi pijakan undakan (step up) dan pergeseran dinding (wall slide).
- **Sistem Navigasi & A* Pathfinding**: Penentuan jalur otomatis `FindPath` dan `RebuildNavigation` berbasis grid dari collider objek berlabel Static, dengan penghalusan jalur garis lurus (line-of-sight smoothing).
- **State Machine AI**: Helper `StateMachine` di `ScriptTimers.h` untuk mempermudah logika Finite State Machine musuh/karakter.
- **Objek Persisten Antar-Scene**: Fungsi `KeepAcrossScenes()` untuk mempertahankan objek (seperti Player atau GameManager) saat berpindah scene dengan `LoadScene()`.
- **Hot-Reload Shader**: Perubahan pada shader `Object3d` HLSL dimuat ulang secara otomatis (≤0.5 detik) tanpa perlu me-restart editor.
- **Toon Shading**: Model pencahayaan kartun (*Reflection Model: 5: Toon*) dengan pembagian bayangan bergradasi tegas dan sorotan spekular tajam.
- **Mesh Outline**: Efek garis tepi (outline) per mesh renderer berbasis *inverted hull* (`outlineWidth` dan `outlineColor`).
- **Panduan Pengembang Lengkap**: Penambahan `GUIDE_JA.md` (panduan alur kerja untuk tim dalam bahasa Jepang) dan `SCRIPTING_GUIDE.md` (referensi lengkap API C++ scripting).

---

## [2.1.0] — 2026-10-29

### Fitur Baru
- **GameScripts Mandiri Per-Project**: Setiap project game memiliki folder `GameScripts` dan solution sendiri sehingga tidak lagi bercampur dengan repositori engine.
- **Template Project Bersih**: Opsi pembuatan project baru dengan struktur folder rapi, `.gitignore`, `.gitattributes`, dan integrasi inisialisasi Git repository.
- **Updater Engine Terintegrasi**: Fitur **Help > Install Engine Update...** serta drag & drop file zip engine langsung ke dalam editor untuk memperbarui instalasi engine secara otomatis.
- **Game Feel Toolkit**: Efek guncangan kamera `ShakeCamera(strength, duration)` dan penghentian waktu benturan `Hitstop(duration)`.
- **Sistem Timer Script**: Helper `Cooldown` dan `TimerQueue` (`After`, `Every`) berbasis waktu permainan.
- **Input Actions**: Pemetaan input aksi (`ProjectSettings/input.json`) dengan dialog konfigurasi di **Options > Input Actions** untuk keyboard, mouse, dan gamepad XInput.
- **Fixed Timestep Physics**: Pembaruan simulasi fisika dengan langkah tetap 60 Hz untuk konsistensi di berbagai framerate.
