/**
 * @file Framework.h
 * @brief アプリケーション全体の流れ（実行、初期化、更新、描画、終了）を統括する基底クラスを定義するファイル
 */
#pragma once

// 標準ライブラリ
#include <memory>

// 基盤系
#include "Audio.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "WinApp.h"

// 描画共通
#include "ModelCommon.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "MeshManager.h"
#include "MaterialManager.h"

// シーンオブジェクト
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include <VideoPlayer.h>

// ライブラリのリンク
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

/**
 * @brief アプリケーションの基本骨格となるクラス
 * @note ゲームのメインループと各基盤システムのサイクルを管理します。具体的なゲーム内容は、このクラスを継承した派生クラス（例: MyGame）で実装
 */
class Framework {
public:

    /**
     * @brief 仮想デストラクタ
     */
    virtual ~Framework() = default;

    /**
     * @brief アプリケーションの実行
     * @note 初期化からメインループ、終了処理までを一括で制御する、エンジンのエントリポイントです
     */
    void Run();

    /**
     * @brief システムの初期化
     * @note ウィンドウ、DirectX、入力、オーディオ等の基盤システムをセットアップします
     * 派生クラスでオーバーライドして、ゲーム固有のリソース読み込みなどを追加してください
     */
    virtual void Initialize();

    /**
     * @brief システムの終了処理
     * @note メインループを抜けた後に呼ばれ、各種基盤システムの解放を行います
     */
    virtual void Finalize();

    /**
     * @brief システムの更新処理
     * @note 毎フレーム呼び出され、Windowsメッセージの処理や基盤システムの更新を行います
     */
    virtual void Update();

    /**
     * @brief 描画処理（純粋仮想関数）
     * @note 各ゲーム固有の描画コマンドを積み込むために、必ず派生クラスで実装してください
     */
    virtual void Draw() = 0;

    /**
     * @brief 終了リクエストの確認
     * @return bool 終了すべきなら true
     * @note ウィンドウの閉じるボタンが押されたか、ゲーム内から終了要求があったかを判定します
     */
    virtual bool IsEndRequest() { return endRequest_ || winApp_->ProcessMessage() || input_->TriggerKey(DIK_ESCAPE); }

protected:

    // --- 主要な基盤システム（スマートポインタによる自動管理） ---

    /** @brief ウィンドウ管理 */
    std::unique_ptr<WinApp> winApp_;

    /** @brief DirectX12基盤 */
    std::unique_ptr<DirectXCommon> dxCommon_;

    /** @brief 入力管理（キーボード・コントローラー） */
    std::unique_ptr<Input> input_;

    /** @brief オーディオ管理 */
    std::unique_ptr<Audio> audio_;

    /** @brief ImGui管理（デバッグUI用） */
    std::unique_ptr<ImGuiManager> imguiManager_;

    /** @brief ビデオプレイヤー管理（必要に応じて初期化/終了） */
    std::unique_ptr<VideoPlayer> videoPlayer_;

    /** @brief 終了リクエストフラグ */
    bool endRequest_ = false;
};