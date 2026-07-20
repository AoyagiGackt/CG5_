/**
 * @file ScoreDisplay.h
 */
#pragma once
#include "Sprite.h"
#include "SpriteCommon.h"
#include <array>
#include <memory>
#include <vector>

class ScoreDisplay {
public:
    /** @brief プールサイズ（最大同時描画桁数） */
    static constexpr int kPoolSize = 128;

    /**
     * @brief 初期化。プールスプライトと全桁テクスチャを生成する
     * @param spriteCommon スプライト描画の共通設定
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief フレームごとにプールを先頭へリセットする
     * @note Draw 系メソッドを呼ぶ前に必ず一度呼ぶこと
     */
    void Reset();

    /**
     * @brief 整数値をスプライトで描画する
     * @param value     表示する値（0以上）
     * @param pos       数値左上座標
     * @param digitSize 1桁あたりのサイズ (幅, 高さ)
     * @param gap       桁間の隙間 (px)
     * @note spriteCommon->CommonDrawSettings() 後に呼ぶこと
     */
    void DrawNumber(int value, Vector2 pos,
                    Vector2 digitSize = { 40.f, 60.f },
                    float gap = 4.f,
                    Vector4 color = { 1.f, 1.f, 1.f, 1.f });

    /**
     * @brief ランキング一覧をスプライトで描画する
     * @param ranking      ランキングデータ（降順）
     * @param currentScore 現在スコア（ハイライト用。一致するエントリを明るく表示）
     * @param topLeft      リスト左上座標
     * @param digitSize    1桁あたりのサイズ
     * @param rowSpacing   行の高さ（次の行までの距離）
     */
    void DrawRanking(const std::vector<int>& ranking,
                     int currentScore,
                     Vector2 topLeft,
                     Vector2 digitSize  = { 32.f, 48.f },
                     float  rowSpacing  = 58.f);

private:
    /**
     * @brief プールから次のスプライトを取り出す
     * @return Sprite* 使用可能なスプライトへのポインタ
     */
    Sprite* AllocSprite();

    /** @brief スプライトプール */
    std::array<std::unique_ptr<Sprite>, kPoolSize> pool_;

    /** @brief 現フレームで使用した数 */
    int poolUsed_ = 0;
};
