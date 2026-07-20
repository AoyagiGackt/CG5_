/**
 * @file GameObject.h
 * @brief ゲーム内に存在する全てのオブジェクトの基底クラスを定義するファイル
 */
#pragma once
#include "CollisionConfig.h"

/**
 * @brief 当たり判定の情報をまとめた構造体
 */
struct Collider {
    AABB aabb; ///< 判定用の箱
    bool isHit; ///< 今当たっているかどうかの結果
};

/**
 * @brief すべてのゲーム内オブジェクトの抽象基底クラス
 * @note プレイヤー、敵、弾など、更新処理と描画処理を必要とする全てのクラスの親
 * このクラスを継承することで、異なる種類のオブジェクトを同一のリストで管理
 */
class GameObject {
public:
    
    /**
     * @brief 仮想デストラクタ
     * @note 派生クラスのインスタンスが適切に破棄されることを保証するために仮想関数にしています
     */
    virtual ~GameObject() = default;

    /**
     * @brief オブジェクトの更新処理（純粋仮想関数）
     * @note 毎フレーム呼び出され、移動、アニメーション、衝突判定などのロジックを記述します
     * 派生クラスで必ず実装してください
     */
    virtual void Update() = 0;

    /**
     * @brief オブジェクトの描画処理（純粋仮想関数）
     * @note 毎フレーム呼び出され、モデルやスプライトの描画コマンドの発行を行います
     * 派生クラスで必ず実装してください
     */
    virtual void Draw() = 0;

    /* *
     * @brief 当たり判定情報を取得する
     * @return Collider& オブジェクトの当たり判定情報への参照
     * @note この関数を呼び出して、オブジェクト同士の衝突をチェックします
     */
    Collider& GetCollider() { return collider_; }

    protected:
    // 継承したクラス（Hogeなど）が自分で自分の形をセットできるようにする
    Collider collider_;
};