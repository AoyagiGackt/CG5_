#pragma once
#include "DirectXCommon.h"
#include <wrl/client.h>

class VignetteEffect {
public:
    static VignetteEffect* GetInstance()
    {
        static VignetteEffect instance;
        return &instance;
    }

    void Initialize(DirectXCommon* dxCommon);
    void Finalize();

    // バックバッファ上にビネットオーバーレイを描画する。シーン描画後に呼ぶ
    void Apply();

    void  SetEnabled(bool enabled) { enabled_ = enabled; }
    bool  IsEnabled()  const { return enabled_; }
    void  SetIntensity(float v);
    float GetIntensity() const;
    void  SetRadius(float v);
    float GetRadius()    const;
    void  SetSoftness(float v);
    float GetSoftness()  const;

private:
    VignetteEffect() = default;
    ~VignetteEffect() = default;
    VignetteEffect(const VignetteEffect&) = delete;
    VignetteEffect& operator=(const VignetteEffect&) = delete;

    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    struct VignetteParams {
        float intensity = 1.0f;
        float radius    = 0.3f;
        float softness  = 0.4f;
        float pad       = 0.0f;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    VignetteParams* cbData_ = nullptr;

    bool enabled_ = false;
};
