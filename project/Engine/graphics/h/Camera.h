/**
 * @file Camera.h
 * @brief カメラの座標やビュー・プロジェクション行列を管理するファイル
 *
 * 使い方:
 *   camera->SetTranslate({0, 5, -10}); // 座標をセット（行列は自動で更新）
 *   camera->SetRotate({0.1f, 0, 0});   // 回転をセット（行列は自動で更新）
 *   // Update() を呼ばなくても GetViewMatrix() は常に最新を返す
 */
#pragma once
#include "MakeAffine.h"

/**
 * @brief 3D空間のカメラを表現するクラス
 *
 * パラメータを変更すると行列が自動的に再計算されます。
 * Update() は後方互換のために残していますが、呼ばなくても動作します。
 */
class Camera {
public:
    Camera();

    /**
     * @brief 行列を強制更新する（後方互換のため残してあります）
     * @note SetTranslate/SetRotate 等を呼ぶと自動で再計算されるため、
     *       通常は明示的に呼ぶ必要はありません。
     */
    void Update() { RecalcMatrices(); }

    // =====================================================
    // セッター（呼ぶだけで行列が自動更新される）
    // =====================================================

    void SetRotate(const Vector3& rotate)       { transform_.rotate    = rotate;    isDirty_ = true; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; isDirty_ = true; }
    void SetFovY(float fovY)                    { fovY_       = fovY;               isDirty_ = true; }
    void SetAspectRatio(float aspectRatio)      { aspectRatio_= aspectRatio;        isDirty_ = true; }
    void SetNearClip(float nearClip)            { nearClip_   = nearClip;           isDirty_ = true; }
    void SetFarClip(float farClip)              { farClip_    = farClip;            isDirty_ = true; }

    // =====================================================
    // ゲッター（常に最新の行列を返す）
    // =====================================================

    const Vector3& GetRotate()    const { return transform_.rotate; }
    const Vector3& GetTranslate() const { return transform_.translate; }
    const Transform& GetTransform() const { return transform_; }

    /** @brief 最新のビュー行列を返す（必要なら自動再計算） */
    const Matrix4x4& GetViewMatrix() const {
        if (isDirty_) { RecalcMatrices(); }
        return viewMatrix_;
    }

    /** @brief 最新のプロジェクション行列を返す（必要なら自動再計算） */
    const Matrix4x4& GetProjectionMatrix() const {
        if (isDirty_) { RecalcMatrices(); }
        return projectionMatrix_;
    }

    // 直接変更したい場合用の非 const 参照ゲッター
    Vector3& GetRotate()    { isDirty_ = true; return transform_.rotate; }
    Vector3& GetTranslate() { isDirty_ = true; return transform_.translate; }

private:
    /** @brief ビュー行列とプロジェクション行列を再計算する */
    void RecalcMatrices() const;

    Transform transform_;

    mutable Matrix4x4 viewMatrix_;
    mutable Matrix4x4 projectionMatrix_;
    mutable bool isDirty_ = true; // true の間、次の Get で再計算される

    float fovY_;
    float aspectRatio_;
    float nearClip_;
    float farClip_;
};
