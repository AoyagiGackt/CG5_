/**
 * @file ClearScene.h
 * @brief ゲームクリア画面のシーンを管理するファイル
 */
#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ScoreDisplay.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>

/**
 * @brief クリア画面のシーンクラス
 * @note スペースキーを押すとタイトルへ戻る
 */
class ClearScene : public BaseScene {
public:
    void Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio) override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

    void SetImGuiManager(ImGuiManager* imgui) { imguiManager_ = imgui; }

    /** @brief このシーンのスコアウィンドウを ImGui で描画 */
    void DrawScoreUI();

private:
    DirectXCommon* dxCommon_ = nullptr;
    Input*         input_    = nullptr;
    Audio*         audio_    = nullptr;
    ImGuiManager*  imguiManager_ = nullptr;

    std::unique_ptr<SpriteCommon> spriteCommon_;
    std::unique_ptr<Sprite>       clearSprite_;

    /** @brief "SCORE" ラベル画像 */
    std::unique_ptr<Sprite> scoreLabel_;

    /** @brief "RANKING" ラベル画像 */
    std::unique_ptr<Sprite> rankingLabel_;

    ScoreDisplay scoreDisplay_;
};
