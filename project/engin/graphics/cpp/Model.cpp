#include "Model.h"
#include "ModelCommon.h"
#include "TextureManager.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include <tuple>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace Microsoft::WRL;

void Model::Initialize(ModelCommon* modelCommon, const std::string& modelFilePath, const std::string& textureFilePath)
{
    modelCommon_ = modelCommon;
    textureFilePath_ = textureFilePath;

    TextureManager::GetInstance()->LoadTexture(textureFilePath);
    isCubemap_ = TextureManager::GetInstance()->GetMetaData(textureFilePath).IsCubemap();

    // 拡張子で読み込み関数を切り替える
    std::string ext = modelFilePath.substr(modelFilePath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    
    if (ext == "obj") {
        LoadObjFile(modelFilePath);
    } else {
        LoadGltfFile(modelFilePath);
    }

    ID3D12Device* device = modelCommon_->GetDxCommon()->GetDevice();
    size_t sizeInBytes = sizeof(VertexData) * vertices_.size();

    D3D12_HEAP_PROPERTIES uploadHeapProperties { D3D12_HEAP_TYPE_UPLOAD };

    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertexResource_));

    VertexData* data = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&data));
    std::copy(vertices_.begin(), vertices_.end(), data);
    vertexResource_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeInBytes);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // インデックスバッファ作成
    size_t indexSizeInBytes = sizeof(uint32_t) * indices_.size();

    D3D12_RESOURCE_DESC indexResourceDesc {};
    indexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    indexResourceDesc.Width = indexSizeInBytes;
    indexResourceDesc.Height = 1;
    indexResourceDesc.DepthOrArraySize = 1;
    indexResourceDesc.MipLevels = 1;
    indexResourceDesc.SampleDesc.Count = 1;
    indexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
        &indexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&indexResource_));

    uint32_t* indexData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
    std::copy(indices_.begin(), indices_.end(), indexData);
    indexResource_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<UINT>(indexSizeInBytes);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Model::Draw(ModelCommon* modelCommon)
{
    ID3D12GraphicsCommandList* commandList = modelCommon->GetDxCommon()->GetCommandList();

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle = TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_);
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandle);

    // スロット5（TextureCube）: 環境マップが指定されていればそちらを、なければ通常テクスチャを流用
    if (!envCubemapFilePath_.empty()) {
        D3D12_GPU_DESCRIPTOR_HANDLE cubeHandle = TextureManager::GetInstance()->GetSrvHandleGPU(envCubemapFilePath_);
        commandList->SetGraphicsRootDescriptorTable(5, cubeHandle);
    } else {
        commandList->SetGraphicsRootDescriptorTable(5, textureSrvHandle);
    }

    commandList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

void Model::DrawGeometryOnly(ModelCommon* modelCommon)
{
    ID3D12GraphicsCommandList* commandList = modelCommon->GetDxCommon()->GetCommandList();
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

// OBJファイル読み込み
void Model::LoadObjFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    assert(file.is_open());

    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    // (v, vt, vn) の組み合わせ → vertices_ 内のインデックスへのマップ
    std::map<std::tuple<int,int,int>, uint32_t> indexMap;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string identifier;
        ss >> identifier;

        if (identifier == "v") {
            Vector4 position;
            ss >> position.x >> position.y >> position.z;
            position.w = 1.0f;
            positions.push_back(position);
        } else if (identifier == "vt") {
            Vector2 texcoord;
            ss >> texcoord.x >> texcoord.y;
            texcoord.y = 1.0f - texcoord.y;
            texcoords.push_back(texcoord);
        } else if (identifier == "vn") {
            Vector3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (identifier == "f") {
            std::vector<uint32_t> faceIndices;
            std::string s;

            while (ss >> s) {
                std::stringstream ss2(s);
                std::string indexStr;
                int idx[3] = { 0, 0, 0 };
                int count = 0;

                while (std::getline(ss2, indexStr, '/')) {
                    if (!indexStr.empty()) {
                        idx[count] = std::stoi(indexStr);
                    }
                    count++;
                }

                auto key = std::make_tuple(idx[0], idx[1], idx[2]);
                auto it = indexMap.find(key);
                if (it != indexMap.end()) {
                    faceIndices.push_back(it->second);
                } else {
                    uint32_t newIndex = static_cast<uint32_t>(vertices_.size());
                    indexMap[key] = newIndex;
                    faceIndices.push_back(newIndex);

                    VertexData vd {};
                    if (idx[0] > 0) vd.position = positions[idx[0] - 1];
                    if (idx[1] > 0) vd.texcoord = texcoords[idx[1] - 1];
                    if (idx[2] > 0) vd.normal = normals[idx[2] - 1];
                    vertices_.push_back(vd);
                }
            }

            // ファン状に三角形へ分割してインデックスを登録
            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                indices_.push_back(faceIndices[0]);
                indices_.push_back(faceIndices[i]);
                indices_.push_back(faceIndices[i + 1]);
            }
        }
    }
}

void Model::LoadGltfFile(const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_MakeLeftHanded |
        aiProcess_FlipWindingOrder);

    assert(scene && scene->mNumMeshes > 0);

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        uint32_t baseVertex = static_cast<uint32_t>(vertices_.size());

        // 頂点データをそのまま追加
        for (uint32_t vIdx = 0; vIdx < mesh->mNumVertices; ++vIdx) {
            VertexData vd {};
            vd.position = {
                mesh->mVertices[vIdx].x,
                mesh->mVertices[vIdx].y,
                mesh->mVertices[vIdx].z,
                1.0f
            };
            if (mesh->HasNormals()) {
                vd.normal = {
                    mesh->mNormals[vIdx].x,
                    mesh->mNormals[vIdx].y,
                    mesh->mNormals[vIdx].z
                };
            }
            if (mesh->HasTextureCoords(0)) {
                vd.texcoord = {
                    mesh->mTextureCoords[0][vIdx].x,
                    mesh->mTextureCoords[0][vIdx].y
                };
            }
            vertices_.push_back(vd);
        }

        // Assimpのフェイスインデックスをそのままインデックスバッファへ追加
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const aiFace& face = mesh->mFaces[faceIndex];
            for (uint32_t i = 0; i < face.mNumIndices; ++i) {
                indices_.push_back(baseVertex + face.mIndices[i]);
            }
        }
    }
}