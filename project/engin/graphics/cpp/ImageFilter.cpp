#include "ImageFilter.h"
#include "WinApp.h"
#include <cassert>
#include <cmath>
#include <algorithm>

using namespace Microsoft::WRL;

// =====================================================
// ヘルパー: カラーテクスチャ + RTV + SRV を作成
// =====================================================

static void CreateOffscreenTexture(
    ID3D12Device* device,
    SrvManager* srvManager,
    uint32_t width, uint32_t height,
    Microsoft::WRL::ComPtr<ID3D12Resource>&       outTexture,
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& outRtvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE&                  outRtvHandle,
    uint32_t&                                     outSrvIndex)
{
    constexpr DXGI_FORMAT kResourceFmt = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    constexpr DXGI_FORMAT kRtvFmt      = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    constexpr DXGI_FORMAT kSrvFmt      = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    constexpr float       kClear[4]    = { 0.1f, 0.25f, 0.5f, 1.0f };

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format    = kRtvFmt;
    clearValue.Color[0]  = kClear[0]; clearValue.Color[1] = kClear[1];
    clearValue.Color[2]  = kClear[2]; clearValue.Color[3] = kClear[3];

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width               = width;
    desc.Height              = height;
    desc.DepthOrArraySize    = 1;
    desc.MipLevels           = 1;
    desc.Format              = kResourceFmt;
    desc.SampleDesc.Count    = 1;
    desc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue, IID_PPV_ARGS(&outTexture));
    assert(SUCCEEDED(hr));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 1;
    hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&outRtvHeap));
    assert(SUCCEEDED(hr));

    outRtvHandle = outRtvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format        = kRtvFmt;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(outTexture.Get(), &rtvDesc, outRtvHandle);

    outSrvIndex = srvManager->Allocate();
    srvManager->CreateSRVforTexture2D(outSrvIndex, outTexture.Get(), kSrvFmt, 1);
}

// =====================================================
// バリアヘルパー
// =====================================================

static void Barrier(ID3D12GraphicsCommandList* cmd, ID3D12Resource* res,
    D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER b = {};
    b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource   = res;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter  = after;
    cmd->ResourceBarrier(1, &b);
}

// =====================================================
// Initialize
// =====================================================

void ImageFilter::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device   = dxCommon->GetDevice();
    const uint32_t width   = WinApp::kClientWidth;
    const uint32_t height  = WinApp::kClientHeight;

    // シーンキャプチャ用テクスチャ
    CreateOffscreenTexture(device, srvManager, width, height,
        sceneTexture_, sceneRtvHeap_, sceneRtvHandle_, sceneSrvIndex_);

    // 水平パス中間テクスチャ
    CreateOffscreenTexture(device, srvManager, width, height,
        intermediateTexture_, intermediateRtvHeap_, intermediateRtvHandle_, intermediateSrvIndex_);

    // 深度バッファ SRV（R24G8_TYPELESS → R24_UNORM_X8_TYPELESS で深度チャンネルを読む）
    depthSrvIndex_ = srvManager->Allocate();
    srvManager->CreateSRVforDepthTexture(depthSrvIndex_, dxCommon->GetDepthStencilResource());

    // ブラー用定数バッファ（H と V の 2 スロット × 256 バイト）
    cbResource_ = dxCommon->CreateBufferResource(512);
    void* mapped = nullptr;
    cbResource_->Map(0, nullptr, &mapped);
    cbH_ = reinterpret_cast<FilterParams*>(mapped);
    cbV_ = reinterpret_cast<FilterParams*>(reinterpret_cast<uint8_t*>(mapped) + 256);

    cbH_->texelSizeX = 1.0f / float(width);
    cbH_->texelSizeY = 1.0f / float(height);
    cbH_->dirX = 1.0f; cbH_->dirY = 0.0f;

    cbV_->texelSizeX = 1.0f / float(width);
    cbV_->texelSizeY = 1.0f / float(height);
    cbV_->dirX = 0.0f; cbV_->dirY = 1.0f;

    // アウトライン用定数バッファ（1 スロット × 256 バイト）
    outlineCbResource_ = dxCommon->CreateBufferResource(256);
    void* outlineMapped = nullptr;
    outlineCbResource_->Map(0, nullptr, &outlineMapped);
    outlineCb_ = reinterpret_cast<OutlineParams*>(outlineMapped);

    outlineCb_->texelSizeX  = 1.0f / float(width);
    outlineCb_->texelSizeY  = 1.0f / float(height);
    outlineCb_->threshold   = 0.05f;
    outlineCb_->edgeStrength = 5.0f;
    outlineCb_->outlineR    = 0.0f;
    outlineCb_->outlineG    = 0.0f;
    outlineCb_->outlineB    = 0.0f;
    outlineCb_->outlineA    = 1.0f;
    outlineCb_->depthScale  = 100.0f;

    // ラジアルブラー用定数バッファ（1 スロット × 256 バイト）
    radialBlurCbResource_ = dxCommon->CreateBufferResource(256);
    void* radialMapped = nullptr;
    radialBlurCbResource_->Map(0, nullptr, &radialMapped);
    radialBlurCb_ = reinterpret_cast<RadialBlurParams*>(radialMapped);
    radialBlurCb_->centerX     = 0.5f;
    radialBlurCb_->centerY     = 0.5f;
    radialBlurCb_->strength    = 0.1f;
    radialBlurCb_->sampleCount = 16;

    // ----- ブラー用 Root Signature（b0 + t0）-----
    D3D12_DESCRIPTOR_RANGE srvRange0 = {};
    srvRange0.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange0.NumDescriptors                    = 1;
    srvRange0.BaseShaderRegister                = 0;
    srvRange0.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER blurParams[2] = {};
    blurParams[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    blurParams[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    blurParams[0].Descriptor.ShaderRegister = 0;
    blurParams[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    blurParams[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    blurParams[1].DescriptorTable.pDescriptorRanges   = &srvRange0;
    blurParams[1].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC blurSigDesc = {};
    blurSigDesc.NumParameters    = 2;
    blurSigDesc.pParameters      = blurParams;
    blurSigDesc.NumStaticSamplers = 1;
    blurSigDesc.pStaticSamplers  = &sampler;
    blurSigDesc.Flags            = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&blurSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    // ----- 深度アウトライン用 Root Signature（b0 + t0 + t1）-----
    D3D12_DESCRIPTOR_RANGE srvRange1 = {};
    srvRange1.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange1.NumDescriptors                    = 1;
    srvRange1.BaseShaderRegister                = 1;  // t1
    srvRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER outlineParams[3] = {};
    outlineParams[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    outlineParams[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    outlineParams[0].Descriptor.ShaderRegister = 0;
    outlineParams[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    outlineParams[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    outlineParams[1].DescriptorTable.pDescriptorRanges   = &srvRange0;
    outlineParams[1].DescriptorTable.NumDescriptorRanges = 1;
    outlineParams[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    outlineParams[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    outlineParams[2].DescriptorTable.pDescriptorRanges   = &srvRange1;
    outlineParams[2].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_ROOT_SIGNATURE_DESC outlineSigDesc = {};
    outlineSigDesc.NumParameters     = 3;
    outlineSigDesc.pParameters       = outlineParams;
    outlineSigDesc.NumStaticSamplers = 1;
    outlineSigDesc.pStaticSamplers   = &sampler;
    outlineSigDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> outlineSigBlob, outlineErrBlob;
    hr = D3D12SerializeRootSignature(&outlineSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &outlineSigBlob, &outlineErrBlob);
    assert(SUCCEEDED(hr));
    hr = device->CreateRootSignature(0, outlineSigBlob->GetBufferPointer(), outlineSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&outlineRootSignature_));
    assert(SUCCEEDED(hr));

    // ----- シェーダーコンパイル -----
    IDxcBlob* vsBlob           = dxCommon->CompileShader(L"Resources/shaders/postprocess/FullscreenVS.hlsl",     L"vs_6_0");
    IDxcBlob* boxPsBlob        = dxCommon->CompileShader(L"Resources/shaders/postprocess/KernelFilterPS.hlsl",   L"ps_6_0");
    IDxcBlob* gaussianPsBlob   = dxCommon->CompileShader(L"Resources/shaders/postprocess/GaussianFilterPS.hlsl", L"ps_6_0");
    IDxcBlob* prewittPsBlob    = dxCommon->CompileShader(L"Resources/shaders/postprocess/PrewittEdgePS.hlsl",    L"ps_6_0");
    IDxcBlob* depthOlPsBlob    = dxCommon->CompileShader(L"Resources/shaders/postprocess/DepthOutlinePS.hlsl",   L"ps_6_0");
    IDxcBlob* radialBlurPsBlob = dxCommon->CompileShader(L"Resources/shaders/postprocess/RadialBlurPS.hlsl",     L"ps_6_0");

    // ----- 共通 PSO ベース設定 -----
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    D3D12_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rastDesc.CullMode = D3D12_CULL_MODE_NONE;
    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS                  = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.BlendState          = blendDesc;
    psoDesc.RasterizerState     = rastDesc;
    psoDesc.DepthStencilState   = depthDesc;
    psoDesc.SampleMask          = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets    = 1;
    psoDesc.RTVFormats[0]       = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count    = 1;

    // Box 用 PSO
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.PS = { boxPsBlob->GetBufferPointer(), boxPsBlob->GetBufferSize() };
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&boxPso_));
    assert(SUCCEEDED(hr));

    // Gaussian 用 PSO
    psoDesc.PS = { gaussianPsBlob->GetBufferPointer(), gaussianPsBlob->GetBufferSize() };
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gaussianPso_));
    assert(SUCCEEDED(hr));

    // Prewitt 用 PSO（rootSignature_ と同じ b0+t0 ルートシグネチャを使用）
    psoDesc.PS = { prewittPsBlob->GetBufferPointer(), prewittPsBlob->GetBufferSize() };
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&prewittPso_));
    assert(SUCCEEDED(hr));

    // 深度アウトライン用 PSO（outlineRootSignature_: b0+t0+t1）
    psoDesc.pRootSignature = outlineRootSignature_.Get();
    psoDesc.PS = { depthOlPsBlob->GetBufferPointer(), depthOlPsBlob->GetBufferSize() };
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&depthOutlinePso_));
    assert(SUCCEEDED(hr));

    // ラジアルブラー用 PSO（rootSignature_ と同じ b0+t0）
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.PS = { radialBlurPsBlob->GetBufferPointer(), radialBlurPsBlob->GetBufferSize() };
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&radialBlurPso_));
    assert(SUCCEEDED(hr));

    RebuildKernel();
}

// =====================================================
// Finalize
// =====================================================

void ImageFilter::Finalize()
{
    if (radialBlurCb_) {
        radialBlurCbResource_->Unmap(0, nullptr);
        radialBlurCb_ = nullptr;
    }
    radialBlurCbResource_.Reset();

    if (outlineCb_) {
        outlineCbResource_->Unmap(0, nullptr);
        outlineCb_ = nullptr;
    }
    outlineCbResource_.Reset();

    if (cbH_) {
        cbResource_->Unmap(0, nullptr);
        cbH_ = cbV_ = nullptr;
    }
    cbResource_.Reset();

    radialBlurPso_.Reset();
    depthOutlinePso_.Reset();
    outlineRootSignature_.Reset();
    prewittPso_.Reset();
    gaussianPso_.Reset();
    boxPso_.Reset();
    rootSignature_.Reset();
    intermediateRtvHeap_.Reset();
    intermediateTexture_.Reset();
    sceneRtvHeap_.Reset();
    sceneTexture_.Reset();
}

// =====================================================
// BeginScene / EndScene
// =====================================================

void ImageFilter::BeginScene()
{
    auto* cmd = dxCommon_->GetCommandList();
    if (!isSceneFirstFrame_) {
        Barrier(cmd, sceneTexture_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    isSceneFirstFrame_ = false;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();
    cmd->OMSetRenderTargets(1, &sceneRtvHandle_, FALSE, &dsv);
    constexpr float kClear[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    cmd->ClearRenderTargetView(sceneRtvHandle_, kClear, 0, nullptr);

    D3D12_VIEWPORT vp      = { 0.f, 0.f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.f, 1.f };
    D3D12_RECT     scissor = { 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
}

void ImageFilter::EndScene()
{
    Barrier(dxCommon_->GetCommandList(), sceneTexture_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

// =====================================================
// Apply
// =====================================================

void ImageFilter::Apply(SrvManager* srvManager)
{
    auto* cmd = dxCommon_->GetCommandList();

    D3D12_VIEWPORT vp      = { 0.f, 0.f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.f, 1.f };
    D3D12_RECT     scissor = { 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    D3D12_CPU_DESCRIPTOR_HANDLE backRtv = dxCommon_->GetCurrentBackBufferHandle();

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ----- Prewitt エッジ検出（輝度ベース）: シングルパス -----
    if (mode_ == Mode::PrewittEdge) {
        cmd->SetGraphicsRootSignature(rootSignature_.Get());
        cmd->SetPipelineState(prewittPso_.Get());
        cmd->OMSetRenderTargets(1, &backRtv, FALSE, nullptr);
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &scissor);
        cmd->SetGraphicsRootConstantBufferView(0, outlineCbResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(sceneSrvIndex_));
        cmd->DrawInstanced(3, 1, 0, 0);
        return;
    }

    // ----- 深度ベースアウトライン: シングルパス、深度バリア付き -----
    if (mode_ == Mode::DepthOutline) {
        auto* depthRes = dxCommon_->GetDepthStencilResource();
        Barrier(cmd, depthRes,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        cmd->SetGraphicsRootSignature(outlineRootSignature_.Get());
        cmd->SetPipelineState(depthOutlinePso_.Get());
        cmd->OMSetRenderTargets(1, &backRtv, FALSE, nullptr);
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &scissor);
        cmd->SetGraphicsRootConstantBufferView(0, outlineCbResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(sceneSrvIndex_));
        cmd->SetGraphicsRootDescriptorTable(2, srvManager->GetGPUDescriptorHandle(depthSrvIndex_));
        cmd->DrawInstanced(3, 1, 0, 0);

        Barrier(cmd, depthRes,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        return;
    }

    // ----- ラジアルブラー: シングルパス -----
    if (mode_ == Mode::RadialBlur) {
        cmd->SetGraphicsRootSignature(rootSignature_.Get());
        cmd->SetPipelineState(radialBlurPso_.Get());
        cmd->OMSetRenderTargets(1, &backRtv, FALSE, nullptr);
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &scissor);
        cmd->SetGraphicsRootConstantBufferView(0, radialBlurCbResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(sceneSrvIndex_));
        cmd->DrawInstanced(3, 1, 0, 0);
        return;
    }

    // ----- Box / Gaussian: 水平→垂直の 2 パス -----
    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->SetPipelineState(mode_ == Mode::Box ? boxPso_.Get() : gaussianPso_.Get());

    // Pass 1: 水平（sceneTexture → intermediateTexture）
    if (!isIntermediateFirstFrame_) {
        Barrier(cmd, intermediateTexture_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    isIntermediateFirstFrame_ = false;

    cmd->OMSetRenderTargets(1, &intermediateRtvHandle_, FALSE, nullptr);
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
    cmd->SetGraphicsRootConstantBufferView(0, cbResource_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(sceneSrvIndex_));
    cmd->DrawInstanced(3, 1, 0, 0);

    Barrier(cmd, intermediateTexture_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Pass 2: 垂直（intermediateTexture → backbuffer）
    cmd->OMSetRenderTargets(1, &backRtv, FALSE, nullptr);
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
    cmd->SetGraphicsRootConstantBufferView(0, cbResource_->GetGPUVirtualAddress() + 256);
    cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(intermediateSrvIndex_));
    cmd->DrawInstanced(3, 1, 0, 0);
}

// =====================================================
// カーネル重みの計算（Box / Gaussian のみ）
// =====================================================

void ImageFilter::RebuildKernel()
{
    if (!cbH_) return;
    if (mode_ == Mode::PrewittEdge || mode_ == Mode::DepthOutline || mode_ == Mode::RadialBlur) return;

    int   r        = 1;
    float weights[17] = {};

    if (mode_ == Mode::Box) {
        r = std::clamp(boxRadius_, 0, 8);
        float w = (r == 0) ? 1.0f : 1.0f / float(2 * r + 1);
        for (int i = 0; i <= 2 * r; ++i) weights[i] = w;

    } else { // Gaussian
        float sigma = (std::max)(gaussianSigma_, 0.01f);
        r = std::clamp(static_cast<int>(sigma * 3.0f), 1, 8);
        float s2    = 2.0f * sigma * sigma;
        float total = 0.0f;
        for (int i = 0; i <= 2 * r; ++i) {
            float k = float(i - r);
            weights[i] = std::exp(-(k * k) / s2);
            total += weights[i];
        }
        for (int i = 0; i <= 2 * r; ++i) weights[i] /= total;
    }

    cbH_->radius = r;
    cbV_->radius = r;
    for (int i = 0; i <= 2 * r; ++i) {
        cbH_->kernel[i] = weights[i];
        cbV_->kernel[i] = weights[i];
    }
}

void ImageFilter::SetRadius(int r)
{
    boxRadius_ = r;
    if (mode_ == Mode::Box) RebuildKernel();
}

void ImageFilter::SetSigma(float s)
{
    gaussianSigma_ = s;
    if (mode_ == Mode::Gaussian) RebuildKernel();
}
