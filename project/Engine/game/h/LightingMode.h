/**
 * @file LightingMode.h
 * @brief 3Dオブジェクトのライティング計算方式を定義するファイル
 */
#pragma once

/**
 * @brief ライティングの計算方式を指定する列挙型
 * @note マテリアル設定（Material構造体）の shadingType 等に使用します
 */
enum LightingMode {

    /** @brief ライティングなし。テクスチャの色をそのまま出力します（Unlit） */
    Lighting_None = 0,

    /** * @brief ランバート反射（Lambert Reflection）
     * @note 最も標準的な拡散反射モデル。光の当たらない部分は真っ暗になります
     */
    Lighting_Lambert = 1,

    /** * @brief ハーフランバート（Half-Lambert）
     * @note 影の範囲を 0.5~1.0 に補正する方式。影が真っ暗にならず、柔らかい印象になります
     */
    Lighting_HalfLambert = 2,

    /** @brief Lambert 拡散 + Phong 鏡面反射 */
    Lighting_Lambert_Phong = 3,

    /** @brief HalfLambert 拡散 + Phong 鏡面反射 */
    Lighting_HalfLambert_Phong = 4
};