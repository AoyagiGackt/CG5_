#include "WinApp.h"
#include "ImGuiManager.h"

#pragma comment(lib, "winmm.lib")

#ifdef USE_IMGUI

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

// ウィンドウプロシージャ
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    #ifdef USE_IMGUI

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
        return true;
    }

    #endif

    // メッセージに応じてゲーム固有の処理を行う
    switch (uMsg) {
        // ウィンドウが破棄された
    case WM_DESTROY:
        // OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    // 標準のメッセージ処理を行う
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void WinApp::Initialize()
{
    // DPI スケーリングを無効化して実ピクセルで描画する
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

    // WNDCLASSWを使用
    WNDCLASSW wc = {};

    // ウィンドクラスプロシージャ
    wc.lpfnWndProc = WindowProc;

    // ウィンドクラス名
    wc.lpszClassName = L"WindowClass";

    // インスタンスハンドル
    wc.hInstance = GetModuleHandleW(nullptr);

    // カーソル
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);

    // ウィンドクラスの登録
    RegisterClassW(&wc);

    // メンバ変数のwcにコピーしておく
    this->wc = wc;


    // ウィンドウサイズ（クライアント領域が 1280x720）
    RECT wr = { 0, 0, kClientWidth, kClientHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    int winWidth  = wr.right - wr.left;
    int winHeight = wr.bottom - wr.top;
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth  - winWidth)  / 2;
    int posY = (screenHeight - winHeight) / 2;

    hwnd = CreateWindowW(
        wc.lpszClassName, // 利用するクラス名
        L"myGame", // タイトルバーの文字
        WS_OVERLAPPEDWINDOW, // 通常ウィンドウ
        posX, // 表示x座標（画面中央）
        posY, // 表示y座標（画面中央）
        winWidth,  // ウィンドウ横幅
        winHeight, // ウィンドウ縦幅
        nullptr, // 親ウィンドウハンドル
        nullptr, // メニューハンドル
        wc.hInstance, // インスタンスハンドル
        nullptr // オプション
    );


    // ウィンドウを表示する
    ShowWindow(hwnd, SW_NORMAL);

    // システムタイマーの分解能を上げる
    timeBeginPeriod(1);
}

void WinApp::Update()
{
}

void WinApp::Finalize()
{
    CloseWindow(hwnd);
    CoUninitialize();
}

void WinApp::ToggleFullscreen()
{
    isFullscreen_ = !isFullscreen_;

    if (isFullscreen_) {
        // ボーダーレスフルスクリーンへ
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        SetWindowLongW(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(hwnd, SW_SHOW);
    } else {
        // ウィンドウモードへ（1280x720、画面中央）
        RECT wr = { 0, 0, kClientWidth, kClientHeight };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
        int winWidth  = wr.right - wr.left;
        int winHeight = wr.bottom - wr.top;
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenWidth  - winWidth)  / 2;
        int posY = (screenHeight - winHeight) / 2;
        SetWindowLongW(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP, posX, posY, winWidth, winHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(hwnd, SW_NORMAL);
    }
}

bool WinApp::ProcessMessage()
{
    MSG msg {};

    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT) {
        return true;
    }

    return false;
}