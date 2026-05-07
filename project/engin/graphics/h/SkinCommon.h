#pragma once
#include "DirectXCommon.h"
#include "BlendMode.h"
#include <wrl/client.h>

// スキニング（ボーンアニメーション）専用の PSO / Root Signature を管理するクラス。
// ModelCommon と同じスロット 0-5 を保ちつつ、スロット 6 (VS b1) にスキニングパレットを追加。
class SkinCommon {
public:
    void Initialize(DirectXCommon* dxCommon);
    void CommonDrawSettings(BlendMode blendMode = BlendMode_Alpha);

    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[BlendMode_Count];
};
