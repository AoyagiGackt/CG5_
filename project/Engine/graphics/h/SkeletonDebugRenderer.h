#pragma once
#ifdef USE_IMGUI

#include "DirectXCommon.h"
#include "MakeAffine.h"
#include "Skeleton.h"
#include <wrl/client.h>
#include <vector>

class Camera;

// 3Dワールド空間にスケルトンを描画するデバッグクラス
// - ジョイント : 白い球
// - ボーン     : カメラ向きのビルボードクワッド（白）
// 深度テスト無効でメッシュに埋まっていても常に表示
class SkeletonDebugRenderer {
public:
    void Initialize(DirectXCommon* dxCommon);
    void Draw(const Skeleton& skeleton, const Matrix4x4& worldMatrix, Camera* camera);

private:
    struct DebugVertex { float x, y, z, w; };
    struct DebugCB     { Matrix4x4 wvp; Vector4 color; };

    void BuildSphere();

    static constexpr int   kStacks       = 6;
    static constexpr int   kSlices       = 8;
    static constexpr float kSphereRadius = 0.04f;
    static constexpr float kBoneHalfWidth = 0.015f; // ボーンの太さ（ワールド単位）
    static constexpr int   kMaxBones     = 256;

    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoTri_;

    Microsoft::WRL::ComPtr<ID3D12Resource> sphereVB_;
    Microsoft::WRL::ComPtr<ID3D12Resource> sphereIB_;
    D3D12_VERTEX_BUFFER_VIEW sphereVBV_{};
    D3D12_INDEX_BUFFER_VIEW  sphereIBV_{};
    uint32_t sphereIndexCount_ = 0;

    // ボーン用ビルボードクワッド（毎フレーム CPU で構築）
    Microsoft::WRL::ComPtr<ID3D12Resource> lineVB_;
    Microsoft::WRL::ComPtr<ID3D12Resource> lineIB_;
    D3D12_VERTEX_BUFFER_VIEW lineVBV_{};
    D3D12_INDEX_BUFFER_VIEW  lineIBV_{};
    DebugVertex* lineMapped_    = nullptr;
    uint16_t*   lineIdxMapped_ = nullptr;
};

#endif // USE_IMGUI
