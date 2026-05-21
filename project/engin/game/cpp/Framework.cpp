#include "Framework.h"
#include "SrvManager.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "LightingMode.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "GrayscaleEffect.h"
#include "ImageFilter.h"
#include "VignetteEffect.h"

void Framework::Run()
{
    Initialize();
    while (true) {
        Update();
        
        if (IsEndRequest()) {
            break;
        }

        Draw();
    }

    Finalize();
}

void Framework::Initialize()
{
    winApp_ = std::make_unique<WinApp>();
    winApp_->Initialize();

    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(winApp_.get());

    SrvManager::GetInstance()->Initialize(dxCommon_.get());
    GrayscaleEffect::GetInstance()->Initialize(dxCommon_.get(), SrvManager::GetInstance());
    ImageFilter::GetInstance()->Initialize(dxCommon_.get(), SrvManager::GetInstance());
    VignetteEffect::GetInstance()->Initialize(dxCommon_.get());
    TextureManager::GetInstance()->Initialize(dxCommon_.get());
    ParticleManager::GetInstance()->Initialize(dxCommon_.get());

    input_ = std::make_unique<Input>();
    input_->Initialize(winApp_.get());

    audio_ = std::make_unique<Audio>();
    audio_->Initialize();

    imguiManager_ = std::make_unique<ImGuiManager>();
    imguiManager_->Initialize(winApp_.get(), dxCommon_.get());
}

void Framework::Update()
{
    input_->Update();
    imguiManager_->Begin();

    // F11 でフルスクリーン/ウィンドウ切り替え
    if (input_->TriggerKey(DIK_F11)) {
        winApp_->ToggleFullscreen();
    }
}

void Framework::Finalize()
{
    // ImGuiの終了
    if (imguiManager_) {
        imguiManager_->Finalize();
    }

    if (videoPlayer_) {
        videoPlayer_->Finalize();
    }

    // 各種マネージャーのGPUリソースを解放する
    VignetteEffect::GetInstance()->Finalize();
    ImageFilter::GetInstance()->Finalize();
    GrayscaleEffect::GetInstance()->Finalize();
    ParticleManager::GetInstance()->Finalize();
    MeshManager::GetInstance()->Finalize();
    MaterialManager::GetInstance()->Finalize();
    TextureManager::GetInstance()->Finalize();
    SrvManager::GetInstance()->Finalize();
    ModelManager::GetInstance()->Finalize();

    // Audioの終了
    if (audio_) {
        audio_->Finalize();
    }

    // クラス自体の削除
    imguiManager_.reset();
    audio_.reset();
    input_.reset();

    // 最後にデバイスとウィンドウを削除（Finalizeでまずフルスクリーン解除とGPU完了待ち）
    dxCommon_->Finalize();
    dxCommon_.reset();
    winApp_.reset();
}