/**
 * @file Object3dCommon.h
 * @brief 3Dオブジェクト共通のライティング設定を管理するファイル
 *
 * - DirectionalLight の動的更新（時刻に応じたライト方向・色・アンビエント）
 * - シャドウマップは ShadowManager クラスが担当
 */
#pragma once
#include "DirectXCommon.h"
#include "MakeAffine.h"
#include <wrl/client.h>

/**
 * @brief 3Dオブジェクト全体で共有されるライティング設定を管理するクラス
 */
class Object3dCommon {
public:
    /** @brief 初期化（ライト定数バッファの作成） */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief ライト情報をコマンドリストのスロット 3 にセットする
     * @note CommonDrawSettings() の後、描画前に毎フレーム呼ぶ
     */
    void SetDefaultLight(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief 時刻に合わせてライト方向・色・アンビエントを更新する
     * @param timeRatio ゲーム内時刻の進行率（0.0=18:00 〜 1.0=翌6:00）
     */
    void UpdateLight(float timeRatio);

    /** @brief 現在のライト方向を取得（ShadowManager に渡す用） */
    Vector3 GetLightDirection() const;

private:
    DirectXCommon* dxCommon_ = nullptr;

    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float   intensity;
        Vector3 ambientColor;
        float   ambientIntensity;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
    DirectionalLight* lightData_ = nullptr; ///< Map 済みポインタ（Unmap しない）
};
