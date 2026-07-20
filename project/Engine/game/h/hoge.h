/**
 * @file Hoge.h
 * @brief GameObjectを継承した具体的なゲームオブジェクトのサンプル（テンプレート）クラス
 */
#pragma once

// エンジンの基本機能をインクルード
#include "Audio.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "GameObject.h"
#include "Animation.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3d.h"
#include <memory>

/**
 * @brief サンプルオブジェクトクラス
 * @note GameObject を継承しており、シーンに追加して Update/Draw を呼び出すことで動作します
 * 新しいキャラクターやオブジェクトを作成する際のベースとして利用してください
 */
class Hoge : public GameObject {
public:
    
    /**
     * @brief コンストラクタ
     */
    Hoge();
    
    /**
     * @brief デストラクタ
     */
    ~Hoge();

    /**
     * @brief 初期化処理
     * @param dxCommon DirectX基盤のポインタ
     * @param input 入力管理のポインタ
     * @param audio 音響管理のポインタ
     * @note メンバ変数の初期化や、使用するモデル・テクスチャのロードなどを行います
     */
    void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, Input* input, Audio* audio);

    /**
     * @brief 更新処理
     * @note 毎フレーム呼び出され、オブジェクト独自の移動ロジックなどを記述します
     */
    void Update() override;

    /**
     * @brief 描画処理
     * @note 3Dモデルやエフェクトの描画コマンドを発行します
     */
    void Draw() override;

    /**
     * @brief 終了処理
     * @note オブジェクト破棄時に必要な後片付けがあればここに記述します
     */
    void Finalize();

    void SetWorldOffset(const Vector3& offset) { worldOffset_ = offset; }
    void SetAnimSpeed(float speed)             { animSpeed_   = speed;  }
    Vector3 GetWorldOffset() const             { return worldOffset_;   }
    float   GetAnimSpeed()   const             { return animSpeed_;     }

private:
   
    // --- 外部から提供される基盤システム（借りてくるもの） ---

    /** @brief DirectX基盤のポインタ */
    DirectXCommon* dxCommon_ = nullptr;

    /** @brief 入力管理のポインタ */
    Input* input_ = nullptr;

    /** @brief 音響管理のポインタ */
    Audio* audio_ = nullptr;

    // --- メンバ変数 ---

    std::unique_ptr<Model>    model_;
    std::unique_ptr<Object3d> object_;
    Animation                 animation_;
    float                     animTime_    = 0.0f;
    float                     animSpeed_   = 1.0f;
    Vector3                   worldOffset_ = { 10.0f, 2.0f, 0.0f };
};