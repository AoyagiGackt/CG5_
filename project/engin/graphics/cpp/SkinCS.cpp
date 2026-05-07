#include "SkinCS.h"
#include <cassert>

using namespace Microsoft::WRL;

void SkinCS::Initialize(DirectXCommon* dxCommon,
                        ID3D12Resource* inputBuffer,
                        UINT vertexCount)
{
    assert(inputBuffer);
    assert(vertexCount > 0);

    vertexCount_  = vertexCount;
    inputGpuVA_   = inputBuffer->GetGPUVirtualAddress();
    ID3D12Device* device = dxCommon->GetDevice();

    // =====================================================
    // Compute Root Signature
    //   Slot 0 (t0): 入力頂点バッファ (SRV)
    //   Slot 1 (b0): スキニングパレット (CBV)
    //   Slot 2 (u0): 出力頂点バッファ (UAV)
    // =====================================================
    D3D12_ROOT_PARAMETER params[3]{};

    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0; // t0
    params[0].Descriptor.RegisterSpace  = 0;

    params[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[1].Descriptor.ShaderRegister = 0; // b0
    params[1].Descriptor.RegisterSpace  = 0;

    params[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[2].Descriptor.ShaderRegister = 0; // u0
    params[2].Descriptor.RegisterSpace  = 0;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    rsDesc.pParameters   = params;
    rsDesc.NumParameters = _countof(params);

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    device->CreateRootSignature(
        0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&csRootSig_));

    // =====================================================
    // Compute PSO
    // =====================================================
    IDxcBlob* csBlob = dxCommon->CompileShader(
        L"Resources/shaders/skinned/SkinningCS.hlsl", L"cs_6_0");
    assert(csBlob);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = csRootSig_.Get();
    psoDesc.CS             = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&csPSO_));

    // =====================================================
    // 出力バッファ (DEFAULT heap, UAV フラグ付き)
    // =====================================================
    UINT64 outputSize = static_cast<UINT64>(sizeof(OutputVertex)) * vertexCount;

    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width            = outputSize;
    rd.Height           = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels        = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(
        &heap, D3D12_HEAP_FLAG_NONE, &rd,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&outputBuffer_));

    outputVBV_.BufferLocation = outputBuffer_->GetGPUVirtualAddress();
    outputVBV_.SizeInBytes    = static_cast<UINT>(outputSize);
    outputVBV_.StrideInBytes  = sizeof(OutputVertex); // 36 bytes
}

void SkinCS::Dispatch(ID3D12GraphicsCommandList* cmd,
                      D3D12_GPU_VIRTUAL_ADDRESS paletteCBAddress)
{
    // 前フレームの描画パスで VERTEX_AND_CONSTANT_BUFFER に遷移済みなら UAV に戻す
    if (needsPreTransition_) {
        D3D12_RESOURCE_BARRIER b{};
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = outputBuffer_.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        b.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
    }

    cmd->SetComputeRootSignature(csRootSig_.Get());
    cmd->SetPipelineState(csPSO_.Get());

    // Slot 0: 入力頂点バッファ (SRV, t0)
    cmd->SetComputeRootShaderResourceView(0, inputGpuVA_);
    // Slot 1: スキニングパレット (CBV, b0)
    cmd->SetComputeRootConstantBufferView(1, paletteCBAddress);
    // Slot 2: 出力頂点バッファ (UAV, u0)
    cmd->SetComputeRootUnorderedAccessView(2, outputBuffer_->GetGPUVirtualAddress());

    UINT threadGroups = (vertexCount_ + 63) / 64;
    cmd->Dispatch(threadGroups, 1, 1);

    // CS の書き込み完了を保証する UAV バリア
    D3D12_RESOURCE_BARRIER uavBarrier{};
    uavBarrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = outputBuffer_.Get();
    cmd->ResourceBarrier(1, &uavBarrier);

    // 描画パスで頂点バッファとして読めるよう状態を遷移
    D3D12_RESOURCE_BARRIER vbBarrier{};
    vbBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    vbBarrier.Transition.pResource   = outputBuffer_.Get();
    vbBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    vbBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    vbBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &vbBarrier);

    needsPreTransition_ = true;
}
