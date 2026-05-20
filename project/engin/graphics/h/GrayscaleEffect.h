#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl/client.h>

class GrayscaleEffect {
public:
    static GrayscaleEffect* GetInstance()
    {
        static GrayscaleEffect instance;
        return &instance;
    }

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    // シーン描画の前後に呼び出してオフスクリーンテクスチャへ描画先を切り替える
    void BeginScene();
    void EndScene();

    // バックバッファへのフルスクリーングレースケールパス。EndScene() 後に呼ぶ
    void Apply(SrvManager* srvManager);

    void  SetAmount(float amount);
    float GetAmount() const;
    void  SetEnabled(bool enabled) { enabled_ = enabled; }
    bool  IsEnabled() const { return enabled_; }

    // グレースケール有効時のオフスクリーン RTV を返す
    D3D12_CPU_DESCRIPTOR_HANDLE GetSceneRTVHandle() const { return rtvHandle_; }

private:
    GrayscaleEffect() = default;
    ~GrayscaleEffect() = default;
    GrayscaleEffect(const GrayscaleEffect&) = delete;
    GrayscaleEffect& operator=(const GrayscaleEffect&) = delete;

    DirectXCommon* dxCommon_ = nullptr;

    // オフスクリーンレンダーターゲット (TYPELESS にすることで RTV と SRV の両方に SRGB を使用可能)
    Microsoft::WRL::ComPtr<ID3D12Resource>       sceneTexture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  rtvHandle_ = {};
    uint32_t                                     srvIndex_  = UINT32_MAX;
    bool                                         isFirstFrame_ = true;

    // フルスクリーングレースケール PSO
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    // 定数バッファ
    struct GrayscaleParams {
        float amount = 0.f;
        float pad[3] = {};
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    GrayscaleParams*                       cbData_   = nullptr;

    bool enabled_ = false;
};
