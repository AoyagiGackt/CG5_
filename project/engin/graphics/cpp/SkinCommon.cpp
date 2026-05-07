#include "SkinCommon.h"
#include <cassert>

using namespace Microsoft::WRL;

void SkinCommon::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // =====================================================
    // Root Signature (ModelCommon と同一 + スロット 6 = SkinningPalette)
    // Slot 0 (PS, b0) : Material
    // Slot 1 (VS, b0) : TransformationMatrix
    // Slot 2 (PS, t0) : Texture SRV
    // Slot 3 (PS, b1) : DirectionalLight
    // Slot 4 (PS, t1) : ShadowMap SRV
    // Slot 5 (PS, t2) : TextureCube SRV
    // Slot 6 (VS, b1) : SkinningPalette CBV  ← スキニング専用追加
    // =====================================================
    D3D12_DESCRIPTOR_RANGE texRange[1]{};
    texRange[0].BaseShaderRegister                = 0; // t0
    texRange[0].NumDescriptors                    = 1;
    texRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    texRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE shadowRange[1]{};
    shadowRange[0].BaseShaderRegister                = 1; // t1
    shadowRange[0].NumDescriptors                    = 1;
    shadowRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE cubemapRange[1]{};
    cubemapRange[0].BaseShaderRegister                = 2; // t2
    cubemapRange[0].NumDescriptors                    = 1;
    cubemapRange[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    cubemapRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[7]{};
    // 0: Material (PS, b0)
    rootParameters[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    // 1: TransformationMatrix (VS, b0)
    rootParameters[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    // 2: Texture (PS, t0)
    rootParameters[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges   = texRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    // 3: DirectionalLight (PS, b1)
    rootParameters[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;
    // 4: ShadowMap (PS, t1)
    rootParameters[4].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].DescriptorTable.pDescriptorRanges   = shadowRange;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
    // 5: TextureCube (PS, t2)
    rootParameters[5].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[5].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[5].DescriptorTable.pDescriptorRanges   = cubemapRange;
    rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
    // 6: SkinningPalette (VS, b1)
    rootParameters[6].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[6].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[6].Descriptor.ShaderRegister = 1;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
    staticSamplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ShaderRegister   = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    staticSamplers[1].Filter             = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    staticSamplers[1].AddressU           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressV           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressW           = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].BorderColor        = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSamplers[1].ComparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    staticSamplers[1].MaxLOD             = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister     = 1;
    staticSamplers[1].ShaderVisibility   = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rsDesc.pParameters       = rootParameters;
    rsDesc.NumParameters     = 7;
    rsDesc.pStaticSamplers   = staticSamplers;
    rsDesc.NumStaticSamplers = _countof(staticSamplers);

    ComPtr<ID3DBlob> sigBlob, errBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    // 入力レイアウト（スキニング用: 法線+ボーン情報追加）
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    IDxcBlob* vsBlob = dxCommon_->CompileShader(L"Resources/shaders/skinned/SkinnedVS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon_->CompileShader(L"Resources/shaders/object3d/Object3dPS.hlsl", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature                      = rootSignature_.Get();
    psoDesc.InputLayout                         = { inputLayout, _countof(inputLayout) };
    psoDesc.VS                                  = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS                                  = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.RasterizerState.CullMode            = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode            = D3D12_FILL_MODE_SOLID;
    psoDesc.DepthStencilState.DepthEnable       = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask    = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc         = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DSVFormat                           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets                    = 1;
    psoDesc.RTVFormats[0]                       = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType               = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask                          = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.SampleDesc.Count                    = 1;

    for (int i = 0; i < BlendMode_Count; ++i) {
        D3D12_RENDER_TARGET_BLEND_DESC& b = psoDesc.BlendState.RenderTarget[0];
        b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.BlendEnable = TRUE;
        switch (i) {
        case BlendMode_None:  b.BlendEnable = FALSE; break;
        case BlendMode_Alpha:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA; b.DestBlend = D3D12_BLEND_INV_SRC_ALPHA; b.BlendOp = D3D12_BLEND_OP_ADD; break;
        case BlendMode_Add:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA; b.DestBlend = D3D12_BLEND_ONE; b.BlendOp = D3D12_BLEND_OP_ADD; break;
        case BlendMode_Subtract:
            b.SrcBlend = D3D12_BLEND_SRC_ALPHA; b.DestBlend = D3D12_BLEND_ONE; b.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; break;
        case BlendMode_Multiply:
            b.SrcBlend = D3D12_BLEND_DEST_COLOR; b.DestBlend = D3D12_BLEND_ZERO; b.BlendOp = D3D12_BLEND_OP_ADD; break;
        }
        b.SrcBlendAlpha  = D3D12_BLEND_ONE;
        b.DestBlendAlpha = D3D12_BLEND_ZERO;
        b.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineStates_[i]));
    }
}

void SkinCommon::CommonDrawSettings(BlendMode blendMode)
{
    ID3D12GraphicsCommandList* cmd = dxCommon_->GetCommandList();
    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->SetPipelineState(graphicsPipelineStates_[blendMode].Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
