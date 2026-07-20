#include "SceneFactory.h"
#include "ClearScene.h"
#include "GameOverScene.h"
#include "GamePlayScene.h"
#include "LoadingScene.h"
#include "TitleScene.h"

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
    // 次のシーンを生成
    std::unique_ptr<BaseScene> newScene = nullptr;

    if (sceneName == "TITLE") {
        newScene = std::make_unique<TitleScene>();
    } else if (sceneName == "GAMEPLAY") {
        newScene = std::make_unique<GamePlayScene>();
    } else if (sceneName == "CLEAR") {
        newScene = std::make_unique<ClearScene>();
    } else if (sceneName == "GAMEOVER") {
        newScene = std::make_unique<GameOverScene>();
    } else if (sceneName == "LOADING") {
        newScene = std::make_unique<LoadingScene>();
    }

    return newScene;
}