/**
 * @file ScoreManager.h
 * @brief スコアの管理・保存・ランキングを行うクラス
 */
#pragma once
#include <vector>

class ScoreManager {
public:
    static ScoreManager* GetInstance();

    // --- 現セッション ---

    /** @brief 現在のスコアを0にリセット（ゲーム開始時に呼ぶ） */
    void ResetCurrentScore();

    /**
     * @brief スコアを加算する
     * @param points 加算するポイント（デフォルト200）
     */
    void AddScore(int points = 200);

    int GetCurrentScore() const { return currentScore_; }

    // --- ランキング ---

    /** @brief 現在スコアをランキングに登録してファイル保存（ゲームクリア時に呼ぶ） */
    void SubmitAndSave();

    /** @brief ランキングを全消去してファイルにも反映 */
    void ResetAllScores();

    /** @brief 保存ファイルからランキングを読み込む */
    void LoadScores();

    const std::vector<int>& GetRanking() const { return ranking_; }

private:
    ScoreManager() = default;

    int currentScore_ = 0;
    std::vector<int> ranking_;

    static constexpr int kMaxRank = 10;
    static constexpr const char* kSaveFile = "scores.txt";

    void SaveScores();
};
