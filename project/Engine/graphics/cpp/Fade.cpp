/**
 * @file Fade.cpp
 * @brief フェードイン・フェードアウト処理
 *
 * 画面全体を覆う黒いスプライトのアルファ値（透明度）を時間に応じて変化させる。
 * フェードイン: 黒い画面 → 透明（ゲーム画面が見えてくる）
 * フェードアウト: 透明 → 黒い画面（画面が暗くなる）
 */
#include "Fade.h"
#include "GameConstants.h"
#include "WinApp.h"

// フェード用スプライトを初期化する。
// spriteCommon: 2D描画の共通設定（シェーダーなど）
void Fade::Initialize(SpriteCommon* spriteCommon)
{
    // 画面全体を覆う黒いスプライトを作成する
    // テクスチャは白い画像を使い、スプライトの色設定で黒にする
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(spriteCommon, "Resources/uvChecker.png");

    sprite_->SetPosition({ 0.0f, 0.0f }); // 左上を起点にして
    sprite_->SetSize({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) }); // 画面いっぱいに広げる
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f }); // 最初は完全に透明な黒
}

// フェードを開始する。
// status: フェードの種類（FadeIn = 明るくなる、FadeOut = 暗くなる）
// duration: フェードにかかる時間（秒）
void Fade::Start(Status status, float duration)
{
    status_   = status;
    duration_ = duration;
    counter_  = 0.0f; // タイマーをリセット
}

// 毎フレーム呼ぶ。フェードの進行度に応じてアルファ値を更新する。
void Fade::Update()
{
    // フェード中でなければ何もしない
    if (status_ == Status::None) {
        return;
    }

    // タイマーを1フレーム分（約0.0167秒）進める
    counter_ += GameConstants::kFrameDeltaTime;

    // 進捗率（0.0f = 開始直後 、1.0f = 完了）を計算する
    float progress = counter_ / duration_;
    if (progress > 1.0f) {
        progress = 1.0f; // 1.0 を超えないようにクランプ
    }

    float alpha = 0.0f;
    if (status_ == Status::FadeIn) {
        // フェードイン: アルファが 1.0（黒）→ 0.0（透明）に変化する
        alpha = 1.0f - progress;
    } else if (status_ == Status::FadeOut) {
        // フェードアウト: アルファが 0.0（透明）→ 1.0（黒）に変化する
        alpha = progress;
    }

    // 計算したアルファ値をスプライトの色に反映する（RGB は黒のまま）
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });
    sprite_->Update();

    // 進捗が完了したらフェードを終了状態にする（色反映の後に判定することが重要）
    if (progress >= 1.0f) {
        status_ = Status::None;
    }
}

// フェードスプライトを描画する。
void Fade::Draw()
{
    // アルファが 0.0（完全に透明）なら描画をスキップして処理負荷を下げる
    if (sprite_->GetColor().w <= 0.0f) {
        return;
    }

    // アルファブレンドを使って黒いスプライトを画面に重ねる
    sprite_->Draw();
}
