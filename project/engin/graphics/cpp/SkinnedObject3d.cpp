#include "SkinnedObject3d.h"
#include "Camera.h"
#include "LightManager.h"
#include "Logger.h"
#include "ModelCommon.h"
#include "Object3dCommon.h"
#include "ShadowManager.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

using namespace Microsoft::WRL;

Camera*         SkinnedObject3d::commonCamera_        = nullptr;
Matrix4x4       SkinnedObject3d::commonLightVP_       = MakeIdentity4x4();
Object3dCommon* SkinnedObject3d::commonObjectCommon_  = nullptr;
ShadowManager*  SkinnedObject3d::commonShadowManager_ = nullptr;
ModelCommon*    SkinnedObject3d::commonModelCommon_   = nullptr;

void SkinnedObject3d::SetCommonCamera(Camera* camera)              { commonCamera_        = camera; }
void SkinnedObject3d::SetLightViewProjection(const Matrix4x4& lvp) { commonLightVP_       = lvp; }
void SkinnedObject3d::SetCommonObjectCommon(Object3dCommon* oc)    { commonObjectCommon_  = oc; }
void SkinnedObject3d::SetCommonShadowManager(ShadowManager* sm)    { commonShadowManager_ = sm; }
void SkinnedObject3d::SetCommonModelCommon(ModelCommon* mc)        { commonModelCommon_   = mc; }

void SkinnedObject3d::SetModel(SkinnedModel* model)
{
    model_ = model;
    InitializeSkinCS();
}

void SkinnedObject3d::InitializeSkinCS()
{
    if (!model_ || !skinCommon_) return;
    skinCS_.Initialize(skinCommon_->GetDxCommon(),
                       model_->GetVertexResource(),
                       model_->GetVertexCount());
    skinCSReady_ = true;
}

void SkinnedObject3d::Initialize(SkinCommon* skinCommon)
{
    skinCommon_ = skinCommon;
    ID3D12Device* device = skinCommon_->GetDxCommon()->GetDevice();
    D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };

    auto makeBuffer = [&](UINT64 size) -> ComPtr<ID3D12Resource> {
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

    transformCB_ = makeBuffer(sizeof(TransformationMatrix));
    transformCB_->Map(0, nullptr, reinterpret_cast<void**>(&transformData_));

    materialCB_ = makeBuffer(sizeof(Material));
    materialCB_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color           = { 1,1,1,1 };
    materialData_->enableLighting  = 1;
    materialData_->shadingType     = 1; // HalfLambert
    materialData_->useCubemap      = 0;
    materialData_->useTexture      = 1;
    materialData_->uvTransform     = MakeIdentity4x4();
    materialData_->specularColor   = { 1,1,1 };
    materialData_->shininess       = 32.0f;
    materialData_->envMapIntensity = 0.0f;

    // パレット: 256 バイト境界に合わせる（128 * 64 = 8192 はすでに倍数）
    paletteCB_ = makeBuffer(sizeof(Matrix4x4) * kMaxJoints);
    paletteCB_->Map(0, nullptr, reinterpret_cast<void**>(&paletteData_));
    for (int i = 0; i < kMaxJoints; ++i) paletteData_[i] = MakeIdentity4x4();

#ifdef USE_IMGUI
    skeletonDebugRenderer_ = std::make_unique<SkeletonDebugRenderer>();
    skeletonDebugRenderer_->Initialize(skinCommon_->GetDxCommon());
#endif
}

void SkinnedObject3d::Update()
{
    if (!model_) return;

    // アニメーション時刻を進める
    if (animation_.duration > 0.0f) {
        animTime_ = std::fmod(animTime_ + animSpeed_ / 60.0f, animation_.duration);
    }

    // スケルトン各ジョイントにアニメーションを適用
    for (Joint& joint : skeleton_.joints) {
        auto it = animation_.nodeAnimations.find(joint.name);
        if (it != animation_.nodeAnimations.end()) {
            const NodeAnimation& na = it->second;
            if (!na.translate.keyframes.empty())
                joint.transform.translate = CalculateValue(na.translate, animTime_);
            if (!na.rotate.keyframes.empty())
                joint.transform.rotate    = CalculateValue(na.rotate,    animTime_);
            if (!na.scale.keyframes.empty())
                joint.transform.scale     = CalculateValue(na.scale,     animTime_);
        }
    }
    skeleton_.Update();

    // スキニングパレット = inverseBindMatrix * skeletonSpaceMatrix
    const auto& boneNames = model_->GetBoneNames();
    const auto& ibms      = model_->GetInverseBindMatrices();
    static std::unordered_set<std::string> warnedBones;
    for (uint32_t i = 0; i < static_cast<uint32_t>(boneNames.size()) && i < kMaxJoints; ++i) {
        auto jt = skeleton_.jointMap.find(boneNames[i]);
        if (jt != skeleton_.jointMap.end()) {
            paletteData_[i] = Multiply(ibms[i], skeleton_.joints[jt->second].skeletonSpaceMatrix);
        } else {
            // ボーン名がスケルトンに存在しない場合、パレットは単位行列のまま（初期化済み）
            if (warnedBones.insert(boneNames[i]).second) {
                Logger::Log("[SkinnedObject3d] Bone '" + boneNames[i] +
                            "' not found in skeleton. Palette index " +
                            std::to_string(i) + " will remain identity.\n");
            }
        }
    }

    // ワールド行列 / WVP を計算
    worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 world = worldMatrix_;
    Matrix4x4 vp    = MakeIdentity4x4();
    if (commonCamera_) {
        vp = Multiply(commonCamera_->GetViewMatrix(), commonCamera_->GetProjectionMatrix());
        materialData_->cameraWorldPos = commonCamera_->GetTranslate();
    }
    transformData_->WVP     = Multiply(world, vp);
    transformData_->World   = world;
    transformData_->LightVP = commonLightVP_;

    materialData_->shadingType = LightManager::GetInstance()->GetLightingMode();
}

void SkinnedObject3d::DebugDraw()
{
#ifdef USE_IMGUI
    if (skeletonDebugRenderer_) {
        skeletonDebugRenderer_->Draw(skeleton_, worldMatrix_, commonCamera_);
    }
#endif
}

void SkinnedObject3d::Draw()
{
    if (!model_) return;
    ID3D12GraphicsCommandList* cmd = skinCommon_->GetDxCommon()->GetCommandList();

    if (skinCSReady_ && commonModelCommon_) {
        // CS スキニング済みパス:
        // Draw() 内で Dispatch することでコマンドリストが確実に開いている状態で実行される
        skinCS_.Dispatch(cmd, paletteCB_->GetGPUVirtualAddress());

        // 計算済み頂点バッファ (position/texcoord/normal のみ) を使い、
        // ModelCommon の標準 PSO (Object3dVS + Object3dPS) で描画する。
        commonModelCommon_->CommonDrawSettings();

        // スロット 0 (b0): マテリアル
        cmd->SetGraphicsRootConstantBufferView(0, materialCB_->GetGPUVirtualAddress());
        // スロット 1 (b0 VS): 座標変換行列
        cmd->SetGraphicsRootConstantBufferView(1, transformCB_->GetGPUVirtualAddress());
        // スロット 2 (t0): テクスチャ
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle =
            TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTextureFilePath());
        cmd->SetGraphicsRootDescriptorTable(2, texHandle);
        // スロット 3 (b1): ライト
        if (commonObjectCommon_) {
            commonObjectCommon_->SetDefaultLight(cmd);
        }
        // スロット 4 (t1): シャドウマップ
        if (commonShadowManager_) {
            commonShadowManager_->SetShadowMap(cmd, SrvManager::GetInstance());
        }
        // スロット 5 (t2): 環境マップ（未設定時は通常テクスチャをフォールバック）
        if (!envCubemapFilePath_.empty()) {
            D3D12_GPU_DESCRIPTOR_HANDLE cubeHandle =
                TextureManager::GetInstance()->GetSrvHandleGPU(envCubemapFilePath_);
            cmd->SetGraphicsRootDescriptorTable(5, cubeHandle);
        } else {
            cmd->SetGraphicsRootDescriptorTable(5, texHandle);
        }

        // CS が書き込んだ頂点バッファをセット (ボーンデータなし: pos/uv/normal のみ)
        const D3D12_VERTEX_BUFFER_VIEW& vbv = skinCS_.GetOutputVBV();
        cmd->IASetVertexBuffers(0, 1, &vbv);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->DrawInstanced(model_->GetVertexCount(), 1, 0, 0);
    } else {
        // フォールバック: SkinCommon の PSO (SkinnedVS) でボーン計算を VS 内で行う従来パス
        cmd->SetGraphicsRootConstantBufferView(0, materialCB_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootConstantBufferView(1, transformCB_->GetGPUVirtualAddress());

        if (commonObjectCommon_) {
            commonObjectCommon_->SetDefaultLight(cmd);
        }
        if (commonShadowManager_) {
            commonShadowManager_->SetShadowMap(cmd, SrvManager::GetInstance());
        }

        if (!envCubemapFilePath_.empty()) {
            D3D12_GPU_DESCRIPTOR_HANDLE cubeHandle =
                TextureManager::GetInstance()->GetSrvHandleGPU(envCubemapFilePath_);
            cmd->SetGraphicsRootDescriptorTable(5, cubeHandle);
        } else {
            D3D12_GPU_DESCRIPTOR_HANDLE texHandle =
                TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTextureFilePath());
            cmd->SetGraphicsRootDescriptorTable(5, texHandle);
        }

        cmd->SetGraphicsRootConstantBufferView(6, paletteCB_->GetGPUVirtualAddress());
        model_->Draw(cmd);
    }
}
