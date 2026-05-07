#include "SkeletonDebugRenderer.h"
#ifdef USE_IMGUI

#include "Camera.h"
#include <cmath>
#include <cassert>

using namespace Microsoft::WRL;

static constexpr float kPi = 3.14159265358979323846f;

void SkeletonDebugRenderer::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ---- Root Signature: 20 root constants (16=WVP + 4=color) ----
    D3D12_ROOT_PARAMETER rp{};
    rp.ParameterType                  = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rp.ShaderVisibility               = D3D12_SHADER_VISIBILITY_VERTEX;
    rp.Constants.ShaderRegister      = 0;
    rp.Constants.RegisterSpace       = 0;
    rp.Constants.Num32BitValues      = 20;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters   = &rp;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    // ---- Shaders ----
    IDxcBlob* vs = dxCommon_->CompileShader(L"Resources/shaders/debug/SkeletonDebugVS.hlsl", L"vs_6_0");
    IDxcBlob* ps = dxCommon_->CompileShader(L"Resources/shaders/debug/SkeletonDebugPS.hlsl", L"ps_6_0");

    // ---- Input Layout ----
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ---- Base PSO ----
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature                   = rootSignature_.Get();
    psoDesc.InputLayout                      = { inputLayout, 1 };
    psoDesc.VS                               = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS                               = { ps->GetBufferPointer(), ps->GetBufferSize() };
    psoDesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode         = D3D12_FILL_MODE_SOLID;
    psoDesc.DepthStencilState.DepthEnable    = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DSVFormat                        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets                 = 1;
    psoDesc.RTVFormats[0]                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleMask                       = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.SampleDesc.Count                 = 1;

    auto& b = psoDesc.BlendState.RenderTarget[0];
    b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    b.BlendEnable           = TRUE;
    b.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    b.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    b.BlendOp               = D3D12_BLEND_OP_ADD;
    b.SrcBlendAlpha         = D3D12_BLEND_ONE;
    b.DestBlendAlpha        = D3D12_BLEND_ZERO;
    b.BlendOpAlpha          = D3D12_BLEND_OP_ADD;

    // Triangle PSO (joint spheres)
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoTri_));

    // Line PSO (bones)
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoLine_));

    BuildSphere();

    // ---- Dynamic line VB (persistent mapped) ----
    UINT lineVBSize = kMaxBones * 2 * static_cast<UINT>(sizeof(DebugVertex));
    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC vbDesc{};
    vbDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width            = lineVBSize;
    vbDesc.Height           = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels        = 1;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lineVB_));
    lineVB_->Map(0, nullptr, reinterpret_cast<void**>(&lineMapped_));

    lineVBV_.BufferLocation = lineVB_->GetGPUVirtualAddress();
    lineVBV_.SizeInBytes    = lineVBSize;
    lineVBV_.StrideInBytes  = sizeof(DebugVertex);
}

void SkeletonDebugRenderer::BuildSphere()
{
    std::vector<DebugVertex> verts;
    verts.reserve((kStacks + 1) * (kSlices + 1));
    for (int i = 0; i <= kStacks; ++i) {
        float phi = kPi * i / kStacks;
        for (int j = 0; j <= kSlices; ++j) {
            float theta = 2.0f * kPi * j / kSlices;
            verts.push_back({
                std::sin(phi) * std::cos(theta) * kSphereRadius,
                std::cos(phi)                   * kSphereRadius,
                std::sin(phi) * std::sin(theta) * kSphereRadius,
                1.0f
            });
        }
    }

    std::vector<uint16_t> indices;
    indices.reserve(kStacks * kSlices * 6);
    for (int i = 0; i < kStacks; ++i) {
        for (int j = 0; j < kSlices; ++j) {
            auto a = static_cast<uint16_t>(i       * (kSlices + 1) + j);
            auto b = static_cast<uint16_t>(a + 1);
            auto c = static_cast<uint16_t>((i + 1) * (kSlices + 1) + j);
            auto d = static_cast<uint16_t>(c + 1);
            indices.insert(indices.end(), { a, c, b, b, c, d });
        }
    }
    sphereIndexCount_ = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
    auto makeBuffer = [&](size_t size) -> ComPtr<ID3D12Resource> {
        D3D12_RESOURCE_DESC d{};
        d.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Width            = size;
        d.Height           = 1;
        d.DepthOrArraySize = 1;
        d.MipLevels        = 1;
        d.SampleDesc.Count = 1;
        d.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> res;
        dxCommon_->GetDevice()->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
        return res;
    };

    sphereVB_ = makeBuffer(verts.size()   * sizeof(DebugVertex));
    sphereIB_ = makeBuffer(indices.size() * sizeof(uint16_t));

    void* mapped;
    sphereVB_->Map(0, nullptr, &mapped);
    memcpy(mapped, verts.data(),   verts.size()   * sizeof(DebugVertex));
    sphereVB_->Unmap(0, nullptr);

    sphereIB_->Map(0, nullptr, &mapped);
    memcpy(mapped, indices.data(), indices.size() * sizeof(uint16_t));
    sphereIB_->Unmap(0, nullptr);

    sphereVBV_ = { sphereVB_->GetGPUVirtualAddress(),
                   static_cast<UINT>(verts.size() * sizeof(DebugVertex)), sizeof(DebugVertex) };
    sphereIBV_ = { sphereIB_->GetGPUVirtualAddress(),
                   static_cast<UINT>(indices.size() * sizeof(uint16_t)), DXGI_FORMAT_R16_UINT };
}

void SkeletonDebugRenderer::Draw(const Skeleton& skeleton, const Matrix4x4& worldMatrix, Camera* camera)
{
    if (!camera || skeleton.joints.empty()) return;

    ID3D12GraphicsCommandList* cmd = dxCommon_->GetCommandList();
    Matrix4x4 vp = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

    // joint local pos → world pos (row-vector convention: p_world = p_local * worldMatrix)
    auto toWorld = [&](const Matrix4x4& m) -> Vector3 {
        float lx = m.m[3][0], ly = m.m[3][1], lz = m.m[3][2];
        return {
            worldMatrix.m[0][0] * lx + worldMatrix.m[1][0] * ly + worldMatrix.m[2][0] * lz + worldMatrix.m[3][0],
            worldMatrix.m[0][1] * lx + worldMatrix.m[1][1] * ly + worldMatrix.m[2][1] * lz + worldMatrix.m[3][1],
            worldMatrix.m[0][2] * lx + worldMatrix.m[1][2] * ly + worldMatrix.m[2][2] * lz + worldMatrix.m[3][2]
        };
    };

    cmd->SetGraphicsRootSignature(rootSignature_.Get());

    // ---- ボーン（ライン）----
    {
        uint32_t vc = 0;
        for (const Joint& joint : skeleton.joints) {
            if (!joint.parent || vc + 2 > kMaxBones * 2) continue;
            Vector3 p0 = toWorld(skeleton.joints[*joint.parent].skeletonSpaceMatrix);
            Vector3 p1 = toWorld(joint.skeletonSpaceMatrix);
            lineMapped_[vc++] = { p0.x, p0.y, p0.z, 1.0f };
            lineMapped_[vc++] = { p1.x, p1.y, p1.z, 1.0f };
        }

        if (vc > 0) {
            DebugCB cb{ vp, { 1.0f, 1.0f, 1.0f, 1.0f } };
            cmd->SetPipelineState(psoLine_.Get());
            cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
            cmd->IASetVertexBuffers(0, 1, &lineVBV_);
            cmd->SetGraphicsRoot32BitConstants(0, 20, &cb, 0);
            cmd->DrawInstanced(vc, 1, 0, 0);
        }
    }

    // ---- ジョイント（球）----
    {
        cmd->SetPipelineState(psoTri_.Get());
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &sphereVBV_);
        cmd->IASetIndexBuffer(&sphereIBV_);

        for (const Joint& joint : skeleton.joints) {
            Vector3 wp = toWorld(joint.skeletonSpaceMatrix);
            Matrix4x4 wvp = Multiply(MakeTranslateMatrix(wp), vp);
            Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

            DebugCB cb{ wvp, color };
            cmd->SetGraphicsRoot32BitConstants(0, 20, &cb, 0);
            cmd->DrawIndexedInstanced(sphereIndexCount_, 1, 0, 0, 0);
        }
    }
}

#endif // USE_IMGUI
