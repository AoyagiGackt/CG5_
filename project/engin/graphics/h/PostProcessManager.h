#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl/client.h>

// ポストプロセスの全パラメータ（定数バッファと1対1対応, 16バイト整列）
struct PostProcessParams
{
    float grayscaleIntensity    = 0.0f;   // グレースケール強度 [0,1]
    float vignetteIntensity     = 0.0f;   // ビネット強度       [0,1]
    float vignetteRadius        = 0.7f;   // ビネット半径       [0,1]
    float vignetteSoftness      = 0.3f;   // ビネットぼかし     [0,1]

    float boxFilterIntensity    = 0.0f;   // ボックスフィルタ   [0,1]
    float gaussianIntensity     = 0.0f;   // ガウスフィルタ     [0,1]
    float lumOutlineIntensity   = 0.0f;   // 輝度アウトライン   [0,1]
    float lumOutlineThreshold   = 0.1f;   // 輝度アウトライン閾値

    float depthOutlineIntensity = 0.0f;   // 深度アウトライン   [0,1]
    float depthOutlineThreshold = 0.005f; // 深度アウトライン閾値
    float radialBlurIntensity   = 0.0f;   // ラジアルブラー強度 [0,1]
    float radialBlurWidth       = 0.02f;  // ラジアルブラー幅

    float dissolveThreshold     = 0.0f;   // ディゾルブ閾値     [0,1]
    float randomIntensity       = 0.0f;   // ランダムノイズ     [0,1]
    float randomTime            = 0.0f;   // 内部タイマー（自動更新）
    float texelSizeX            = 1.0f / 1280.0f;

    float texelSizeY            = 1.0f / 720.0f;
    float pad0 = 0.0f;
    float pad1 = 0.0f;
    float pad2 = 0.0f;
};
static_assert(sizeof(PostProcessParams) % 16 == 0, "PostProcessParams must be 16-byte aligned");

class PostProcessManager
{
public:
    static PostProcessManager* GetInstance();

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    // 1フレームに1回以上呼んでよい。最初の呼び出しでオフスクリーンRTをクリア＋セット
    void BeginCapture(ID3D12GraphicsCommandList* cmd, DirectXCommon* dxCommon);

    // シーン描画後に呼ぶ。オフスクリーンRTを読んでスワップチェーンへ出力
    void Apply(ID3D12GraphicsCommandList* cmd, DirectXCommon* dxCommon, SrvManager* srvManager);

    // ImGui コントロールを描画
    void ShowImGui();

    PostProcessParams params_;

private:
    PostProcessManager() = default;

    void CreateOffScreenRT(ID3D12Device* device);
    void CreatePSO(DirectXCommon* dxCommon);

    DirectXCommon* dxCommon_ = nullptr;

    // オフスクリーンカラーRT
    Microsoft::WRL::ComPtr<ID3D12Resource>       colorRT_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  rtvHandle_ = {};
    uint32_t colorSrvIndex_ = 0;

    // 深度バッファ SRV
    uint32_t depthSrvIndex_ = 0;

    // PSO
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    PostProcessParams* mappedParams_ = nullptr;

    // 状態管理
    bool colorRTInRenderTarget_ = true;  // 初期状態はRT
    bool clearedThisFrame_      = false;
};
