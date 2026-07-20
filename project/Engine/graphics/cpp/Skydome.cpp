#include "Skydome.h"
#include <numbers>

void Skydome::Initialize(ModelCommon* modelCommon, Model* model)
{
    object_ = std::make_unique<Object3d>();
    object_->Initialize(modelCommon);
    object_->SetModel(model);

    // SkyDome.obj の球半径は約500unit。farClip=100 内に収めるため 0.19 倍(半径≒95) にする
    object_->SetScale({ 0.19f, 0.19f, 0.19f });

    object_->SetEnableLighting(false);
    object_->SetUseCubemap(true);
}

void Skydome::Update(Camera* camera, float timeRatio)
{
    if (camera != nullptr) {
        Vector3 camPos = camera->GetTransform().translate;

        object_->SetPosition(camPos);
    }

    // 時間に合わせてY軸回転（0.0=18:00 → 1.0=翌6:00 → 半周π）
    // ゲームは12時間分(18:00→6:00)なので、24時間テクスチャの半周分だけ回す
    float rotY = timeRatio * std::numbers::pi_v<float>;
    object_->SetRotation({ 0.0f, -rotY + rotationOffsetY_, 0.0f });

    // 行列の更新
    object_->Update();
}

void Skydome::Draw()
{
     if (object_) {
        object_->Draw();
    }
}

void Skydome::SetSkyColor(const Vector4& color)
{
    object_->SetColor(color);
}