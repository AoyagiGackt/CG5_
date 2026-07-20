#include "ShadowManager.h"
#include "SrvManager.h"
#include <cassert>
#include <cmath>

using namespace Microsoft::WRL;

// =====================================================
// 初期化
// =====================================================

void ShadowManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // R32_TYPELESS テクスチャ（DSV: D32_FLOAT, SRV: R32_FLOAT）
    D3D12_HEAP_PROPERTIES defaultHeap { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC texDesc {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = kShadowMapSize;
    texDesc.Height = kShadowMapSize;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&shadowMapResource_));
    assert(SUCCEEDED(hr));

    // DSV ヒープ（シャドウマップ専用）
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&shadowDsvHeap_));
    assert(SUCCEEDED(hr));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(shadowMapResource_.Get(), &dsvDesc,
        shadowDsvHeap_->GetCPUDescriptorHandleForHeapStart());

    // SRV（PS でシャドウマップを読む用）
    shadowSrvIndex_ = srvManager->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(shadowMapResource_.Get(), &srvDesc,
        srvManager->GetCPUDescriptorHandle(shadowSrvIndex_));
}

// =====================================================
// ライト方向 → LightVP 行列を更新
// =====================================================

void ShadowManager::Update(const Vector3& lightDir)
{
    Vector3 sceneCenter = { 15.0f, 5.0f, 0.0f };
    Vector3 lightEye = {
        sceneCenter.x - lightDir.x * 40.0f,
        sceneCenter.y - lightDir.y * 40.0f,
        sceneCenter.z - lightDir.z * 40.0f,
    };

    // LookAt 行列（ライト視点）
    Vector3 fwd = {
        sceneCenter.x - lightEye.x,
        sceneCenter.y - lightEye.y,
        sceneCenter.z - lightEye.z,
    };

    float fwdLen = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
    fwd = { fwd.x / fwdLen, fwd.y / fwdLen, fwd.z / fwdLen };

    Vector3 up = { 0.0f, 1.0f, 0.0f };

    if (std::abs(fwd.y) > 0.99f) {
        up = { 0.0f, 0.0f, 1.0f };
    }

    // right = up × fwd
    Vector3 right = {
        up.y * fwd.z - up.z * fwd.y,
        up.z * fwd.x - up.x * fwd.z,
        up.x * fwd.y - up.y * fwd.x,
    };

    float rightLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    right = { right.x / rightLen, right.y / rightLen, right.z / rightLen };

    // realUp = fwd × right
    Vector3 realUp = {
        fwd.y * right.z - fwd.z * right.y,
        fwd.z * right.x - fwd.x * right.z,
        fwd.x * right.y - fwd.y * right.x,
    };

    float dotR = right.x * lightEye.x + right.y * lightEye.y + right.z * lightEye.z;
    float dotU = realUp.x * lightEye.x + realUp.y * lightEye.y + realUp.z * lightEye.z;
    float dotF = fwd.x * lightEye.x + fwd.y * lightEye.y + fwd.z * lightEye.z;

    // View 行列（行ベクトル形式）
    Matrix4x4 view = {};
    view.m[0][0] = right.x;
    view.m[0][1] = right.y;
    view.m[0][2] = right.z;
    view.m[0][3] = 0;
    view.m[1][0] = realUp.x;
    view.m[1][1] = realUp.y;
    view.m[1][2] = realUp.z;
    view.m[1][3] = 0;
    view.m[2][0] = fwd.x;
    view.m[2][1] = fwd.y;
    view.m[2][2] = fwd.z;
    view.m[2][3] = 0;
    view.m[3][0] = -dotR;
    view.m[3][1] = -dotU;
    view.m[3][2] = -dotF;
    view.m[3][3] = 1;

    // 平行投影行列（DirectX 深度 0-1）
    float w = 40.0f;
    float h = 25.0f;
    float nearZ = 0.1f;
    float farZ = 80.0f;

    Matrix4x4 proj = {};
    proj.m[0][0] = 2.0f / w;
    proj.m[1][1] = 2.0f / h;
    proj.m[2][2] = 1.0f / (farZ - nearZ);
    proj.m[3][2] = -nearZ / (farZ - nearZ);
    proj.m[3][3] = 1.0f;

    lightVP_ = Multiply(view, proj);
}

// =====================================================
// シャドウパス開始・終了
// =====================================================

void ShadowManager::BeginShadowPass(ID3D12GraphicsCommandList* commandList)
{
    if (!shadowMapInDepthWrite_) {
        D3D12_RESOURCE_BARRIER barrier {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = shadowMapResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    shadowMapInDepthWrite_ = true;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = shadowDsvHeap_->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT vp = { 0, 0, (float)kShadowMapSize, (float)kShadowMapSize, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)kShadowMapSize, (LONG)kShadowMapSize };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);
}

void ShadowManager::EndShadowPass(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = shadowMapResource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
    shadowMapInDepthWrite_ = false;
}

// =====================================================
// SRV セット（スロット 4）
// =====================================================

void ShadowManager::SetShadowMap(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager)
{
    commandList->SetGraphicsRootDescriptorTable(4,
        srvManager->GetGPUDescriptorHandle(shadowSrvIndex_));
}
