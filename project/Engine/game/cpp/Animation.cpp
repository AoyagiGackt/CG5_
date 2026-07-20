#include "Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <cmath>

// =================================================================
// ファイル読み込み
// =================================================================

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
    Animation animation;

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);

    assert(scene->mNumAnimations != 0); // アニメーションがなければ止める

    // 最初のアニメーションだけ採用
    aiAnimation* animationAssimp = scene->mAnimations[0];

    // 時間単位をtick→秒に変換した尺を記録
    animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

    for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {

        aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
        NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

        // ---- Translate（位置） ----
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            // assimpは右手系なのでX軸を反転して左手系に変換する
            keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
            nodeAnimation.translate.keyframes.push_back(keyframe);
        }

        // ---- Rotate（回転） ----
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
            aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
            KeyframeQuaternion keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            // y, z を反転する
            keyframe.value = {
                keyAssimp.mValue.x,
                -keyAssimp.mValue.y,
                -keyAssimp.mValue.z,
                keyAssimp.mValue.w
            };
            nodeAnimation.rotate.keyframes.push_back(keyframe);
        }

        // ---- Scale（スケール） ----
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
            nodeAnimation.scale.keyframes.push_back(keyframe);
        }
    }

    return animation;
}

// =================================================================
// 線形補間
// =================================================================

Vector3 CalculateValue(const AnimationCurve<Vector3>& curve, float time)
{
    if (curve.keyframes.empty()) {
        return { 0.0f, 0.0f, 0.0f };
    }

    // キーが1つ、または時刻がアニメーション開始前なら先頭の値をそのまま返す
    if (curve.keyframes.size() == 1 || time <= curve.keyframes.front().time) {
        return curve.keyframes.front().value;
    }

    // この区間内かを調べて補間する
    for (size_t index = 0; index < curve.keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        if (curve.keyframes[index].time <= time && time <= curve.keyframes[nextIndex].time) {
            // 区間内にある → 正規化した補間係数 t を求めて線形補間
            float t = (time - curve.keyframes[index].time)
                / (curve.keyframes[nextIndex].time - curve.keyframes[index].time);
            return Lerp(curve.keyframes[index].value, curve.keyframes[nextIndex].value, t);
        }
    }

    // ループで見つからなかったら最後の値を返す
    return curve.keyframes.back().value;
}

// =================================================================
// 球面線形補間
// =================================================================

Quaternion CalculateValue(const AnimationCurve<Quaternion>& curve, float time)
{
    if (curve.keyframes.empty()) {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    if (curve.keyframes.size() == 1 || time <= curve.keyframes.front().time) {
        return curve.keyframes.front().value;
    }

    for (size_t index = 0; index < curve.keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        if (curve.keyframes[index].time <= time && time <= curve.keyframes[nextIndex].time) {
            float t = (time - curve.keyframes[index].time)
                / (curve.keyframes[nextIndex].time - curve.keyframes[index].time);
            // 球面線形補間で滑らかに回転を補間する
            return Slerp(curve.keyframes[index].value, curve.keyframes[nextIndex].value, t);
        }
    }

    return curve.keyframes.back().value;
}