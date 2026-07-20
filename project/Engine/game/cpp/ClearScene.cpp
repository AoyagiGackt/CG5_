#include "ClearScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include <string>

// =====================================================
// レイアウト定数（調整はここだけ）
// =====================================================

// "SCORE" ラベル
static constexpr Vector2 kScoreLabelPos  = { 490.f, 130.f }; // 左上座標
static constexpr Vector2 kScoreLabelSize = { 300.f,  80.f }; // 表示サイズ

// 現在スコア数字
static constexpr Vector2 kScoreDigitSize = {  70.f, 100.f };
static constexpr float   kScoreDigitGap  =    5.f;
static constexpr float   kScoreY         =  230.f;

// "RANKING" ラベル
static constexpr Vector2 kRankLabelPos   = { 470.f, 345.f };
static constexpr Vector2 kRankLabelSize  = { 340.f,  60.f };

// ランキング数字
static constexpr Vector2 kRankDigitSize  = {  36.f,  52.f };
static constexpr float   kRankDigitGap   =    3.f;
static constexpr float   kRankRowSpacing =   60.f;
static constexpr float   kRankTopY       =  420.f;
static constexpr float   kRankLeftX      =  430.f;

// =====================================================
// 初期化
// =====================================================

void ClearScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    dxCommon_ = dxCommon;
    input_    = input;
    audio_    = audio;

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    // 背景（白）
    clearSprite_ = std::make_unique<Sprite>();
    clearSprite_->Initialize(spriteCommon_.get(), "Resources/white.png");
    clearSprite_->SetPosition({ 0.0f, 0.0f });
    clearSprite_->SetSize({ 1280.0f, 720.0f });

    // "SCORE" ラベル
    scoreLabel_ = std::make_unique<Sprite>();
    scoreLabel_->Initialize(spriteCommon_.get(), "Resources/score/score.png");
    scoreLabel_->SetPosition(kScoreLabelPos);
    scoreLabel_->SetSize(kScoreLabelSize);

    // "RANKING" ラベル
    rankingLabel_ = std::make_unique<Sprite>();
    rankingLabel_->Initialize(spriteCommon_.get(), "Resources/ranking/ranking.png");
    rankingLabel_->SetPosition(kRankLabelPos);
    rankingLabel_->SetSize(kRankLabelSize);

    // スコア数字表示
    scoreDisplay_.Initialize(spriteCommon_.get());

    ScoreManager::GetInstance()->SubmitAndSave();
}

// =====================================================
// 終了
// =====================================================

void ClearScene::Finalize()
{
}

// =====================================================
// 更新
// =====================================================

void ClearScene::Update()
{
    if (input_->TriggerKey(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene("TITLE");
    }

    clearSprite_->Update();
    scoreLabel_->Update();
    rankingLabel_->Update();

    DrawScoreUI();
}

// =====================================================
// 描画
// =====================================================

void ClearScene::Draw()
{
    spriteCommon_->CommonDrawSettings();

    // 背景
    clearSprite_->Draw();

    // "SCORE" ラベル
    scoreLabel_->Draw();

    // 現在スコア数字（中央揃え）
    scoreDisplay_.Reset();
    {
        int currentScore = ScoreManager::GetInstance()->GetCurrentScore();
        std::string s = std::to_string(currentScore < 0 ? 0 : currentScore);
        float totalW  = s.size() * (kScoreDigitSize.x + kScoreDigitGap) - kScoreDigitGap;
        float startX  = (1280.f - totalW) * 0.5f;
        scoreDisplay_.DrawNumber(currentScore, { startX, kScoreY },
                                 kScoreDigitSize, kScoreDigitGap);
    }

    // "RANKING" ラベル
    rankingLabel_->Draw();

    // ランキング数字
    const auto& ranking = ScoreManager::GetInstance()->GetRanking();
    scoreDisplay_.DrawRanking(ranking,
                              ScoreManager::GetInstance()->GetCurrentScore(),
                              { kRankLeftX, kRankTopY },
                              kRankDigitSize, kRankRowSpacing);
}

// =====================================================
// デバッグ UI（ImGui）
// =====================================================

void ClearScene::DrawScoreUI()
{
#ifdef USE_IMGUI
    if (!imguiManager_) { return; }

    ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::Begin("Debug", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Score : %d", ScoreManager::GetInstance()->GetCurrentScore());
    ImGui::Separator();
    if (ImGui::Button("Reset All Scores")) {
        ScoreManager::GetInstance()->ResetAllScores();
    }
    ImGui::Separator();
    ImGui::Text("Press SPACE -> Title");
    ImGui::End();
#endif
}
