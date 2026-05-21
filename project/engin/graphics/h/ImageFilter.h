#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl/client.h>

class ImageFilter {
public:
    enum class Mode { Box, Gaussian, PrewittEdge, DepthOutline, RadialBlur, Dissolve, NoiseGen };

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

    // ディゾルブパラメータ
    void  SetDissolveThreshold(float t) { if (dissolveCb_) dissolveCb_->threshold = t; }
    float GetDissolveThreshold() const  { return dissolveCb_ ? dissolveCb_->threshold : 0.0f; }
    void  SetDissolveEdgeWidth(float w) { if (dissolveCb_) dissolveCb_->edgeWidth = w; }
    float GetDissolveEdgeWidth() const  { return dissolveCb_ ? dissolveCb_->edgeWidth : 0.05f; }
    void  SetDissolveEdgeColor(float r, float g, float b, float a = 1.0f) {
        if (!dissolveCb_) return;
        dissolveCb_->edgeR = r; dissolveCb_->edgeG = g;
        dissolveCb_->edgeB = b; dissolveCb_->edgeA = a;
    }
    void  GetDissolveEdgeColor(float out[4]) const {
        if (!dissolveCb_) { out[0]=1.f; out[1]=0.5f; out[2]=0.f; out[3]=1.f; return; }
        out[0]=dissolveCb_->edgeR; out[1]=dissolveCb_->edgeG;
        out[2]=dissolveCb_->edgeB; out[3]=dissolveCb_->edgeA;
    }
    void  SetDissolveMaskIndex(int i) { dissolveMaskIndex_ = (i == 0) ? 0 : 1; }
    int   GetDissolveMaskIndex() const { return dissolveMaskIndex_; }

    // プロシージャルノイズパラメータ
    void  SetNoiseScale(float x, float y) { if (!noiseGenCb_) return; noiseGenCb_->scaleX = x; noiseGenCb_->scaleY = y; }
    float GetNoiseScaleX()       const { return noiseGenCb_ ? noiseGenCb_->scaleX      : 4.0f; }
    float GetNoiseScaleY()       const { return noiseGenCb_ ? noiseGenCb_->scaleY      : 4.0f; }
    // seed は CPU 側の manualSeed を見せる（アニメーション時も表示値は変わらない）
    void  SetNoiseSeed(float s)        { noiseManualSeed_ = s; }
    float GetNoiseSeed()         const { return noiseManualSeed_; }
    void  SetNoiseOctaves(int n)       { if (noiseGenCb_) noiseGenCb_->octaves     = n; }
    int   GetNoiseOctaves()      const { return noiseGenCb_ ? noiseGenCb_->octaves     : 4; }
    void  SetNoisePersistence(float p) { if (noiseGenCb_) noiseGenCb_->persistence = p; }
    float GetNoisePersistence()  const { return noiseGenCb_ ? noiseGenCb_->persistence : 0.5f; }
    void  SetNoiseLacunarity(float l)  { if (noiseGenCb_) noiseGenCb_->lacunarity  = l; }
    float GetNoiseLacunarity()   const { return noiseGenCb_ ? noiseGenCb_->lacunarity  : 2.0f; }
    void  SetNoiseColorMode(int m)     { if (noiseGenCb_) noiseGenCb_->colorMode   = m; }
    int   GetNoiseColorMode()    const { return noiseGenCb_ ? noiseGenCb_->colorMode   : 0; }
    void  SetNoiseOpacity(float o)     { if (noiseGenCb_) noiseGenCb_->opacity     = o; }
    float GetNoiseOpacity()      const { return noiseGenCb_ ? noiseGenCb_->opacity     : 1.0f; }
    void  SetNoiseAnimate(bool v)      { animateNoise_ = v; }
    bool  GetNoiseAnimate()      const { return animateNoise_; }
    void  SetNoiseSpeed(float s)       { noiseSpeed_ = s; }
    float GetNoiseSpeed()        const { return noiseSpeed_; }
    void  ResetNoiseTime()             { noiseTime_ = 0.0f; }

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

    // ディゾルブ用 CBuffer（1スロット）
    struct DissolveParams {
        float threshold; // 0  溶解進行度（0=なし, 1=完全消滅）
        float edgeWidth; // 4  エッジ帯幅
        float pad0[2];   // 8
        float edgeR;     // 16
        float edgeG;     // 20
        float edgeB;     // 24
        float edgeA;     // 28
    };

    // プロシージャルノイズ用 CBuffer（1スロット）
    struct NoiseGenParams {
        float scaleX;      // 0  UV スケール X
        float scaleY;      // 4  UV スケール Y
        float seed;        // 8  シード（CPU が毎フレーム上書き）
        int   octaves;     // 12 オクターブ数
        float persistence; // 16 振幅減衰率
        float lacunarity;  // 20 周波数倍率
        int   colorMode;   // 24 0=グレー, 1=カラー
        float opacity;     // 28 ノイズ不透明度（0=シーンのみ, 1=ノイズのみ）
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
    Microsoft::WRL::ComPtr<ID3D12PipelineState> dissolvePso_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> noiseGenPso_;

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

    // ディゾルブ用定数バッファ（1スロット = 256 bytes）
    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveCbResource_;
    DissolveParams* dissolveCb_ = nullptr;

    // プロシージャルノイズ用定数バッファ（1スロット = 256 bytes）
    Microsoft::WRL::ComPtr<ID3D12Resource> noiseGenCbResource_;
    NoiseGenParams* noiseGenCb_ = nullptr;

    // 深度バッファ SRV
    uint32_t depthSrvIndex_ = UINT32_MAX;

    // ノイズマスク SRV（noise0.png, noise1.png）
    uint32_t noiseSrvIndex_[2] = { UINT32_MAX, UINT32_MAX };
    int      dissolveMaskIndex_ = 0;

    Mode  mode_          = Mode::Box;
    int   boxRadius_     = 2;
    float gaussianSigma_ = 1.5f;
    bool  enabled_       = false;

    // ノイズアニメーション（CPU 側のみ、GPU には seed として書き込む）
    float noiseManualSeed_ = 0.0f;
    float noiseTime_       = 0.0f;
    float noiseSpeed_      = 0.005f;
    bool  animateNoise_    = false;
};
