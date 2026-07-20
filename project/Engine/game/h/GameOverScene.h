/**
 * @file GameOverScene.h
 * @brief ゲームオーバー画面のシーンを管理するファイル
 */
#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>

/**
 * @brief ゲームオーバー画面のシーンクラス
 * @note ↑キー=リスタート(GAMEPLAY)、↓キー=タイトルに戻る(TITLE)
 *       Space/Enterキーで選択を確定する
 */
class GameOverScene : public BaseScene {
public:
    void Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio) override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

    void SetImGuiManager(ImGuiManager* imgui) { imguiManager_ = imgui; }

private:
    DirectXCommon* dxCommon_     = nullptr;
    Input*         input_        = nullptr;
    Audio*         audio_        = nullptr;
    ImGuiManager*  imguiManager_ = nullptr;

    std::unique_ptr<SpriteCommon> spriteCommon_;

    /** @brief 半透明の黒背景 */
    std::unique_ptr<Sprite> overlay_;

    /** @brief 選択肢1: リスタート（↑） */
    std::unique_ptr<Sprite> option1_;

    /** @brief 選択肢2: タイトルに戻る（↓） */
    std::unique_ptr<Sprite> option2_;

    /** @brief カーソル位置（0=リスタート, 1=タイトル） */
    int cursor_ = 0;
};
