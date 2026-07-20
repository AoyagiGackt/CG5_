/**
 * @file ShadowManager.h
 * @brief シャドウマップの生成・管理を担当するクラス
 *
 * Object3dCommon からシャドウ関連処理を分離。
 * ライト方向を受け取り LightVP 行列を計算し、
 * シャドウパスの開始/終了/SRVセットを提供します。
 */
#pragma once
#include "DirectXCommon.h"
#include "MakeAffine.h"
#include <wrl/client.h>

class SrvManager;

class ShadowManager {
public:
    static const UINT kShadowMapSize = 2048; ///< シャドウマップ解像度

    /**
     * @brief 初期化（シャドウマップリソース・DSV・SRV を作成）
     * @param dxCommon DirectXCommon
     * @param srvManager SRV 管理クラス
     */
    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

    /**
     * @brief ライト方向からライト空間行列を更新
     * @param lightDir 正規化済みライト方向ベクトル（Object3dCommon から取得）
     */
    void Update(const Vector3& lightDir);

    /**
     * @brief シャドウパス開始（バリア遷移・DSVクリア・ビューポート設定）
     */
    void BeginShadowPass(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief シャドウパス終了（バリア遷移: DEPTH_WRITE → SRV）
     */
    void EndShadowPass(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief シャドウマップ SRV をスロット 4 にセット
     */
    void SetShadowMap(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager);

    /** @brief ライト空間行列を取得 */
    const Matrix4x4& GetLightViewProjection() const { return lightVP_; }

private:
    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource>       shadowMapResource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> shadowDsvHeap_;
    uint32_t shadowSrvIndex_ = 0;

    Matrix4x4 lightVP_ = MakeIdentity4x4();
    bool shadowMapInDepthWrite_ = true; ///< リソース初期状態は DEPTH_WRITE
};
