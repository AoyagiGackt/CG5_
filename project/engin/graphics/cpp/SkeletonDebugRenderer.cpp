#include "SkeletonDebugRenderer.h"
#ifdef USE_IMGUI

#include "Camera.h"
#include <cmath>

using namespace Microsoft::WRL;

static constexpr float kPi = 3.14159265358979323846f;

void SkeletonDebugRenderer::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ---- ルートシグネチャ: 32ビット定数 20個 (16=WVP + 4=カラー) ----
    D3D12_ROOT_PARAMETER rp{};
    rp.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rp.ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    rp.Constants.ShaderRegister = 0;
    rp.Constants.RegisterSpace  = 0;
    rp.Constants.Num32BitValues = 20;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters   = &rp;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    // ---- シェーダー ----
    IDxcBlob* vs = dxCommon_->CompileShader(L"Resources/shaders/debug/SkeletonDebugVS.hlsl", L"vs_6_0");
    IDxcBlob* ps = dxCommon_->CompileShader(L"Resources/shaders/debug/SkeletonDebugPS.hlsl", L"ps_6_0");

    // ---- 入力レイアウト ----
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ---- 三角形 PSO (ボーン・球ともに使用) ----
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
    psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    auto& b = psoDesc.BlendState.RenderTarget[0];
    b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    b.BlendEnable           = TRUE;
    b.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    b.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    b.BlendOp               = D3D12_BLEND_OP_ADD;
    b.SrcBlendAlpha         = D3D12_BLEND_ONE;
    b.DestBlendAlpha        = D3D12_BLEND_ZERO;
    b.BlendOpAlpha          = D3D12_BLEND_OP_ADD;

    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoTri_));

    BuildSphere();

    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
    auto makeBuffer = [&](UINT size) -> ComPtr<ID3D12Resource> {
        D3D12_RESOURCE_DESC d{};
        d.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Width            = size;
        d.Height           = 1;
        d.DepthOrArraySize = 1;
        d.MipLevels        = 1;
        d.SampleDesc.Count = 1;
        d.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> res;
        device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
        return res;
    };

    // ---- ボーン用ビルボードクワッド（1ボーン = 4頂点 + 6インデックス）----
    UINT vbSize  = kMaxBones * 4 * static_cast<UINT>(sizeof(DebugVertex));
    UINT ibSize  = kMaxBones * 6 * static_cast<UINT>(sizeof(uint16_t));

    lineVB_ = makeBuffer(vbSize);
    lineIB_ = makeBuffer(ibSize);
    lineVB_->Map(0, nullptr, reinterpret_cast<void**>(&lineMapped_));
    lineIB_->Map(0, nullptr, reinterpret_cast<void**>(&lineIdxMapped_));

    lineVBV_ = { lineVB_->GetGPUVirtualAddress(), vbSize, sizeof(DebugVertex) };
    lineIBV_ = { lineIB_->GetGPUVirtualAddress(), ibSize, DXGI_FORMAT_R16_UINT };
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
            auto bv = static_cast<uint16_t>(a + 1);
            auto c  = static_cast<uint16_t>((i + 1) * (kSlices + 1) + j);
            auto d  = static_cast<uint16_t>(c + 1);
            indices.insert(indices.end(), { a, c, bv, bv, c, d });
        }
    }
    sphereIndexCount_ = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
    auto makeStatic = [&](size_t size) -> ComPtr<ID3D12Resource> {
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

    sphereVB_ = makeStatic(verts.size()   * sizeof(DebugVertex));
    sphereIB_ = makeStatic(indices.size() * sizeof(uint16_t));

    void* mapped;
    sphereVB_->Map(0, nullptr, &mapped);
    memcpy(mapped, verts.data(), verts.size() * sizeof(DebugVertex));
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

    // ジョイントのローカル座標 → ワールド座標
    auto toWorld = [&](const Matrix4x4& m) -> Vector3 {
        float lx = m.m[3][0], ly = m.m[3][1], lz = m.m[3][2];
        return {
            worldMatrix.m[0][0]*lx + worldMatrix.m[1][0]*ly + worldMatrix.m[2][0]*lz + worldMatrix.m[3][0],
            worldMatrix.m[0][1]*lx + worldMatrix.m[1][1]*ly + worldMatrix.m[2][1]*lz + worldMatrix.m[3][1],
            worldMatrix.m[0][2]*lx + worldMatrix.m[1][2]*ly + worldMatrix.m[2][2]*lz + worldMatrix.m[3][2]
        };
    };

    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->SetPipelineState(psoTri_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const Vector3 camPos = camera->GetTranslate();

    // ---- ボーン（ビルボードクワッド）----
    {
        uint32_t vc = 0, ic = 0;

        for (const Joint& joint : skeleton.joints) {
            if (!joint.parent || vc + 4 > kMaxBones * 4) continue;

            Vector3 p0 = toWorld(skeleton.joints[*joint.parent].skeletonSpaceMatrix);
            Vector3 p1 = toWorld(joint.skeletonSpaceMatrix);

            // 骨の向き
            Vector3 boneDir = Normalize(Subtract(p1, p0));
            // 骨の中点からカメラへの向き
            Vector3 mid     = { (p0.x+p1.x)*0.5f, (p0.y+p1.y)*0.5f, (p0.z+p1.z)*0.5f };
            Vector3 toCamera = Normalize(Subtract(camPos, mid));
            // 垂直方向（ビルボード法線）
            Vector3 perp = Normalize(Cross(boneDir, toCamera));
            float   hw   = kBoneHalfWidth;

            // 4頂点（p0側2, p1側2）
            auto base = static_cast<uint16_t>(vc);
            lineMapped_[vc++] = { p0.x + perp.x*hw, p0.y + perp.y*hw, p0.z + perp.z*hw, 1.f };
            lineMapped_[vc++] = { p0.x - perp.x*hw, p0.y - perp.y*hw, p0.z - perp.z*hw, 1.f };
            lineMapped_[vc++] = { p1.x - perp.x*hw, p1.y - perp.y*hw, p1.z - perp.z*hw, 1.f };
            lineMapped_[vc++] = { p1.x + perp.x*hw, p1.y + perp.y*hw, p1.z + perp.z*hw, 1.f };

            // 2三角形（クワッド）
            lineIdxMapped_[ic++] = base + 0;
            lineIdxMapped_[ic++] = base + 1;
            lineIdxMapped_[ic++] = base + 2;
            lineIdxMapped_[ic++] = base + 0;
            lineIdxMapped_[ic++] = base + 2;
            lineIdxMapped_[ic++] = base + 3;
        }

        if (ic > 0) {
            DebugCB cb{ vp, { 1.0f, 1.0f, 1.0f, 1.0f } };
            cmd->IASetVertexBuffers(0, 1, &lineVBV_);
            cmd->IASetIndexBuffer(&lineIBV_);
            cmd->SetGraphicsRoot32BitConstants(0, 20, &cb, 0);
            cmd->DrawIndexedInstanced(ic, 1, 0, 0, 0);
        }
    }

    // ---- ジョイント（球）----
    {
        cmd->IASetVertexBuffers(0, 1, &sphereVBV_);
        cmd->IASetIndexBuffer(&sphereIBV_);

        for (const Joint& joint : skeleton.joints) {
            Vector3   wp  = toWorld(joint.skeletonSpaceMatrix);
            Matrix4x4 wvp = Multiply(MakeTranslateMatrix(wp), vp);
            DebugCB   cb  { wvp, { 1.0f, 1.0f, 1.0f, 1.0f } };
            cmd->SetGraphicsRoot32BitConstants(0, 20, &cb, 0);
            cmd->DrawIndexedInstanced(sphereIndexCount_, 1, 0, 0, 0);
        }
    }
}

#endif // USE_IMGUI
