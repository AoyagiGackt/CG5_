/**
 * @file GamePlayScene.h
 * @brief ゲームプレイ本編のシーンロジックを管理するファイル
 *
 * このシーンは「ゲームを実際に遊ぶ画面」を担当する。
 * Initialize（開始時の初期設定）→ Update（毎フレーム更新）→ Draw（毎フレーム描画）
 * → Finalize（終了時の後片付け）の順に呼ばれる。
 */
#pragma once

 // --- 標準ライブラリ ---
#include <deque>
#include <memory>
#include <string>
#include <vector>

// --- エンジンシステム・基盤 ---
#include "Audio.h"
#include "BaseScene.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "GameObject.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3dCommon.h"
#include "ShadowManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"

// --- ゲームロジック・オブジェクト ---
#include "Object3d.h"
#include "GameTime.h"
#include "MapChipField.h"
#include "Skydome.h"
#include "Ring.h"
#include "Cylinder.h"
#include "SkinCommon.h"
#include "SkinnedModel.h"
#include "SkinnedObject3d.h"
#include "Skeleton.h"
#include "Animation.h"
#include "ImageFilter.h"
#include "RenderTexture.h"
#include "SceneEditor.h"

// Initialize で一度取得してキャッシュするシステムポインタ（実体はシングルトンが管理）
// ※ ヘッダに #include を書くとコンパイルが遅くなるため、前方宣言で済ませている
class GrayscaleEffect;
class HsvFilter;
class ImageFilter;
class ParticleManager;
class ScoreManager;

/**
 * @brief ゲームプレイ本編のシーンクラス
 *
 * BaseScene を継承しており、SceneManager から
 * Initialize / Update / Draw / Finalize が呼ばれる。
 */
class GamePlayScene : public BaseScene{
public:
    // --- 基本関数 ---
    // ゲームが始まるときに1度だけ呼ばれる。オブジェクト生成・初期設定をここでやる
    void Initialize(DirectXCommon* dxCommon,Input* input,Audio* audio) override;

    // ゲームが終わるとき（シーン切替含む）に1度だけ呼ばれる。メモリの解放などをここでやる
    void Finalize() override;

    // 毎フレーム呼ばれる。ゲームの状態を1フレーム分進める
    void Update() override;

    // 毎フレーム呼ばれる。画面に絵を描く
    void Draw() override;

    // --- パブリックメソッド ---
    // 外部から ImGuiManager を受け取って保持する（ImGui はデバッグ用の GUI ライブラリ）
    void SetImGuiManager(ImGuiManager* imgui){ imguiManager_ = imgui; }

private:
    // --- 内部処理関数 ---

    // シャドウマップ（影の元になる深度画像）の描画パスを実行する
    void DrawShadowPass();

    // カメラ位置の「ぬるぬる補間」を毎フレーム処理する
    // 過去数フレームの位置の平均をとることで、急な動きを滑らかにする（ボックスフィルタ）
    void UpdateCameraSmoothing();

    // SceneEditor（ImGuiデバッグUI）に渡す「現在のシーンのデータ一覧」を組み立てる
    SceneEditor::EditContext BuildEditContext();

    // =================================================
    // メンバ変数
    // =================================================

    // --- 外部システムポインタ（注入済み）---
    // Initialize の引数で受け取って保持する。delete しない（所有権は呼び出し元）
    DirectXCommon* dxCommon_    = nullptr; // DirectX12 の中核。GPU命令を出す窓口
    Input*         input_       = nullptr; // キーボード・コントローラー入力
    Audio*         audio_       = nullptr; // BGM・SE の再生
    ImGuiManager*  imguiManager_ = nullptr; // デバッグ用のGUI（開発中だけ表示）

    // --- キャッシュ済みシステムポインタ（Initialize で取得、生存期間はシステムが保証）---
    // 毎フレーム GetInstance() を呼ぶとコストがかかるので、起動時に1度だけ取得して保存しておく
    ParticleManager* particleManager_ = nullptr; // パーティクル（炎・光の粒など）の管理
    ScoreManager*    scoreManager_    = nullptr; // スコアの計算・保存
    SrvManager*      srvManager_      = nullptr; // GPU のテクスチャ用メモリ（ディスクリプタヒープ）管理
    GrayscaleEffect* grayscaleEffect_ = nullptr; // 画面をグレースケールにするポストエフェクト
    ImageFilter*     imageFilter_     = nullptr; // 画像フィルタ全般のポストエフェクト
    HsvFilter*       hsvFilter_       = nullptr; // 色相・彩度・明度を調整するポストエフェクト

    // --- 描画・共通基盤リソース ---
    // unique_ptr = 「このクラスだけが所有者」という意味。delete も自動でやってくれる
    std::unique_ptr<SpriteCommon>  spriteCommon_;  // 2Dスプライト描画に共通する設定（シェーダーなど）
    std::unique_ptr<ModelCommon>   modelCommon_;   // 3Dモデル描画に共通する設定
    std::unique_ptr<Object3dCommon> objectCommon_; // 3Dオブジェクト描画に共通する設定（ライトなど）
    std::unique_ptr<ShadowManager>  shadowManager_; // 影描画の管理（シャドウマップ方式）
    std::unique_ptr<Camera>         camera_;        // 視点となるカメラ

    // --- 天球（無効時は nullptr）---
    // ゲームの背景を覆う大きな球。現在は使用していないため nullptr のまま
    std::unique_ptr<Skydome> skydome_;

    // --- スキンメッシュ（human） ---
    // スキンメッシュ = ボーン（骨格）に沿ってポリゴンが動く、関節を曲げられる3Dモデル
    std::unique_ptr<SkinCommon>      skinCommon_;  // スキンメッシュ描画に共通する設定
    std::unique_ptr<SkinnedModel>    modelHuman_;  // human の3Dモデルデータ（頂点・マテリアル）
    std::unique_ptr<SkinnedObject3d> human_;       // ワールドに配置されている human インスタンス

    // --- ゲームオブジェクト群 ---
    std::vector<std::unique_ptr<GameObject>> gameObjects_; // 汎用ゲームオブジェクトのリスト
    std::unique_ptr<MapChipField> mapField_;               // マップチップ（タイル）フィールド

    // --- 進行・状態管理 ---
    GameTime gameTime_; // ゲーム内時刻（時・分）を管理する

    // --- 白パーティクル（1024個静的配置） ---
    // 画面全体にふわふわ漂う白い光の粒の設定値
    Vector3  whiteParticlePos_   = { 14.5f, 3.0f, 0.0f };    // 散布の中心座標
    Vector4  whiteParticleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 色（RGBA）
    float    whiteParticleScale_ = 0.5f;                        // 粒の大きさ
    int      whiteParticleCount_ = 1024;                        // 粒の個数

    // --- 楕円パーティクル（Ring 周回） ---
    // リングの周りを円軌道で飛び続ける水色の光の粒
    float ellipseParticleTimer_ = 0.0f;                           // 次の放出までの残り時間
    static constexpr float kEllipseEmitInterval = 0.05f;          // 放出間隔（秒）
    float ringOrbitAngle_ = 0.0f;                                  // 現在の軌道角度（ラジアン）

    // --- Ring ---
    // リング形状のメッシュオブジェクト（ドーナツ型）
    std::unique_ptr<Ring> ring_;
    Vector3 ringPosition_    = { 14.5f, 0.0f, 0.0f }; // ワールド座標
    Vector3 ringRotation_    = {};                      // 回転角度（XYZ）
    Vector4 ringColor_       = { 1.0f, 1.0f, 1.0f, 1.0f }; // 色（RGBA）
    float   ringScale_       = 1.0f;     // 全体のスケール
    float   ringInnerRadius_ = 1.5f;     // リングの内側の半径（穴の大きさ）
    float   ringOuterRadius_ = 2.5f;     // リングの外側の半径

    // --- Cylinder ---
    // 円柱形状のメッシュオブジェクト
    std::unique_ptr<Cylinder> cylinder_;
    Vector3 cylinderPosition_     = { 17.0f, 0.0f, 0.0f }; // ワールド座標
    Vector3 cylinderRotation_     = {};                      // 回転角度（XYZ）
    Vector4 cylinderColor_        = { 1.0f, 1.0f, 1.0f, 1.0f }; // 色（RGBA）
    float   cylinderScale_        = 1.0f;  // 全体のスケール
    float   cylinderTopRadius_    = 1.0f;  // 円柱の上面の半径
    float   cylinderBottomRadius_ = 1.0f;  // 円柱の下面の半径
    float   cylinderHeight_       = 3.0f;  // 円柱の高さ
    float   cylinderAlphaRef_     = 0.0f;  // アルファ参照値（透明度の閾値）

    // --- Human パラメータ ---
    // スキンメッシュキャラクターの初期設定値
    Vector3 humanPosition_     = { 5.0f, 0.0f, 0.0f }; // ワールド座標
    Vector3 humanRotation_     = { 0.0f, 0.0f, 0.0f }; // 回転角度（XYZ）
    Vector3 humanScale_        = { 1.0f, 1.0f, 1.0f }; // スケール（1.0 = 等倍）
    float   humanAnimSpeed_    = 1.0f;                  // アニメーション再生速度（1.0 = 等速）
    bool    showSkeletonDebug_ = false;                 // ボーン（骨格）のデバッグ表示をするか

    // --- Skydome パラメータ ---
    Vector4 skyColor_      = { 1.0f, 1.0f, 1.0f, 1.0f }; // 天球の色（RGBA）
    float   skyRotOffsetY_ = 0.0f;                         // 天球を Y 軸方向に回転させるオフセット

    // --- オフスクリーンレンダリング ---
    // 画面に直接描かずに「一旦テクスチャに描いて、それを画面に貼る」仕組み
    // ポストエフェクト（グレースケールなど）をかけるために使う
    std::unique_ptr<RenderTexture> renderTexture_;       // テクスチャとして使えるレンダーターゲット
    std::unique_ptr<Sprite>        renderTextureSprite_; // 上記をそのまま画面に貼るためのスプライト

    // --- Camera Box Filter Smoothing ---
    // カメラが急に動いたとき、過去数フレームの位置を平均して滑らかにする仕組み
    Vector3 cameraTargetPos_ = { 14.5f, 6.0f, -30.0f }; // カメラが向かう目標位置
    Vector3 cameraTargetRot_ = { 0.0f, 0.0f, 0.0f };    // カメラが向かう目標角度
    std::deque<Vector3> cameraPosHistory_; // 過去の位置履歴（先頭から古い順）
    std::deque<Vector3> cameraRotHistory_; // 過去の角度履歴
    int cameraSmoothFrames_ = 1;           // 平均をとるフレーム数（1なら補間なし、大きいほど滑らか）

    // --- デバッグ・エディタ ---
    // ImGui を使ってゲーム実行中にパラメータをリアルタイムで調整できるエディタ
    SceneEditor sceneEditor_;
};
