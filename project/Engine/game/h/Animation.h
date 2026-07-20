/**
 * @file Animation.h
 * @brief glTFアニメーションの読み込み・再生・補間を管理するファイル
 *
 * 使い方:
 *   Animation anim = LoadAnimationFile("./resources/AnimatedCube", "AnimatedCube.gltf");
 *   float animTime = 0.0f;
 *   // 毎フレーム
 *   animTime = std::fmod(animTime + 1.0f/60.0f, anim.duration);
 *   NodeAnimation& nodeAnim = anim.nodeAnimations["Armature"];
 *   Vector3    t = CalculateValue(nodeAnim.translate, animTime);
 *   Quaternion r = CalculateValue(nodeAnim.rotate,    animTime);
 *   Vector3    s = CalculateValue(nodeAnim.scale,     animTime);
 *   Matrix4x4 localMatrix = MakeAffineMatrix(s, r, t);
 */
#pragma once
#include "MakeAffine.h"
#include <cassert>
#include <map>
#include <string>
#include <vector>

// =================================================================
// キーフレーム
// =================================================================

/**
 * @brief 任意の型に対応する汎用キーフレーム構造体
 * @tparam tValue キーフレームの値の型（Vector3 or Quaternion）
 */
template <typename tValue>
struct Keyframe {
    float  time;   ///< このキーフレームの時刻（秒）
    tValue value;  ///< このキーフレームの値
};

using KeyframeVector3    = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

// =================================================================
// アニメーションカーブ
// =================================================================

/**
 * @brief キーフレームの列を持つアニメーションカーブ
 * @tparam tValue Vector3 または Quaternion
 */
template <typename tValue>
struct AnimationCurve {
    std::vector<Keyframe<tValue>> keyframes; ///< 時刻順に並んだキーフレームの配列
};

// =================================================================
// ノードアニメーション・アニメーション全体
// =================================================================

/**
 * @brief 1つのノード（ボーン）のアニメーションデータ
 */
struct NodeAnimation {
    AnimationCurve<Vector3>    translate; ///< 位置のアニメーション（線形補間）
    AnimationCurve<Quaternion> rotate;    ///< 回転のアニメーション（球面線形補間）
    AnimationCurve<Vector3>    scale;     ///< スケールのアニメーション（線形補間）
};

/**
 * @brief アニメーション全体のデータ
 */
struct Animation {
    float duration = 0.0f; ///< アニメーション全体の長さ（秒）
    std::map<std::string, NodeAnimation> nodeAnimations; ///< ノード名 → NodeAnimation
};

// =================================================================
// ファイル読み込み
// =================================================================

/**
 * @brief assimpを使ってglTFからアニメーションデータを読み込む
 * @param directoryPath ファイルが置かれているフォルダのパス
 * @param filename       ファイル名（例: "AnimatedCube.gltf"）
 * @return Animation     読み込まれたアニメーションデータ
 */
Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

// =================================================================
// 補間計算
// =================================================================

/**
 * @brief Vector3アニメーションカーブから指定時刻の値を線形補間で取得する
 * @param keyframes 時刻順に並んだキーフレーム配列
 * @param time      取得したい時刻（秒）
 * @return Vector3  補間された値
 */
Vector3 CalculateValue(const AnimationCurve<Vector3>& curve, float time);

/**
 * @brief Quaternionアニメーションカーブから指定時刻の値を球面線形補間で取得する
 * @param keyframes 時刻順に並んだキーフレーム配列
 * @param time      取得したい時刻（秒）
 * @return Quaternion 補間された値
 */
Quaternion CalculateValue(const AnimationCurve<Quaternion>& curve, float time);
