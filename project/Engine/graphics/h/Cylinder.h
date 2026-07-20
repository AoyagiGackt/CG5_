#pragma once
#include "MakeAffine.h"
#include "Camera.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

class DirectXCommon;

class Cylinder
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
    void SetTopRadius(float r)           { topRadius_ = r;    RebuildVertices(); }
    void SetBottomRadius(float r)        { bottomRadius_ = r; RebuildVertices(); }
    void SetHeight(float h)              { height_ = h;       RebuildVertices(); }
    void SetAlphaReference(float a)      { materialData_->alphaReference = a; }

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
        float     alphaReference;
        float     _pad[3];
    };

    int divisions_   = 32;
    int vertexCount_ = 0;

    DirectXCommon* dxCommon_      = nullptr;
    std::string    textureFilePath_;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    Microsoft::WRL::ComPtr<ID3D12Resource>      vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW                    vbv_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
    MaterialCB* materialData_ = nullptr;
    VertexData* vertexData_   = nullptr;

    Vector3 position_     = {};
    Vector3 rotation_     = {};
    float   scale_        = 1.0f;
    float   topRadius_    = 1.0f;
    float   bottomRadius_ = 1.0f;
    float   height_       = 3.0f;
};
