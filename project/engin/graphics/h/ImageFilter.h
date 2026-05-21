#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl/client.h>

class ImageFilter {
public:
    enum class Mode { Box, Gaussian, PrewittEdge, DepthOutline, RadialBlur };

    static ImageFilter* GetInstance()
    {
        static ImageFilter instance;
        return &instance;
    }

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    void BeginScene();
    void EndScene();
    void Apply(SrvManager* srvManager);

    void  SetEnabled(bool v) { enabled_ = v; }
    bool  IsEnabled()  const { return enabled_; }
    void  SetMode(Mode mode) { mode_ = mode; RebuildKernel(); }
    Mode  GetMode()    const { return mode_; }

    // Box / Gaussian パラメータ
    void  SetRadius(int r);
    int   GetRadius()  const { return boxRadius_; }
    void  SetSigma(float s);
    float GetSigma()   const { return gaussianSigma_; }

    // アウトライン共通パラメータ
    void  SetOutlineThreshold(float t) { if (outlineCb_) outlineCb_->threshold    = t; }
    float GetOutlineThreshold() const  { return outlineCb_ ? outlineCb_->threshold    : 0.05f; }
    void  SetOutlineStrength(float s)  { if (outlineCb_) outlineCb_->edgeStrength = s; }
    float GetOutlineStrength() const   { return outlineCb_ ? outlineCb_->edgeStrength : 5.0f; }
    void  SetOutlineColor(float r, float g, float b, float a = 1.0f) {
        if (!outlineCb_) return;
        outlineCb_->outlineR = r; outlineCb_->outlineG = g;
        outlineCb_->outlineB = b; outlineCb_->outlineA = a;
    }
    void  GetOutlineColor(float out[4]) const {
        if (!outlineCb_) { out[0]=out[1]=out[2]=0.f; out[3]=1.f; return; }
        out[0]=outlineCb_->outlineR; out[1]=outlineCb_->outlineG;
        out[2]=outlineCb_->outlineB; out[3]=outlineCb_->outlineA;
    }

    // 深度アウトライン専用
    void  SetDepthScale(float s) { if (outlineCb_) outlineCb_->depthScale = s; }
    float GetDepthScale() const  { return outlineCb_ ? outlineCb_->depthScale : 100.0f; }

    // ラジアルブラーパラメータ
    void  SetRadialCenter(float x, float y) {
        if (!radialBlurCb_) return;
        radialBlurCb_->centerX = x; radialBlurCb_->centerY = y;
    }
    float GetRadialCenterX()    const { return radialBlurCb_ ? radialBlurCb_->centerX    : 0.5f; }
    float GetRadialCenterY()    const { return radialBlurCb_ ? radialBlurCb_->centerY    : 0.5f; }
    void  SetRadialStrength(float s)  { if (radialBlurCb_) radialBlurCb_->strength    = s; }
    float GetRadialStrength()   const { return radialBlurCb_ ? radialBlurCb_->strength    : 0.1f; }
    void  SetRadialSampleCount(int n) { if (radialBlurCb_) radialBlurCb_->sampleCount = n; }
    int   GetRadialSampleCount() const { return radialBlurCb_ ? radialBlurCb_->sampleCount : 16; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetSceneRTVHandle() const { return sceneRtvHandle_; }

private:
    ImageFilter() = default;
    ~ImageFilter() = default;
    ImageFilter(const ImageFilter&) = delete;
    ImageFilter& operator=(const ImageFilter&) = delete;

    void RebuildKernel();

    // ブラーフィルター用 CBuffer（H/V 2スロット、256バイトアライン）
    struct FilterParams {
        float texelSizeX;
        float texelSizeY;
        int   radius;
        float pad0;
        float kernel[20];  // float4[5] = 80 bytes
        float dirX;
        float dirY;
        float pad1[2];
    };

    // ラジアルブラー用 CBuffer（1スロット）
    struct RadialBlurParams {
        float centerX;     // 0  ブラー中心 X（UV空間）
        float centerY;     // 4  ブラー中心 Y（UV空間）
        float strength;    // 8  ブラー強度
        int   sampleCount; // 12 サンプリング回数
    };

    // アウトライン用 CBuffer（1スロット）
    struct OutlineParams {
        float texelSizeX;   // 0
        float texelSizeY;   // 4
        float threshold;    // 8
        float edgeStrength; // 12
        float outlineR;     // 16
        float outlineG;     // 20
        float outlineB;     // 24
        float outlineA;     // 28
        float depthScale;   // 32
        float pad[3];       // 36
    };

    DirectXCommon* dxCommon_ = nullptr;

    // シーンキャプチャ用テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource>       sceneTexture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  sceneRtvHandle_ = {};
    uint32_t                                     sceneSrvIndex_  = UINT32_MAX;
    bool                                         isSceneFirstFrame_ = true;

    // 水平パス出力用中間テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource>       intermediateTexture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> intermediateRtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  intermediateRtvHandle_ = {};
    uint32_t                                     intermediateSrvIndex_  = UINT32_MAX;
    bool                                         isIntermediateFirstFrame_ = true;

    // ブラー / アウトライン / ラジアルブラー 用 PSO / Root Signature
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> boxPso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gaussianPso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> prewittPso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPso_;

    // 深度アウトライン用 PSO / Root Signature（t0+t1の2テクスチャ）
    Microsoft::WRL::ComPtr<ID3D12RootSignature> outlineRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> depthOutlinePso_;

    // ブラー用定数バッファ（H/V 2スロット = 512 bytes）
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    FilterParams* cbH_ = nullptr;
    FilterParams* cbV_ = nullptr;

    // アウトライン用定数バッファ（1スロット = 256 bytes）
    Microsoft::WRL::ComPtr<ID3D12Resource> outlineCbResource_;
    OutlineParams* outlineCb_ = nullptr;

    // ラジアルブラー用定数バッファ（1スロット = 256 bytes）
    Microsoft::WRL::ComPtr<ID3D12Resource> radialBlurCbResource_;
    RadialBlurParams* radialBlurCb_ = nullptr;

    // 深度バッファ SRV
    uint32_t depthSrvIndex_ = UINT32_MAX;

    Mode  mode_          = Mode::Box;
    int   boxRadius_     = 2;
    float gaussianSigma_ = 1.5f;
    bool  enabled_       = false;
};
