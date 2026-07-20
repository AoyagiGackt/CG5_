#pragma once
#include "MakeAffine.h"

// 球体
struct Sphere {
    Vector3 center; // 中心点
    float radius; // 半径
};

// AABB (回転しない立方体)
struct AABB {
    Vector3 min; // 最小座標 (左下奥)
    Vector3 max; // 最大座標 (右上手前)
};