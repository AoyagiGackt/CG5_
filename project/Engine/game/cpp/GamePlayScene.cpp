#include "GamePlayScene.h"
#include <cmath>
#include "GameConstants.h"
#include "GrayscaleEffect.h"
#include "HsvFilter.h"
#include "ImageFilter.h"
#include "ParticleManager.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include "TextureManager.h"

// =====================================================
// 初期化
// =====================================================

void GamePlayScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    // 引数のポインタをメンバ変数に保存しておく（後で Update/Draw からも使えるように）
    dxCommon_ = dxCommon;
    input_ = input;
    audio_ = audio;

    // ----- 描画共通設定 -----
    // スプライト（2D画像）描画に必要な共通設定（シェーダーのセットアップなど）
    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // 3Dモデル描画に必要な共通設定
    modelCommon_ = std::make_unique<ModelCommon>();
    modelCommon_->Initialize(dxCommon_);

    // 3Dオブジェクト描画に必要な共通設定（ライト情報なども含む）
    objectCommon_ = std::make_unique<Object3dCommon>();
    objectCommon_->Initialize(dxCommon_);

    // ----- シングルトンをキャッシュ（以降はポインタ経由でアクセス）-----
    // シングルトン = ゲーム中に1つだけ存在するシステム管理クラス
    // 毎フレーム GetInstance() を呼ぶとオーバーヘッドが生じるので、起動時に1度だけ取得して変数に保存する
    srvManager_      = SrvManager::GetInstance();
    particleManager_ = ParticleManager::GetInstance();
    scoreManager_    = ScoreManager::GetInstance();
    grayscaleEffect_ = GrayscaleEffect::GetInstance();
    imageFilter_     = ImageFilter::GetInstance();
    hsvFilter_       = HsvFilter::GetInstance();

    // ----- 影の描画（シャドウマップ）-----
    // シャドウマップ = 光源視点から深度画像を作り、それを使ってオブジェクトの影を描く手法
    shadowManager_ = std::make_unique<ShadowManager>();
    shadowManager_->Initialize(dxCommon_, srvManager_);

    // ----- カメラ -----
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 14.5f, 6.0f, -30.0f }); // 初期位置（少し高く、手前から見下ろす角度）
    Object3d::SetCommonCamera(camera_.get()); // 全 3D オブジェクトがこのカメラを使うよう設定

    // ----- スキンメッシュ（human）-----
    // スキンメッシュ = ボーン（骨格）でポリゴンを動かすアニメーション付き3Dモデル
    skinCommon_ = std::make_unique<SkinCommon>();
    skinCommon_->Initialize(dxCommon_);

    // gltf ファイルからモデルデータ（頂点・マテリアル）を読み込む
    modelHuman_ = std::make_unique<SkinnedModel>();
    modelHuman_->Initialize(dxCommon_,
        "Resources/human/sneakWalk.gltf",
        "Resources/human/white.png");

    // 同じ gltf からボーン階層（NodeHierarchy）とアニメーションデータを読み込む
    Node humanRootNode = LoadNodeHierarchyFromFile("Resources/human", "sneakWalk.gltf");
    Skeleton humanSkeleton = Skeleton::Create(humanRootNode);
    Animation humanAnimation = LoadAnimationFile("Resources/human", "sneakWalk.gltf");

    // ワールドに配置するインスタンスを作り、モデル・スケルトン・アニメーションを紐付ける
    human_ = std::make_unique<SkinnedObject3d>();
    human_->Initialize(skinCommon_.get());
    human_->SetModel(modelHuman_.get());
    human_->SetSkeleton(std::move(humanSkeleton));
    human_->SetAnimation(std::move(humanAnimation));
    human_->SetPosition(humanPosition_);

    // SkinnedObject3d の「クラス全体で共有する設定」を一括でセットする
    SkinnedObject3d::SetCommonCamera(camera_.get());
    SkinnedObject3d::SetCommonObjectCommon(objectCommon_.get());
    SkinnedObject3d::SetCommonShadowManager(shadowManager_.get());
    SkinnedObject3d::SetCommonModelCommon(modelCommon_.get());

    // ----- エフェクト・進行管理 -----

    // 白パーティクル（1024個を個別ライフタイムで散布・自動再配置）
    // 一度に大量に生成して画面全体にふわふわ漂わせる演出
    particleManager_->CreateParticleGroup("white", "Resources/white.png");
    particleManager_->SetAdditiveBlend("white", false); // 加算合成オフ（通常のアルファ合成）
    particleManager_->EmitScatterLoop(
        "white", whiteParticlePos_, GameConstants::kWhiteParticleScatterRadius,
        static_cast<uint32_t>(whiteParticleCount_),
        whiteParticleColor_,
        GameConstants::kWhiteParticleLifetimeMin, GameConstants::kWhiteParticleLifetimeMax,
        whiteParticleScale_);

    // 楕円パーティクルグループ（circle2.png を使用）
    // リング周回用の水色の粒。Update() で毎フレーム少しずつ放出する
    particleManager_->CreateParticleGroup("ellipse", "Resources/circle2.png");

    // 斬撃パーティクルグループ（gradationLine.png を使用）
    particleManager_->CreateParticleGroup("slash", "Resources/gradationLine.png");

    // Ring（ドーナツ型メッシュ）を生成して初期パラメータをセット
    ring_ = std::make_unique<Ring>();
    ring_->Initialize(dxCommon_);
    ring_->SetPosition(ringPosition_);
    ring_->SetInnerRadius(ringInnerRadius_);
    ring_->SetOuterRadius(ringOuterRadius_);

    // Cylinder（円柱メッシュ）を生成して初期パラメータをセット
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(dxCommon_);
    cylinder_->SetPosition(cylinderPosition_);
    cylinder_->SetTopRadius(cylinderTopRadius_);
    cylinder_->SetBottomRadius(cylinderBottomRadius_);
    cylinder_->SetHeight(cylinderHeight_);

    // スコアをファイルから読み込み、今回プレイのスコアをリセットする
    scoreManager_->LoadScores();
    scoreManager_->ResetCurrentScore();

    // ゲーム内時刻を初期化（ゲーム開始 = 0時0分）
    gameTime_.Initialize();

    // ----- オフスクリーンレンダリング -----
    // 画面に直接描かずに一旦テクスチャに描いておく仕組み
    // これにより、後からポストエフェクト（グレースケールなど）を重ねて適用できる
    renderTexture_ = std::make_unique<RenderTexture>();
    renderTexture_->Initialize(dxCommon_, srvManager_,
        WinApp::kClientWidth, WinApp::kClientHeight);

    // レンダーテクスチャを画面いっぱいに貼るためのスプライト
    renderTextureSprite_ = std::make_unique<Sprite>();
    renderTextureSprite_->Initialize(spriteCommon_.get(), "Resources/white.png");
    renderTextureSprite_->SetExternalTexture(renderTexture_->GetSrvIndex()); // テクスチャをレンダーテクスチャに差し替え
    renderTextureSprite_->SetPosition({ 0.0f, 0.0f });
    renderTextureSprite_->SetSize({ static_cast<float>(WinApp::kClientWidth),
                                    static_cast<float>(WinApp::kClientHeight) });

    // ----- デバッグパラメータ読み込み -----
    // 前回エディタで保存したカメラ位置・UIレイアウトなどを JSON から復元する
    sceneEditor_.LoadAll(BuildEditContext());

    // カメラスムージング用の初期目標値を現在のカメラ位置から取る
    cameraTargetPos_ = camera_->GetTranslate();
    cameraTargetRot_ = camera_->GetRotate();
}

// =====================================================
// EditContext 構築ヘルパー
// =====================================================

// SceneEditor（ImGuiデバッグUI）が各オブジェクトにアクセスするための「参照集」を組み立てる。
// ポインタを渡すので、エディタ側でスライダーを動かすと即座にゲーム側の変数に反映される。
SceneEditor::EditContext GamePlayScene::BuildEditContext()
{
    SceneEditor::EditContext ctx;

    // オブジェクト本体へのポインタ（エディタが直接メソッドを呼ぶために必要）
    ctx.camera       = camera_.get();
    ctx.ring         = ring_.get();
    ctx.cylinder     = cylinder_.get();
    ctx.skydome      = skydome_.get(); // 現在 nullptr（無効）
    ctx.human        = human_.get();
    ctx.spriteCommon = spriteCommon_.get();

    // カメラ制御パラメータへのポインタ
    ctx.cameraTargetPos    = &cameraTargetPos_;
    ctx.cameraTargetRot    = &cameraTargetRot_;
    ctx.cameraSmoothFrames = &cameraSmoothFrames_;
    ctx.cameraPosHistory   = &cameraPosHistory_;
    ctx.cameraRotHistory   = &cameraRotHistory_;

    // Ring パラメータへのポインタ
    ctx.ringPosition    = &ringPosition_;
    ctx.ringRotation    = &ringRotation_;
    ctx.ringColor       = &ringColor_;
    ctx.ringScale       = &ringScale_;
    ctx.ringInnerRadius = &ringInnerRadius_;
    ctx.ringOuterRadius = &ringOuterRadius_;

    // Cylinder パラメータへのポインタ
    ctx.cylinderPosition     = &cylinderPosition_;
    ctx.cylinderRotation     = &cylinderRotation_;
    ctx.cylinderColor        = &cylinderColor_;
    ctx.cylinderScale        = &cylinderScale_;
    ctx.cylinderTopRadius    = &cylinderTopRadius_;
    ctx.cylinderBottomRadius = &cylinderBottomRadius_;
    ctx.cylinderHeight       = &cylinderHeight_;
    ctx.cylinderAlphaRef     = &cylinderAlphaRef_;

    // Skydome パラメータへのポインタ
    ctx.skyColor      = &skyColor_;
    ctx.skyRotOffsetY = &skyRotOffsetY_;

    // Human パラメータへのポインタ
    ctx.humanPosition  = &humanPosition_;
    ctx.humanRotation  = &humanRotation_;
    ctx.humanScale     = &humanScale_;
    ctx.humanAnimSpeed = &humanAnimSpeed_;
    ctx.showSkeleton   = &showSkeletonDebug_;

    // 白パーティクルパラメータへのポインタ
    ctx.whiteParticlePos   = &whiteParticlePos_;
    ctx.whiteParticleColor = &whiteParticleColor_;
    ctx.whiteParticleScale = &whiteParticleScale_;
    ctx.whiteParticleCount = &whiteParticleCount_;

    // ゲーム内時刻（表示用の値コピー）
    ctx.gameHour   = gameTime_.GetHour();
    ctx.gameMinute = gameTime_.GetMinute();
    return ctx;
}

// =====================================================
// カメラスムージング（毎フレーム呼ぶ）
// =====================================================

// 過去 cameraSmoothFrames_ フレーム分の位置・角度を記録しておき、
// その平均値を実際のカメラ座標として適用する（ボックスフィルタ補間）。
// フレーム数が多いほど動きが滑らかになるが、反応が遅くなる。
void GamePlayScene::UpdateCameraSmoothing()
{
    // 今フレームの目標位置・角度を履歴に追加する
    cameraPosHistory_.push_back(cameraTargetPos_);
    cameraRotHistory_.push_back(cameraTargetRot_);

    // 履歴が指定フレーム数を超えたら古いものを先頭から捨てる
    while ((int)cameraPosHistory_.size() > cameraSmoothFrames_) {
        cameraPosHistory_.pop_front();
        cameraRotHistory_.pop_front();
    }

    // 履歴全体の平均を計算する
    Vector3 avgPos = {};
    Vector3 avgRot = {};
    for (const auto& p : cameraPosHistory_) {
        avgPos.x += p.x; avgPos.y += p.y; avgPos.z += p.z;
    }
    for (const auto& r : cameraRotHistory_) {
        avgRot.x += r.x; avgRot.y += r.y; avgRot.z += r.z;
    }
    float n = static_cast<float>(cameraPosHistory_.size());

    // 平均値をカメラに適用する
    camera_->SetTranslate({ avgPos.x / n, avgPos.y / n, avgPos.z / n });
    camera_->SetRotate({ avgRot.x / n, avgRot.y / n, avgRot.z / n });
}

// =====================================================
// 更新処理（毎フレーム呼ばれる）
// =====================================================

void GamePlayScene::Update()
{
    // ゲーム内時刻を1フレーム分進める（引数は時刻の進み速度。1.0f = リアルタイムと同じ速度）
    gameTime_.Update(1.0f);

    // カメラのスムージング（ぬるぬる補間）を更新する
    UpdateCameraSmoothing();

    // ----- 影の更新 -----
    // 光源方向が変わったときに影用の視錐台（光が届く範囲）を更新する
    shadowManager_->Update(objectCommon_->GetLightDirection());

    // 3Dオブジェクトとスキンメッシュに、最新の「影行列」を渡す（シャドウマップ参照に使う）
    Object3d::SetLightViewProjection(shadowManager_->GetLightViewProjection());
    SkinnedObject3d::SetLightViewProjection(shadowManager_->GetLightViewProjection());

    // ----- ゲームオブジェクトの更新 -----
    for (auto& obj : gameObjects_) {
        obj->Update();
    }

    // human のアニメーション・トランスフォームを更新する
    human_->Update();

    // ----- デバッグ UI 更新 -----
    // ImGui パネルを描画し、エディタで変更した値をゲームに反映する
    sceneEditor_.Update(BuildEditContext());

    // ----- 楕円パーティクル（リング周回軌道）の放出 -----
    // kOrbitSpeed = 1周（2π ラジアン）にかかるフレーム数から逆算した1フレームあたりの角速度
    static constexpr float kOrbitSpeed = GameConstants::kTwoPi / GameConstants::kOrbitPeriodSeconds;

    // 軌道角度を1フレーム分進める
    ringOrbitAngle_ += kOrbitSpeed * GameConstants::kFrameDeltaTime;

    // 360度（2π ラジアン）を超えたら0に戻す（無限に増え続けないようにするため）
    if (ringOrbitAngle_ > GameConstants::kTwoPi) {
        ringOrbitAngle_ -= GameConstants::kTwoPi;
    }

    // タイマーを1フレーム分進めて、放出間隔に達したらパーティクルを出す
    ellipseParticleTimer_ += GameConstants::kFrameDeltaTime;
    while (ellipseParticleTimer_ >= kEllipseEmitInterval) {
        ellipseParticleTimer_ -= kEllipseEmitInterval;

        // リングの中間半径の位置（円軌道上）を放出座標として計算する
        float r = (ringInnerRadius_ + ringOuterRadius_) * 0.5f;
        Vector3 emitPos = {
            ringPosition_.x + std::cos(ringOrbitAngle_) * r, // X: cos で横方向の位置
            ringPosition_.y,
            ringPosition_.z + std::sin(ringOrbitAngle_) * r  // Z: sin で奥行き方向の位置
        };

        // 接線方向（円の接線 = 進行方向）に速度ベクトルを計算する
        // sin/cos を使って軌道の接線（90度ずれた方向）を出す
        Vector3 vel = {
            -std::sin(ringOrbitAngle_) * GameConstants::kEllipseTangentSpeed,
             GameConstants::kEllipseYVelocity, // 少し上方向にも動かす
             std::cos(ringOrbitAngle_) * GameConstants::kEllipseTangentSpeed
        };

        // 水色（R=0.6, G=0.85, B=1.0, A=1.0）で楕円パーティクルを1粒放出する
        particleManager_->EmitEllipse(
            "ellipse",
            emitPos,
            vel,
            { 0.6f, 0.85f, 1.0f, 1.0f },
            GameConstants::kEllipseLifetime,
            GameConstants::kEllipseBaseScale,
            GameConstants::kEllipseScaleRandom
        );
    }

    // Ring・Cylinder は毎フレーム、カメラとの距離に応じた処理（ビルボードなど）を行う
    ring_->Update(camera_.get());
    cylinder_->Update(camera_.get());
}

// =====================================================
// 描画（毎フレーム呼ばれる）
// =====================================================

// シャドウマップパスを実行する（影用の深度テクスチャを生成する）。
// この後の通常描画パスでこの深度テクスチャを参照して影を描く。
void GamePlayScene::DrawShadowPass()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 光源視点で深度バッファに描画する（この結果がシャドウマップになる）
    shadowManager_->BeginShadowPass(commandList);
    modelCommon_->BeginShadowPass();
    // ※ ここでシャドウキャスター（影を落とすオブジェクト）の Draw を呼ぶ場所

    shadowManager_->EndShadowPass(commandList);

    // ----- レンダーターゲットをメイン描画先に切り替える -----
    // ポストエフェクトが有効なら専用 RTV（レンダーターゲットビュー）へ、
    // そうでなければ通常のバックバッファへ描画先を設定する
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    if (imageFilter_->IsEnabled()) {
        rtv = imageFilter_->GetSceneRTVHandle();
    } else if (grayscaleEffect_->IsEnabled()) {
        rtv = grayscaleEffect_->GetSceneRTVHandle();
    } else if (hsvFilter_->IsEnabled()) {
        rtv = hsvFilter_->GetSceneRTVHandle();
    } else {
        rtv = dxCommon_->GetCurrentBackBufferHandle();
    }

    // 深度ステンシルビュー（奥行き判定に使うバッファ）
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();

    // 描画先（RTV + DSV）と、描画範囲（ビューポート・シザー矩形）を GPU にセットする
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    D3D12_VIEWPORT vp = { 0, 0,
        static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight),
        0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);
}

void GamePlayScene::Draw()
{
    // ----- オフスクリーンパス（赤でクリアしてSRVに遷移）-----
    // 一旦テクスチャに描画してからスプライトとして貼ることで、ポストエフェクトをかけられる
    renderTexture_->BeginRendering();
    renderTexture_->EndRendering();

    // シャドウマップの生成 + レンダーターゲットの切り替え
    DrawShadowPass();

    // ----- 背景としてレンダーテクスチャを描画 -----
    // 2D描画モードに切り替えてから、オフスクリーンで作ったテクスチャを画面いっぱいに貼る
    spriteCommon_->CommonDrawSettings();
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), srvManager_);
    renderTextureSprite_->Update();
    renderTextureSprite_->Draw();

    // ----- 3Dオブジェクトを描画 -----
    modelCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList()); // ライト情報を GPU に送る
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), srvManager_); // シャドウマップをバインド

    for (auto& obj : gameObjects_) {
        obj->Draw();
    }

    // ----- スキンメッシュ描画 -----
    // PSO（パイプラインステートオブジェクト）をスキンメッシュ用に切り替えてから描画する
    // PSO 切り替え後はライト・シャドウの再バインドが必要
    skinCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList());
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), srvManager_);
    human_->Draw();

    // スケルトンのデバッグ表示（USE_IMGUI ビルド + フラグが ON の場合のみ）
#ifdef USE_IMGUI
    if (showSkeletonDebug_ && human_) {
        human_->DebugDraw(); // ボーンをワイヤーフレームで描画する
    }
#endif

    // PSO を通常の 3D 描画用に戻す
    modelCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList());
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), srvManager_);

    // パーティクル・Ring・Cylinder を描画する
    particleManager_->Update(camera_.get()); // GPU コンピュートで粒の位置を更新
    particleManager_->Draw(camera_.get());   // 更新後のパーティクルを描画
    ring_->Draw();
    cylinder_->Draw();

    // ----- 2D UI（ImGuiで追加したスプライト要素）-----
    // SceneEditor で追加したスプライト UI をすべて描画する
    spriteCommon_->CommonDrawSettings();
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), srvManager_);

    for (auto& e : sceneEditor_.GetUIElements()) {
        e.sprite->Update();
        e.sprite->Draw();
    }
}

// =====================================================
// 終了（シーン切り替えやゲーム終了時に1度だけ呼ばれる）
// =====================================================

void GamePlayScene::Finalize()
{
    // 音を全部止める（BGM・SE どちらも）
    if (audio_) {
        audio_->StopBGM();
        audio_->StopAllSE();
    }

    // パーティクルを全グループ削除する（メモリリーク防止）
    if (particleManager_) { particleManager_->ClearAllGroups(); }
}
