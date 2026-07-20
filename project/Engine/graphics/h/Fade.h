#pragma once
#include "Sprite.h"
#include <memory>

/**
 * @brief 画面のフェードイン・フェードアウトを管理するクラス
 */
class Fade {
public:
    enum class Status {
        None, ///< 待機中
        FadeIn, ///< 暗い状態から明るくなる
        FadeOut, ///< 明るい状態から暗くなる
    };

    /**
     * @brief 初期化
     * @param spriteCommon 2D描画共通設定
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画処理
     */
    void Draw();

    /**
     * @brief フェード開始
     * @param status フェードの種類（In または Out）
     * @param duration かける時間（秒）
     */
    void Start(Status status, float duration);

    /**
     * @brief フェードが完了したか確認
     */
    bool IsFinished() const { return status_ == Status::None; }

    Status GetStatus() const{ return status_; }

private:
    std::unique_ptr<Sprite> sprite_; /// 画面を覆う板
    Status status_ = Status::None; /// 現在の状態
    float duration_ = 0.0f; /// 目標時間
    float counter_ = 0.0f; /// 経過時間タイマー
};