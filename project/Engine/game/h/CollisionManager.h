#pragma once
#include "GameObject.h"
#include "Collision.h"
#include <list>
#include <utility>

/**
 * @brief 当たり判定を一括管理するクラス
 */
class CollisionManager {
public:
    /**
     * @brief 判定したいペアを登録する
     * @param objA 物体A（プレイヤーなど）
     * @param objB 物体B（敵やブロックなど）
     */
    void AddCollisionPair(GameObject* objA, GameObject* objB)
    {
        checkPairs_.push_back(std::make_pair(objA, objB));
    }

    /**
     * @brief 毎フレーム実行する一括チェック
     * 登録された全てのペアに対して判定を行い、isHitフラグを更新する
     */
    void UpdateAllCollisions()
    {
        // 判定を始める前に、全てのオブジェクトのフラグを一旦リセットする
        for (auto& pair : checkPairs_) {
            pair.first->GetCollider().isHit = false;
            pair.second->GetCollider().isHit = false;
        }

        // 当たった時だけ true を書き込むようにする
        for (auto& pair : checkPairs_) {
            if (Collision::CheckCollision(pair.first->GetCollider().aabb,
                    pair.second->GetCollider().aabb)) {
                pair.first->GetCollider().isHit = true;
                pair.second->GetCollider().isHit = true;
            }
            // ここにあった else { ... = false; } を消すのがポイントです
        }
    }

    // 次のシーンに行く時などにリストを空にする
    void ClearPairs() { checkPairs_.clear(); }

private:
    // 判定待ちペアのリスト
    std::list<std::pair<GameObject*, GameObject*>> checkPairs_;
};