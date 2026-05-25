/**
 * @file MakeAffine.h
 * @brief 3Dゲームエンジンに必要な数学演算（ベクトル、行列、座標変換）を定義するファイル
 */
#pragma once
#include <cmath>

 // =================================================================
 // 構造体定義
 // =================================================================

 /** @brief 4x4 行列構造体 */
struct Matrix4x4
{
    float m[4][4];
};

/** @brief 3x3 行列構造体 */
struct Matrix3x3
{
    float m[3][3];
};

/** @brief 2次元ベクトル */
struct Vector2
{
    float x;
    float y;
};

/** @brief 3次元ベクトル */
struct Vector3
{
    float x;
    float y;
    float z;
};

/** @brief 4次元ベクトル */
struct Vector4
{
    float x;
    float y;
    float z;
    float w;
};

/** @brief クォータニオン（四元数）。回転をあらわす */
struct Quaternion
{
    float x;
    float y;
    float z;
    float w;
};

/** @brief オブジェクトのトランスフォーム情報をまとめた構造体 */
struct Transform
{
    Vector3 scale; ///< 拡大縮小
    Vector3 rotate; ///< 回転（ラジアン）
    Vector3 translate; ///< 座標
};

// =================================================================
// 行列演算関数
// =================================================================

/**
 * @brief 4x4 単位行列を作成する
 * @return Matrix4x4 単位行列
 */
inline Matrix4x4 MakeIdentity4x4()
{
    Matrix4x4 identity;
    identity.m[0][0] = 1.0f;
    identity.m[0][1] = 0.0f;
    identity.m[0][2] = 0.0f;
    identity.m[0][3] = 0.0f;
    identity.m[1][0] = 0.0f;
    identity.m[1][1] = 1.0f;
    identity.m[1][2] = 0.0f;
    identity.m[1][3] = 0.0f;
    identity.m[2][0] = 0.0f;
    identity.m[2][1] = 0.0f;
    identity.m[2][2] = 1.0f;
    identity.m[2][3] = 0.0f;
    identity.m[3][0] = 0.0f;
    identity.m[3][1] = 0.0f;
    identity.m[3][2] = 0.0f;
    identity.m[3][3] = 1.0f;
    return identity;
}

/**
 * @brief 4x4 行列同士の掛け算を行う
 * @param m1 左側の行列
 * @param m2 右側の行列
 * @return Matrix4x4 計算結果
 */
inline Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2)
{
    Matrix4x4 result;
    result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
    result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
    result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
    result.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];

    result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
    result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
    result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
    result.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];

    result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
    result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
    result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
    result.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];

    result.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
    result.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
    result.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
    result.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];

    return result;
}

// =================================================================
// 座標変換行列作成
// =================================================================

/** @brief X軸回転行列の作成 */
inline Matrix4x4 MakeRotateXMatrix(float radian)
{
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return { 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cosTheta, sinTheta, 0.0f,
        0.0f, -sinTheta, cosTheta, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };
}

/** @brief Y軸回転行列の作成 */
inline Matrix4x4 MakeRotateYMatrix(float radian)
{
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return { cosTheta, 0.0f, -sinTheta, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        sinTheta, 0.0f, cosTheta, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };
}

/** @brief Z軸回転行列の作成 */
inline Matrix4x4 MakeRotateZMatrix(float radian)
{
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return { cosTheta, sinTheta, 0.0f, 0.0f,
        -sinTheta, cosTheta, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };
}

/**
 * @brief スケール、回転、平行移動を合成したアフィン変換行列を作成する
 */
inline Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate)
{
    Matrix4x4 result = Multiply(Multiply(MakeRotateXMatrix(rotate.x), MakeRotateYMatrix(rotate.y)), MakeRotateZMatrix(rotate.z));
    result.m[0][0] *= scale.x;
    result.m[0][1] *= scale.x;
    result.m[0][2] *= scale.x;

    result.m[1][0] *= scale.y;
    result.m[1][1] *= scale.y;
    result.m[1][2] *= scale.y;

    result.m[2][0] *= scale.z;
    result.m[2][1] *= scale.z;
    result.m[2][2] *= scale.z;

    result.m[3][0] = translate.x;
    result.m[3][1] = translate.y;
    result.m[3][2] = translate.z;
    return result;
}

// =================================================================
// 投影行列作成
// =================================================================

/**
 * @brief 透視投影行列（パース用）の作成
 * @param fovY 垂直画角（ラジアン）
 * @param aspectRatio 画面比率（幅/高）
 * @param nearClip 近クリップ面
 * @param farClip 遠クリップ面
 */
inline Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
    float cotHalfFovV = 1.0f / std::tan(fovY / 2.0f);
    return {
        (cotHalfFovV / aspectRatio), 0.0f, 0.0f, 0.0f,
        0.0f, cotHalfFovV, 0.0f, 0.0f,
        0.0f, 0.0f, farClip / (farClip - nearClip), 1.0f,
        0.0f, 0.0f, -(nearClip * farClip) / (farClip - nearClip), 0.0f
    };
}

/**
 * @brief 正射影行列（2DスプライトやUI用）の作成
 */
inline Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
    return {
        2.0f / (right - left),
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        2.0f / (top - bottom),
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f / (farClip - nearClip),
        0.0f,
        (left + right) / (left - right),
        (top + bottom) / (bottom - top),
        nearClip / (nearClip - farClip),
        1.0f,
    };
}

/**
 * @brief スケール（拡大縮小）行列を作成する
 * @param scale 各軸の拡大倍率
 * @return Matrix4x4 スケール行列
 */
inline Matrix4x4 MakeScaleMatrix(const Vector3 &scale)
{
    Matrix4x4 result = MakeIdentity4x4();
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return result;
}

/**
 * @brief 平行移動行列を作成する
 * @param translate 各軸の移動量
 * @return Matrix4x4 平行移動行列
 */
inline Matrix4x4 MakeTranslateMatrix(const Vector3 &translate)
{
    Matrix4x4 result = MakeIdentity4x4();
    result.m[3][0] = translate.x;
    result.m[3][1] = translate.y;
    result.m[3][2] = translate.z;
    return result;
}

// =================================================================
// 特殊演算
// =================================================================

/**
 * @brief 4x4 行列の逆行列を求める
 * @note カメラのビュー行列作成などで使用します
 */
inline Matrix4x4 Inverse(const Matrix4x4 &m)
{
    float determinant = +m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3]
        + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1]
        + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2]

        - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1]
        - m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3]
        - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2]

        - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3]
        - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1]
        - m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2]

        + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1]
        + m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3]
        + m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2]

        + m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3]
        + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1]
        + m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2]

        - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1]
        - m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3]
        - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2]

        - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0]
        - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0]
        - m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0]

        + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0]
        + m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0]
        + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];

    Matrix4x4 result;
    float recpDeterminant = 1.0f / determinant;
    result.m[0][0] = (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] - m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2]) * recpDeterminant;
    result.m[0][1] = (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]) * recpDeterminant;
    result.m[0][2] = (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] - m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2]) * recpDeterminant;
    result.m[0][3] = (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] + m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2]) * recpDeterminant;

    result.m[1][0] = (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] + m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2]) * recpDeterminant;
    result.m[1][1] = (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2]) * recpDeterminant;
    result.m[1][2] = (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] + m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2]) * recpDeterminant;
    result.m[1][3] = (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] - m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2]) * recpDeterminant;

    result.m[2][0] = (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1]) * recpDeterminant;
    result.m[2][1] = (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] + m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1]) * recpDeterminant;
    result.m[2][2] = (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1]) * recpDeterminant;
    result.m[2][3] = (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] + m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1]) * recpDeterminant;

    result.m[3][0] = (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1]) * recpDeterminant;
    result.m[3][1] = (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1]) * recpDeterminant;
    result.m[3][2] = (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] + m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1]) * recpDeterminant;
    result.m[3][3] = (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1]) * recpDeterminant;

    return result;
}

inline Matrix4x4 Transpose(const Matrix4x4& m)
{
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.m[i][j] = m.m[j][i];
    return result;
}

/**
 * @brief 3次元ベクトルの長さを求める
 */
inline float Length(const Vector3 &v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/**
 * @brief 2点間の距離を求める
 */
inline float Distance(const Vector3 &p1, const Vector3 &p2)
{
    Vector3 diff = { p1.x - p2.x, p1.y - p2.y, p1.z - p2.z };
    return Length(diff);
}

/**
 * @brief 指定した値をminとmaxの範囲に収める
 */
inline float Clamp(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

/**
 * @brief 球体とAABB（箱）の当たり判定
 * @param spherePos 球体の中心座標
 * @param radius 球体の半径
 * @param aabbMin AABBの最小座標
 * @param aabbMax AABBの最大座標
 * @return bool 接触していれば true
 */
inline bool IsCollisionSphereAABB(const Vector3 &spherePos, float radius, const Vector3 &aabbMin, const Vector3 &aabbMax)
{
    // AABB上で、球の中心に最も近い点を探す
    Vector3 closestPoint = {
        Clamp(spherePos.x, aabbMin.x, aabbMax.x),
        Clamp(spherePos.y, aabbMin.y, aabbMax.y),
        Clamp(spherePos.z, aabbMin.z, aabbMax.z)
    };

    // その最も近い点と、球の中心との距離を測る
    float dist = Distance(spherePos, closestPoint);

    // 距離が半径以下なら当たっている
    return dist <= radius;
}

/**
 * @brief AABB（軸並行境界箱）同士の当たり判定
 * @param min1 物体1の最小座標（左下奥）
 * @param max1 物体1の最大座標（右上おもて）
 * @param min2 物体2の最小座標
 * @param max2 物体2の最大座標
 * @return bool 接触していれば true
 */
inline bool IsCollisionAABB(const Vector3 &min1, const Vector3 &max1, const Vector3 &min2, const Vector3 &max2)
{
    // 1つでも重なっていない軸があれば当たっていない
    if (max1.x < min2.x || min1.x > max2.x) {
        return false;
    }
    if (max1.y < min2.y || min1.y > max2.y) {
        return false;
    }
    if (max1.z < min2.z || min1.z > max2.z) {
        return false;
    }

    // 全ての軸で重なっていれば当たっている
    return true;
}

/**
 * @brief ベクトルの正規化（長さを1にする）
 * @note 移動ベクトルを一定の速度にする際などに使用します
 */
inline Vector3 Normalize(const Vector3 &v)
{
    float len = Length(v);
    if (len != 0.0f) {
        return { v.x / len, v.y / len, v.z / len };
    }
    return v;
}

/**
 * @brief ベクトルの内積 (Dot Product)
 * @note 2つのベクトルがどれくらい同じ方向を向いているかを判定します
 */
inline float Dot(const Vector3 &v1, const Vector3 &v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/**
 * @brief ベクトルの外積 (Cross Product)
 * @note 2つのベクトルに垂直なベクトルを求めます（面の法線計算などに使用）
 */
inline Vector3 Cross(const Vector3 &v1, const Vector3 &v2)
{
    return {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
}

/**
 * @brief レイ（半直線）と球体の当たり判定
 * @param rayOrigin レイの発射地点
 * @param rayDirection レイの飛んでいく方向（必ずNormalizeしてないとバグ）
 * @param spherePos 球の中心座標
 * @param radius 球の半径
 * @return bool レイが球に当たっていれば true
 */
inline bool IsCollisionRaySphere(const Vector3 &rayOrigin, const Vector3 &rayDirection, const Vector3 &spherePos, float radius)
{
    // レイの始点から球の中心へのベクトル
    Vector3 m = { spherePos.x - rayOrigin.x, spherePos.y - rayOrigin.y, spherePos.z - rayOrigin.z };

    // レイの方向ベクトルへの射影（内積）
    float b = Dot(m, rayDirection);

    // 球の中心からレイの始点までの距離の2乗
    float c = Dot(m, m) - radius * radius;

    // 始点が球の外側にあり、かつレイが球と反対方向を向いている場合は当たらない
    if (c > 0.0f && b <= 0.0f) {
        return false;
    }

    // 判別式
    float discr = b * b - c;

    // 判別式が負ならレイは球をかすりもしない
    if (discr < 0.0f) {
        return false;
    }

    return true;
}

/** @brief ベクトルの引き算 (v1 - v2) */
inline Vector3 Subtract(const Vector3 &v1, const Vector3 &v2)
{
    return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

/**
 * @brief ベクトルの線形補間
 */
inline Vector3 Lerp(const Vector3 &start, const Vector3 &end, float t)
{
    return { start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t,
        start.z + (end.z - start.z) * t };
}

/**
 * @brief クォータニオンの球面線形補間 (Slerp)
 * @param q1 開始クォータニオン
 * @param q2 終了クォータニオン
 * @param t  補間係数 (0.0f ~ 1.0f)
 */
inline Quaternion Slerp(const Quaternion &q1, Quaternion q2, float t)
{
    float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

    // 内積が負なら片方を反転して最短経路を取る
    if (dot < 0.0f) {
        q2 = { -q2.x, -q2.y, -q2.z, -q2.w };
        dot = -dot;
    }

    // 角度がほぼゼロなら線形補間で代用
    if (dot > 0.9995f) {
        return {
            q1.x + t * (q2.x - q1.x),
            q1.y + t * (q2.y - q1.y),
            q1.z + t * (q2.z - q1.z),
            q1.w + t * (q2.w - q1.w)
        };
    }

    float theta0 = std::acos(dot);
    float theta = theta0 * t;
    float sinTheta = std::sin(theta);
    float sinTheta0 = std::sin(theta0);

    float s1 = std::cos(theta) - dot * sinTheta / sinTheta0;
    float s2 = sinTheta / sinTheta0;

    return {
        s1 * q1.x + s2 * q2.x,
        s1 * q1.y + s2 * q2.y,
        s1 * q1.z + s2 * q2.z,
        s1 * q1.w + s2 * q2.w
    };
}

/**
 * @brief クォタニオンから回転行列を作成する
 */
inline Matrix4x4 MakeRotateMatrix(const Quaternion &q)
{
    Matrix4x4 m = MakeIdentity4x4();
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    m.m[0][0] = 1.0f - 2.0f * (yy + zz);
    m.m[0][1] = 2.0f * (xy + wz);
    m.m[0][2] = 2.0f * (xz - wy);

    m.m[1][0] = 2.0f * (xy - wz);
    m.m[1][1] = 1.0f - 2.0f * (xx + zz);
    m.m[1][2] = 2.0f * (yz + wx);

    m.m[2][0] = 2.0f * (xz + wy);
    m.m[2][1] = 2.0f * (yz - wx);
    m.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return m;
}

/**
 * @brief スケール・クォータニオン回転・平行移動を合成したアフィン行列を作成する
 * @note アニメーション再生時に使用。MakeAffineMatrix(Vector3 rotate版)とは別関数
 */
inline Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Quaternion &rotate, const Vector3 &translate)
{
    Matrix4x4 rot = MakeRotateMatrix(rotate);

    rot.m[0][0] *= scale.x;
    rot.m[0][1] *= scale.x;
    rot.m[0][2] *= scale.x;
    rot.m[1][0] *= scale.y;
    rot.m[1][1] *= scale.y;
    rot.m[1][2] *= scale.y;
    rot.m[2][0] *= scale.z;
    rot.m[2][1] *= scale.z;
    rot.m[2][2] *= scale.z;

    rot.m[3][0] = translate.x;
    rot.m[3][1] = translate.y;
    rot.m[3][2] = translate.z;
    rot.m[3][3] = 1.0f;

    return rot;
}

// Vector3 の足し算
inline Vector3 operator+(const Vector3& v1,const Vector3& v2){
    return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}
// Vector3 * float
inline Vector3 operator*(const Vector3& v,float s){
    return {v.x * s, v.y * s, v.z * s};
}
