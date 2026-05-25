#include "LoadingScene.h"
#include "GameConstants.h"
#include "SceneManager.h"

void LoadingScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio) {
    dxCommon_ = dxCommon;

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // 黒背景（white.png を黒く塗る）
    bgSprite_ = std::make_unique<Sprite>();
    bgSprite_->Initialize(spriteCommon_.get(), "Resources/white.png");
    bgSprite_->SetPosition({ 0.0f, 0.0f });
    bgSprite_->SetSize({ 1280.0f, 720.0f });
    bgSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

    // 3 つのドット（画面中央下あたり）
    static constexpr float kDotSize = 20.0f;
    static constexpr float kSpacing = 35.0f;
    static constexpr float kCenterX = 640.0f - kDotSize / 2.0f;
    static constexpr float kCenterY = 400.0f - kDotSize / 2.0f;

    for (int i = 0; i < 3; ++i) {
        dotSprites_[i] = std::make_unique<Sprite>();
        dotSprites_[i]->Initialize(spriteCommon_.get(), "Resources/white.png");
        dotSprites_[i]->SetPosition({ kCenterX + (i - 1) * kSpacing, kCenterY });
        dotSprites_[i]->SetSize({ kDotSize, kDotSize });
        dotSprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.25f }); // 暗め
    }
    dotSprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 最初のドットだけ明るい

    timer_    = 0.0f;
    dotTimer_ = 0.0f;
    activeDot_ = 0;
    sceneChangeRequested_ = false;
}

void LoadingScene::Update() {
    constexpr float dt = GameConstants::kFrameDeltaTime;
    timer_    += dt;
    dotTimer_ += dt;

    // ドットアニメーション
    if (dotTimer_ >= kDotInterval) {
        dotTimer_ -= kDotInterval;
        dotSprites_[activeDot_]->SetColor({ 1.0f, 1.0f, 1.0f, 0.25f });
        activeDot_ = (activeDot_ + 1) % 3;
        dotSprites_[activeDot_]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }

    bgSprite_->Update();
    for (int i = 0; i < 3; ++i) { dotSprites_[i]->Update(); }

    // 最低表示時間が過ぎたら次のシーンへ（一度だけ）
    if (!sceneChangeRequested_ && timer_ >= kMinDisplayTime) {
        sceneChangeRequested_ = true;
        const std::string& target = SceneManager::GetInstance()->GetLoadingTarget();
        if (!target.empty()) {
            SceneManager::GetInstance()->ChangeScene(target);
        }
    }
}

void LoadingScene::Draw() {
    spriteCommon_->CommonDrawSettings();
    bgSprite_->Draw();
    for (int i = 0; i < 3; ++i) { dotSprites_[i]->Draw(); }
}

void LoadingScene::Finalize() {
}
