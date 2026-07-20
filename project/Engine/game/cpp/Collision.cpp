#include "Collision.h"
#include <algorithm>
#include <cmath>

// --- 球と球の判定 ---
bool Collision::CheckCollision(const Sphere& s1, const Sphere& s2)
{
    // 中心点同士の距離の2乗を計算
    float distSq = std::pow(s2.center.x - s1.center.x, 2.0f) + std::pow(s2.center.y - s1.center.y, 2.0f) + std::pow(s2.center.z - s1.center.z, 2.0f);

    // 半径の合計の2乗と比較
    float radiusSum = s1.radius + s2.radius;
    return distSq <= (radiusSum * radiusSum);
}

// --- AABBとAABBの判定 ---
bool Collision::CheckCollision(const AABB& a, const AABB& b)
{
    // X, Y, Z 全ての軸で重なっているかチェックする
    // 一箇所でも離れていれば当たっていない
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}