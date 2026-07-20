#include "PostProcessManager.h"
#include "ImGuiManager.h"
#include <cassert>
#include <cstring>

using namespace Microsoft::WRL;

static constexpr UINT  kWidth  = 1280;
static constexpr UINT  kHeight = 720;
static constexpr DXGI_FORMAT kColorFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
static constexpr DXGI_FORMAT kRTVFmt   = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

PostProcessManager* PostProcessManager::GetInstance()
{
    static PostProcessManager instance;
    return &instance;
}

// =====================================================
// 初期化
// =====================================================

void PostProcessManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon->GetDevice();

    CreateOffScreenRT(device);
    CreatePSO(dxCommon);

    // カラーRT SRV
    colorSrvIndex_ = srvManager->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format                        = kColorFmt;
    srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels           = 1;
    device->CreateShaderResourceView(colorRT_.Get(), &srvDesc,
        srvManager->GetCPUDescriptorHandle(colorSrvIndex_));

    // 深度テクスチャ SRV (R24_UNORM_X8_TYPELESS で深度値を読む)
    depthSrvIndex_ = srvManager->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc {};
    depthSrvDesc.Format                  = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.Texture2D.MipLevels     = 1;
    device->CreateShaderResourceView(dxCommon->GetDepthStencilResource(), &depthSrvDesc,
        srvManager->GetCPUDescriptorHandle(depthSrvIndex_));

    // 定数バッファ
    D3D12_HEAP_PROPERTIES heapProps { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC   cbDesc {};
    cbDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width            = (sizeof(PostProcessParams) + 255) & ~255;
    cbDesc.Height           = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels        = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constBuffer_));
    assert(SUCCEEDED(hr));
    constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedParams_));
}

void PostProcessManager::Finalize()
{
    if (constBuffer_ && mappedParams_) {
        constBuffer_->Unmap(0, nullptr);
        mappedParams_ = nullptr;
    }
}

// =====================================================
// オフスクリーン RT 作成
// =====================================================

void PostProcessManager::CreateOffScreenRT(ID3D12Device* device)
{
    D3D12_HEAP_PROPERTIES defaultHeap { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC texDesc {};
    texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width            = kWidth;
    texDesc.Height           = kHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels        = 1;
    texDesc.Format           = kColorFmt;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    D3D12_CLEAR_VALUE clearVal {};
    clearVal.Format   = kRTVFmt;
    std::memcpy(clearVal.Color, clearColor, sizeof(clearColor));

    HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal,
        IID_PPV_ARGS(&colorRT_));
    assert(SUCCEEDED(hr));

    // RTV ヒープ
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc {};
    rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 1;
    hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
    assert(SUCCEEDED(hr));

    rtvHandle_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format        = kRTVFmt;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(colorRT_.Get(), &rtvDesc, rtvHandle_);

    colorRTInRenderTarget_ = true;
}

// =====================================================
// PSO 作成
// =====================================================

void PostProcessManager::CreatePSO(DirectXCommon* dxCommon)
{
    ID3D12Device* device = dxCommon->GetDevice();

    // ルートシグネチャ
    // [0] b0: 定数バッファ
    // [1] t0: カラーRT SRV
    // [2] t1: 深度 SRV
    D3D12_DESCRIPTOR_RANGE colorRange {};
    colorRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    colorRange.NumDescriptors                    = 1;
    colorRange.BaseShaderRegister                = 0; // t0
    colorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE depthRange {};
    depthRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors                    = 1;
    depthRange.BaseShaderRegister                = 1; // t1
    depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParams[3] {};
    rootParams[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParams[0].Descriptor.ShaderRegister = 0; // b0

    rootParams[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParams[1].DescriptorTable.pDescriptorRanges   = &colorRange;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;

    rootParams[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParams[2].DescriptorTable.pDescriptorRanges   = &depthRange;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;

    // s0: リニアクランプ
    D3D12_STATIC_SAMPLER_DESC samplers[2] {};
    samplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].MaxLOD           = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister   = 0; // s0
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // s1: ポイントクランプ（深度読み取り用）
    samplers[1].Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplers[1].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    samplers[1].MaxLOD           = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister   = 1; // s1
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc {};
    rsDesc.NumParameters     = _countof(rootParams);
    rsDesc.pParameters       = rootParams;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers   = samplers;
    rsDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));

    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    // シェーダーコンパイル
    IDxcBlob* vsBlob = dxCommon->CompileShader(
        L"Resources/shaders/postProcess/PostProcess.VS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon->CompileShader(
        L"Resources/shaders/postProcess/PostProcess.PS.hlsl", L"ps_6_0");
    assert(vsBlob && psBlob);

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
    psoDesc.pRootSignature        = rootSignature_.Get();
    psoDesc.VS                    = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS                    = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0].BlendEnable            = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable       = FALSE;
    psoDesc.DepthStencilState.DepthEnable         = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask      = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.InputLayout.pInputElementDescs        = nullptr;
    psoDesc.InputLayout.NumElements               = 0;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = kRTVFmt;
    psoDesc.SampleDesc.Count      = 1;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
    assert(SUCCEEDED(hr));

    vsBlob->Release();
    psBlob->Release();
}

// =====================================================
// BeginCapture
// =====================================================

void PostProcessManager::BeginCapture(ID3D12GraphicsCommandList* cmd, DirectXCommon* dxCommon)
{
    if (!clearedThisFrame_) {
        // 初回: RT状態でなければバリア遷移
        if (!colorRTInRenderTarget_) {
            D3D12_RESOURCE_BARRIER b {};
            b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = colorRT_.Get();
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            b.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmd->ResourceBarrier(1, &b);
            colorRTInRenderTarget_ = true;
        }
        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        cmd->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
        clearedThisFrame_ = true;
    }

    // オフスクリーン RT + メイン深度バッファをセット
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon->GetDsvHandle();
    cmd->OMSetRenderTargets(1, &rtvHandle_, FALSE, &dsv);

    D3D12_VIEWPORT vp { 0, 0, (float)kWidth, (float)kHeight, 0.0f, 1.0f };
    D3D12_RECT     scissor { 0, 0, (LONG)kWidth, (LONG)kHeight };
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);
}

// =====================================================
// Apply（ポストプロセスをスワップチェーンへ出力）
// =====================================================

void PostProcessManager::Apply(
    ID3D12GraphicsCommandList* cmd,
    DirectXCommon* dxCommon,
    SrvManager* srvManager)
{
    clearedThisFrame_ = false;

    // カラーRT: RT → SRV
    {
        D3D12_RESOURCE_BARRIER b {};
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = colorRT_.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        b.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
        colorRTInRenderTarget_ = false;
    }

    // 深度: DEPTH_WRITE → PIXEL_SHADER_RESOURCE
    {
        D3D12_RESOURCE_BARRIER b {};
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = dxCommon->GetDepthStencilResource();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        b.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
    }

    // パラメータ更新
    params_.randomTime += 1.0f / 60.0f;
    std::memcpy(mappedParams_, &params_, sizeof(PostProcessParams));

    // スワップチェーン RTV へ切り替え（深度なし）
    D3D12_CPU_DESCRIPTOR_HANDLE swapRTV = dxCommon->GetCurrentBackBufferHandle();
    cmd->OMSetRenderTargets(1, &swapRTV, FALSE, nullptr);
    D3D12_VIEWPORT vp { 0, 0, (float)kWidth, (float)kHeight, 0.0f, 1.0f };
    D3D12_RECT     scissor { 0, 0, (LONG)kWidth, (LONG)kHeight };
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &scissor);

    // PSO セット
    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->SetPipelineState(pso_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 定数バッファ / SRV バインド
    cmd->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(colorSrvIndex_));
    cmd->SetGraphicsRootDescriptorTable(2, srvManager->GetGPUDescriptorHandle(depthSrvIndex_));

    // フルスクリーン三角形描画（頂点バッファ不要）
    cmd->DrawInstanced(3, 1, 0, 0);

    // 深度: PIXEL_SHADER_RESOURCE → DEPTH_WRITE（次フレームの PreDraw 用）
    {
        D3D12_RESOURCE_BARRIER b {};
        b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource   = dxCommon->GetDepthStencilResource();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
    }
}

// =====================================================
// エフェクト ON/OFF 切り替え
// =====================================================

float* PostProcessManager::GetIntensityPtr(PostEffectType type)
{
    switch (type) {
    case PostEffectType::Grayscale:    return &params_.grayscaleIntensity;
    case PostEffectType::Vignette:     return &params_.vignetteIntensity;
    case PostEffectType::BoxFilter:    return &params_.boxFilterIntensity;
    case PostEffectType::Gaussian:     return &params_.gaussianIntensity;
    case PostEffectType::LumOutline:   return &params_.lumOutlineIntensity;
    case PostEffectType::DepthOutline: return &params_.depthOutlineIntensity;
    case PostEffectType::RadialBlur:   return &params_.radialBlurIntensity;
    case PostEffectType::Dissolve:     return &params_.dissolveThreshold;
    case PostEffectType::Random:       return &params_.randomIntensity;
    default:                           return nullptr;
    }
}

void PostProcessManager::ToggleEffect(PostEffectType type)
{
    float* intensity = GetIntensityPtr(type);
    if (!intensity) {
        return;
    }

    size_t index = static_cast<size_t>(type);
    if (*intensity > 0.001f) {
        // ON → OFF（現在の強度を退避してから0にする）
        effectSavedIntensity_[index] = *intensity;
        *intensity                   = 0.0f;
    } else {
        // OFF → ON（退避しておいた強度を復元）
        *intensity = effectSavedIntensity_[index];
    }
}

// =====================================================
// ImGui
// =====================================================

void PostProcessManager::ShowImGui()
{
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("Post Process")) {
        return;
    }

    ImGui::Indent();

    if (ImGui::TreeNodeEx("Grayscale", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##gs", &params_.grayscaleIntensity, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Vignetting", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##vg",  &params_.vignetteIntensity,  0.0f, 1.0f);
        ImGui::SliderFloat("Radius##vg",     &params_.vignetteRadius,     0.0f, 1.0f);
        ImGui::SliderFloat("Softness##vg",   &params_.vignetteSoftness,   0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Box Filter", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##bf", &params_.boxFilterIntensity, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Gaussian Filter", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##gf", &params_.gaussianIntensity, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Luminance Based Outline", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##lo",  &params_.lumOutlineIntensity,  0.0f, 1.0f);
        ImGui::SliderFloat("Threshold##lo",  &params_.lumOutlineThreshold,  0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Depth Based Outline", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##do",  &params_.depthOutlineIntensity,  0.0f, 1.0f);
        ImGui::SliderFloat("Threshold##do",  &params_.depthOutlineThreshold,  0.0001f, 0.1f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Radial Blur", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##rb", &params_.radialBlurIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Width##rb",     &params_.radialBlurWidth,     0.0f, 0.1f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Dissolve", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Threshold##dv", &params_.dissolveThreshold, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Random", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Intensity##rn", &params_.randomIntensity, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::Button("Reset All##pp")) {
        params_ = PostProcessParams{};
    }

    ImGui::Unindent();
#endif
}
