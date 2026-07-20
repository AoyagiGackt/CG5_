/**
 * @file PauseScene.h
 * @brief ポーズ画面のオーバーレイUIを管理するクラス
 * @note SceneManagerでは管理しない。GamePlaySceneが所有し、
 *       ポーズ中にUpdate/Drawを委譲する。
 *       背景のゲーム描画はGamePlaySceneが引き続き行う。
 */
#pragma once
#include "Input.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>

/**
 * @brief ポーズ画面クラス
 * @note GamePlaySceneから SpriteCommon* を受け取って初期化する。
 *       Update()の戻り値でゲームに操作を伝える。
 */
class PauseScene {
public:
    /** @brief Update()の戻り値：ゲームへの指示 */
    enum class Result {
        None,      ///< 何もしない（ポーズ継続）
        Continue,  ///< ポーズ解除してゲーム続行
        GoTitle,   ///< タイトルシーンへ遷移
    };

    /**
     * @brief 初期化
     * @param spriteCommon GamePlaySceneが持つSpriteCommonを借りて使う
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief ポーズ画面を開く（カーソルを先頭に戻す）
     */
    void Open();

    /**
     * @brief 毎フレーム更新
     * @return Result ゲームへの指示
     */
    Result Update(Input* input);

    /**
     * @brief 描画（GamePlaySceneのDraw内で呼ぶ）
     */
    void Draw();

private:
    /** @brief カーソル位置（0=つづける, 1=タイトルに戻る） */
    int cursor_ = 0;

    /** @brief 半透明の黒背景オーバーレイ */
    std::unique_ptr<Sprite> overlay_;

    /** @brief 選択肢1: つづける（↑） */
    std::unique_ptr<Sprite> option1_;

    /** @brief 選択肢2: タイトルに戻る（↓） */
    std::unique_ptr<Sprite> option2_;
};
