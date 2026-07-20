/**
 * @file Input.h
 * @brief DirectInput 8 を使用したキーボード入力管理を行うファイル
 */
#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <array>
#include <windows.h>
#include <XInput.h>

#include <wrl/client.h>
#include "WinApp.h"

#pragma comment(lib, "xinput.lib")

/**
 * @brief キーボード入力を一括管理するクラス
 * @note DirectInput 8 を使用して、全256キーの状態を取得・管理します
 * 押しっぱなし判定（PushKey）と、押した瞬間判定（TriggerKey）を提供します
 */
class Input {
public: // メンバ関数
    // namespace省略
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    /**
     * @brief 入力システムの初期化
     * @param winApp ウィンドウ管理クラスのポインタ
     * @note DirectInput インスタンスの生成、キーボードデバイスの初期化、データ形式の設定などを行います
     */
    void Initialize(WinApp* winApp);

    /**
     * @brief キー状態の更新
     * @note 毎フレーム呼び出すことで、現在のキー状態を「前回の状態」にコピーし、
     * デバイスから最新のキー状態を取得して更新します
     */
    void Update();

    /**
     * @brief キーが押されている（押しっぱなし）かチェック
     * @param keyNumber キーの番号（例: DIK_SPACE）
     * @return bool 押されていれば true
     */
    bool PushKey(BYTE keyNumber);
    
    /**
     * @brief キーが押された瞬間かチェック
     * @param keyNumber キーの番号（例: DIK_RETURN）
     * @return bool このフレームで押された瞬間なら true
     * @note 1フレーム前が「離されていた」かつ「現在が押されている」場合に true を返します
     */
    bool TriggerKey(BYTE keyNumber);

    /** @brief 毎フレームの更新（コントローラー用） */
    void UpdateGamepad();

    /** @brief ボタンが押されているか */
    bool PushButton(WORD button) const { return (state_.Gamepad.wButtons & button); }

    /** @brief ボタンが押された瞬間か */
    bool TriggerButton(WORD button) const
    {
        return (state_.Gamepad.wButtons & button) && !(previousState_.Gamepad.wButtons & button);
    }

    /** @brief 左スティックの値を 0.0 ~ 1.0 の範囲で取得 */
    struct Stick {
        float x, y;
    };

    Stick GetLeftStick() const;

    // --- マウス関連 ---
    bool TriggerMouseButton(int32_t buttonNumber);

    /** @brief マウスのホイールスクロール量を取得する */
    int32_t GetWheel() const { return mouseState_.lZ; }

private:

    /** @brief DirectInput 8 の本体ポインタ */
    ComPtr<IDirectInput8> directInput_;

    /** @brief キーボードデバイスのポインタ */
    ComPtr<IDirectInputDevice8> keyboard_;

    /** @brief インスタンスハンドル */
    HINSTANCE hInstance_ = nullptr;

    /** @brief ウィンドウハンドル */
    HWND hwnd_ = nullptr;

    // --- キー状態管理用バッファ ---

    /** @brief 最新のキー状態（256個のキー分） */
    BYTE key[256] = {};

    /** @brief 1フレーム前のキー状態（256個のキー分） */
    BYTE keyPre[256] = {};

    /** @brief 最新のキー状態（std::array版） */
    std::array<BYTE, 256> keyStates_ {};

    /** @brief 1フレーム前のキー状態（std::array版） */
    std::array<BYTE, 256> prevKeyStates_ {};

    /** @brief ウィンドウ管理のポインタ */
    WinApp* winApp_ = nullptr;


    // --- コントローラー状態管理用 ---

    XINPUT_STATE state_ {}; /// 現在のコントローラー状態
    XINPUT_STATE previousState_ {}; /// 前回のコントローラー状態
    const float deadzone_ = 0.2f; /// デッドゾーン

    // --- マウス状態管理用 ---

    /** @brief マウスデバイスのポインタ */
    Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;
    /** @brief マウスの現在の状態 */
    DIMOUSESTATE2 mouseState_ = {};
    /** @brief マウスの前回の状態 */
    DIMOUSESTATE2 mouseStatePre_ = {};
};