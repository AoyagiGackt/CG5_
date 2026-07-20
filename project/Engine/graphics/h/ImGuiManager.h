/**
 * @file ImGuiManager.h
 * @brief デバッグ用UIライブラリImGuiの初期化・更新・描画・終了処理を一元管理するファイル
 */
#pragma once
#include "DirectXCommon.h"
#include "WinApp.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#endif

/**
 * @brief ImGuiの統合管理クラス
 * @note マクロ USE_IMGUI が定義されている場合のみ有効に機能
 */
class ImGuiManager {
public:
    
    /**
     * @brief ImGuiの初期化処理。コンテキストの作成やDX12・Win32への紐づけを行う
     * @param winApp ウィンドウアプリケーションのポインタ（HWND取得用）
     * @param dxCommon DirectX基盤のポインタ（デバイスやヒープ取得用）
     */
    void Initialize(WinApp* winApp, DirectXCommon* dxCommon);

    /**
     * @brief ImGuiの新しいフレーム開始処理
     * @note 毎フレームの更新（Update）処理の一番最初に呼び出すこと。
     */
    void Begin();

    /**
     * @brief ImGuiのフレーム終了（UI構築完了）処理
     * @note 毎フレームの更新（Update）処理の最後、かつ描画（Draw）の前に呼び出すこと。
     * 内部で ImGui::Render() を呼び出し、描画用のコマンドデータを生成します。
     */
    void End();

    /**
     * @brief ImGuiの実際の描画コマンドを積む
     * @param dxCommon DirectX基盤のポインタ（コマンドリスト取得用）
     * @note 実際の描画フェーズ（PreDrawとPostDrawの間）で呼び出すこと。
     */
    void Draw(DirectXCommon* dxCommon);

    /**
     * @brief ImGuiの終了処理とリソース解放
     * @note アプリケーションの終了時（Finalize）に必ず呼び出すこと。
     */
    void Finalize();
};