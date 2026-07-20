#include "Game.h"
#include "D3DResourceLeakChecker.h"
#include <memory>

// --------------------------------------------------
// メイン関数
// --------------------------------------------------

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // リークチェッカーを最初に宣言
    D3D12ResourceLeakChecker leakCheck;

    // ゲームインスタンス生成
    std::unique_ptr<Framework> game = std::make_unique<MyGame>();

    // ゲーム実行
    game->Run();

    return 0;
}