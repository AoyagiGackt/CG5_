/**
 * @file D3DResourceLeakChecker.h
 * @brief DirectX12のリソースリーク（メモリの解放忘れ）を自動でチェック・報告するファイル
 */
#pragma once
#include <d3d12.h>
#include <dxgidebug.h>
#include <wrl/client.h>

/**
 * @brief アプリケーション終了時にCOMオブジェクトの解放忘れがないかをチェックする構造体
 * @note グローバル変数として宣言するか、メイン関数の最初にローカル変数として宣言することで、
 * プログラム終了時（デストラクタ呼び出し時）に自動的にリークチェックが実行されるはず。
 */
struct D3D12ResourceLeakChecker {

    /**
     * @brief デストラクタ。dxgidebug.dll を動的に読み込み、出力ウィンドウにリーク情報をレポートする
     */
    ~D3D12ResourceLeakChecker()
    {
        // リソースリークチェック
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;

        // dxgidebug.dllから関数を動的に取り出すための型定義
        typedef HRESULT(WINAPI * PFN_DXGI_GET_DEBUG_INTERFACE1)(UINT, REFIID, void**);

        // DLLをロード
        HMODULE dxgidebug = LoadLibraryExW(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (dxgidebug) {
            // 関数ポインタを取得
            auto dxgiGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(
                GetProcAddress(dxgidebug, "DXGIGetDebugInterface1"));

            // 関数が見つかったら実行
            if (dxgiGetDebugInterface1) {
                if (SUCCEEDED(dxgiGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
                    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
                    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
                    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
                }
            }
            FreeLibrary(dxgidebug);
        }
    }
};