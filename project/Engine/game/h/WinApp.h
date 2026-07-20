/**
 * @file WinApp.h
 * @brief Windowsアプリケーションのウィンドウ生成・メッセージ管理を行うファイル
 */
#pragma once
#include <Windows.h>
#include <cstdint>

/**
 * @brief WindowsAPIを使用したウィンドウ管理クラス
 * @note ウィンドウクラスの登録、ウィンドウの生成、およびOSからのメッセージ（入力や閉じる操作）
 * の処理を担いDirectXの描画対象となる土台を作成します
 */
class WinApp {
public:

    /**
     * @brief ウィンドウプロシージャ（静的メンバ関数）
     * @param hwnd ウィンドウハンドル
     * @param uMsg メッセージ番号
     * @param wParam パラメータ1
     * @param lParam パラメータ2
     * @return LRESULT 処理結果
     * @note Windows OSからのイベント通知（リサイズ、破棄など）を処理するためのコールバック関数です
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public: // メンバ関数
    
    /**
     * @brief ウィンドウシステムの初期化
     * @note ウィンドウクラスの登録を行い、指定された解像度でウィンドウを表示します
     */
    void Initialize();
    
    /**
     * @brief ウィンドウの更新処理
     * @note 本エンジンではメッセージ処理を ProcessMessage() で行っているため、
     * 拡張用の空関数として用意されています
     */
    void Update();
    
    /**
     * @brief ウィンドウシステムの終了処理
     * @note 登録したウィンドウクラスの解除（UnregisterClass）などを行います。
     */
    void Finalize();
    
    /** @brief ウィンドウハンドルの取得 */
    HWND GetHwnd() const { return hwnd; }

    /** @brief インスタンスハンドルの取得 */
    HINSTANCE GetHInstance() const { return wc.hInstance; }
    
    /**
     * @brief Windowsメッセージを処理する
     * @return bool 終了メッセージ(WM_QUIT)を受け取った場合は true を返す
     * @note メインループの継続判定に使用します。内部で PeekMessage を呼び出します
     */
    bool ProcessMessage();

    /**
     * @brief フルスクリーンとウィンドウモードを切り替える
     */
    void ToggleFullscreen();

public: // 定数

    /** @brief ウィンドウ領域の幅（1280px） */
    static const int32_t kClientWidth = 1280;

    /** @brief ウィンドウ領域の高さ（720px） */
    static const int32_t kClientHeight = 720;

private:

    /** @brief OSから発行されるウィンドウ識別用のハンドル */
    HWND hwnd = nullptr;

    /** @brief ウィンドウクラスの設定情報 */
    WNDCLASSW wc = {};

    /** @brief 自クラスへのポインタ（必要に応じて使用） */
    WinApp* winApp_ = nullptr;

    /** @brief 現在フルスクリーンかどうか */
    bool isFullscreen_ = false;
};