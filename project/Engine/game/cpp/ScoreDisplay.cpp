/**
 * @file ScoreDisplay.cpp
 */
#include "ScoreDisplay.h"
#include "TextureManager.h"
#include <cassert>
#include <string>

// 数字画像のパスを返す
static std::string DigitPath(int digit)
{
    return "Resources/Time/" + std::to_string(digit) + ".png";
}

// =====================================================
// 初期化
// =====================================================

void ScoreDisplay::Initialize(SpriteCommon* spriteCommon)
{
    // 全桁テクスチャ (0〜9) をキャッシュに登録しておく
    for (int i = 0; i <= 9; ++i) {
        TextureManager::GetInstance()->LoadTexture(DigitPath(i));
    }

    // スプライトを全て初期化
    for (auto& sp : pool_) {
        sp = std::make_unique<Sprite>();
        sp->Initialize(spriteCommon, DigitPath(0));
    }
}

// =====================================================
// リセット（毎フレーム Draw 前に呼ぶ）
// =====================================================

void ScoreDisplay::Reset()
{
    poolUsed_ = 0;
}

// =====================================================
// 描画
// =====================================================

void ScoreDisplay::DrawNumber(int value, Vector2 pos, Vector2 digitSize, float gap, Vector4 color)
{
    std::string s = std::to_string(value < 0 ? 0 : value);
    float x = pos.x;
    for (char c : s) {
        Sprite* sp = AllocSprite();
        if (!sp) { return; }

        int d = c - '0';
        sp->SetTexture(DigitPath(d));
        sp->SetPosition({ x, pos.y });
        sp->SetSize(digitSize);
        sp->SetColor(color);
        sp->Update();
        sp->Draw();

        x += digitSize.x + gap;
    }
}

void ScoreDisplay::DrawRanking(const std::vector<int>& ranking,
                               int currentScore,
                               Vector2 topLeft,
                               Vector2 digitSize,
                               float rowSpacing)
{
    // 順位番号列の幅（最大2桁 + gap）
    const float rankColW  = digitSize.x * 2.f + 8.f;
    // スコア列の開始 X オフセット
    const float scoreOffX = rankColW + 16.f;

    for (int i = 0; i < (int)ranking.size(); ++i) {
        float y = topLeft.y + i * rowSpacing;

        // ---- 順位番号 ----
        DrawNumber(i + 1, { topLeft.x, y }, digitSize, 2.f);

        // ---- スコア ----
        bool isCurrentScore = (ranking[i] == currentScore);

        // ハイライト用にスコア描画前のプール位置を記録
        int scoreStartIdx = poolUsed_;
        DrawNumber(ranking[i], { topLeft.x + scoreOffX, y }, digitSize, 2.f);

        // 現在スコアと一致するエントリを黄色でハイライト
        if (isCurrentScore) {
            for (int k = scoreStartIdx; k < poolUsed_; ++k) {
                pool_[k]->SetColor({ 1.f, 0.9f, 0.2f, 1.f });
            }
        }
    }
}

// =====================================================
// プライベート
// =====================================================

Sprite* ScoreDisplay::AllocSprite()
{
    if (poolUsed_ >= kPoolSize) {
        return nullptr;
    }
    pool_[poolUsed_]->SetColor({ 1.f, 1.f, 1.f, 1.f });
    return pool_[poolUsed_++].get();
}
