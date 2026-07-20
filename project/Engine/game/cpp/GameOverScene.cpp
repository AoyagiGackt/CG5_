#include "GameOverScene.h"
#include "SceneManager.h"

// =====================================================
// 初期化
// =====================================================

void GameOverScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    dxCommon_ = dxCommon;
    input_    = input;
    audio_    = audio;

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // 半透明の黒背景
    overlay_ = std::make_unique<Sprite>();
    overlay_->Initialize(spriteCommon_.get(), "Resources/white.png");
    overlay_->SetPosition({ 0.0f, 0.0f });
    overlay_->SetSize({ 1280.0f, 720.0f });
    overlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.85f });

    // 選択肢1: リスタート
    option1_ = std::make_unique<Sprite>();
    option1_->Initialize(spriteCommon_.get(), "Resources/white.png");
    option1_->SetPosition({ 440.0f, 310.0f });
    option1_->SetSize({ 400.0f, 60.0f });

    // 選択肢2: タイトルに戻る
    option2_ = std::make_unique<Sprite>();
    option2_->Initialize(spriteCommon_.get(), "Resources/white.png");
    option2_->SetPosition({ 440.0f, 390.0f });
    option2_->SetSize({ 400.0f, 60.0f });

    cursor_ = 0;
}

// =====================================================
// 終了
// =====================================================

void GameOverScene::Finalize()
{
}

// =====================================================
// 更新
// =====================================================

void GameOverScene::Update()
{
    // カーソル移動
    if (input_->TriggerKey(DIK_W)) {
        cursor_ = 0;
    }
    if (input_->TriggerKey(DIK_S)) {
        cursor_ = 1;
    }

    // 決定
    if (input_->TriggerKey(DIK_SPACE) || input_->TriggerKey(DIK_RETURN)) {
        if (cursor_ == 0) {
            SceneManager::GetInstance()->ChangeScene("GAMEPLAY"); // リスタート
        } else {
            SceneManager::GetInstance()->ChangeScene("TITLE");    // タイトルへ
        }
    }

    // 選択中=緑、非選択=グレー
    option1_->SetColor(cursor_ == 0
        ? Vector4{ 0.2f, 0.8f, 0.2f, 0.9f }
        : Vector4{ 0.4f, 0.4f, 0.4f, 0.7f });
    option2_->SetColor(cursor_ == 1
        ? Vector4{ 0.2f, 0.8f, 0.2f, 0.9f }
        : Vector4{ 0.4f, 0.4f, 0.4f, 0.7f });

    overlay_->Update();
    option1_->Update();
    option2_->Update();
}

// =====================================================
// 描画
// =====================================================

void GameOverScene::Draw()
{
    spriteCommon_->CommonDrawSettings();
    overlay_->Draw();
    option1_->Draw();
    option2_->Draw();
}
