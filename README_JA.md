# Rizeallday Engine v2

[English](README_EN.md) | **日本語** | [Bahasa Indonesia](README.md)

**Rizeallday Engine v2** へようこそ！  
本プロジェクトは、現代的な **Engine + Sandbox アーキテクチャ**（[TheCherno Hazel Engine](https://www.youtube.com/@TheCherno) や商用C++ゲームエンジンの設計思想）を採用し、再利用可能な静的ライブラリ（`.lib`）とゲーム制作 sandbox（`.exe`）に完全分離して設計された独自の **DirectX 12 3Dゲームエンジン** です。

純粋な **DirectX 12**、**C++20**、**Assimp**（FBX・OBJ・GLTF・DAE対応）、GPUアクセラレーションによる **スケルタルアニメーション（ボーンパレット最大128本）**、**マルチライティング**（平行光源・点光源・スポットライト・環境光）、`StructuredBuffer + SV_InstanceID` による **GPUパーティクルインスタンシング描画**、**完全な3D衝突判定システム（Sphere / AABB / OBB 15軸分離軸定理 / Plane / Triangle / Segment / OnCollisionコールバック）**、および **Dear ImGui** によるランタイムデバッグ機能を標準搭載しています。

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
   - [3D衝突判定システム (`Collider` / `CollisionManager`)](#3d衝突判定システム-collider--collisionmanager)
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
- **GPUインスタンシング描画**:
  - `StructuredBuffer<TransformationMatrix>` と `SV_InstanceID` を用いた高速インスタンス描画。
  - `DrawInstanced(6, N, 0, 0)` により1回のドローコールで多数の板ポリゴンを一括描画。
  - 独立した専用RootSignature・PSO・SRV（スロット20）を備えた `ParticleQuad` クラス。
- **3D衝突判定システム（Collision Detection System）**:
  - 多彩な形状プリミティブ: `SphereCollider`（球）、`AABBCollider`（軸平行境界ボックス）、`OBBCollider`（自由回転3D境界ボックス）、`PlaneCollider`（無限平面）、`TriangleCollider`（3Dポリゴン三角形）、`Segment`（線分）、`Ray`（半直線）。
  - **15軸分離軸判定（Separating Axis Theorem / SAT）** による高精度なOBB対OBBの回転衝突判定。
  - **Slab法** による高速なAABBおよびOBBに対する線分・レイ判定。
  - 全登録オブジェクトを1フレーム内で判定する `CollisionManager`（総当たり判定方式）。
  - `ICollidable` インターフェースによるポリモーフィックな衝突コールバック（`OnCollision()`）。
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
│   │   ├── Collider.h / .cpp               # Sphere, AABB, OBB, Plane, Triangle, Segment交差判定
│   │   └── CollisionManager.h / .cpp       # CollisionManager シングルトン & ICollidable
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
│       ├── Particle.VS.hlsl                # GPUインスタンシング頂点シェーダー
│       ├── Particle.PS.hlsl                # パーティクルピクセルシェーダー（透過破棄）
│       ├── Sprite.hlsli                    # 2Dスプライト共通構造体
│       ├── Sprite.VS.hlsl                  # 2Dスプライト頂点シェーダー
│       └── Sprite.PS.hlsl                  # 2Dスプライトピクセルシェーダー
│
├── RizealldaySandbox/                      # 【ゲーム本体】制作ワークスペース
│   ├── main.cpp                            # ゲームループ & キャラクターコントローラー
│   ├── ParticleQuad.h / .cpp               # 板ポリインスタンシング管理クラス
│   ├── dxcompiler.dll, dxil.dll            # DXCシェーダーコンパイラバイナリ
│   ├── assimp-vc143-mt.dll                 # AssimpランタイムDLL
│   └── Resources/                          # ゲームアセットフォルダ
│       ├── Model/                          # .fbx, .obj, .gltf 3Dモデル & テクスチャ
│       ├── circle.png                      # 256x256 白丸板ポリ用テクスチャ
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

---

## 4. API・サブシステムリファレンス

### エンジンライフサイクル (`RizeEngine`)
- `RizeEngine::Initialize(const char* title, int width, int height)`: ウィンドウ生成、DirectX 12デバイス、DXC、パイプライン、テクスチャマネージャ、入力、サウンドを一括初期化。
- `bool RizeEngine::ProcessMessage()`: Windowsメッセージを処理。終了リクエストで `false` を返却。
- `void RizeEngine::BeginFrame()`: バックバッファのクリア、入力状態ポーリング、ImGui新規フレーム開始。
- `void RizeEngine::EndFrame()`: ImGui描画、コマンドリスト実行、スワップチェーンPresent呼び出し。
- `void RizeEngine::Finalize()`: GPUフェンス待機後、全COMリソースを安全に破棄。

---

### DirectX 12 描画基盤 (`DirectXCommon`)
- `ID3D12Device* GetDevice() const`: DirectX 12 デバイス本体への直接アクセス。
- `ID3D12GraphicsCommandList* GetCommandList() const`: 描画記録用コマンドリスト。
- `ID3D12Resource* CreateBufferResource(size_t sizeInBytes)`: GPUアップロードバッファ生成ヘルパー。
- `void SignalAndWaitFence()`: CPU-GPU間の完全同期フェンス待機。

---

### パイプライン & ブレンドモード (`GraphicPipeline` / `BlendMode`)
```cpp
enum class BlendMode : int {
    Opaque     = 0, // 不透明描画・深度書き込みON（デフォルト）
    AlphaBlend = 1, // 通常半透明合成・深度書き込みOFF
    Add        = 2, // 加算合成（炎・魔法エフェクト・光）・深度書き込みOFF
    Multiply   = 3, // 乗算合成（暗影・呪縛オーラ）・深度書き込みOFF
    Count      = 4
};
```

---

### 3Dモデル & スケルタルメッシュ (`Model`)
- `Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename)`: Assimpを用いたモデルファイル（`.fbx`, `.obj`, `.gltf`, `.glb`, `.dae`）の自動キャッシュ読み込み。
- `bool HasBones() const`: スケルトンボーンデータが含まれているかを判定。
- `size_t GetBoneCount() const`: ボーン総数を取得。
- `void SetLockRootMotion(bool lock)`: ルートボーンの水平移動を固定し、アニメーションループ時のラバーバンド引き戻しを防止。

---

### 3Dオブジェクト実体 (`Object3d`)
- `void Initialize()`: 定数バッファおよびGPUボーンパレットバッファ（128行列分）を割り当て。
- `void SetModel()`, `SetCamera()`, `PlayAnimation()`.
- `WorldTransform worldTransform`: `scale`, `rotation`, `translation` を管理。
- `void Update(float deltaTime)`, `void Draw(ID3D12GraphicsCommandList* cmdList)`.

---

### プロシージャル丸影 (`CircleShadow`)
床面クアッドに専用シェーダーで描画するリアルタイム円形ドロップシャドウ：
```cpp
CircleShadow shadow;
shadow.Initialize();
shadow.GetMaterial()->color   = { 0.0f, 0.0f, 0.0f, 0.45f };
shadow.GetMaterial()->softness = 0.5f;

// ゲームループ内:
shadow.Update(character.worldTransform.translation, -1.5f, 0.35f, 0.45f);
shadow.Draw(cmdList, &camera);
```

---

### カメラ (`Camera`, `DebugCamera`)
- **`Camera`**: 透視投影3Dカメラ（`translation`, `rotation`, `fovY`, `Update()`, `GetViewProjectionMatrix()`）。
- **`DebugCamera`**: マウス右ドラッグ旋回、中ドラッグパン、WASDQE移動による自由視点カメラ。

---

### ライティングシステム (`LightGroup`)
1つの定数バッファにまとめた高効率マルチライティング（平行光源1基、点光源3基、スポットライト3基、環境光）。

---

### GPUインスタンシング・パーティクル板ポリ (`ParticleQuad`)
GPU Instancingによる高速板ポリゴン描画システム：
- 1つの頂点バッファ（6頂点＝クアッド1枚分）のみをGPUに配置。
- `StructuredBuffer<TransformationMatrix>` に各インスタンスの行列を一括転送。
- 頂点シェーダー側で `SV_InstanceID` を受け取り各インスタンスを個別変換。
- `DrawInstanced(6, instanceCount, 0, 0)` により1回のドローコールで全インスタンスを描画。

---

### 3D衝突判定システム (`Collider` / `CollisionManager`)

モジュール化された高機能3D当たり判定システム：

#### 1. 形状プリミティブ一覧:
- **`SphereCollider`**: 中心座標と半径。
- **`AABBCollider`**: 軸平行直方体（`min`, `max`）。
- **`OBBCollider`**: **自由回転3D直方体**（`center`, 各軸方向ベクトル `orientations[3]`, 半サイズ `size`, `SetRotation()`）。
- **`PlaneCollider`**: 無限平面（法線 `normal`, 距離 `distance`）。
- **`TriangleCollider`**: 3Dポリゴン三角形（頂点3点 `vertices[3]`, 法線 `Normal()`）。
- **`Segment`**: 線分（始点 `origin`, 変位ベクトル `different`）。
- **`Ray` & `Line`**: 半直線および無限直線。

#### 2. 交差判定関数一覧:
| 判定関数 | 採用アルゴリズム・計算手法 |
|---|---|
| `CheckSphereVsSphere(Sphere, Sphere)` | 平方距離比較（平方根 `sqrt` を排除した高速計算） |
| `CheckSphereVsAABB(Sphere, AABB)` | AABBに対する最近接点（Closest Point）距離判定 |
| `CheckSphereVsOBB(Sphere, OBB)` | 球の中心をOBBローカル空間へ逆変換して判定 |
| `CheckSphereVsPlane(Sphere, Plane)` | 内積による平面からの垂直距離判定 |
| `CheckAABBvsAABB(AABB, AABB)` | X, Y, Z各軸の区間重複判定 |
| `CheckAABBvsSegment(AABB, Segment)` | スラブ法（Slab Method）によるパラメータ $t$ 区間計算 |
| `CheckOBBvsOBB(OBB, OBB)` | **15軸分離軸判定（Separating Axis Theorem / SAT）** |
| `CheckOBBvsAABB(OBB, AABB)` | AABBを回転なしOBBとみなした15軸SAT判定 |
| `CheckOBBvsSegment(OBB, Segment)` | OBBローカル空間でのスラブ法判定 |
| `CheckPlaneVsSegment(Plane, Segment)` | 平面との貫通交差パラメータ $t$ 計算 |
| `CheckTriangleVsSegment(Triangle, Segment)` | 三角形のエッジ外積によるPoint-in-Triangle判定 |

#### 3. 幾何ベクトル支援関数:
- `ClosestPoint(point, aabb)`: AABB上の最寄り点座標
- `ClosestPoint(point, obb)`: OBB上の最寄り点座標
- `ClosestPoint(point, segment)`: 線分上の最寄り点座標
- `Project(v1, v2)`: ベクトル射影
- `Reflect(v, normal)`: 法線面に対する反射ベクトル計算

#### 4. CollisionManager と `ICollidable`:
```cpp
class Player : public ICollidable {
public:
    OBBCollider obb_;

    ColliderShape GetColliderShape() const override { return ColliderShape::OBB; }
    const OBBCollider& GetOBBCollider() const override { return obb_; }

    void Initialize() {
        obb_.size = { 0.4f, 0.9f, 0.4f };
        obb_.attribute = CollisionAttribute::Player;
        obb_.mask      = CollisionAttribute::Obstacle | CollisionAttribute::EnemyBullet;
    }

    void Update(const Vector3& pos, const Vector3& rot) {
        obb_.center = pos;
        obb_.SetRotation(rot); // 回転角に連動
    }

    void OnCollision(ICollidable* other) override {
        // 衝突時自動コールバック
    }
};
```

---

### 2Dスプライト描画 (`Sprite`)
2D UI、背景、HUD描画用：`Initialize()`, `SetTexture()`, `transformSprite`, `Update()`, `Draw()`.

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