/**
 * @file MyGame.h
 * @brief Frameworkを継承し、ゲーム固有の進行（初期化・更新・描画）を記述するファイル
 */
#pragma once
#include "Framework.h"
#include "BaseScene.h"
#include "SceneFactory.h"
#include <memory>

/**
 * @brief ゲーム全体の具体的な進行を管理するクラス
 * @note Framework クラスを継承し、ゲーム固有のライフサイクルを実装します
 * シーンファクトリを用いてシーンを生成し、シーン遷移の制御を行います
 */
class MyGame : public Framework {
public:

    /**
     * @brief ゲーム固有の初期化処理
     * @note 基盤システムの初期化後、シーンファクトリの生成や
     * 最初のシーン（タイトルシーン等）のセットアップを行います
     */
    void Initialize() override;

    /**
     * @brief ゲーム固有の終了処理
     * @note 実行中のシーンの解放や、ゲーム全体で使用したリソースの破棄を行います
     */
    void Finalize() override;

    /**
     * @brief ゲーム固有の更新処理
     * @note 現在のシーンの更新処理を呼び出します。
     * また、シーンの終了フラグを監視し、シーンの切り替え（遷移）を制御します
     */
    void Update() override;

    /**
     * @brief ゲーム固有の描画処理
     * @note 現在のシーンの描画関数を呼び出し、モデルやスプライトなどの描画コマンドを積み込みます
     */
    void Draw() override;

private:

    /** @brief 現在アクティブなシーンのポインタ */
    BaseScene* scene_ = nullptr;

    /** @brief シーン生成を担う抽象ファクトリのスマートポインタ */
    std::unique_ptr<AbstractSceneFactory> sceneFactory_;
};