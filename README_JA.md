# Rizeallday Engine v2

[English](README_EN.md) | **日本語** | [Bahasa Indonesia](README.md)

**Rizeallday Engine v2** へようこそ！  
本プロジェクトは、日本工学院専門学校（ゲームクリエイター科・デザインカレッジ）のゲームプログラミングカリキュラムをベースに発展させた、独自の **DirectX 12 3Dゲームエンジン** です。

業界標準の現代的な **Engine + Sandbox アーキテクチャ**（[TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) や国内ゲーム制作フレームワーク [KamataEngine](https://github.com) の設計思想）を採用し、再利用可能な静的ライブラリ（`.lib`）とゲーム制作 sandbox（`.exe`）に完全分離して再構築しました。

純粋な **DirectX 12**、**C++20**、**Assimp**（FBX・OBJ・GLTF・DAE対応）、GPUアクセラレーションによる **スケルタルアニメーション（ボーンパレット最大128本）**、**マルチライティング**（平行光源・点光源・スポットライト・環境光）、`StructuredBuffer + SV_InstanceID` による **GPUパーティクルインスタンシング描画**、GE3カリキュラム準拠の **衝突判定システム（Sphere / AABB / 総当たり判定 / OnCollisionコールバック）**、および **Dear ImGui** によるランタイムデバッグ機能を標準搭載しています。

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
   - [カメラ (`Camera`, `DebugCamera`)](#カメラ-camera-debugcamera)
   - [ライティングシステム (`LightGroup`)](#ライティングシステム-lightgroup)
   - [GPUインスタンシング・パーティクル板ポリ (`ParticleQuad`)](#gpuインスタンシングパーティクル板ポリ-particlequad)
   - [衝突判定システム (`Collider` / `CollisionManager`)](#衝突判定システム-collider--collisionmanager)
   - [2Dスプライト描画 (`Sprite`)](#2dスプライト描画-sprite)
   - [プリミティブデバッグ描画 (`PrimitiveDrawer`)](#プリミティブデバッグ描画-primitivedrawer)
   - [テクスチャマネージャ (`TextureManager`)](#テクスチャマネージャ-texturemanager)
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
- **GPUインスタンシング描画（課題 01_00「たくさんの板ポリを出そう」対応）**:
  - `StructuredBuffer<TransformationMatrix>` と `SV_InstanceID` を用いた高速インスタンス描画。
  - `DrawInstanced(6, N, 0, 0)` により1回のドローコールで多数の板ポリゴンを一括描画。
  - 独立した専用RootSignature・PSO・SRV（スロット20）を備えた `ParticleQuad` クラス。
- **衝突判定システム（Collision Detection System）**【新規搭載】:
  - `SphereCollider`（球判定）および `AABBCollider`（直方体境界判定）。
  - 全登録オブジェクトを1フレーム内で判定する `CollisionManager`（総当たり判定方式）。
  - GE3講義準拠の `ICollidable` インターフェースと `OnCollision()` コールバック設計。
  - `attribute`（属性）と `mask`（対象）のビットマスクによる柔軟な当たり判定フィルタリング。
- **プロシージャル丸影（Blob Shadow / Circle Shadow）**:
  - キャラクターの足元地面に投影される滑らかな円形ドロップシャドウ。
  - `smoothstep` によるリアルタイム放射状グラデーション。外部テクスチャ画像不要。
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
- **Development | x64 開発用ハイブリッド構成**:
  - `assert()` の検証を有効化（`NDEBUG` 未定義）、Dear ImGui 有効、ゲームサンドボックス側のみ `/Od`（最適化無効）でステップ実行デバッグを快適にし、エンジンコア側は `/O2` で高フレームレートを維持する実用構成。
- **入力システム**: DirectInput 8（キーボード・マウス） + XInput（Xboxコントローラー、デッドゾーン処理・デュアルモーター振動対応）。
- **サウンド再生**: XAudio2による低遅延PCM WAV再生。
- **デバッグUI**: Dear ImGuiを標準統合。トランスフォーム、ライト色、アニメーション再生速度、ブレンドモード、影、パーティクルの各プロパティを実行中にリアルタイム調整可能。

---

## 2. アーキテクチャとディレクトリ構成

ソリューション（`Rizeallday_Engine_v2_GE3.slnx`）は2つのプロジェクトで構成されています：
- **`RizealldayEngine`**: **スタティックライブラリ（`.lib`）**。DX12描画、シェーダー、数学、Assimpロード、アニメーション、入力、サウンド、衝突判定等の全コア機能をカプセル化。
- **`RizealldaySandbox`**: **実行可能アプリケーション（`.exe`）**。ゲームロジック、シーン、キャラクター制御を記述するスタートアッププロジェクト。

```
D:\GE3\Rizeallday_Engine_v2_GE3\
│
├── Rizeallday_Engine_v2_GE3.slnx          # Visual Studio ソリューションファイル
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
│   │   ├── ConvertString.h / .cpp          # 文字列型変換ヘルパー
│   │   └── CrashHandler.h / .cpp           # クラッシュ時ミニダンプ出力ハンドラ
│   ├── 3d/                                 # 3Dグラフィックス・照明・当たり判定
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
│   │   ├── PrimitiveDrawer.h / .cpp        # デバッグ用3Dライン & グリッド描画
│   │   ├── Collider.h / .cpp               # 【新規】SphereCollider, AABBCollider & 交差判定関数
│   │   └── CollisionManager.h / .cpp       # 【新規】CollisionManager シングルトン & ICollidable
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
│       ├── Particle.VS.hlsl                # 【新規】GPUインスタンシング頂点シェーダー
│       ├── Particle.PS.hlsl                # 【新規】パーティクルピクセルシェーダー（透過破棄）
│       ├── Sprite.hlsli                    # 2Dスプライト共通構造体
│       ├── Sprite.VS.hlsl                  # 2Dスプライト頂点シェーダー
│       └── Sprite.PS.hlsl                  # 2Dスプライトピクセルシェーダー
│
├── RizealldaySandbox/                      # 【ゲーム本体】制作ワークスペース
│   ├── main.cpp                            # ゲームループ & キャラクターコントローラー
│   ├── ParticleQuad.h / .cpp               # 【新規】10枚板ポリインスタンシング管理クラス
│   ├── dxcompiler.dll, dxil.dll            # DXCシェーダーコンパイラバイナリ
│   ├── assimp-vc143-mt.dll                 # AssimpランタイムDLL
│   └── Resources/                          # ゲームアセットフォルダ
│       ├── Model/                          # .fbx, .obj, .gltf 3Dモデル & テクスチャ
│       ├── circle.png                      # 【新規】256x256 白丸板ポリ用テクスチャ
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
1. Visual Studio 2022 で **`Rizeallday_Engine_v2_GE3.slnx`** を開きます。
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
    heroModel->SetLockRootMotion(true); // 水平ルート移動を固定

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
    hero.worldTransform.scale = { 0.006f, 0.006f, 0.006f };

    // 5. ライティング設定
    LightGroup* lights = LightGroup::Create();
    lights->SetDefaultLighting();

    // 6. ゲームループ
    while (RizeEngine::ProcessMessage()) {
        RizeEngine::BeginFrame();

        float deltaTime = 0.016f;
        Input* input = RizeEngine::GetInput();

        if (input->PushKey(DIK_W)) {
            hero.worldTransform.translation.z += 4.5f * deltaTime;
        }

        camera.Update();
        hero.Update(deltaTime);
        lights->Update();

        auto* cmdList = RizeEngine::GetDirectXCommon()->GetCommandList();
        lights->Draw(cmdList, 3);
        hero.Draw(cmdList);

        RizeEngine::EndFrame();
    }

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

---

### 3Dモデル & スケルタルメッシュ (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`:
  Assimpを用いてモデルファイル（`.fbx`, `.obj`, `.gltf`, `.glb`, `.dae`）を読み込み、GPUバッファ（頂点・インデックス）を自動生成。ロード済みモデルはメモリ内に自動キャッシュされます。
- `bool HasBones() const`: スケルトンボーンデータが含まれているかを判定。
- `size_t GetBoneCount() const`: メッシュに関連付けられたボーン総数を取得。
- `float GetAnimationDuration(int animIndex = 0) const`: アニメーションの総時間を秒単位で取得。
- `void CalculateBoneTransforms(...)`: 指定時刻の階層ツリーを走査し、Slerp/Lerp補間により最終的なスキニング変形行列を算出。
- `void SetLockRootMotion(bool lock)`: ルートボーンの水平移動を固定し、アニメーションループ時のラバーバンド引き戻しを防止。

---

### 3Dオブジェクト実体 (`Object3d`)
- `void Initialize()`: マテリアル用定数バッファ、ワールド変換、およびGPUボーンパレットバッファ（128行列分）を割り当て。
- `void SetModel(Model* model, bool resetAnim = true)`: 描画対象の3Dモデルをバインド。
- `void SetCamera(Camera* camera)`: 撮影用カメラをバインド。
- `WorldTransform worldTransform`: `scale`, `rotation`, `translation` を管理。
- `BlendMode blendMode`: オブジェクトのブレンドモード。
- `void Update(float deltaTime = 1.0f / 60.0f)`: アニメーション時間を進め、ボーン行列を評価してWVP行列とともにGPU定数バッファへ転送。
- `void Draw(ID3D12GraphicsCommandList* cmdList)`: 指定ブレンドモードのPSOを自動バインドしメッシュを描画。

---

### プロシージャル丸影 (`CircleShadow`)
- **描画技術**: 専用シェーダー（`CircleShadow.VS.hlsl` / `PS.hlsl`）で床面クアッドに描画。テクスチャ画像不要で滑らかな影を描画。
- `void Initialize()`, `void Update(const Vector3& characterPos, float floorY, float radius, float opacity)`, `void Draw(ID3D12GraphicsCommandList* cmdList, Camera* camera)`.

---

### カメラ (`Camera`, `DebugCamera`)
- **`Camera`**:
  - `Vector3 translation`, `Vector3 rotation`（ラジアン指定）。
  - `float fovY`, `float aspectRatio`, `float nearClip`, `float farClip`。
  - `void Update()`: アフィン逆行列によるビュー行列と透視投影行列を再計算。
- **`DebugCamera`**:
  - 開発用マウスフリー操作カメラ。右ドラッグで回転、中ドラッグでパン、WASDQEで移動。

---

### ライティングシステム (`LightGroup`)
- `static LightGroup* Create()`, `SetDefaultLighting()`, `SetAmbientColor()`, `SetAmbientIntensity()`.
- `DirectionalLightData& GetDirectionalLight()`, `PointLightData& GetPointLight(int index)`, `SpotLightData& GetSpotLight(int index)`.

---

### GPUインスタンシング・パーティクル板ポリ (`ParticleQuad`)
課題01_00「たくさんの板ポリを出そう」に対応した、GPU Instancingによる高速板ポリゴン描画システム：
- **特徴**:
  - 1つの頂点バッファ（6頂点＝クアッド1枚分）のみをGPUに配置。
  - 各インスタンスのワールド行列＋WVP行列を `StructuredBuffer<TransformationMatrix>` に一括転送。
  - 頂点シェーダー側で `SV_InstanceID` を受け取り、各インスタンスの位置・回転・拡大を個別に適用。
  - 1回のドローコール `DrawInstanced(6, instanceCount, 0, 0)` でN個の板ポリを同時に描画。
- **使い方**:
  ```cpp
  uint32_t circleTex = TextureManager::GetInstance()->LoadTexture("Resources/circle.png");
  ParticleQuad particleQuads;
  particleQuads.Initialize(RizeEngine::GetDirectXCommon(), &dxc, circleTex, 10);

  // ゲームループ内:
  particleQuads.Update(camera.GetViewProjectionMatrix());
  particleQuads.Draw(cmdList);
  ```

---

### 衝突判定システム (`Collider` / `CollisionManager`)
日本工学院専門学校（GE3）の当たり判定設計に準拠した、3D衝突判定システム：
- **コライダー形状**:
  - `SphereCollider`: `Vector3 center`, `float radius`, `attribute`, `mask`.
  - `AABBCollider`: `Vector3 min`, `Vector3 max`, `attribute`, `mask`.
- **ビットマスク属性（Attribute & Mask）**:
  ```cpp
  enum CollisionAttribute : uint32_t {
      Player       = 1 << 0,
      Enemy        = 1 << 1,
      PlayerBullet = 1 << 2,
      EnemyBullet  = 1 << 3,
      Ground       = 1 << 4,
      All          = 0xFFFFFFFF,
  };
  ```
- **ゲームオブジェクトへの組み込み (`ICollidable`)**:
  ```cpp
  class Player : public ICollidable {
  public:
      SphereCollider collider_;

      const SphereCollider& GetSphereCollider() const override { return collider_; }

      void OnCollision(ICollidable* other) override {
          // 衝突時のコールバック処理（被弾・ダメージ処理など）
      }

      void Update() {
          // コライダーの中心座標をワールド位置に同期
          collider_.center = worldTransform.translation;
      }
  };
  ```
- **毎フレームの判定実行手順（`GameScene::Update` 内）**:
  ```cpp
  // 1. 各オブジェクトの移動・更新
  player.Update();
  enemy.Update();

  // 2. 判定マネージャへ登録
  CollisionManager::GetInstance()->RegisterCollider(&player);
  CollisionManager::GetInstance()->RegisterCollider(&enemy);

  // 3. 総当たり判定を実行（衝突したペアに対して OnCollision() を自動呼び出し）
  CollisionManager::GetInstance()->CheckAllCollisions();
  ```
- **独立判定関数**:
  - `CheckSphereVsSphere(const SphereCollider& a, const SphereCollider& b)`: 平方距離比較（$dx^2 + dy^2 + dz^2 \le (r_a + r_b)^2$）により平方根（`sqrt`）を排除した高速計算。
  - `CheckAABBvsAABB(const AABBCollider& a, const AABBCollider& b)`.
  - `CheckSphereVsAABB(const SphereCollider& sphere, const AABBCollider& aabb)`.

---

### 2Dスプライト描画 (`Sprite`)
- `void Initialize(DirectXCommon* dxCommon)`, `void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle)`, `void Update()`, `void Draw(ID3D12GraphicsCommandList* cmdList)`.

---

### プリミティブデバッグ描画 (`PrimitiveDrawer`)
- `DrawLine3d(p1, p2, color)`, `DrawGrid(size, subdivisions, color)`, `Render(cmdList)`.

---

### テクスチャマネージャ (`TextureManager`)
- `uint32_t LoadTexture(filePath)`: 画像をロードしてSRVを作成、`textureHandle` を返却。
- `D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(textureHandle)`: GPUデスクリプタハンドルを取得。

---

### 入力処理システム (`Input`)
- **キーボード**: `PushKey()`, `TriggerKey()`, `ReleaseKey()`.
- **マウス**: `PushMouseButton()`, `IsTriggerMouse()`, `GetMouseMoveX()`, `GetMouseMoveY()`, `GetMouseWheel()`.
- **ゲームパッド**: `IsGamepadConnected()`, `PushGamepadButton()`, `TriggerGamepadButton()`, `GetLeftStickX()`, `GetLeftStickY()`, `SetVibration()`.

---

### オーディオ再生システム (`Audio`)
- `SoundLoadWave(filename)`, `SoundPlayWave(soundData)`, `SoundUnload(soundData)`.

---

### 数学ライブラリ (`Mathematics` / `MathUtil`)
- `MakeIdentity4x4()`, `MakeAffineMatrix()`, `MakePerspectiveFovMatrix()`, `MakeOrthographicMatrix()`, `Multiply()`, `Inverse()`, `Transform()`.

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

### ルートシグネチャ構成 (`GraphicPipeline.cpp` — Object3d):
| ルートスロット | レジスタ | 可視シェーダー | タイプ | バッファ内容 |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material（基本色、ライティング有効フラグ、陰影計算タイプ） |
| **1** | `b0` | Vertex Shader | CBV | TransformationMatrix（WVP行列、ワールド行列） |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2Dディフューズテクスチャ |
| **3** | `b1` | Pixel Shader | CBV | LightGroup（環境光、平行光源、点光源3基、スポットライト3基） |
| **4** | `b1` | Vertex Shader | CBV | **BonePalette**（128本のボーンスキニング変形行列配列） |

### パーティクルインスタンシングルートシグネチャ (`ParticleQuad`):
| ルートスロット | レジスタ | 可視シェーダー | タイプ | バッファ内容 |
|:---:|:---:|:---:|:---:|---|
| **0** | `b0` | Pixel Shader | CBV | Material（カラー、透明度） |
| **1** | `t0` | Vertex Shader | Descriptor Table (SRV) | `StructuredBuffer<TransformationMatrix>` (SRV Slot 20) |
| **2** | `t0` | Pixel Shader | Descriptor Table (SRV) | 2Dディフューズテクスチャ (`circle.png`) |