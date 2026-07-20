#include "SceneManager.h"
#include "ClearScene.h"
#include "GameOverScene.h"
#include "GamePlayScene.h"
#include "TitleScene.h"

SceneManager* SceneManager::GetInstance()
{
    static SceneManager instance;
    return &instance;
}

void SceneManager::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio, ImGuiManager* imgui)
{
    dxCommon_ = dxCommon;
    input_ = input;
    audio_ = audio;
    imguiManager_ = imgui;

    // 最初のシーン（ゲームプレイシーンから開始）
    currentScene_ = std::make_unique<GamePlayScene>();
    currentScene_->Initialize(dxCommon_, input_, audio_);

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // フェードの初期化
    fade_.Initialize(spriteCommon_.get());

    auto gameplayScene = dynamic_cast<GamePlayScene*>(currentScene_.get());
    
    if (gameplayScene) {
        gameplayScene->SetImGuiManager(imguiManager_);
    }
}

void SceneManager::Update()
{

    fade_.Update(); // フェードのタイマー更新

    // シーン切り替えの予約（FadeOutが終わったタイミング）
    if (isChanging_ && fade_.IsFinished()) {
        audio_->StopWave(); // 前のシーンの音を止める

        // 前のシーンを終了処理してからリソースを解放
        if (currentScene_) {
            currentScene_->Finalize();
        }

        // 工場を使って新しいシーンを作成・初期化
        currentScene_ = sceneFactory_->CreateScene(nextSceneName_);
        currentScene_->Initialize(dxCommon_, input_, audio_);

        // ImGuiのセット
        auto gameplayScene = dynamic_cast<GamePlayScene*>(currentScene_.get());
        
        if (gameplayScene) {
            gameplayScene->SetImGuiManager(imguiManager_);
        }

        auto clearScene = dynamic_cast<ClearScene*>(currentScene_.get());
        
        if (clearScene) {
            clearScene->SetImGuiManager(imguiManager_);
        }

        auto gameOverScene = dynamic_cast<GameOverScene*>(currentScene_.get());
        
        if (gameOverScene) {
            gameOverScene->SetImGuiManager(imguiManager_);
        }

        // シーンが切り替わったので、画面を明るくし始める
        fade_.Start(Fade::Status::FadeIn, 1.0f);
        isChanging_ = false;
    }

    // フェード中であっても、今のシーンの更新は続ける
    if (currentScene_) {
        currentScene_->Update();
    }
}

void SceneManager::Draw()
{
    if (currentScene_) {
        currentScene_->Draw();
    }

    fade_.Draw();
}

void SceneManager::Finalize()
{
    if (currentScene_) {
        currentScene_->Finalize();
    }

    currentScene_.reset();
    nextScene_.reset();

    // Fade内のSpriteが持つD3D12リソースを解放する
    fade_ = Fade{};
    // SpriteCommonのPSO・ルートシグネチャ・バッファを解放する
    spriteCommon_.reset();
}

// ロード画面経由でシーン切り替え
void SceneManager::ChangeSceneWithLoading(const std::string& targetScene)
{
    loadingTargetScene_ = targetScene;
    ChangeScene("LOADING");
}

// シーン切り替え予約
void SceneManager::ChangeScene(const std::string& sceneName)
{

    if (isChanging_) {
        return;
    }

    nextSceneName_ = sceneName;
    isChanging_ = true;

    // 暗転開始
    fade_.Start(Fade::Status::FadeOut, 1.0f);
}