/**
 * @file DirectXCommon.h
 * @brief DirectX12のデバイス生成、コマンド管理、スワップチェーンなどの基盤機能を管理するファイル
 */
#pragma once
#include <Windows.h>
#include <cassert>
#include <chrono>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <thread>
#include <wrl/client.h>

class WinApp;

/**
 * @brief DirectX12の共通基盤クラス
 * @note アプリケーション全体で共有されるDirectX12の基本オブジェクト（Device, CommandList等）を保持します
 * 描画の開始(PreDraw)と終了(PostDraw)の管理、およびFPSの固定化機能も備えています
 */
class DirectXCommon {
public: // メンバ関数
    /**
     * @brief DirectX12基盤の初期化
     * @param winApp 管理対象となるWinApp（ウィンドウ管理）のポインタ
     */
    void Initialize(WinApp* winApp);

    /**
     * @brief DirectX12基盤の終了処理
     * @note GPU完了を待ち、スワップチェーンのフルスクリーン解除など安全な順序で解放する
     */
    void Finalize();

    /**
     * @brief 描画前処理
     * @note コマンドアロケータとリストのリセット、バックバッファの状態遷移、画面クリアなどを行います
     */
    void PreDraw();

    /**
     * @brief 描画後処理
     * @note バックバッファをPresentation状態に戻し、コマンドの実行、画面の入れ替え（Flip）、FPS固定の待ちを行います
     */
    void PostDraw();

    /**
     * @brief シェーダーファイルをコンパイルする
     * @param filePath シェーダーファイルのパス
     * @param profile コンパイルプロファイル（例: L"vs_6_0", L"ps_6_0"）
     * @return IDxcBlob* コンパイルされたシェーダーデータ
     */
    IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

    /** @brief デバイスの取得 */
    ID3D12Device* GetDevice() { return device_.Get(); }

    /** @brief グラフィックスコマンドリストの取得 */
    ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }

    /** @brief コマンドキューの取得 */
    ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }

    /** @brief スワップチェーンの取得 */
    IDXGISwapChain4* GetSwapChain() { return swapChain_.Get(); }

    /** @brief コマンドアロケータの取得 */
    ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator_.Get(); }

    /** @brief SRV用デスクリプタヒープの取得 */
    ID3D12DescriptorHeap* GetSrvDescriptorHeap() { return srvDescriptorHeap_.Get(); }

    /**
     * @brief バッファリソース（定数バッファ等）を生成する
     * @param sizeInBytes 生成するバッファのサイズ
     * @return Microsoft::WRL::ComPtr<ID3D12Resource> 生成されたリソース
     */
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

    // --- RTV関連 ---

    /** @brief 現在のバックバッファのRTVハンドルを取得 */
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle();

    /** @brief 現在のバックバッファリソースを取得 */
    ID3D12Resource* GetCurrentBackBufferResource();

    /** @brief バックバッファのフォーマットを取得（デフォルトはSRGB） */
    DXGI_FORMAT GetBackBufferFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; }

    /** @brief スワップチェーンのバッファ数を取得（ダブルバッファリングなら2） */
    UINT GetBufferCount() const { return 2; }

    /** @brief DSV（深度バッファ）のハンドルを取得 */
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() { return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(); }

    /** @brief 深度ステンシルリソースを取得（ポストプロセス SRV 作成用） */
    ID3D12Resource* GetDepthStencilResource() const { return depthStencilResource_.Get(); }

    // --- フェンス関連 ---

    /** @brief フェンスの取得 */
    ID3D12Fence* GetFence() { return fence_.Get(); }

    /** @brief 現在のフェンス値を取得 */
    uint64_t GetFenceValue() const { return fenceValue_; }

    /** @brief フェンスイベントのハンドルを取得 */
    HANDLE GetFenceEvent() { return fenceEvent_; }

    /** @brief フェンス値をインクリメントする */
    void IncrementFenceValue() { fenceValue_++; }

private:
    // 内部初期化関数群

    // デバイスとファクトリの生成
    void InitializeDevice();

    // コマンドキュー・アロケータ・リストの生成
    void CreateCommand();

    // スワップチェーンの生成
    void CreateSwapChain();

    // 各種デスクリプタヒープの生成
    void CreateDescriptorHeaps();

    // レンダーターゲットビューの生成
    void CreateRTV();

    // 深度バッファリソースとDSVの生成
    void CreateDepthBuffer();

    // GPU同期用フェンスの生成
    void CreateFence();

    // DXCシェーダーコンパイラの初期化
    void InitializeDXC();

    // FPS固定関連
    void InitializeFixFPS(); ///< FPS固定用の参照時間初期化
    void UpdateFixFPS(); ///< 1/60秒に満たない場合に待機する

private:
    // DirectX12基盤オブジェクト
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

    // デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResoures_[2];
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

    // DXC (Shader Compiler)
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    // 同期・管理用メンバ
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
    uint64_t fenceValue_;
    HANDLE fenceEvent_;

    // FPS固定用
    std::chrono::steady_clock::time_point reference_;

    WinApp* winApp_ = nullptr;
};