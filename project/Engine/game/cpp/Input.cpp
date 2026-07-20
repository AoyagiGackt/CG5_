#include "Input.h"
#include <cassert>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(WinApp* winApp)
{
    HRESULT result;
    
    // 借りてきたWinAppのインスタンスを記録
    this->winApp_ = winApp;

    // DirectInputのインスタンス生成
    result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
    assert(SUCCEEDED(result));
    // キーボードデバイス生成
    result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
    assert(SUCCEEDED(result));
    // 入力データ形式のセット
    result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(result));
    // 排他制御レベルのセット
    result = keyboard_->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(result));

    // マウスデバイス生成
    result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
    assert(SUCCEEDED(result));
    // 入力データ形式のセット
    result = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(result));
    // 排他制御レベルのセット
    result = mouse_->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(result));
}

void Input::Update() {
    HRESULT result;

    // ゲームコントローラー更新
    UpdateGamepad();

    // 前回のキー入力を保存
    memcpy(keyPre, key, sizeof(key));
    // キーボード情報の取得開始
    result = keyboard_->Acquire();
    // 全キーの入力情報を取得する
    result = keyboard_->GetDeviceState(sizeof(key), key);

    // 前回のマウス入力を保存
    mouseStatePre_ = mouseState_;
    // マウス情報の取得開始
    mouse_->Acquire();
    // 全マウスの入力情報を取得する
    mouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &mouseState_);
}

bool Input::PushKey(BYTE keyNumber)
{
    // 指定キーを押していればtrueを返す
    if (key[keyNumber]) {
        return true;
    }

    // そうでなければfalseを返す
    return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
    // 前回は押してなくて今回は押している
    if (key[keyNumber] && !keyPre[keyNumber]) {
        return true;
    }

    // そうでなければfalseを返す
    prevKeyStates_[keyNumber] = key[keyNumber];
    return false;
}

/**
 * @brief ゲームパッドの状態更新
 */
void Input::UpdateGamepad()
{
    previousState_ = state_; // 前回の状態を保存

    // 0番目のコントローラーを取得
    DWORD result = XInputGetState(0, &state_);

    if (result != ERROR_SUCCESS) {
        // コントローラーが接続されていない場合はデータをゼロにする
        ZeroMemory(&state_, sizeof(XINPUT_STATE));
    }
}

/**
 * @brief スティックの正規化（遊びを考慮して 0.0 ~ 1.0 に変換）
 */
Input::Stick Input::GetLeftStick() const
{
    float x = (float)state_.Gamepad.sThumbLX / 32767.0f;
    float y = (float)state_.Gamepad.sThumbLY / 32767.0f;

    // デッドゾーンの処理
    if (std::abs(x) < deadzone_) {
        x = 0.0f;
    }

    if (std::abs(y) < deadzone_) {
        y = 0.0f;
    }

    return { x, y };
}

bool Input::TriggerMouseButton(int32_t buttonNumber)
{
    // 今回押されていて、前回押されていないかチェック
    if (mouseState_.rgbButtons[buttonNumber] && !mouseStatePre_.rgbButtons[buttonNumber]) {
        return true;
    }

    return false;
}