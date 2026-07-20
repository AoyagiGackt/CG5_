#include "Object3dCommon.h"
#include "MakeAffine.h"
#include <cmath>
#include <numbers>
#include <cassert>

using namespace Microsoft::WRL;

// =====================================================
// 初期化
// =====================================================

void Object3dCommon::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ライト定数バッファの作成
    D3D12_HEAP_PROPERTIES heapProps { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC resDesc {};
    resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width            = 256;
    resDesc.Height           = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels        = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightResource_));

    // Map したまま保持（毎フレーム UpdateLight() で書き換える）
    lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

    // 固定ライト（白昼光）
    lightData_->color            = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->direction        = { 0.5f, -0.8f, 0.3f };
    lightData_->intensity        = 1.0f;
    lightData_->ambientColor     = { 1.0f, 1.0f, 1.0f };
    lightData_->ambientIntensity = 0.3f;
}

// =====================================================
// ライト更新（時刻に合わせて方向・色・アンビエントを変える）
// =====================================================

void Object3dCommon::UpdateLight(float timeRatio)
{
    // ------ ライト方向アニメーション ------
    // timeRatio 0.0 = 18:00（夕方）、0.5 = 0:00（真夜中）、1.0 = 6:00（朝方）
    // X 成分: 夕方は右から(+)、朝は左から(-)
    float angle = std::numbers::pi_v<float> * timeRatio; // 0 → π
    float dirX  = std::cos(angle) * 0.7f;                // +0.7 → 0 → -0.7
    float dirY  = -(std::abs(std::sin(angle)) * 0.7f + 0.3f); // 常に下向き
    float dirZ  = 0.3f;

    // 正規化
    float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    lightData_->direction = { dirX / len, dirY / len, dirZ / len };

    // ------ 光源色・強度 ------
    // 夕焼け(0.0) → 月明かり(0.5) → 朝焼け(1.0)
    struct ColorKey { float t; Vector4 color; float intensity; };
    static constexpr ColorKey kLightKeys[] = {
        { 0.00f, { 1.0f, 0.70f, 0.35f, 1.0f }, 1.1f }, // 18:00 夕焼け
        { 0.17f, { 0.60f, 0.65f, 0.90f, 1.0f }, 0.5f }, // 20:00 夜へ
        { 0.42f, { 0.55f, 0.60f, 0.85f, 1.0f }, 0.3f }, // 23:00 深夜
        { 0.58f, { 0.55f, 0.60f, 0.85f, 1.0f }, 0.3f }, // 3:00 深夜
        { 0.83f, { 0.70f, 0.60f, 0.75f, 1.0f }, 0.6f }, // 5:00 夜明け前
        { 1.00f, { 1.0f, 0.65f, 0.30f, 1.0f }, 1.1f }, // 6:00 朝焼け
    };

    // ------ アンビエント色 ------
    struct AmbientKey { float t; Vector3 color; float intensity; };
    static constexpr AmbientKey kAmbientKeys[] = {
        { 0.00f, { 0.25f, 0.18f, 0.12f }, 0.40f }, // 18:00 暖色アンビエント
        { 0.17f, { 0.08f, 0.10f, 0.20f }, 0.25f }, // 20:00 青みがかった夜
        { 0.42f, { 0.05f, 0.07f, 0.15f }, 0.20f }, // 23:00 深夜
        { 0.58f, { 0.05f, 0.07f, 0.15f }, 0.20f }, // 3:00 深夜
        { 0.83f, { 0.12f, 0.10f, 0.20f }, 0.25f }, // 5:00 夜明け前
        { 1.00f, { 0.25f, 0.15f, 0.10f }, 0.40f }, // 6:00 朝の暖色
    };

    constexpr int kCount = 6;
    for (int i = 0; i + 1 < kCount; ++i) {
        if (timeRatio <= kLightKeys[i + 1].t) {
            float span = kLightKeys[i + 1].t - kLightKeys[i].t;
            float t    = (timeRatio - kLightKeys[i].t) / span;

            // 光源色補間
            const Vector4& a = kLightKeys[i].color;
            const Vector4& b = kLightKeys[i + 1].color;
            lightData_->color = { a.x + (b.x - a.x) * t,
                                  a.y + (b.y - a.y) * t,
                                  a.z + (b.z - a.z) * t, 1.0f };
            lightData_->intensity = kLightKeys[i].intensity
                + (kLightKeys[i + 1].intensity - kLightKeys[i].intensity) * t;

            // アンビエント補間
            const Vector3& ac = kAmbientKeys[i].color;
            const Vector3& bc = kAmbientKeys[i + 1].color;
            lightData_->ambientColor = { ac.x + (bc.x - ac.x) * t,
                                         ac.y + (bc.y - ac.y) * t,
                                         ac.z + (bc.z - ac.z) * t };
            lightData_->ambientIntensity = kAmbientKeys[i].intensity
                + (kAmbientKeys[i + 1].intensity - kAmbientKeys[i].intensity) * t;
            break;
        }
    }

}

// =====================================================
// ライト方向の取得（ShadowManager に渡す用）
// =====================================================

Vector3 Object3dCommon::GetLightDirection() const
{
    if (!lightData_) { 
        return { 0.0f, -1.0f, 0.0f }; 
    }

    return lightData_->direction;
}

// =====================================================
// ライトをコマンドリストにセット（スロット 3）
// =====================================================

void Object3dCommon::SetDefaultLight(ID3D12GraphicsCommandList* commandList)
{
    commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());
}

