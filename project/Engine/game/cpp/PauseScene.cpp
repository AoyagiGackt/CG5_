#include "PauseScene.h"

// =====================================================
// 初期化
// =====================================================

void PauseScene::Initialize(SpriteCommon* spriteCommon)
{
    // 半透明の黒背景
    overlay_ = std::make_unique<Sprite>();
    overlay_->Initialize(spriteCommon, "Resources/white.png");
    overlay_->SetPosition({ 0.0f, 0.0f });
    overlay_->SetSize({ 1280.0f, 720.0f });
    overlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.6f });

    // 選択肢1: つづける
    option1_ = std::make_unique<Sprite>();
    option1_->Initialize(spriteCommon, "Resources/white.png");
    option1_->SetPosition({ 440.0f, 310.0f });
    option1_->SetSize({ 400.0f, 60.0f });

    // 選択肢2: タイトルに戻る
    option2_ = std::make_unique<Sprite>();
    option2_->Initialize(spriteCommon, "Resources/white.png");
    option2_->SetPosition({ 440.0f, 390.0f });
    option2_->SetSize({ 400.0f, 60.0f });
}

// =====================================================
// ポーズ画面を開く
// =====================================================

void PauseScene::Open()
{
    cursor_ = 0;
}

// =====================================================
// 更新
// =====================================================

PauseScene::Result PauseScene::Update(Input* input)
{
    // カーソル移動
    if (input->TriggerKey(DIK_W)) {
        cursor_ = 0;
    }
    if (input->TriggerKey(DIK_S)) {
        cursor_ = 1;
    }

    // 決定
    if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
        if (cursor_ == 0) {
            return Result::Continue;
        } else {
            return Result::GoTitle;
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

    return Result::None;
}

// =====================================================
// 描画
// =====================================================

void PauseScene::Draw()
{
    overlay_->Draw();
    option1_->Draw();
    option2_->Draw();
}
