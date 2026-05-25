/**
 * @file ModelCommon.h
 * @brief 3Dモデルを描画するための共通グラフィックスパイプライン（PSO）やルートシグネチャを管理するファイル
 */
#pragma once
#include "DirectXCommon.h"
#include "BlendMode.h"
#include <wrl/client.h>

/**
 * @brief 3Dモデル描画の共通設定を保持するクラス
 */
class ModelCommon {
public:
    /**
     * @brief 共通描画設定の初期化。ルートシグネチャとPSOを生成する
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief 描画コマンドにこの共通設定をセットする
     * @param blendMode 使用するブレンドモード
     */
    void CommonDrawSettings(BlendMode blendMode = BlendMode_Alpha);

    /**
     * @brief シャドウパス開始（シャドウ用 PSO + Root Signature をセット）
     */
    void BeginShadowPass();

    /**
     * @brief シャドウパス終了（通常 PSO への切り替えは次の CommonDrawSettings() で行う）
     */
    void EndShadowPass();

    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    DirectXCommon* dxCommon_ = nullptr;

    // ----- 通常描画用 -----
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[BlendMode_Count];

    // ----- シャドウパス用 -----
    /** @brief シャドウパス専用ルートシグネチャ（CBV 1つ：TransformationMatrix） */
    Microsoft::WRL::ComPtr<ID3D12RootSignature> shadowRootSignature_;
    /** @brief シャドウパス専用 PSO（深度のみ書き込み） */
    Microsoft::WRL::ComPtr<ID3D12PipelineState> shadowPipelineState_;
};
