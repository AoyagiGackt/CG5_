/**
 * @file TitleScene.h
 * @brief ゲームのタイトル画面シーンを管理するファイル
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
 * @brief タイトル画面のシーンクラス
 * @note ゲーム起動時に最初に表示されるシーンです。
 * ユーザーの入力（スペースキーなど）を検知すると finished_ フラグを立て、
 * シーンマネージャーにシーン遷移を促します
 */
class TitleScene : public BaseScene {
public:
    /**
     * @brief シーンの初期化
     * @param dxCommon DirectX基盤のポインタ
     * @param input 入力管理のポインタ
     * @param audio 音響管理のポインタ
     * @note タイトルロゴのスプライト生成や、タイトルBGMの再生開始などを行います
     */
    void Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio) override;

    /**
     * @brief シーンの終了処理
     * @note 次のシーンへ移る際のリソース解放や、タイトルBGMの停止などを行います
     */
    void Finalize() override;

    /**
     * @brief シーンの更新処理
     * @note 入力待ちのロジックを記述します。特定のキーが押されたら finished_ を true にします
     */
    void Update() override;

    /**
     * @brief シーンの描画処理
     * @note タイトルロゴとかのスプライトの描画コマンドを積み込みます
     */
    void Draw() override;

    /**
     * @brief シーン終了フラグの取得
     * @return bool シーンが終了したかどうか（true: 終了して次へ / false: 継続）
     * @note 内部の finished_ フラグの状態を返します
     */
    bool IsFinished() const { return finished_; }

    /**
     * @brief デバッグ用UIマネージャーをセットする
     * @param imgui ImGuiManagerのポインタ
     */
    void SetImGuiManager(ImGuiManager* imgui) { imguiManager_ = imgui; }

private:
    
    // --- 外部から提供される基盤システム（借りてくるもの） ---

    /** @brief DirectX基盤のポインタ */
    DirectXCommon* dxCommon_ = nullptr;
    
    /** @brief 入力管理のポインタ */
    Input* input_ = nullptr;

    /** @brief 音響管理のポインタ */
    Audio* audio_ = nullptr;

    /** @brief デバッグUI用のImGuiマネージャー */
    ImGuiManager* imguiManager_ = nullptr;

    // --- このシーンが所有・管理するリソース ---

    /** @brief スプライト描画の共通設定 */
    std::unique_ptr<SpriteCommon> spriteCommon_;

    /** @brief 白背景スプライト */
    std::unique_ptr<Sprite> titleSprite_;

    /** @brief タイトル文字スプライト */
    std::unique_ptr<Sprite> titleTextSprite_;

    /** @brief シーン終了フラグ（trueになるとシーンが切り替わる） */
    bool finished_ = false;
};
