/**
 * @file SpriteCommon.h
 * @brief 2Dスプライト描画のための共通グラフィックスパイプライン（PSO）やルートシグネチャを管理するファイル
 */
#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>

/**
 * @brief 2Dスプライト描画の共通設定を保持するクラス
 * @note 全てのスプライトで共有されるルートシグネチャ、パイプラインステート、
 * および正投影（プロジェクション）行列などの設定を管理
 */
class SpriteCommon {
public:

    /**
     * @brief 2D描画共通設定の初期化。PSOの生成やルートシグネチャの作成を行う
     * @param dxCommon DirectX基盤のポインタ（デバイスやコマンドリストの取得用）
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief 描画コマンドに2Dスプライト用の共通設定（PSO, ルートシグネチャ, プリミティブトポロジ）をセットする
     * @note 個々のスプライト（Sprite::Draw）を呼ぶ前に、毎フレーム必ず一度実行すること
     */
    void CommonDrawSettings();

    /** @brief DirectX基盤のポインタを取得する */
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    
    /** @brief DirectX12デバイスを取得する */
    ID3D12Device* GetDevice() const { return dxCommon_->GetDevice(); }
    
    /** @brief 現在のグラフィックスコマンドリストを取得する */
    ID3D12GraphicsCommandList* GetCommandList() const { return dxCommon_->GetCommandList(); }
    
    /** @brief 2D用のルートシグネチャを取得する */
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    
    /** @brief 2D用のパイプラインステートオブジェクトを取得する */
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

private:
    /** @brief DirectX基盤のポインタ */
    DirectXCommon* dxCommon_ = nullptr;

    /** @brief 2D描画用のルートシグネチャ */
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

    /** @brief 2D描画用のグラフィックスパイプラインステート（PSO） */
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

    /** @brief デフォルトのライト情報用リソース（スプライトにライティングを適用する場合に使用） */
    Microsoft::WRL::ComPtr<ID3D12Resource> defaultLightResource_;
};