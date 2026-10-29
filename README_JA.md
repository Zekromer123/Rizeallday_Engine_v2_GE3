# Rizeallday Engine v2

[English](README_EN.md) | **日本語** | [Bahasa Indonesia](README.md)

**DirectX 12** と **C++20** で開発した 3D ゲームエンジンです。Unity 風のエディタ、ホットリロード対応の
C++ スクリプト、ゲームのビルド／エクスポート機能を備え、プロトタイプからリリースビルドまで一貫して制作できます。

![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![API](https://img.shields.io/badge/graphics-DirectX%2012-green)
![Language](https://img.shields.io/badge/C%2B%2B-20-orange)

---

## 主な機能

**描画**
- DirectX 12：2 フレーム並行処理、フレーム単位のアップロードアロケータ、フリーリスト方式のディスクリプタヒープ
- シャドウマップ（3×3 PCF）、キューブマップスカイボックス、マルチライト（平行光源・点光源・スポットライト・環境光）
- Assimp による FBX / OBJ / glTF 読み込み、GPU スキニングアニメーション（最大 128 ボーン）
- GPU インスタンシングによるパーティクル、2D スプライト、Release ビルドでも動作するゲーム UI（テキスト・画像・ゲージ・ボタン）

**エディタ**
- プロジェクトハブ、親子階層対応のヒエラルキー、インスペクター、ギズモ（移動・回転・拡縮）、アセットブラウザ
- Undo / Redo、JSON シーン、プレハブ、再生・一時停止・コマ送り、全画面ゲームプレビュー（F11）
- GUID アセットデータベース（`.meta`）：プロジェクトパネルでアセットを名前変更・移動してもシーンとプレハブの参照が切れない
- 日本語テキスト対応（エディタ・ゲーム UI・ファイル名）とエディタの表示言語切り替え：English / 日本語 / Bahasa Indonesia（Options > Language）
- アニメーター：ノードエディタ（Window > Animator）で組むアニメーションステートマシン、クロスフェード、スクリプトからのパラメーター・イベント
- エディタ内からのアセットインポート（ダイアログ／エクスプローラーからのドラッグ）と最近使ったプロジェクト一覧（File > Open Recent）
- プロファイラ（パスごとの CPU / GPU 時間、ドローコール数、アップロード使用量）、ゲームのビルド＆エクスポート

**ゲームプレイ・スクリプト**
- `GameScripts.dll` の C++ スクリプト（ホットリロード、`EXPOSE_FIELD` でインスペクターから編集可能）
- キーボード、マウス（ワールド座標での照準）、XInput ゲームパッド入力
- Sphere / AABB / OBB の衝突判定（レイヤー／マスク、グリッド方式ブロードフェーズ）、簡易リジッドボディ
- オブジェクトプール付きプレハブ、シーン切り替え、セーブデータ、タイムスケール（一時停止・スロー）
- `OnCollisionEnter/Exit` イベント、レイキャスト・球オーバーラップ・タグ検索、スクリプトからのサウンド・パーティクル・マテリアル・ライト・リジッドボディ・アニメーション制御
- XAudio2 によるサウンド

**品質**
- ヘッドレス自己テスト（`--run-tests`、WARP ソフトウェアアダプタにも対応）
- GitHub Actions CI：Development / Release を警告なしでビルドし、テストを実行

---

## 動作環境

- Direct3D 12 対応 GPU を搭載した Windows 10 / 11
- Visual Studio 2022（v143）または Visual Studio 2026（v145）、「C++ によるデスクトップ開発」ワークロード
- Windows 10 / 11 SDK

## はじめ方

```bash
git clone --recursive https://github.com/Zekromer123/Rizeallday_Engine_v2_GE3.git
```

1. `Rizeallday_Engine_v2_GE3.slnx` を開きます。
2. 構成 **Development | x64** を選択します（スタートアッププロジェクト：`RizealldaySandbox`）。
3. **F5** で起動し、プロジェクトハブでプロジェクトを作成または開きます。
4. サンプル：**GameObject > Samples > Survivors Demo** を選び、**Play** を押します。

| 構成 | 用途 |
|---|---|
| `Debug` | エディタ + D3D12 デバッグレイヤー |
| `Development` | エディタ（ImGui）＋最適化済みエンジン。日常の開発用 |
| `Release` | エディタなしのゲームランタイム。配布用ビルド |

## チーム向けリリース

```powershell
powershell -ExecutionPolicy Bypass -File Tools\MakeRelease.ps1
```

`dist\RizealldayEngine-<バージョン>.zip`（`Editor/`、`SDK/`、`Templates/`、`engine.json`、`README_TEAM.md`）を作成します。
リリースのたびに `RizealldayEngine/base/EngineVersion.h` の `kEngineVersion` を上げてください。
チームは zip を展開して `Editor\RizealldaySandbox.exe` を起動します。ゲームプロジェクトはそれぞれ C++ スクリプト（`GameScripts/`）ごと自分のリポジトリを持ちます。
エンジン更新：**Help > Install Engine Update...**（または zip をエディターにドロップ）。新しいバージョンをインストールして再起動します。

## スクリプト例

```cpp
#include "base/ScriptAPI.h"

class Mover : public ScriptBase {
public:
    EXPOSE_FIELD(float, speed, 6.0f);

    void Update(float deltaTime) override {
        GameObject* self = GetOwner();
        const Vector2 move = GetMoveAxis();          // WASD / 左スティック
        self->translation.x += move.x * speed * deltaTime;
        self->translation.z += move.y * speed * deltaTime;
        if (IsKeyTriggered(KEY_SPACE)) InstantiatePrefab("Bullet", self->translation);
        UIText("Hello!", 24.0f, 24.0f);              // ゲーム UI（1280x720 キャンバス）
    }
};
REGISTER_SCRIPT(Mover)
```

エディタで **Ctrl+Shift+B** を押すとスクリプトがビルドされ、DLL が自動で再読み込みされます。

## リポジトリ構成

```
RizealldayEngine/     静的ライブラリ：描画、入力、サウンド、衝突判定、スクリプト実行環境、HLSL シェーダー
RizealldaySandbox/    エディタ＋ゲームランタイム（.exe）、自己テスト、サンプルアセット
GameScripts/          ゲームプレイスクリプト（.dll、ホットリロード）
Templates/            New Project 用のスクリプトプロジェクトのテンプレート
Tools/                MakeRelease.ps1（チーム向け zip）と README_TEAM.md
externals/            サードパーティライブラリ
.github/workflows/    CI
```

## テスト

```bash
bin\Development\RizealldaySandbox.exe --run-tests [filter] [--warp]
```

`RizealldaySandbox` フォルダから実行します。終了コード `0` で全テスト合格、結果は `test_results.txt` に出力されます。

## 使用ライブラリ

- [DirectXTex](https://github.com/microsoft/DirectXTex) — MIT
- [Dear ImGui](https://github.com/ocornut/imgui) — MIT
- [Assimp](https://github.com/assimp/assimp) — BSD-3-Clause
- [nlohmann/json](https://github.com/nlohmann/json) — MIT
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) — University of Illinois/NCSA
