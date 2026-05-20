#include "ModelCommon.h"
#include <cassert>

using namespace Microsoft::WRL;

// シャドウパス用ラスタライザバイアス（セルフシャドウ防止）
// kShadowMapSize は ShadowManager::kShadowMapSize と一致させること
static constexpr INT   kShadowDepthBias       = 10;
static constexpr float kShadowSlopeScaledBias = 1.0f;

void ModelCommon::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // =====================================================
    // 通常描画用 Root Signature
    // スロット 0 (PS, b0) : マテリアル
    // スロット 1 (VS, b0) : 変換行列
    // スロット 2 (PS, t0) : テクスチャ SRV
    // スロット 3 (PS, b1) : 平行光源
    // スロット 4 (PS, t1) : シャドウマップ SRV
    // =====================================================
    D3D12_DESCRIPTOR_RANGE texRange[1] = {};
    texRange[0].BaseShaderRegister                = 0; // t0
    texRange[0].NumDescriptors                    = 1;
    texRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    texRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE shadowRange[1] = {};
    shadowRange[0].BaseShaderRegister                = 1; // t1
    shadowRange[0].NumDescriptors                    = 1;
    shadowRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE cubemapRange[1] = {};
    cubemapRange[0].BaseShaderRegister                = 2; // t2
    cubemapRange[0].NumDescriptors                    = 1;
    cubemapRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    cubemapRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[6] = {};
    // 0: マテリアル (PS, b0)
    rootParameters[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    // 1: 変換行列 (VS, b0)
    rootParameters[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    // 2: テクスチャ (PS, t0)
    rootParameters[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges   = texRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    // 3: 平行光源 (PS, b1)
    rootParameters[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;
    // 4: シャドウマップ (PS, t1)
    rootParameters[4].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].DescriptorTable.pDescriptorRanges   = shadowRange;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
    // 5: TextureCube (PS, t2) ― 天球キューブマップ用
    rootParameters[5].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[5].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[5].DescriptorTable.pDescriptorRanges   = cubemapRange;
    rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;

    // 静的サンプラー（s0: 通常テクスチャ、s1: シャドウマップ比較用）
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
    // s0: 通常テクスチャ用リニアフィルタ
    staticSamplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ShaderRegister   = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    // s1: シャドウマップ PCF 比較サンプラー
    staticSamplers[1].Filter             = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    staticSamplers[1].AddressU           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressV           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressW           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].BorderColor        = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSamplers[1].ComparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    staticSamplers[1].MaxLOD             = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister     = 1; // s1
    staticSamplers[1].ShaderVisibility   = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc {};
    rsDesc.Flags           = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rsDesc.pParameters     = rootParameters;
    rsDesc.NumParameters   = 6;
    rsDesc.pStaticSamplers = staticSamplers;
    rsDesc.NumStaticSamplers = _countof(staticSamplers);

    ComPtr<ID3DBlob> signatureBlob, errorBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    // 頂点レイアウト（共通）
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    IDxcBlob* vsBlob = dxCommon_->CompileShader(L"Resources/shaders/object3d/Object3dVS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon_->CompileShader(L"Resources/shaders/object3d/Object3dPS.hlsl", L"ps_6_0");

    // =====================================================
    // 通常描画用 PSO（ブレンドモード別）
    // =====================================================
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
    psoDesc.pRootSignature        = rootSignature_.Get();
    psoDesc.InputLayout           = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.VS                    = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS                    = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.RasterizerState.CullMode  = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode  = D3D12_FILL_MODE_SOLID;
    psoDesc.DepthStencilState.DepthEnable    = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DSVFormat             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask            = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.SampleDesc.Count      = 1;

    for (int i = 0; i < BlendMode_Count; ++i) {
        D3D12_RENDER_TARGET_BLEND_DESC& b = psoDesc.BlendState.RenderTarget[0];
        b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.BlendEnable = TRUE;
        switch (i) {
        case BlendMode_None:
            b.BlendEnable = FALSE; break;
        case BlendMode_Alpha:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            b.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            b.BlendOp = D3D12_BLEND_OP_ADD; break;
        case BlendMode_Add:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            b.DestBlend = D3D12_BLEND_ONE;
            b.BlendOp = D3D12_BLEND_OP_ADD; break;
        case BlendMode_Subtract:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            b.DestBlend = D3D12_BLEND_ONE;
            b.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; break;
        case BlendMode_Multiply:
            b.SrcBlend = D3D12_BLEND_DEST_COLOR;
            b.DestBlend = D3D12_BLEND_ZERO;
            b.BlendOp = D3D12_BLEND_OP_ADD; break;
        }

        b.SrcBlendAlpha  = D3D12_BLEND_ONE;
        b.DestBlendAlpha = D3D12_BLEND_ZERO;
        b.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineStates_[i]));
    }

    // =====================================================
    // シャドウパス用 Root Signature（CBV 1つ: TransformationMatrix）
    // =====================================================
    D3D12_ROOT_PARAMETER shadowParam[1] = {};
    shadowParam[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    shadowParam[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    shadowParam[0].Descriptor.ShaderRegister = 0; // VS, b0

    D3D12_ROOT_SIGNATURE_DESC shadowRsDesc {};
    shadowRsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    shadowRsDesc.pParameters   = shadowParam;
    shadowRsDesc.NumParameters = _countof(shadowParam);

    ComPtr<ID3DBlob> shadowSigBlob, shadowErrBlob;
    D3D12SerializeRootSignature(&shadowRsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &shadowSigBlob, &shadowErrBlob);
    device->CreateRootSignature(0, shadowSigBlob->GetBufferPointer(), shadowSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&shadowRootSignature_));

    // =====================================================
    // シャドウパス用 PSO（深度のみ書き込み）
    // =====================================================
    IDxcBlob* shadowVsBlob = dxCommon_->CompileShader(L"Resources/shaders/object3d/ShadowVS.hlsl", L"vs_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc {};
    shadowPsoDesc.pRootSignature        = shadowRootSignature_.Get();
    shadowPsoDesc.InputLayout           = { inputElementDescs, _countof(inputElementDescs) };
    shadowPsoDesc.VS                    = { shadowVsBlob->GetBufferPointer(), shadowVsBlob->GetBufferSize() };
    shadowPsoDesc.PS                    = { nullptr, 0 }; // PSなし（深度のみ）
    shadowPsoDesc.RasterizerState.CullMode          = D3D12_CULL_MODE_BACK;
    shadowPsoDesc.RasterizerState.FillMode          = D3D12_FILL_MODE_SOLID;
    shadowPsoDesc.RasterizerState.DepthBias            = kShadowDepthBias;
    shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = kShadowSlopeScaledBias;
    shadowPsoDesc.DepthStencilState.DepthEnable     = TRUE;
    shadowPsoDesc.DepthStencilState.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ALL;
    shadowPsoDesc.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    shadowPsoDesc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
    shadowPsoDesc.NumRenderTargets       = 0; // RTVなし
    shadowPsoDesc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadowPsoDesc.SampleMask             = D3D12_DEFAULT_SAMPLE_MASK;
    shadowPsoDesc.SampleDesc.Count       = 1;

    device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&shadowPipelineState_));
}

void ModelCommon::CommonDrawSettings(BlendMode blendMode)
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(graphicsPipelineStates_[blendMode].Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ModelCommon::BeginShadowPass()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    commandList->SetGraphicsRootSignature(shadowRootSignature_.Get());
    commandList->SetPipelineState(shadowPipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ModelCommon::EndShadowPass()
{
    // 通常描画への切り替えは次の CommonDrawSettings() で行う
}
