/**
 * @file TimeDisplay.cpp
 */
#include "TimeDisplay.h"
#include <string>

// =====================================================
// 初期化
// =====================================================

void TimeDisplay::Initialize(SpriteCommon* spriteCommon)
{
    spriteCommon_ = spriteCommon;

    // 時計枠
    clockFrame_ = std::make_unique<Sprite>();
    clockFrame_->Initialize(spriteCommon_, "Resources/Time/clockFrame.png");

    // コロン
    colon_ = std::make_unique<Sprite>();
    colon_->Initialize(spriteCommon_, "Resources/Time/middle.png");

    // 桁スプライト
    digitH1_ = std::make_unique<Sprite>();
    digitH1_->Initialize(spriteCommon_, "Resources/Time/1.png");

    digitH2_ = std::make_unique<Sprite>();
    digitH2_->Initialize(spriteCommon_, "Resources/Time/1.png");

    digitM1_ = std::make_unique<Sprite>();
    digitM1_->Initialize(spriteCommon_, "Resources/Time/1.png");

    digitM2_ = std::make_unique<Sprite>();
    digitM2_->Initialize(spriteCommon_, "Resources/Time/1.png");

    // フレームのサイズを設定
    clockFrame_->SetSize(kFrameSize);

    // 初回レイアウト
    UpdateLayout();
}

// =====================================================
// 更新
// =====================================================

void TimeDisplay::Update(int hour, int minute)
{
    // 各桁を分解
    int h1 = hour   / 10;
    int h2 = hour   % 10;
    int m1 = minute / 10;
    int m2 = minute % 10;

    // テクスチャを設定（0 は非表示）
    ApplyDigit(digitH1_.get(), h1, showH1_);
    ApplyDigit(digitH2_.get(), h2, showH2_);
    ApplyDigit(digitM1_.get(), m1, showM1_);
    ApplyDigit(digitM2_.get(), m2, showM2_);

    // スプライトの座標・サイズを更新
    UpdateLayout();
}

void TimeDisplay::Draw()
{
    // 枠を最初に描画
    clockFrame_->Draw();

    // コロン
    colon_->Draw();

    // 各桁（表示フラグが立っているもののみ）
    if (showH1_) { digitH1_->Draw(); }
    if (showH2_) { digitH2_->Draw(); }
    if (showM1_) { digitM1_->Draw(); }
    if (showM2_) { digitM2_->Draw(); }
}

void TimeDisplay::SetPosition(Vector2 pos)
{
    framePos_ = pos;
    UpdateLayout();
}

// =====================================================
// プライベート
// =====================================================

void TimeDisplay::ApplyDigit(Sprite* sprite, int digit, bool& visible)
{
    visible = true;
    sprite->SetTexture("Resources/Time/" + std::to_string(digit) + ".png");
}

void TimeDisplay::UpdateLayout()
{
    // ----- フレーム -----
    clockFrame_->SetPosition(framePos_);
    clockFrame_->SetSize(kFrameSize);
    clockFrame_->Update();

    // ----- コロン -----
    colon_->SetPosition({ framePos_.x + kColonX, framePos_.y + kDigitY });
    colon_->SetSize({ kColonW, kDigitH });
    colon_->Update();

    // ----- 桁スプライト -----
    auto setupDigit = [&](Sprite* sprite, float offsetX) {
        sprite->SetPosition({ framePos_.x + offsetX, framePos_.y + kDigitY });
        sprite->SetSize({ kDigitW, kDigitH });
        sprite->Update();
    };

    setupDigit(digitH1_.get(), kH1X);
    setupDigit(digitH2_.get(), kH2X);
    setupDigit(digitM1_.get(), kM1X);
    setupDigit(digitM2_.get(), kM2X);
}
