#include "SkinnedModel.h"
#include "TextureManager.h"
#include <cassert>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace Microsoft::WRL;

// assimp の mOffsetMatrix (右手・列ベクトル規則) を
// このプロジェクトの Matrix4x4 (左手・行ベクトル規則) に変換する。
// Skeleton.cpp::ConvertAiNode と同じ Decompose → flip → MakeAffineMatrix の手順を踏む。
static Matrix4x4 ConvertOffsetMatrix(const aiMatrix4x4& m)
{
    aiVector3D    scale, translate;
    aiQuaternion  rotate;
    m.Decompose(scale, rotate, translate);

    // 右手系 → 左手系（X 軸反転）：ConvertAiNode と同一の変換
    Vector3    s = { scale.x,      scale.y,     scale.z };
    Quaternion r = { rotate.x,    -rotate.y,   -rotate.z,  rotate.w };
    Vector3    t = { -translate.x,  translate.y, translate.z };

    return MakeAffineMatrix(s, r, t);
}

void SkinnedModel::Initialize(DirectXCommon* dxCommon,
                              const std::string& gltfFilePath,
                              const std::string& textureFilePath)
{
    textureFilePath_ = textureFilePath;
    TextureManager::GetInstance()->LoadTexture(textureFilePath);

    LoadGltfFile(dxCommon, gltfFilePath);

    size_t sizeInBytes = sizeof(VertexData) * vertices_.size();
    assert(sizeInBytes > 0);

    D3D12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC   resDesc{};
    resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width            = sizeInBytes;
    resDesc.Height           = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels        = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    dxCommon->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertexResource_));

    VertexData* mapped = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    std::copy(vertices_.begin(), vertices_.end(), mapped);
    vertexResource_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes    = static_cast<UINT>(sizeInBytes);
    vertexBufferView_.StrideInBytes  = sizeof(VertexData);
}

void SkinnedModel::Draw(ID3D12GraphicsCommandList* cmd)
{
    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

    D3D12_GPU_DESCRIPTOR_HANDLE texHandle =
        TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_);
    cmd->SetGraphicsRootDescriptorTable(2, texHandle);
    cmd->SetGraphicsRootDescriptorTable(5, texHandle);

    cmd->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
}

void SkinnedModel::LoadGltfFile(DirectXCommon* /*dxCommon*/, const std::string& filePath)
{
    Assimp::Importer importer;
    // MakeLeftHanded / FlipWindingOrder は使わず手動で座標変換する。
    // これにより Skeleton.cpp::LoadNodeHierarchyFromFile と同一の変換規則になる。
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate      |
        aiProcess_FlipUVs          |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights);
    assert(scene && scene->mNumMeshes > 0);

    // パス1: ボーン名 → index, inverse bind matrix を収集
    std::map<std::string, uint32_t> boneMap;
    for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* mesh = scene->mMeshes[mi];
        for (uint32_t bi = 0; bi < mesh->mNumBones; ++bi) {
            aiBone* bone = mesh->mBones[bi];
            std::string name = bone->mName.C_Str();
            if (boneMap.find(name) == boneMap.end()) {
                uint32_t idx = static_cast<uint32_t>(boneNames_.size());
                boneMap[name] = idx;
                boneNames_.push_back(name);
                // Decompose → 右手→左手変換 → MakeAffineMatrix で再構成
                inverseBindMatrices_.push_back(ConvertOffsetMatrix(bone->mOffsetMatrix));
            }
        }
    }

    // パス2: 三角形リストを構築（ボーン重み付き）
    for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* mesh = scene->mMeshes[mi];
        uint32_t numVerts = mesh->mNumVertices;

        std::vector<uint32_t> indices(numVerts * 4, 0);
        std::vector<float>    weights(numVerts * 4, 0.0f);
        std::vector<uint32_t> counts(numVerts, 0);

        for (uint32_t bi = 0; bi < mesh->mNumBones; ++bi) {
            aiBone*  bone    = mesh->mBones[bi];
            uint32_t boneIdx = boneMap[bone->mName.C_Str()];
            for (uint32_t wi = 0; wi < bone->mNumWeights; ++wi) {
                uint32_t vIdx = bone->mWeights[wi].mVertexId;
                float    w    = bone->mWeights[wi].mWeight;
                if (counts[vIdx] < 4) {
                    indices[vIdx * 4 + counts[vIdx]] = boneIdx;
                    weights[vIdx * 4 + counts[vIdx]] = w;
                    ++counts[vIdx];
                }
            }
        }

        for (uint32_t fi = 0; fi < mesh->mNumFaces; ++fi) {
            const aiFace& face = mesh->mFaces[fi];
            assert(face.mNumIndices == 3); // Triangulate 済み

            // X 軸反転後にワインディングが逆転するため、インデックス 1 と 2 を入れ替える
            const uint32_t order[3] = { face.mIndices[0], face.mIndices[2], face.mIndices[1] };

            for (uint32_t i = 0; i < 3; ++i) {
                uint32_t vIdx = order[i];
                VertexData vd{};

                // 右手系 → 左手系：X を反転
                vd.position = {
                    -mesh->mVertices[vIdx].x,
                     mesh->mVertices[vIdx].y,
                     mesh->mVertices[vIdx].z,
                     1.0f
                };
                if (mesh->HasNormals())
                    vd.normal = {
                        -mesh->mNormals[vIdx].x,
                         mesh->mNormals[vIdx].y,
                         mesh->mNormals[vIdx].z
                    };
                if (mesh->HasTextureCoords(0))
                    vd.texcoord = {
                        mesh->mTextureCoords[0][vIdx].x,
                        mesh->mTextureCoords[0][vIdx].y
                    };

                for (int k = 0; k < 4; ++k) {
                    vd.boneIndices[k] = indices[vIdx * 4 + k];
                    vd.boneWeights[k] = weights[vIdx * 4 + k];
                }
                vertices_.push_back(vd);
            }
        }
    }
}
