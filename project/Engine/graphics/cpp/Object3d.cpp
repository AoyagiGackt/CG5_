#include "Object3d.h"
#include "Camera.h"
#include "LightManager.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include <cmath>

using namespace Microsoft::WRL;

Camera*    Object3d::commonCamera_ = nullptr;
Matrix4x4  Object3d::commonLightVP_ = MakeIdentity4x4();

void Object3d::SetCommonCamera(Camera* camera)
{
    commonCamera_ = camera;
}

void Object3d::SetLightViewProjection(const Matrix4x4& lvp)
{
    commonLightVP_ = lvp;
}

void Object3d::Initialize(ModelCommon* modelCommon)
{
    modelCommon_ = modelCommon;
    ID3D12Device* device = modelCommon_->GetDxCommon()->GetDevice();

    // Transform用リソース作成
    D3D12_HEAP_PROPERTIES heapProps { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC resDesc {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    // CBV は 256 バイトアライン必須なので切り上げる
    resDesc.Width = (sizeof(TransformationMatrix) + 255) & ~255u;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformationMatrixResource_));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    *transformationMatrixData_ = { MakeIdentity4x4(), MakeIdentity4x4(), MakeIdentity4x4(), MakeIdentity4x4() };

    // Material用リソース作成
    resDesc.Width = sizeof(Material);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color          = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = true;
    materialData_->shadingType    = 1; // HalfLambert
    materialData_->useCubemap     = 0;
    materialData_->useTexture     = 1; // デフォルトはテクスチャあり
    materialData_->uvTransform    = MakeIdentity4x4();
    materialData_->specularColor  = { 1.0f, 1.0f, 1.0f }; // 白いハイライト
    materialData_->shininess      = 32.0f;                  // ほどよい光沢
    materialData_->cameraWorldPos = { 0.0f, 0.0f, 0.0f };
    materialData_->envMapIntensity = 0.0f;
}

void Object3d::Update()
{
    Matrix4x4 worldMatrix = useLocalMatrix_
        ? localMatrix_
        : MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;

    // カメラがあればカメラから行列をもらう
    Camera* camera = camera_ ? camera_ : commonCamera_;

    if (camera) {
        viewMatrix       = camera->GetViewMatrix();
        projectionMatrix = camera->GetProjectionMatrix();
        // カメラのワールド座標をマテリアルに書き込む（スペキュラ計算に使用）
        materialData_->cameraWorldPos = camera->GetTranslate();
    }

    // WVP行列の合成
    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

    // 定数バッファへ転送
    transformationMatrixData_->WVP                  = worldViewProjectionMatrix;
    transformationMatrixData_->World                = worldMatrix;
    transformationMatrixData_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
    transformationMatrixData_->LightVP              = commonLightVP_;

    // 毎フレームライティングモードをマテリアルに反映させる
    materialData_->shadingType = LightManager::GetInstance()->GetLightingMode();

    collider_.aabb.min = { transform_.translate.x - 0.5f, transform_.translate.y - 0.5f, transform_.translate.z - 0.5f };
    collider_.aabb.max = { transform_.translate.x + 0.5f, transform_.translate.y + 0.5f, transform_.translate.z + 0.5f };
}

void Object3d::DrawShadow()
{
    if (!model_) {
        return;
    }

    ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList();
    // シャドウパス用ルートシグネチャはスロット 0 に TransformationMatrix を期待する
    commandList->SetGraphicsRootConstantBufferView(0, transformationMatrixResource_->GetGPUVirtualAddress());
    model_->DrawGeometryOnly(modelCommon_);
}

void Object3d::SetModel(const std::string& filePath)
{
    // マネージャーからモデルを検索してセット
    model_ = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::Draw()
{
    if (!model_) {
        return;
    }

    // ModelCommonからコマンドリストを取得
    ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList();

    // マテリアルと座標変換を設定
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    model_->Draw(modelCommon_);
}