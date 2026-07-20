#pragma once
#include "MakeAffine.h"
#include "Camera.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

class DirectXCommon;

class Ring
{
public:
    void Initialize(DirectXCommon* dxCommon,
                    const std::string& textureFilePath = "Resources/gradationLine.png",
                    int divisions = 32);
    void Update(Camera* camera);
    void Draw();

    void SetPosition(const Vector3& pos) { position_ = pos; }
    void SetRotation(const Vector3& rot) { rotation_ = rot; }
    void SetScale(float scale)           { scale_ = scale; }
    void SetColor(const Vector4& color)  { materialData_->color = color; }
    void SetInnerRadius(float r)         { innerRadius_ = r; RebuildVertices(); }
    void SetOuterRadius(float R)         { outerRadius_ = R; RebuildVertices(); }

    const Vector3& GetPosition() const { return position_; }

private:
    void CreatePipeline();
    void RebuildVertices();

    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct MaterialCB {
        Matrix4x4 WVP;
        Vector4   color;
    };

    int divisions_   = 32;
    int vertexCount_ = 0;

    DirectXCommon*  dxCommon_         = nullptr;
    std::string     textureFilePath_;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    Microsoft::WRL::ComPtr<ID3D12Resource>      vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW                    vbv_{};

    Microsoft::WRL::ComPtr<ID3D12Resource>      materialBuffer_;
    MaterialCB*                                 materialData_ = nullptr;
    VertexData*                                 vertexData_   = nullptr;

    Vector3 position_    = {};
    Vector3 rotation_    = {};
    float   scale_       = 1.0f;
    float   innerRadius_ = 1.0f;
    float   outerRadius_ = 2.0f;
};
