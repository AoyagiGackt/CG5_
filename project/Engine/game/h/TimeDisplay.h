/**
 * @file TimeDisplay.h
 * @brief Resources/Time/ の画像を使ってデジタル時計風に時刻を表示するクラス
 *
 * clockFrame.png : 時計の外枠
 * middle.png     : コロン（:）
 * 1.png ~ 9.png  : 数字スプライト
 *
 * @note 0.png が存在しないため、桁が 0 の場合はそのスロットを非表示にします。
 *       0.png を Resources/Time/ に追加するとすべての桁が表示されます。
 */
#pragma once
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>

class TimeDisplay {
public:
    /**
     * @brief 初期化。スプライトを生成しデフォルト位置を設定する
     * @param spriteCommon スプライト描画の共通設定
     */
    void Initialize(SpriteCommon* spriteCommon);

    /**
     * @brief 毎フレーム呼ぶ更新処理。時刻に合わせてスプライトを更新する
     * @param hour   表示する時（0〜23）
     * @param minute 表示する分（0〜59）
     */
    void Update(int hour, int minute);

    /** @brief 時計UIを描画する（spriteCommon の CommonDrawSettings() 後に呼ぶこと） */
    void Draw();

    /**
     * @brief 時計フレームの左上座標を設定する
     * @param pos 画面上の位置（ピクセル）
     */
    void SetPosition(Vector2 pos);

    /** @brief 現在の表示位置を取得する */
    Vector2 GetPosition() const { return framePos_; }

private:
    /**
     * @brief 数字スプライトのテクスチャを設定し、表示フラグを更新する
     * @param sprite  対象スプライト
     * @param digit   表示する数字（0〜9）。0 は非表示
     * @param visible 表示フラグへの参照
     */
    void ApplyDigit(Sprite* sprite, int digit, bool& visible);

    /** @brief 全スプライトの座標・サイズをフレーム位置に合わせて更新する */
    void UpdateLayout();

    // ----- フレーム内レイアウト定数 -----

    static constexpr Vector2 kFrameSize   = { 220.0f,  80.0f }; ///< 時計枠のサイズ
    static constexpr float   kDigitY      =  10.0f;              ///< 桁の上パディング
    static constexpr float   kDigitH      =  60.0f;              ///< 桁の高さ
    static constexpr float   kDigitW      =  40.0f;              ///< 桁の幅
    static constexpr float   kColonW      =  22.0f;              ///< コロンの幅

    /// 各要素の X オフセット（フレーム左端からの距離）
    static constexpr float kH1X     =  10.0f;
    static constexpr float kH2X     =  55.0f;
    static constexpr float kColonX  =  98.0f;
    static constexpr float kM1X     = 124.0f;
    static constexpr float kM2X     = 169.0f;

    // ----- スプライト -----
    std::unique_ptr<Sprite> clockFrame_; ///< 時計の外枠
    std::unique_ptr<Sprite> colon_;      ///< コロン（:）

    std::unique_ptr<Sprite> digitH1_;    ///< 時 十の位
    std::unique_ptr<Sprite> digitH2_;    ///< 時 一の位
    std::unique_ptr<Sprite> digitM1_;    ///< 分 十の位
    std::unique_ptr<Sprite> digitM2_;    ///< 分 一の位

    // ----- 表示フラグ -----
    bool showH1_ = true;
    bool showH2_ = true;
    bool showM1_ = true;
    bool showM2_ = true;

    // ----- フレームの左上座標 -----
    Vector2 framePos_ = { 1050.0f, 10.0f };

    SpriteCommon* spriteCommon_ = nullptr;
};
