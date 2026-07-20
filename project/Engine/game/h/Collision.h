#pragma once
#include "CollisionConfig.h"

class Collision {
public:
    // 円と円の判定
    static bool CheckCollision(const Sphere& s1, const Sphere& s2);

    // AABBとAABBの判定
    static bool CheckCollision(const AABB& a, const AABB& b);
};