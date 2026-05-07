#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl/client.h>

// Compute Shader による GPU スキニングを管理するクラス。
//
// データフロー:
//   元の頂点バッファ (SRV, t0)
//   × スキニングパレット (CBV, b0)
//   → 計算済み頂点バッファ (UAV, u0)
//   → 描画パスで頂点バッファ (VBV) として使用
//
// 使い方:
//   1. Initialize() で初期化
//   2. 毎フレーム Update() より後で Dispatch() を呼ぶ
//   3. Draw() で GetOutputVBV() を IASetVertexBuffers に渡す
class SkinCS {
public:
    // 出力頂点レイアウト: ModelCommon の入力レイアウトと完全一致
    // POSITION (R32G32B32A32_FLOAT) + TEXCOORD (R32G32_FLOAT) + NORMAL (R32G32B32_FLOAT)
    struct OutputVertex {
        float position[4]; // 16 bytes
        float texcoord[2]; //  8 bytes
        float normal[3];   // 12 bytes
                           // total: 36 bytes
    };

    // inputBuffer : SkinnedModel::GetVertexResource() を渡す (GENERIC_READ 状態)
    // vertexCount : SkinnedModel::GetVertexCount()
    void Initialize(DirectXCommon* dxCommon,
                    ID3D12Resource* inputBuffer,
                    UINT vertexCount);

    // スキニング計算を Dispatch する。Draw() より前に呼ぶこと。
    // paletteCBAddress : paletteCB_->GetGPUVirtualAddress()
    void Dispatch(ID3D12GraphicsCommandList* cmd,
                  D3D12_GPU_VIRTUAL_ADDRESS paletteCBAddress);

    // 計算済み頂点バッファのビューを返す。IASetVertexBuffers に渡す。
    const D3D12_VERTEX_BUFFER_VIEW& GetOutputVBV() const { return outputVBV_; }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> csRootSig_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csPSO_;

    // CS 出力バッファ (DEFAULT heap, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    Microsoft::WRL::ComPtr<ID3D12Resource> outputBuffer_;
    D3D12_VERTEX_BUFFER_VIEW               outputVBV_{};

    D3D12_GPU_VIRTUAL_ADDRESS inputGpuVA_ = 0; // 入力バッファの GPU VA
    UINT                      vertexCount_ = 0;

    // 2 フレーム目以降、Dispatch 前に VERTEX_AND_CONSTANT_BUFFER → UAV の遷移が必要
    bool needsPreTransition_ = false;
};
