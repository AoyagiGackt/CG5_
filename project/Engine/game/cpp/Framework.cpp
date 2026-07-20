#include "Framework.h"
#include "SrvManager.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "LightingMode.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "PostProcessManager.h"

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
    TextureManager::GetInstance()->Initialize(dxCommon_.get());
    ParticleManager::GetInstance()->Initialize(dxCommon_.get());

    input_ = std::make_unique<Input>();
    input_->Initialize(winApp_.get());

    audio_ = std::make_unique<Audio>();
    audio_->Initialize();

    imguiManager_ = std::make_unique<ImGuiManager>();
    imguiManager_->Initialize(winApp_.get(), dxCommon_.get());

    PostProcessManager::GetInstance()->Initialize(dxCommon_.get(), SrvManager::GetInstance());
}

void Framework::Update()
{
    input_->Update();
    imguiManager_->Begin();

    // F11 でフルスクリーン/ウィンドウ切り替え
    if (input_->TriggerKey(DIK_F11)) {
        winApp_->ToggleFullscreen();
    }

    // 数字キー(1〜9)でポストエフェクトのON/OFFを切り替え（リリースビルドでも確認できるように）
    if (input_->TriggerKey(DIK_1)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::Grayscale); }
    if (input_->TriggerKey(DIK_2)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::Vignette); }
    if (input_->TriggerKey(DIK_3)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::BoxFilter); }
    if (input_->TriggerKey(DIK_4)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::Gaussian); }
    if (input_->TriggerKey(DIK_5)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::LumOutline); }
    if (input_->TriggerKey(DIK_6)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::DepthOutline); }
    if (input_->TriggerKey(DIK_7)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::RadialBlur); }
    if (input_->TriggerKey(DIK_8)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::Dissolve); }
    if (input_->TriggerKey(DIK_9)) { PostProcessManager::GetInstance()->ToggleEffect(PostEffectType::Random); }
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
    PostProcessManager::GetInstance()->Finalize();
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