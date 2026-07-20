#pragma once
#include "DirectXCommon.h"
#include "MakeAffine.h"
#include <string>
#include <vector>
#include <wrl/client.h>

// ボーン重み付き頂点データを持つ GLTF スキンメッシュ
class SkinnedModel {
public:
    struct VertexData {
        Vector4  position;
        Vector2  texcoord;
        Vector3  normal;
        uint32_t boneIndices[4];
        float    boneWeights[4];
    };

    void Initialize(DirectXCommon* dxCommon,
                    const std::string& gltfFilePath,
                    const std::string& textureFilePath);

    // コマンドリストに頂点バッファとテクスチャ SRV をセットして DrawInstanced を発行する
    // スロット 2 (テクスチャ) と 5 (キューブマップ枠) を書き込む
    void Draw(ID3D12GraphicsCommandList* cmd);

    const std::string&               GetTextureFilePath()     const { return textureFilePath_; }
    const std::vector<Matrix4x4>&   GetInverseBindMatrices() const { return inverseBindMatrices_; }
    const std::vector<std::string>& GetBoneNames()           const { return boneNames_; }
    ID3D12Resource*                  GetVertexResource()      const { return vertexResource_.Get(); }
    UINT                             GetVertexCount()         const { return static_cast<UINT>(vertices_.size()); }

private:
    void LoadGltfFile(DirectXCommon* dxCommon, const std::string& filePath);

    std::string textureFilePath_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView_{};

    std::vector<VertexData>  vertices_;
    std::vector<Matrix4x4>   inverseBindMatrices_;  // bone index → inverse bind matrix
    std::vector<std::string> boneNames_;             // bone index → bone name
};
