# Panduan Scripting — Rizeallday Engine v2

Panduan singkat untuk membuat game (contoh: survivors top-down 3D) dengan C++ script di `GameScripts.dll`.
Semua contoh API di sini ada di `RizealldayEngine/base/ScriptAPI.h` (kelas `ScriptBase`).

---

## 1. Mulai cepat (5 menit)

1. Buka/buat project di Project Hub.
2. **GameObject > Samples > Survivors Demo (new scene)**. Ini membuat:
   - `Assets/Scenes/SurvivorsDemo.scene.json` (Main Camera, Directional Light, Ground, Player, GameManager)
   - `Assets/Prefabs/Enemy.prefab.json` dan `Assets/Prefabs/Bullet.prefab.json`
3. Tekan **Play** (atau **F11** untuk preview layar penuh).
   - Gerak: WASD / stik kiri. Bidik: mouse / stik kanan (tembak otomatis).
   - Pause: P / Esc / Start.
4. Buka script contoh di `GameScripts/Scripts/Survivors*.cpp`, ubah, lalu **Build Scripts** (Ctrl+Shift+B).
   DLL di-reload otomatis (hot reload). Nilai `EXPOSE_FIELD` tetap tersimpan.

| Script | Dipasang di | Isi |
|---|---|---|
| `SurvivorsPlayer` | Player | gerak, bidik, tembak prefab `Bullet`, HP, kena damage dari layer Enemy |
| `SurvivorsBullet` | prefab Bullet | terbang lurus, kembali ke pool setelah `lifetime` / kena musuh |
| `SurvivorsEnemy` | prefab Enemy | mengejar player, mati setelah `maxHp` peluru |
| `SurvivorsCamera` | Main Camera | kamera top-down yang mengikuti player |
| `SurvivorsGameManager` | GameManager (Empty) | spawner, HUD, pause menu, game over, high score |
| `SurvivorsShared.h` | — | state bersama (kills, posisi player, dll.) |

---

## 2. Bentuk sebuah script

```cpp
// MyScript.h
#pragma once
#include "base/ScriptAPI.h"

class MyScript : public ScriptBase {
public:
    EXPOSE_FIELD(float, speed, 5.0f);   // tampil & bisa diedit di Inspector
    EXPOSE_FIELD(int, hp, 3);

    void Initialize() override;          // sekali saat Play (dan setiap kali diambil dari pool)
    void Update(float deltaTime) override;
    void OnCollision(GameObject& other) override;
    void OnDestroy() override {}

private:
    float timer_ = 0.0f;                 // TIDAK disimpan saat hot reload -> reset di Initialize()
};

// MyScript.cpp
#include "MyScript.h"
void MyScript::Initialize() { timer_ = 0.0f; }
void MyScript::Update(float dt) {
    GameObject* self = GetOwner();       // view milik object ini
    if (!self) return;
    self->translation.x += speed * dt;
}
void MyScript::OnCollision(GameObject& other) {
    if (other.layer == LAYER_ENEMY) { /* ... */ }
}
REGISTER_SCRIPT(MyScript)
```

- Script baru: Inspector > **Add Component > Script > New Script...** (file dibuat + didaftarkan ke project).
- Tipe `EXPOSE_FIELD`: `float`, `int`, `bool`, `Vector3`, `char[N]` (`ScriptText`).
- `GameObject` di dalam script adalah view sederhana: `id`, `name`, `tag`, `layer`, `collisionMask`,
  `translation`, `rotation` (Euler, radian), `rotationQuat`, `scale`, `velocity`, `isActive`, `isGrounded`, `isColliding`.
  Jika mengubah `rotation`, ubah juga `rotationQuat = Quaternion::FromEuler(rotation)` (lihat `Survivors::SetYaw`).
- Batas DLL: jangan mengirim `std::string` / `std::vector` ke engine. Di dalam DLL sendiri boleh.

---

## 3. Referensi API

### Input
| Fungsi | Keterangan |
|---|---|
| `IsKeyDown(KEY_W)`, `IsKeyTriggered(KEY_SPACE)` | gunakan konstanta `KEY_*` (bukan `'W'`) |
| `GetMoveAxis()` | `Vector2` dari WASD + panah + stik kiri, panjang ≤ 1 (x = kanan, y = maju/+Z) |
| `IsMouseDown(0)`, `IsMouseTriggered(0)` | 0 kiri, 1 kanan, 2 tengah; hanya saat kursor di Game view |
| `GetMouseWorldPosition(out, planeY)` | titik di lantai `y = planeY` di bawah kursor (untuk membidik) |
| `GetMousePosition(px, &size)`, `GetMouseCanvasPosition(out)` | posisi mouse (pixel / kanvas UI) |
| `IsPadConnected()`, `IsPadButtonDown(PAD_A)`, `IsPadButtonTriggered(PAD_START)` | gamepad XInput (pad 0..3) |
| `GetLeftStick()`, `GetRightStick()`, `GetPadTrigger(0/1)`, `SetPadVibration(l, r)` | stik −1..1 (Y atas = +1), trigger 0..1 |

### Waktu
| Fungsi | Keterangan |
|---|---|
| `deltaTime` / `GetDeltaTime()` | sudah dikali time scale |
| `SetTimeScale(0)` | pause game (script tetap jalan dengan dt = 0; physics & animasi berhenti) |
| `GetUnscaledDeltaTime()` | untuk UI/kamera saat pause |
| `GetTime()` | detik (scaled) sejak scene dimulai |

### Object, prefab, pool
| Fungsi | Keterangan |
|---|---|
| `FindGameObjectByName("Player")`, `FindGameObjectById(id)` | lookup id sekarang O(1) |
| `InstantiatePrefab("Enemy", pos)` | spawn `Assets/Prefabs/Enemy.prefab.json`; view langsung bisa diubah, object dibuat di akhir frame |
| `Despawn(id)`, `DespawnSelf()` | instance prefab -> disembunyikan & masuk pool (murah); object lain -> dihapus |
| `DestroySelf()`, `DestroyGameObject(id)` | hapus permanen |
| `Instantiate("Name", primitiveType, pos)` | primitive tanpa prefab (0 Cube, 1 Sphere, 2 Plane, 3 Quad, 4 Empty) |

**Membuat prefab:** klik kanan object di Hierarchy > **Save as Prefab** (object + semua child-nya).
Di Project panel, prefab tampil sebagai `[PREFAB]`. Double-click untuk menaruhnya di scene.

**Aturan pool:** saat instance diambil lagi dari pool, transform/layer kembali ke nilai prefab,
field `EXPOSE_FIELD` kembali ke nilai prefab, lalu `Initialize()` dipanggil lagi.
Variabel member biasa **tidak** di-reset otomatis, jadi reset di `Initialize()`.

### Collision layer
- Inspector (di bawah nama object): **Layer** dan **Collides With**.
- Layer bernama: `LAYER_DEFAULT, LAYER_PLAYER, LAYER_ENEMY, LAYER_PLAYER_BULLET, LAYER_WALL, LAYER_TRIGGER, LAYER_ENEMY_BULLET, LAYER_PICKUP` (8..31 bebas).
- Dua collider bertabrakan hanya jika **kedua** mask menerima layer lawannya.
  Contoh: Enemy mask = Player | PlayerBullet, sehingga musuh tidak saling cek dan jauh lebih cepat.
- Di script: `self->layer`, `self->collisionMask = LayerBit(LAYER_ENEMY) | LayerBit(LAYER_WALL);`
- Broadphase grid otomatis aktif mulai 32 collider. Ratusan musuh aman.

### UI game (tampil juga di build Release)
Kanvas tetap **1280 × 720** (kiri-atas = 0,0), diregangkan ke layar. Panggil **setiap frame** di `Update`.

| Fungsi | Keterangan |
|---|---|
| `UIText("Score 10", x, y, size, color, align)` | align 0 kiri, 1 tengah, 2 kanan |
| `UIRect(x, y, w, h, color)` | kotak berwarna (alpha didukung) |
| `UIImage("Assets/Textures/heart.png", x, y, w, h)` | gambar dari project |
| `UIProgressBar(x, y, w, h, value01, fill, back)` | bar HP/XP |
| `UIButton("Retry", x, y, w, h, selected)` | true jika diklik, atau `selected` + Enter/Space/pad A |
| `UIMeasureText(text, size)` | lebar teks (untuk layout) |

Menu dengan keyboard/gamepad: simpan `int index`, ubah dengan panah/D-pad, lalu kirim `selected = (index == i)`.
Contoh lengkap: `SurvivorsGameManager::DrawMenu`.

### Scene & save data
| Fungsi | Keterangan |
|---|---|
| `LoadScene("Stage2")` | ganti ke `Assets/Scenes/Stage2.scene.json` di akhir frame |
| `LoadScene(GetSceneName())` | restart scene |
| `SaveInt/LoadInt`, `SaveFloat/LoadFloat`, `SaveText/LoadText`, `HasSaveKey`, `DeleteAllSaveData` | disimpan langsung ke `SaveData/save.json` |

Di editor, scene yang di-load saat Play dikembalikan ke scene awal ketika **Stop**.

---

## 4. Tips & jebakan

- **Rebuild seluruh solution** setelah update engine. Jika exe dan `GameScripts.dll` berbeda versi,
  Console menampilkan *"GameScripts.dll was built against a different engine version"* dan script tidak dimuat.
- State global di DLL (seperti `SurvivorsShared.h`) ter-reset saat hot reload. Untuk test cepat, restart Play.
- Hindari menyimpan pointer `GameObject*` object lain terlalu lama. Simpan `id` lalu `FindGameObjectById(id)`.
- Peluru dan musuh sebaiknya selalu memakai prefab + `Despawn` (pool), bukan `Destroy` + `Instantiate`.
- Profiler: **Ctrl+P** (CPU/GPU ms per pass, draw call, upload memory).
- Export game: **File > Build & Export Game...**. Folder `Assets` (scene, prefab, texture) ikut disalin.
