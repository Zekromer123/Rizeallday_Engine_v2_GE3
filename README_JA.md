# Rizeallday Engine v2

[English](README_EN.md) | **日本語** | [Bahasa Indonesia](README.md)

**Rizeallday Engine v2** へようこそ！  
本プロジェクトは、日本工学院専門学校（ゲームクリエイター科・デザインカレッジ）のゲームプログラミングカリキュラムをベースに発展させた、独自の **DirectX 12 3Dゲームエンジン** です。

業界標準の現代的な **Engine + Sandbox アーキテクチャ**（[TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) や国内ゲーム制作フレームワーク [KamataEngine](https://github.com) の設計思想）を採用し、再利用可能な静的ライブラリ（`.lib`）とゲーム制作 sandbox（`.exe`）に完全分離して再構築しました。

純粋な **DirectX 12**、**C++20**、**Assimp**（FBX・OBJ・GLTF・DAE対応）、GPUアクセラレーションによる **スケルタルアニメーション（ボーンパレット最大128本）**、**マルチライティング**（平行光源・点光源・スポットライト・環境光）、**アイソメトリック（クォータービュー）ARPGカメラ**、および **Dear ImGui** によるランタイムデバッグ機能を標準搭載しています。

---

## 目次
1. [主な特徴・機能](#1-主な特徴機能)
2. [アーキテクチャとディレクトリ構成](#2-アーキテクチャとディレクトリ構成)
3. [クイックスタート（ビルドと実行）](#3-クイックスタートビルドと実行)
4. [API・サブシステムリファレンス](#4-apiサブシステムリファレンス)
   - [エンジンライフサイクル (`RizeEngine`)](#エンジンライフサイクル-rizeengine)
   - [DirectX 12 描画基盤 (`DirectXCommon`)](#directx-12-描画基盤-directxcommon)
   - [パイプライン & ブレンドモード (`GraphicPipeline` / `BlendMode`)](#パイプライン--ブレンドモード-graphicpipeline--blendmode)
   - [3Dモデル & スケルタルメッシュ (`Model`)](#3dモデル--スケルタルメッシュ-model)
   - [3Dオブジェクト実体 (`Object3d`)](#3dオブジェクト実体-object3d)
   - [プロシージャル丸影 (`CircleShadow`)](#プロシージャル丸影-circleshadow)
   - [ワールド変換行列 (`WorldTransform`)](#ワールド変換行列-worldtransform)
   - [カメラ & アイソメトリックカメラ (`Camera`, `IsometricCamera`, `DebugCamera`)](#カメラ--アイソメトリックカメラ)
   - [ライティングシステム (`LightGroup`)](#ライティングシステム-lightgroup)
   - [2Dスプライト描画 (`Sprite`)](#2dスプライト描画-sprite)
   - [入力処理システム (`Input`)](#入力処理システム-input)
   - [オーディオ再生システム (`Audio`)](#オーディオ再生システム-audio)
   - [数学ライブラリ (`Mathematics` / `MathUtil`)](#数学ライブラリ-mathematics--mathutil)
5. [シェーダー & ルートシグネチャパイプライン](#5-シェーダー--ルートシグネチャパイプライン)

---

## 1. 主な特徴・機能

- **純粋なDirectX 12 パイプライン**: コマンドリスト、デスクリプタヒープ、ルートシグネチャ、パイプラインステートオブジェクト（PSO）、Shader Model 6.0 (`dxc.exe` / `dxcompiler.dll`)。
- **マルチブレンドモード描画**:
  - `Opaque`（通常不透明）、`AlphaBlend`（半透明合成）、`Add`（加算合成・エフェクト/発光）、`Multiply`（乗算合成・暗影/呪いオーラ）の独立PSOをキャッシュ。
  - 半透明描画時は自動的に深度書き込みを無効化（`DepthWriteMask = ZERO`）し、前後オブジェクトのクリッピング破綻を防止。
- **プロシージャル丸影（Blob Shadow / Circle Shadow）**:
  - キャラクターの足元地面に投影される滑らかな円形ドロップシャドウ。
  - クォータービュー視点に最適化した楕円スケーリングと、`smoothstep` によるリアルタイム放射状グラデーション。外部テクスチャ画像不要。
  - 半径・不透明度・エッジの柔らかさ・カラーをリアルタイム調整可能。
- **スケルタルメッシュ & ボーンアニメーション**:
  - Assimpライブラリを統合し、Mixamo等のリギング済みFBX、OBJ、GLTF、Collada DAEを自動ロード。
  - 1頂点あたり最大4ボーンの影響度（`BLENDINDICES` + `BLENDWEIGHT`）に対応したスキニング頂点シェーダー。
  - GPU定数バッファによる最大128ボーンのボーンパレット（Bone Palette）。
  - クォータニオン球面線形補間（Slerp）および位置ベクトル線形補間（Lerp）。
  - **In-Place / ルートモーション固定機能**: ループ再生時のキャラクター引き戻し（ラバーバンド現象）を防止するトグルを内蔵。
- **統合マルチライティング（Multi-Lighting）**:
  - ディレクショナルライト（ランバート拡散反射 / ハーフランバート対応、色・光度調整可能）。
  - ポイントライト 3基（位置、到達半径、距離減衰率）。
  - スポットライト 3基（位置、照射方向、照射距離、コーン角度・フォールオフ調整）。
  - 環境光（アンビエントカラー & 強度）。
- **アイソメトリック（クォータービュー）ARPGカメラ**:
  - 『Lunarium』『Hades』『Diablo』スタイルの見下ろし斜め視角（ピッチ45度、ヨー45度）。
  - プレイヤーのスムーズ追従（Smooth Target Damping）。
  - Q/Eキーによるプレイヤー周囲360度オービット回転と、視点角度に連動した8方向移動（Camera-Relative Movement）。
- **Development | x64 開発用ハイブリッド構成**:
  - `assert()` の検証を有効化（`NDEBUG` 未定義）、Dear ImGui 有効、ゲームサンドボックス側のみ `/Od`（最適化無効）でステップ実行デバッグを快適にし、エンジンコア側は `/O2` で高フレームレートを維持する実用構成。
- **入力システム**: DirectInput 8（キーボード・マウス） + XInput（Xboxコントローラー、デッドゾーン処理・デュアルモーター振動対応）。
- **サウンド再生**: XAudio2による低遅延PCM WAV再生。
- **デバッグUI**: Dear ImGuiを標準統合。トランスフォーム、ライト色、アニメーション再生速度、ブレンドモード、影のプロパティを実行中にリアルタイム調整可能。

---

## 2. アーキテクチャとディレクトリ構成

ソリューション（`Rizeallday_Engine_v2.slnx`）は2つのプロジェクトで構成されています：
- **`RizealldayEngine`**: **スタティックライブラリ（`.lib`）**。DX12描画、シェーダー、数学、Assimpロード、アニメーション、入力、サウンド等の全コア機能をカプセル化。
- **`RizealldaySandbox`**: **実行可能アプリケーション（`.exe`）**。ゲームロジック、シーン、キャラクター制御を記述するスタートアッププロジェクト。

```
D:\Rizeallday_Engine_v2\
│
├── Rizeallday_Engine_v2.slnx              # Visual Studio ソリューションファイル
├── README.md                               # インドネシア語ドキュメント
├── README_EN.md                            # 英語ドキュメント
├── README_JA.md                            # 日本語ドキュメント（本ファイル）
│
├── RizealldayEngine/                       # 【静的ライブラリ】コアエンジン基盤
│   ├── base/                               # コアサブシステム
│   │   ├── RizeEngine.h / .cpp             # エンジン全体のライフサイクル & アクセス窓口
│   │   ├── DirectXCommon.h / .cpp          # DX12デバイス、コマンドキュー、スワップチェーン
│   │   ├── WinApp.h / .cpp                 # Win32ウィンドウ管理 & メッセージループ
│   │   ├── BlendMode.h                     # ブレンドモード列挙型 (Opaque, AlphaBlend, Add, Multiply)
│   │   ├── TextureManager.h / .cpp         # テクスチャキャッシュ & SRVデスクリプタ管理
│   │   ├── GraphicPipeline.h / .cpp        # パイプラインステート（4ブレンドモードPSO）& ルートシグネチャ
│   │   ├── DXCManager.h / .cpp             # 実行時HLSLシェーダーコンパイラ（DXC）
│   │   ├── Log.h / .cpp                    # スレッドセーフログ出力
│   │   └── CrashHandler.h / .cpp           # クラッシュ時ミニダンプ出力ハンドラ
│   ├── 3d/                                 # 3Dグラフィックス & アニメーション
│   │   ├── Model.h / .cpp                  # Assimp読み込み、骨アニメーション、GPUバッファ生成
│   │   ├── Object3d.h / .cpp               # 3Dオブジェクト実体（モデル＋変換行列＋マテリアル＋ブレンドモード）
│   │   ├── CircleShadow.h / .cpp           # プロシージャル足元丸影（床面クアッド・ソフトグラデーション）
│   │   ├── Camera.h / .cpp                 # 透視投影ビュープロジェクション行列生成
│   │   ├── DebugCamera.h / .cpp            # マウス操作によるデバッグ用自由視点カメラ
│   │   ├── WorldTransform.h / .cpp         # ワールド行列・WVP定数バッファ管理
│   │   ├── LightGroup.h / .cpp             # マルチライト定数バッファ管理
│   │   ├── DirectionalLight.h              # 平行光源データ構造体
│   │   ├── PointLight.h                    # 点光源データ構造体
│   │   ├── SpotLight.h                     # スポットライトデータ構造体
│   │   └── PrimitiveDrawer.h / .cpp        # デバッグ用3Dライン & グリッド描画
│   ├── 2d/                                 # 2Dグラフィックス
│   │   └── Sprite.h / .cpp                 # 2Dスプライト & UIクアッド描画
│   ├── input/                              # 入力処理
│   │   └── Input.h / .cpp                  # DirectInput8（キーボード/マウス）+ XInput（ゲームパッド）
│   ├── audio/                              # 音響処理
│   │   └── Audio.h / .cpp                  # XAudio2ローダー & サウンド再生
│   ├── math/                               # 数学ライブラリ
│   │   ├── Vector2.h, Vector3.h, Vector4.h # ベクトル構造体 & 各種演算子オーバーロード
│   │   ├── Matrix4x4.h                     # 行優先4x4行列 & Transform構造体
│   │   ├── MathUtil.h / .cpp               # アフィン変換、透視投影、正射影、逆行列計算
│   │   └── VertexData.h                    # 頂点データ構造体（ボーンインデックス・ウェイト対応）
│   └── shaders/                            # HLSLシェーダー
│       ├── Object3d.hlsli                  # VS/PS共通データ入出力構造体
│       ├── Object3d.VS.hlsl                # スキニング対応3D頂点シェーダー
│       ├── Object3d.PS.hlsl                # マルチライト対応ピクセルシェーダー
│       ├── CircleShadow.VS.hlsl            # 丸影専用3D頂点シェーダー（軽量クアッド用）
│       ├── CircleShadow.PS.hlsl            # 丸影専用ピクセルシェーダー（放射状フォールオフ計算）
│       ├── Sprite.hlsli                    # 2Dスプライト共通構造体
│       ├── Sprite.VS.hlsl                  # 2Dスプライト頂点シェーダー
│       └── Sprite.PS.hlsl                  # 2Dスプライトピクセルシェーダー
│
├── RizealldaySandbox/                      # 【ゲーム本体】制作ワークスペース
│   ├── main.cpp                            # ゲームループ & キャラクターコントローラー
│   ├── IsometricCamera.h / .cpp            # スムーズ追従式クォータービューカメラ
│   ├── dxcompiler.dll, dxil.dll            # DXCシェーダーコンパイラバイナリ
│   ├── assimp-vc143-mt.dll                 # AssimpランタイムDLL
│   └── Resources/                          # ゲームアセットフォルダ
│       ├── Model/                          # .fbx, .obj, .gltf 3Dモデル & テクスチャ（Idle, Running, Combo）
│       ├── Texture/                        # .png, .dds 画像ファイル
│       └── Sound/                          # .wav 音声ファイル
│
└── externals/                              # 外部サードパーティライブラリ
    ├── assimp/                             # Open Asset Import Library
    ├── DirectXTex/                         # Microsoft DirectXTex（DDS/テクスチャ処理）
    └── imgui/                              # Dear ImGui（UIデバッグ用）
```

---

## 3. クイックスタート（ビルドと実行）

### ビルド手順
1. Visual Studio 2022 で **`D:\Rizeallday_Engine_v2\Rizeallday_Engine_v2.slnx`** を開きます。
2. ソリューションエクスプローラーで **`RizealldaySandbox`** が太字（スタートアッププロジェクト）になっていることを確認します。
3. ターゲット構成を選択します：
   - **`Development | x64`（推奨・日常の開発構成）**:
     - Dear ImGui有効（`USE_IMGUI`）。
     - アサーション検証有効（`assert()`が機能、`NDEBUG`未定義）。
     - ゲームサンドボックス側は最適化無効（`/Od`）で変数監視・ブレークポイントが正確に動作し、エンジンコア側は最高速度（`/O2`）で快適なフレームレートを両立。
   - **`Debug | x64`**:
     - デバッグ情報完全、Direct3D 12 GPUデバッグレイヤー検証有効、ImGui有効。
   - **`Release | x64`**:
     - 完全最適化ビルド（`/O2`）、ImGui・デバッグログを排した製品リリース向け。
4. **F5キー** を押してビルドおよび実行します。

### 最小コード例（Minimal Sample）
```cpp
#include <RizeEngine.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // 1. エンジン初期化
    RizeEngine::Initialize("Rizeallday Game Project", 1280, 720);

    // 2. 3Dモデルのロード（FBX / OBJ対応）
    Model* heroModel = Model::CreateFromFile("Resources/Model", "Idle.fbx");
    heroModel->SetLockRootMotion(true); // ルートモーションの水平移動を固定し、自然な足踏み走りを実現

    // 3. カメラ初期化
    Camera camera;
    camera.Initialize();
    camera.translation = { 0.0f, 1.5f, -8.0f };
    camera.rotation = { 0.1f, 0.0f, 0.0f };

    // 4. 3Dオブジェクト実体の生成
    Object3d hero;
    hero.Initialize();
    hero.SetModel(heroModel);
    hero.SetCamera(&camera);
    hero.worldTransform.scale = { 0.018f, 0.018f, 0.018f };

    // 5. ライティング設定
    LightGroup* lights = LightGroup::Create();
    lights->SetDefaultLighting();

    // 6. ゲームループ
    while (RizeEngine::ProcessMessage()) {
        RizeEngine::BeginFrame();

        float deltaTime = 0.016f;
        Input* input = RizeEngine::GetInput();

        // 操作処理（Wキーで前進）
        if (input->PushKey(DIK_W)) {
            hero.worldTransform.translation.z += 4.5f * deltaTime;
        }

        camera.Update();
        hero.Update(deltaTime); // アニメーション更新とボーン計算
        lights->Update();

        // 描画コマンド発行
        auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
        lights->Draw(cmdList, 3);
        hero.Draw(cmdList);

        RizeEngine::EndFrame();
    }

    // 7. 解放処理
    RizeEngine::Finalize();
    return 0;
}
```

---

## 4. API・サブシステムリファレンス

### エンジンライフサイクル (`RizeEngine`)
- `RizeEngine::Initialize(const char* title, int width, int height)`: ウィンドウ生成、DirectX 12デバイス、DXC、パイプライン、テクスチャマネージャ、入力、サウンドを一括初期化。
- `bool RizeEngine::ProcessMessage()`: Windowsメッセージを処理。終了リクエスト（閉じるボタン押下等）で `false` を返却。
- `void RizeEngine::BeginFrame()`: バックバッファのクリア、入力状態ポーリング、ImGui新規フレーム開始。
- `void RizeEngine::EndFrame()`: ImGui描画、コマンドリスト実行、スワップチェーンPresent呼び出し。
- `void RizeEngine::Finalize()`: GPUフェンス待機後、全COMリソースを安全に破棄。
- `RizeEngine::GetDirectXCommon()`: DX12共通管理クラスへのポインタを取得。
- `RizeEngine::GetInput()`: 入力管理クラスへのポインタを取得。
- `RizeEngine::GetAudio()`: 音響管理クラスへのポインタを取得。

---

### DirectX 12 描画基盤 (`DirectXCommon`)
- `ID3D12Device* GetDevice() const`: DirectX 12 デバイス本体への直接アクセス。
- `ID3D12GraphicsCommandList* GetCommandList() const`: 描画記録用コマンドリスト。
- `ID3D12CommandQueue* GetCommandQueue() const`: コマンドキュー。
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: GPUアップロードバッファ生成ヘルパー。
- `void SignalAndWaitFence()`: CPU-GPU間の完全同期フェンス待機。

---

### パイプライン & ブレンドモード (`GraphicPipeline` / `BlendMode`)
エンジン内部でブレンドモードごとの専用PSO（Pipeline State Object）を事前構築・キャッシュ管理しています：
```cpp
enum class BlendMode : int {
    Opaque     = 0, // 不透明描画・深度書き込みON（デフォルト）
    AlphaBlend = 1, // 通常半透明合成（SrcAlpha / InvSrcAlpha）・深度書き込みOFF
    Add        = 2, // 加算合成（炎・魔法エフェクト・光）・深度書き込みOFF
    Multiply   = 3, // 乗算合成（暗影・呪縛オーラ）・深度書き込みOFF
    Count      = 4
};
```
- 各 `Object3d` インスタンスは個別の `blendMode` プロパティを保持。
- `AlphaBlend` や `Add` が選択された際、深度書き込みを自動的に無効化（`DepthWriteMask = ZERO`）するため、半透明ポリゴンが背景ジオメトリを不自然に隠蔽する描画順序破綻を回避。
- 利用例：
  ```cpp
  character.SetBlendMode(BlendMode::AlphaBlend);
  character.GetMaterial()->color.w = 0.5f; // 透過率50%
  ```

---

### 3Dモデル & スケルタルメッシュ (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`:
  Assimpを用いてモデルファイル（`.fbx`, `.obj`, `.gltf`, `.glb`, `.dae`）を読み込み、GPUバッファ（頂点・インデックス）を自動生成。ロード済みモデルはメモリ内に自動キャッシュされます。
- `bool HasBones() const`: スケルトンボーンデータが含まれているかを判定。
- `size_t GetBoneCount() const`: メッシュに関連付けられたボーン総数を取得。
- `float GetAnimationDuration(int animIndex = 0) const`: アニメーションの総時間を秒単位で取得。
- `void CalculateBoneTransforms(float timeInSeconds, int animIndex, std::vector<Matrix4x4>& outMatrices)`:
  指定時刻の階層ツリーを走査し、Slerp/Lerp補間により最終的なスキニング変形行列（`offsetMatrix * globalTransform`）を算出。
- `void SetLockRootMotion(bool lock)`:
  ルートボーン（Hips）の水平方向（X・Z座標）移動を初期フレームに固定。コードによる移動処理と競合した際の引き戻し現象（ラバーバンド）を防ぎます（Y軸の自然な上下振動は保持）。

---

### 3Dオブジェクト実体 (`Object3d`)
- `void Initialize()`: マテリアル用定数バッファ、ワールド変換、およびGPUボーンパレットバッファ（128行列分）を割り当て。
- `void SetModel(Model* model, bool resetAnim = true)`: 描画対象の3Dモデルをバインド。
- `void SetCamera(Camera* camera)`: 撮影用カメラをバインド。
- `WorldTransform worldTransform`: 公開メンバ。`scale`（拡大縮小）、`rotation`（回転角）、`translation`（平行移動座標）を保持。
- `BlendMode blendMode`: オブジェクトのブレンドモード（`BlendMode::Opaque`, `AlphaBlend`, `Add`, `Multiply`）。
- `void SetBlendMode(BlendMode mode)` / `BlendMode GetBlendMode() const`: ブレンドモード設定・取得。
- `Material* GetMaterial() const`: GPUマテリアルデータ（RGBAカラー、ライティングフラグ等）への直接ポインタ。
- **ボーンアニメーション制御**:
  - `void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f)`
  - `void SetAnimationSpeed(float speed)`
  - `float GetAnimationTime() const` / `float GetAnimationDuration() const`
- `void Update(float deltaTime = 1.0f / 60.0f)`: アニメーション時間を進め、ボーン行列を評価してWVP行列とともにGPU定数バッファへ転送。
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: 指定ブレンドモードのPSOを自動バインドし、スロット0にマテリアル、スロット1にWVP、スロット2にテクスチャSRV、スロット4にボーンパレットを設定してメッシュを描画。

---

### プロシージャル丸影 (`CircleShadow`)
3Dキャラクターの足元地面に投影するリアルタイム円形ドロップシャドウ（Blob Shadow）：
- **描画技術**: 床面水平クアッドを専用シェーダー（`CircleShadow.VS.hlsl` / `PS.hlsl`）で描画。UV中心 `(0.5, 0.5)` からの幾何学的楕円距離を `smoothstep` でフォールオフ計算するため、外部テクスチャ画像のロードが一切不要。
- `static void StaticInitialize(DirectXCommon* dxCommon, DXCManager* dxcManager)`: `RizeEngine::Initialize()` 内で自動初期化。
- `void Initialize()`: クアッド頂点・インデックス・マテリアル・変換バッファを生成。
- `void Update(const Vector3& characterPos, float floorY, float radius, float opacity)`:
  - `characterPos`: キャラクターのワールド座標（影がXZ座標を追従）。
  - `floorY`: 床面（地面）のY座標高さ（例: `-1.5f`）。
  - `radius`: ワールド空間における影の半径サイズ。
  - `opacity`: 影の暗さ・不透明度（0.0 = 完全透明、1.0 = 濃い黒）。
- `void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera)`: アルファブレンド・深度書き込みOFFで丸影を描画。
- `CircleShadowMaterialGPU* GetMaterial()`: エッジのぼかし（`softness`）、基本色（`color`）をリアルタイム調整可能。

使用例：
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color = { 0.0f, 0.0f, 0.0f, 0.45f }; // 45%黒
shadow.GetMaterial()->softness = 0.5f;

// ゲームループ内:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera); // キャラクターより「先」に描画
character.Draw(cmdList);       // キャラクターを影の上に描画
```

---

### カメラ & アイソメトリックカメラ
- **`Camera`**:
  - `Vector3 translation`, `Vector3 rotation`（ラジアン指定）。
  - `float fovY`, `float aspectRatio`, `float nearClip`, `float farClip`。
  - `void Update()`: アフィン逆行列によるビュー行列と透視投影行列を再計算。
- **`IsometricCamera`**:
  - クォータービュー（ピッチ45度、ヨー45度）に対応した追従カメラ。
  - プレイヤー追従時のジッターや遅延ズレを排除するスムーズダンピング（Smooth Target Damping）内蔵。
  - `RotateYaw(float deltaYaw)`: Q/Eキー等による360度オービット旋回機能。
  - `float GetYaw() const`: カメラ角度に即したプレイヤー8方向移動ベクトルの算出に利用。
- **`DebugCamera`**:
  - 開発用マウスフリー操作カメラ。右ドラッグで回転、中ドラッグでパン、WASDQEで移動。

---

### ライティングシステム (`LightGroup`)
1つの定数バッファにまとめた高効率マルチライティング：
- `static LightGroup* Create()`: インスタンスを生成。
- `void SetDefaultLighting()`: 標準的な白系ディレクショナルライトと環境光を設定。
- `void SetAmbientColor(const Vector3& color)` & `SetAmbientIntensity(float intensity)`: シーン全体の環境光を制御。
- `DirectionalLightData& GetDirectionalLight()`: 平行光源データ（`color`, `direction`, `intensity`, `active`）。
- `PointLightData& GetPointLight(int index)`（インデックス 0〜2）：点光源データ（`position`, `color`, `radius`, `decay`, `intensity`, `active`）。
- `SpotLightData& GetSpotLight(int index)`（インデックス 0〜2）：スポットライトデータ（`position`, `direction`, `distance`, `cosAngle`, `cosFalloffStart`, `active`）。
- `void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t rootParameterIndex = 3)`: ピクセルシェーダーのルートパラメータ3番に定数バッファをバインド。

---

### 入力処理システム (`Input`)
- **キーボード（DirectInput 8）**:
  - `bool PushKey(BYTE keyNumber)`: 押下中判定（例: `DIK_W`, `DIK_SPACE`）。
  - `bool TriggerKey(BYTE keyNumber)`: 押した最初の1フレームのみ判定。
  - `bool ReleaseKey(BYTE keyNumber)`: 離した瞬間の1フレーム判定。
- **マウス**:
  - `bool PushMouseButton(int button)`: 0 = 左クリック, 1 = 右クリック, 2 = 中クリック。
  - `bool IsTriggerMouse(int button)`: クリックトリガー判定。
  - `int GetMouseMoveX()`, `int GetMouseMoveY()`, `int GetMouseWheel()`.
- **ゲームパッド（XInput）**:
  - `bool IsGamepadConnected(int padIndex = 0)`.
  - `bool PushGamepadButton(WORD button)`, `bool TriggerGamepadButton(WORD button)`.
  - `float GetLeftStickX()`, `float GetLeftStickY()`（デッドゾーン自動補正済み）。
  - `void SetVibration(int padIndex, float leftMotor, float rightMotor)`.

---

## 5. シェーダー & ルートシグネチャパイプライン

### 頂点レイアウト構造 (`Object3d.VS.hlsl`):
| セマンティクス | フォーマット | 説明 |
|---|---|---|
| `POSITION0` | `R32G32B32A32_FLOAT` | ローカル空間頂点位置 (x, y, z, 1.0) |
| `TEXCOORD0` | `R32G32_FLOAT` | テクスチャUV座標 (u, v) |
| `NORMAL0`   | `R32G32B32_FLOAT` | 法線ベクトル (nx, ny, nz) |
| `BLENDINDICES0` | `R32G32B32A32_UINT` | GPUスキニング用ボーンインデックス（最大4本） |
| `BLENDWEIGHT0`  | `R32G32B32A32_FLOAT` | ボーンウェイト重み係数（合計1.0） |

### ルートシグネチャ構成 (`GraphicPipeline.cpp`):
| ルートスロット | レジスタ | 可視シェーダー | タイプ | バッファ内容 |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material（基本色、ライティング有効フラグ、陰影計算タイプ） |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix（WVP行列、ワールド行列） |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2Dディフューズテクスチャ |
| **3** | `b1` | Pixel Shader | CBV | LightGroup（環境光、平行光源、点光源3基、スポットライト3基） |
| **4** | `b1` | Vertex Shader | CBV | **BonePalette**（128本のボーンスキニング変形行列配列） |

### 丸影シェーダー構成 (`CircleShadow.VS.hlsl` & `CircleShadow.PS.hlsl`):
- **頂点シェーダー**: スキニング計算を行わず、水平床面クアッド頂点の座標変換を軽量に処理。
- **ピクセルシェーダー**: UV中心 `(0.5, 0.5)` からの楕円ユークリッド距離を解析計算し、`smoothstep` で滑らかに減衰するアルファ値をリアルタイム算出。テクスチャ不要で高品質な丸影を生成。