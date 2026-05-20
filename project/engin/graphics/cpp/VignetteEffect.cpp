#include "VignetteEffect.h"
#include "WinApp.h"
#include <cassert>

using namespace Microsoft::WRL;

void VignetteEffect::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // 定数バッファ
    cbResource_ = dxCommon_->CreateBufferResource(256);
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));
    *cbData_ = VignetteParams{};

    // ルートシグネチャ: b0 CBV のみ（SRV 不要、数式で暗転するだけ）
    D3D12_ROOT_PARAMETER rootParam                 = {};
    rootParam.ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.ShaderVisibility                     = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParam.Descriptor.ShaderRegister            = 0; // b0

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc          = {};
    rootSigDesc.NumParameters                      = 1;
    rootSigDesc.pParameters                        = &rootParam;
    rootSigDesc.Flags                              = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    hr = device->CreateRootSignature(
        0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    // シェーダーコンパイル
    IDxcBlob* vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/postprocess/FullscreenVS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/postprocess/VignettePS.hlsl",   L"ps_6_0");

    // アルファブレンド: PS が出力する (0,0,0,vig) をバックバッファに重ねて周辺を暗転
    D3D12_RENDER_TARGET_BLEND_DESC rtBlend         = {};
    rtBlend.BlendEnable                            = TRUE;
    rtBlend.SrcBlend                               = D3D12_BLEND_SRC_ALPHA;
    rtBlend.DestBlend                              = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlend.BlendOp                                = D3D12_BLEND_OP_ADD;
    rtBlend.SrcBlendAlpha                          = D3D12_BLEND_ONE;
    rtBlend.DestBlendAlpha                         = D3D12_BLEND_ZERO;
    rtBlend.BlendOpAlpha                           = D3D12_BLEND_OP_ADD;
    rtBlend.RenderTargetWriteMask                  = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blendDesc                     = {};
    blendDesc.RenderTarget[0]                      = rtBlend;

    D3D12_RASTERIZER_DESC rastDesc                 = {};
    rastDesc.FillMode                              = D3D12_FILL_MODE_SOLID;
    rastDesc.CullMode                              = D3D12_CULL_MODE_NONE;

    D3D12_DEPTH_STENCIL_DESC depthDesc             = {};
    depthDesc.DepthEnable                          = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc     = {};
    psoDesc.pRootSignature                         = rootSignature_.Get();
    psoDesc.VS                                     = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS                                     = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState                             = blendDesc;
    psoDesc.RasterizerState                        = rastDesc;
    psoDesc.DepthStencilState                      = depthDesc;
    psoDesc.SampleMask                             = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                       = 1;
    psoDesc.RTVFormats[0]                          = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count                       = 1;
    // InputLayout なし: VS が SV_VertexID から三角形を生成する

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
    assert(SUCCEEDED(hr));
}

void VignetteEffect::Finalize()
{
    if (cbData_) {
        cbResource_->Unmap(0, nullptr);
        cbData_ = nullptr;
    }
    cbResource_.Reset();
    pipelineState_.Reset();
    rootSignature_.Reset();
}

void VignetteEffect::Apply()
{
    if (!enabled_)
        return;

    auto* cmdList = dxCommon_->GetCommandList();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetCurrentBackBufferHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT vp     = { 0.f, 0.f, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0.f, 1.f };
    D3D12_RECT     scissor = { 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetGraphicsRootSignature(rootSignature_.Get());
    cmdList->SetPipelineState(pipelineState_.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, cbResource_->GetGPUVirtualAddress());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void VignetteEffect::SetIntensity(float v) { if (cbData_) cbData_->intensity = v; }
float VignetteEffect::GetIntensity() const { return cbData_ ? cbData_->intensity : 0.f; }

void VignetteEffect::SetRadius(float v) { if (cbData_) cbData_->radius = v; }
float VignetteEffect::GetRadius()    const { return cbData_ ? cbData_->radius    : 0.f; }

void VignetteEffect::SetSoftness(float v) { if (cbData_) cbData_->softness = v; }
float VignetteEffect::GetSoftness()  const { return cbData_ ? cbData_->softness  : 0.f; }
