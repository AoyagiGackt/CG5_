#include "TitleScene.h"
#include "SceneManager.h"

void TitleScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    dxCommon_ = dxCommon;
    input_    = input;
    audio_    = audio;

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // 白背景（全画面）
    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize(spriteCommon_.get(), "Resources/white.png");
    titleSprite_->SetPosition({ 0.0f, 0.0f });
    titleSprite_->SetSize({ 1280.0f, 720.0f });

    // タイトル文字（白背景の上に重ねる）
    titleTextSprite_ = std::make_unique<Sprite>();
    titleTextSprite_->Initialize(spriteCommon_.get(), "Resources/title/title.png");
    titleTextSprite_->SetPosition({ 0.0f, 0.0f });
    titleTextSprite_->SetSize({ 1280.0f, 720.0f });

    finished_ = false;
}

void TitleScene::Update()
{
    if (input_->TriggerKey(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeSceneWithLoading("GAMEPLAY");
    }

    titleSprite_->Update();
    titleTextSprite_->Update();
}

void TitleScene::Draw()
{
    spriteCommon_->CommonDrawSettings();

    titleSprite_->Draw();      // 白背景
    titleTextSprite_->Draw();  // タイトル文字
}

void TitleScene::Finalize()
{
}
