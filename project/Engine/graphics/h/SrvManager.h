/**
 * @file SrvManager.h
 * @brief SRV（シェーダーリソースビュー）用のデスクリプタヒープとそのインデックスを一括管理するファイル
 */
#pragma once
#include "DirectXCommon.h"
#include <wrl/client.h>

/**
 * @brief SRV（テクスチャなどのリソース）のデスクリプタを管理するシングルトンクラス
 * @note 全てのテクスチャやインスタンシング用のリソースは、このマネージャーを通じて
 * GPUが参照するためのインデックス（SRV）を割り当てられます
 */
class SrvManager {
public:
    /**
     * @brief SrvManagerの唯一のインスタンスを取得する
     * @return SrvManager* シングルトンインスタンスへのポインタ
     */
    static SrvManager* GetInstance();

    /**
     * @brief マネージャーの初期化。デスクリプタヒープの生成とサイズ設定を行う
     * @param dxCommon DirectX基盤のポインタ（デバイス取得などに使用）
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief 描画前準備。コマンドリストにデスクリプタヒープをセットする
     * @note 毎フレームの描画処理の最初（PreDrawなど）で一度だけ呼び出すこと
     */
    void PreDraw();

    /**
     * @brief 未使用のデスクリプタインデックスを1つ確保する
     * @return uint32_t 確保した場所のインデックス番号
     * @note 確保できる最大数（kMaxSRVCount）を超えるとアサートが発生します!
     */
    uint32_t Allocate();

    /**
     * @brief 指定したインデックスの場所に、テクスチャ2D用のSRVを生成する
     * @param srvIndex Allocate()で確保したインデックス
     * @param pResource SRVを紐づけるGPUリソース（テクスチャバッファ）
     * @param Format テクスチャのフォーマット（DXGI_FORMAT）
     * @param MipLevels ミップマップのレベル数
     */
    void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

    /**
     * @brief 指定したインデックスの場所に、キューブマップ用のSRVを生成する
     * @param srvIndex Allocate()で確保したインデックス
     * @param pResource SRVを紐づけるGPUリソース（キューブマップテクスチャ）
     * @param Format テクスチャのフォーマット（DXGI_FORMAT）
     * @param MipLevels ミップマップのレベル数
     */
    void CreateSRVforTextureCube(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

    /**
     * @brief 深度テクスチャ（R24G8_TYPELESS）用のSRVを生成する
     * @param srvIndex Allocate()で確保したインデックス
     * @param pResource 深度ステンシルリソース（TYPELESS形式で作成済みのもの）
     */
    void CreateSRVforDepthTexture(uint32_t srvIndex, ID3D12Resource* pResource);

    /**
     * @brief マネージャーの終了処理。デスクリプタヒープを解放する
     */
    void Finalize();

    /**
     * @brief 管理しているSRV用デスクリプタヒープ本体を取得する
     * @return ID3D12DescriptorHeap* デスクリプタヒープのポインタ
     */
    ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return descriptorHeap_.Get(); }

    /**
     * @brief 指定したインデックスに対応するCPU側のハンドルを取得する
     * @param index インデックス番号
     * @return D3D12_CPU_DESCRIPTOR_HANDLE CPU用デスクリプタハンドル
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

    /**
     * @brief 指定したインデックスに対応するGPU側のハンドルを取得する
     * @param index インデックス番号
     * @return D3D12_GPU_DESCRIPTOR_HANDLE GPU用デスクリプタハンドル（描画コマンドで使用）
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

private:
    SrvManager() = default;
    ~SrvManager() = default;
    SrvManager(const SrvManager&) = delete;
    SrvManager& operator=(const SrvManager&) = delete;

    /** @brief DirectX基盤のポインタ */
    DirectXCommon* dxCommon_ = nullptr;

    /** @brief 最大SRV確保数 */
    static const uint32_t kMaxSRVCount = 512;

    /** @brief デスクリプタヒープの実体 */
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;

    /** @brief デスクリプタ1つあたりのメモリサイズ（ハードウェアによって異なる） */
    uint32_t descriptorSize_ = 0;

    /** @brief 現在どこまでインデックスを使用しているかのカウント */
    uint32_t useIndex_ = 0;
};