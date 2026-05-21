#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl/client.h>

class ImageFilter {
public:
    enum class Mode { Box, Linear };

    static ImageFilter* GetInstance()
    {
        static ImageFilter instance;
        return &instance;
    }

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    void BeginScene();  // シーン描画前に呼ぶ（RTV をオフスクリーンへ切り替え）
    void EndScene();    // シーン描画後に呼ぶ（SRV に遷移）
    void Apply(SrvManager* srvManager); // 水平→垂直の2パスでバックバッファへ適用

    void  SetEnabled(bool v) { enabled_ = v; }
    bool  IsEnabled()  const { return enabled_; }
    void  SetMode(Mode mode) { mode_ = mode; RebuildKernel(); }
    Mode  GetMode()    const { return mode_; }
    void  SetRadius(int r);
    int   GetRadius()  const { return boxRadius_; }
    void  SetSigma(float s);
    float GetSigma()   const { return gaussianSigma_; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetSceneRTVHandle() const { return sceneRtvHandle_; }

private:
    ImageFilter() = default;
    ~ImageFilter() = default;
    ImageFilter(const ImageFilter&) = delete;
    ImageFilter& operator=(const ImageFilter&) = delete;

    void RebuildKernel(); // CPU側でカーネル重みを計算してCBufferへ書き込む

    // H/V で共通のCBuffer構造体（256バイトアライン2スロット分確保）
    struct FilterParams {
        float texelSizeX;   // 0
        float texelSizeY;   // 4
        int   radius;       // 8
        float pad0;         // 12
        float kernel[20];   // 16  (float4[5] = 80 bytes, 最大17タップ使用)
        float dirX;         // 96
        float dirY;         // 100
        float pad1[2];      // 104
        // 108 bytes → 256バイトスロットに収まる
    };

    DirectXCommon* dxCommon_ = nullptr;

    // ----- シーンキャプチャ用テクスチャ -----
    Microsoft::WRL::ComPtr<ID3D12Resource>       sceneTexture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  sceneRtvHandle_ = {};
    uint32_t                                     sceneSrvIndex_  = UINT32_MAX;
    bool                                         isSceneFirstFrame_ = true;

    // ----- 水平パス出力用中間テクスチャ -----
    Microsoft::WRL::ComPtr<ID3D12Resource>       intermediateTexture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> intermediateRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  intermediateRtvHandle_ = {};
    uint32_t                                     intermediateSrvIndex_  = UINT32_MAX;
    bool                                         isIntermediateFirstFrame_ = true;

    // ----- PSO / Root Signature -----
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    // ----- 定数バッファ（H/V 2スロット）-----
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    FilterParams* cbH_ = nullptr; // offset   0: 水平パス用
    FilterParams* cbV_ = nullptr; // offset 256: 垂直パス用

    Mode  mode_          = Mode::Box;
    int   boxRadius_     = 2;
    float gaussianSigma_ = 1.5f;
    bool  enabled_       = false;
};
