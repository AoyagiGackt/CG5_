/**
 * @file BaseScene.h
 * @brief 全てのシーンクラスの親となる基底クラスを定義するファイル
 */
#pragma once
#include "Audio.h"
#include "DirectXCommon.h"
#include "Input.h"

/**
 * @brief シーンの種類を定義する列挙型
 * @note 新しいシーン（リザルトやステージ選択など）を追加する場合は、ここに定数を追記
 */
enum SceneType {
    kTitle, ///< タイトルシーン
    kGamePlay ///< ゲームプレイシーン
};

/**
 * @brief 全てのシーンの抽象基底クラス
 * @note 各具体的なシーン（TitleScene 等）はこのクラスを継承して実装
 * 共通のインターフェースを提供することで、SceneManager による一括管理を可能に
 */
class BaseScene {
public:

    /**
     * @brief 仮想デストラクタ
     */
    virtual ~BaseScene() = default;

    /**
     * @brief シーンの初期化
     * @param dxCommon DirectX基盤のポインタ
     * @param input 入力管理のポインタ
     * @param audio 音響管理のポインタ
     * @note シーン開始時に一度だけ呼ばれ、必要なリソースのロードなどを行います
     */
    virtual void Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio) = 0;

    /**
     * @brief シーンの更新処理
     * @note 毎フレーム呼び出され、ゲームロジック（移動、衝突判定等）の更新を行います
     */
    virtual void Update() = 0;

    /**
     * @brief シーンの描画処理
     * @note 毎フレーム呼び出され、描画コマンドの積み込みを行います
     */
    virtual void Draw() = 0;

    /**
     * @brief シーンの終了処理
     * @note シーン切り替え時に呼ばれ、リソースの解放や後片付けを行います
     */
    virtual void Finalize() = 0;

    /**
     * @brief シーン終了フラグの取得
     * @return bool シーンが終了したかどうか（true: 終了 / false: 継続）
     * @note この関数が true を返すと、SceneManager は次のシーンへの遷移処理を開始します
     */
    virtual bool IsFinished() const { return false; }
};