/**
 * @file GameTime.h
 * @brief ゲーム内時刻（18:00 → 翌 6:00）を管理するクラス
 */
#pragma once

class GameTime {
public:
    // 開始時刻（18:00）を分に換算
    static constexpr int kStartMinutes = 18 * 60;   // 1080
    // 終了時刻（翌 6:00）を分に換算（24時間超え表現）
    static constexpr int kEndMinutes   = 30 * 60;   // 1800
    // ゲーム内で進める合計分数
    static constexpr float kTotalGameMinutes = float(kEndMinutes - kStartMinutes); // 720

    /** @brief 初期化（経過時間をリセット） */
    void Initialize();

    /**
     * @brief 毎フレーム呼び出す更新処理
     * @param minutesPerSecond 1秒あたりに進むゲーム内分数（デフォルト1.0）
     * @note 60fps 前提で、1フレームあたり (minutesPerSecond/60) ゲーム分を加算する
     */
    void Update(float minutesPerSecond = 1.0f);

    /** @brief 翌6:00 に達したら true を返す */
    bool IsCleared() const { return elapsedMinutes_ >= kTotalGameMinutes; }

    /** @brief 現在の時刻（時） 0-23 */
    int GetHour() const;

    /** @brief 現在の時刻（分） 0-59 */
    int GetMinute() const;

    /** @brief 経過ゲーム分数（0〜720） */
    float GetElapsedMinutes() const { return elapsedMinutes_; }

    /** @brief ゲーム内時刻を指定分数だけ進める（デバッグ用） */
    void SkipMinutes(float minutes);

private:
    // 18:00 からの経過ゲーム分数
    float elapsedMinutes_ = 0.0f;
};
