#include "ImGuiManager.h"
#include <SrvManager.h>

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon)
{
#ifdef USE_IMGUI

    // ImGui生成
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // 日本語フォント読み込み（Windows 標準の游ゴシック）
    // ※ Win32Init より前に登録しないとアトラスに反映されない
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/YuGothM.ttc",
        16.0f,
        nullptr,
        io.Fonts->GetGlyphRangesJapanese()
    );

    // Win32初期化
    ImGui_ImplWin32_Init(winApp->GetHwnd());

    // DirectX12初期化
    uint32_t index = SrvManager::GetInstance()->Allocate();

    ImGui_ImplDX12_Init(
        dxCommon->GetDevice(),
        dxCommon->GetBufferCount(),
        dxCommon->GetBackBufferFormat(),
        SrvManager::GetInstance()->GetSrvDescriptorHeap(), // ヒープを渡す
        SrvManager::GetInstance()->GetCPUDescriptorHandle(index), // 確保したCPUハンドル
        SrvManager::GetInstance()->GetGPUDescriptorHandle(index) // 確保したGPUハンドル
    );

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

#endif
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI

    // フレーム開始
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#endif
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI

    // 内部コマンド生成
    ImGui::Render();

#endif
}

void ImGuiManager::Draw(DirectXCommon* dxCommon)
{
#ifdef USE_IMGUI

    // コマンドリストをdxCommonから取得する
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

    ID3D12DescriptorHeap* ppHeaps[] = { SrvManager::GetInstance()->GetSrvDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // 描画コマンドを発行
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif
}

void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI

    // 終了処理
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

#endif
}