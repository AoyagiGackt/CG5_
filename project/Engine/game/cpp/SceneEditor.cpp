#include "SceneEditor.h"
#include "GameConstants.h"
#include "JsonHelper.h"
#include "ParticleManager.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include <fstream>
#include <sstream>
#include <string>
#ifdef USE_IMGUI
#include <imgui.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#endif

// ============================================================
// 内部ヘルパー
// ============================================================

#ifdef USE_IMGUI
// Windows のファイル選択ダイアログを開き、ユーザーが選んだファイルのパスを返す。
// filter  = 表示するファイルの種類の説明（例: "PNG Files\0*.png\0..."）
// initDir = ダイアログを開いたときに最初に表示するフォルダ
static std::string OpenFileDialog(const char* filter, const char* initDir)
{
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn   = {};
    ofn.lStructSize     = sizeof(ofn);       // 構造体のサイズ（Windows API の決まり）
    ofn.lpstrFilter     = filter;            // 表示するファイル種別フィルタ
    ofn.lpstrFile       = path;              // 選択されたパスの書き込み先バッファ
    ofn.nMaxFile        = MAX_PATH;          // バッファの最大サイズ
    ofn.lpstrInitialDir = initDir;           // 最初に表示するフォルダ
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST; // 実在するファイルのみ選択可能
    if (GetOpenFileNameA(&ofn)) { return path; } // OK を押したらパスを返す
    return {}; // キャンセルしたら空文字列を返す
}
#endif

// ============================================================
// State 遷移
// ============================================================

// Hierarchy でオブジェクトが選ばれたとき、対応する State クラスに切り替える。
// こうすることで Inspector パネルに表示する内容がオブジェクト種別に応じて変わる。
void SceneEditor::ChangeState(Selection sel)
{
    selection_      = sel;
    selectionIndex_ = -1; // UIElement のインデックスをリセット
    switch (sel) {
    case Selection::Camera:         currentState_ = std::make_unique<CameraState>();    break;
    case Selection::Ring:           currentState_ = std::make_unique<RingState>();      break;
    case Selection::Cylinder:       currentState_ = std::make_unique<CylinderState>(); break;
    case Selection::Skydome:        currentState_ = std::make_unique<SkydomeState>();   break;
    case Selection::Human:          currentState_ = std::make_unique<HumanState>();     break;
    case Selection::WhiteParticles: currentState_ = std::make_unique<ParticlesState>();break;
    case Selection::UIElement:      currentState_ = std::make_unique<UIElementState>();break;
    default:                        currentState_ = std::make_unique<NoneState>();      break;
    }
}

// ============================================================
// メインエントリ
// ============================================================

// 毎フレーム呼ばれる。USE_IMGUI ビルドのときだけ各パネルを描画する。
// リリースビルドでは (void)ctx; だけが実行されて何もしない。
void SceneEditor::Update(const EditContext& ctx)
{
#ifdef USE_IMGUI
    RenderHierarchy(ctx);     // 左：オブジェクト一覧
    RenderInspector(ctx);     // 右：選択中オブジェクトのプロパティ
    RenderSceneControls(ctx); // 左下：スコア・ゲーム時刻・シーン操作
    RenderCameraControl(ctx); // 上中央：カメラ位置の簡易操作
#else
    (void)ctx;
#endif
}

// ============================================================
// Hierarchy パネル（画面左）
// ============================================================

// シーン内のオブジェクト一覧を表示する。
// クリックすると Inspector パネルの内容が切り替わる。
void SceneEditor::RenderHierarchy(const EditContext& ctx)
{
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(kHierarchyX, kHierarchyY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(kHierarchyW, kHierarchyH), ImGuiCond_Once);
    ImGui::Begin("Hierarchy");

    // Selectable を選んだら ChangeState を呼んで Inspector を切り替える
    auto selectable = [&](const char* label, Selection type) {
        if (ImGui::Selectable(label, selection_ == type)) { ChangeState(type); }
    };

    selectable("Camera",          Selection::Camera);
    selectable("Skydome",         Selection::Skydome);
    selectable("Human",           Selection::Human);
    selectable("Ring",            Selection::Ring);
    selectable("Cylinder",        Selection::Cylinder);
    selectable("White Particles", Selection::WhiteParticles);

    // UI Elements は可変長リストなのでツリーノードで折りたたみ表示する
    char uiHeader[48];
    snprintf(uiHeader, sizeof(uiHeader), "UI Elements (%d)", (int)uiElements_.size());
    bool uiOpen = ImGui::TreeNodeEx(uiHeader);
    ImGui::SameLine();
    // + ボタンを押すと新しい UI スプライトを追加する
    if (ImGui::SmallButton("+##addUI")) {
        UIEntry entry;
        entry.name    = "UI Element " + std::to_string(uiElements_.size() + 1);
        entry.texPath = "";
        entry.sprite  = std::make_unique<Sprite>();
        entry.sprite->Initialize(ctx.spriteCommon, entry.texPath);
        entry.sprite->SetPosition({ GameConstants::kScreenCenterX, GameConstants::kScreenCenterY }); // 画面中央に配置
        entry.sprite->SetSize({ 100.0f, 100.0f });
        uiElements_.push_back(std::move(entry));
    }
    if (uiOpen) {
        for (int i = 0; i < (int)uiElements_.size(); ++i) {
            bool sel = (selection_ == Selection::UIElement && selectionIndex_ == i);
            char label[80];
            snprintf(label, sizeof(label), "  %s", uiElements_[i].name.c_str());
            if (ImGui::Selectable(label, sel)) {
                // UIElement を選択したときはインデックスも一緒に保存する
                selection_      = Selection::UIElement;
                selectionIndex_ = i;
                currentState_   = std::make_unique<UIElementState>();
            }
        }
        ImGui::TreePop();
    }

    // Save ボタン：選択中オブジェクトのパラメータを JSON に保存する
    ImGui::Separator();
    if (savedTimer_ > 0.0f) {
        // 保存直後は "Saved!" と表示してフェードアウトする
        savedTimer_ -= GameConstants::kFrameDeltaTime;
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Saved!");
    } else {
        if (ImGui::Button("Save", ImVec2(-1, 0))) {
            if (selection_ == Selection::Camera)    { SaveCameraParams(ctx); savedTimer_ = 1.5f; }
            if (selection_ == Selection::UIElement) { SaveUILayout();        savedTimer_ = 1.5f; }
        }
    }

    ImGui::End();
#endif
}

// ============================================================
// Inspector パネル（画面右）
// ============================================================

// 現在選ばれているオブジェクトの詳細プロパティを表示する。
// 描画の実体は currentState_（各 State クラスの RenderInspector）に委譲する。
void SceneEditor::RenderInspector(const EditContext& ctx)
{
#ifdef USE_IMGUI
    // UIElement を選んでいるのにインデックスが範囲外なら選択を解除する
    if (selection_ == Selection::UIElement &&
        (selectionIndex_ < 0 || selectionIndex_ >= (int)uiElements_.size())) {
        ChangeState(Selection::None);
    }

    ImGui::SetNextWindowPos(ImVec2(kInspectorX, kInspectorY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(kInspectorW, kInspectorH), ImGuiCond_Once);
    ImGui::Begin("Inspector");

    if (currentState_) {
        currentState_->RenderInspector(ctx, *this);
    } else {
        // State が初期化されていない場合の安全処理
        currentState_ = std::make_unique<NoneState>();
        currentState_->RenderInspector(ctx, *this);
    }

    ImGui::End();
#endif
}

// ============================================================
// Scene Controls パネル（画面左下）
// ============================================================

// スコアの確認・ゲーム時刻の表示・シーン切り替えボタンをまとめたパネル。
void SceneEditor::RenderSceneControls(const EditContext& ctx)
{
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(kSceneCtrlX, kSceneCtrlY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(kSceneCtrlW, kSceneCtrlH), ImGuiCond_Once);
    ImGui::Begin("Scene Controls");

    // スコアランキングの表示（折りたたみ）
    if (ImGui::CollapsingHeader("Score")) {
        ImGui::Text("Current : %d", ScoreManager::GetInstance()->GetCurrentScore());
        const auto& ranking = ScoreManager::GetInstance()->GetRanking();
        if (ranking.empty()) { ImGui::TextDisabled("  (no records)"); }
        for (int i = 0; i < (int)ranking.size(); ++i) {
            ImGui::Text("  %2d. %d", i + 1, ranking[i]);
        }
        if (ImGui::Button("Reset All Scores")) {
            ScoreManager::GetInstance()->ResetAllScores();
        }
    }

    // ゲーム内時刻の表示（折りたたみ）
    if (ImGui::CollapsingHeader("Game Time")) {
        ImGui::Text("Time : %02d:%02d", ctx.gameHour, ctx.gameMinute);
    }

    // シーン切り替えボタン（折りたたみ）
    if (ImGui::CollapsingHeader("Actions")) {
        if (ImGui::Button("Game Clear"))  { SceneManager::GetInstance()->ChangeScene("CLEAR");    }
        ImGui::SameLine();
        if (ImGui::Button("Game Over"))   { SceneManager::GetInstance()->ChangeScene("GAMEOVER"); }
    }

    ImGui::End();
#endif
}

// ============================================================
// Camera Control パネル（画面上中央）
// ============================================================

// カメラの目標位置・角度・スムージングフレーム数を素早く調整するための簡易パネル。
// Inspector の Camera よりも手軽に操作できるよう別パネルとして用意している。
void SceneEditor::RenderCameraControl(const EditContext& ctx)
{
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(kCamCtrlX, kCamCtrlY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(kCamCtrlW, kCamCtrlH), ImGuiCond_Once);
    ImGui::Begin("Camera Control");

    // ドラッグスライダーで目標座標・角度を変更する
    ImGui::DragFloat3("Pos", &ctx.cameraTargetPos->x, 0.1f);
    ImGui::DragFloat3("Rot", &ctx.cameraTargetRot->x, 0.01f);

    // スムージングのフレーム数を変えたら履歴をクリアする（古い平均が混入しないように）
    if (ImGui::SliderInt("Smooth Frames", ctx.cameraSmoothFrames, 1, 60)) {
        ctx.cameraPosHistory->clear();
        ctx.cameraRotHistory->clear();
    }

    ImGui::Spacing();

    // 原点にリセットするボタン
    if (ImGui::Button("Center")) {
        *ctx.cameraTargetPos = { 0.0f, 0.0f, 0.0f };
        *ctx.cameraTargetRot = { 0.0f, 0.0f, 0.0f };
        ctx.cameraPosHistory->clear();
        ctx.cameraRotHistory->clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save##cam")) { SaveCameraParams(ctx); } // JSON に現在値を保存
    ImGui::SameLine();
    if (ImGui::Button("Load##cam")) { LoadCameraParams(ctx); } // JSON から前回値を復元

    ImGui::End();
#endif
}

// ============================================================
// JSON 永続化
// ============================================================

// カメラの位置・角度・スムージングフレーム数を JSON ファイルに書き出す。
// "Resources/debug_camera.json" に保存されるので、次回起動時に LoadCameraParams で復元できる。
void SceneEditor::SaveCameraParams(const EditContext& ctx)
{
    std::ofstream f("Resources/debug_camera.json");
    if (!f) { return; } // ファイルを開けなければ何もしない
    const Vector3& pos = *ctx.cameraTargetPos;
    const Vector3& rot = *ctx.cameraTargetRot;
    // JSON 形式で書き出す（手書きで組み立てている簡易実装）
    f << "{\n"
      << "  \"camera_pos_x\": " << pos.x << ",\n"
      << "  \"camera_pos_y\": " << pos.y << ",\n"
      << "  \"camera_pos_z\": " << pos.z << ",\n"
      << "  \"camera_rot_x\": " << rot.x << ",\n"
      << "  \"camera_rot_y\": " << rot.y << ",\n"
      << "  \"camera_rot_z\": " << rot.z << ",\n"
      << "  \"camera_smooth_frames\": " << *ctx.cameraSmoothFrames << "\n"
      << "}\n";
}

// カメラの位置・角度・スムージングフレーム数を JSON ファイルから読み込む。
// キーが見つからなければ現在の値をそのまま使う（JsonHelper の第3引数 = デフォルト値）。
void SceneEditor::LoadCameraParams(const EditContext& ctx)
{
    std::ifstream f("Resources/debug_camera.json");
    if (!f) { return; } // ファイルがなければ何もしない（初回起動時など）

    // ファイル全体を文字列として読み込む
    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    Vector3& pos = *ctx.cameraTargetPos;
    Vector3& rot = *ctx.cameraTargetRot;

    // JSON から各値を取り出して変数に代入する
    pos = {
        JsonHelper::ReadFloat(src, "camera_pos_x", pos.x),
        JsonHelper::ReadFloat(src, "camera_pos_y", pos.y),
        JsonHelper::ReadFloat(src, "camera_pos_z", pos.z),
    };
    rot = {
        JsonHelper::ReadFloat(src, "camera_rot_x", rot.x),
        JsonHelper::ReadFloat(src, "camera_rot_y", rot.y),
        JsonHelper::ReadFloat(src, "camera_rot_z", rot.z),
    };
    *ctx.cameraSmoothFrames = JsonHelper::ReadInt(src, "camera_smooth_frames", *ctx.cameraSmoothFrames);

    // 読み込んだ値を実際のカメラに即時反映し、スムージング履歴もリセットする
    ctx.camera->SetTranslate(pos);
    ctx.camera->SetRotate(rot);
    ctx.cameraPosHistory->clear();
    ctx.cameraRotHistory->clear();
}

// UI スプライトのレイアウト（位置・サイズ・色・テクスチャパスなど）を JSON に書き出す。
void SceneEditor::SaveUILayout()
{
    std::ofstream f("Resources/debug_ui.json");
    if (!f) { return; }

    // UI スプライトの個数を最初に書いておく（読み込み時にループ回数として使う）
    f << "{\n  \"count\": " << uiElements_.size();

    for (int i = 0; i < (int)uiElements_.size(); ++i) {
        const UIEntry& e  = uiElements_[i];
        Sprite*        sp = e.sprite.get();
        Vector2 pos = sp->GetPosition();
        Vector2 sz  = sp->GetSize();
        Vector4 col = sp->GetColor();
        float   rot = sp->GetRotation();

        // キー名に連番を付けてユニークにする。例: "ui_0_pos_x", "ui_1_tex" など
        auto mk = [&](const char* field) { return "ui_" + std::to_string(i) + field; };

        f << ",\n"
          << "  \"" << mk("_name")  << "\": \"" << e.name     << "\",\n"
          << "  \"" << mk("_tex")   << "\": \"" << e.texPath  << "\",\n"
          << "  \"" << mk("_pos_x") << "\": " << pos.x << ",\n"
          << "  \"" << mk("_pos_y") << "\": " << pos.y << ",\n"
          << "  \"" << mk("_sz_x")  << "\": " << sz.x  << ",\n"
          << "  \"" << mk("_sz_y")  << "\": " << sz.y  << ",\n"
          << "  \"" << mk("_rot")   << "\": " << rot   << ",\n"
          << "  \"" << mk("_col_r") << "\": " << col.x << ",\n"
          << "  \"" << mk("_col_g") << "\": " << col.y << ",\n"
          << "  \"" << mk("_col_b") << "\": " << col.z << ",\n"
          << "  \"" << mk("_col_a") << "\": " << col.w;
    }
    f << "\n}\n";
}

// 起動時に1度だけ呼ばれ、カメラパラメータと UI レイアウトを一括で読み込む。
void SceneEditor::LoadAll(const EditContext& ctx)
{
    LoadCameraParams(ctx);
    LoadUILayout(ctx);
}

// UI スプライトのレイアウトを JSON ファイルから復元する。
void SceneEditor::LoadUILayout(const EditContext& ctx)
{
    std::ifstream f("Resources/debug_ui.json");
    if (!f) { return; } // ファイルがなければ何もしない

    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // "count" キーから UI スプライトの個数を取得してループ回数にする
    int count = JsonHelper::ReadInt(src, "count", 0);
    uiElements_.clear();
    ChangeState(Selection::None);

    for (int i = 0; i < count; ++i) {
        auto mk = [&](const char* field) { return "ui_" + std::to_string(i) + field; };
        UIEntry entry;
        entry.name    = JsonHelper::ReadString(src, mk("_name"), "UI Element");
        entry.texPath = JsonHelper::ReadString(src, mk("_tex"),  "");

        entry.sprite = std::make_unique<Sprite>();
        entry.sprite->Initialize(ctx.spriteCommon, entry.texPath);

        // 各プロパティを JSON から復元する（キーがなければデフォルト値を使う）
        entry.sprite->SetPosition({
            JsonHelper::ReadFloat(src, mk("_pos_x"), GameConstants::kScreenCenterX),
            JsonHelper::ReadFloat(src, mk("_pos_y"), GameConstants::kScreenCenterY) });
        entry.sprite->SetSize({
            JsonHelper::ReadFloat(src, mk("_sz_x"), 100.0f),
            JsonHelper::ReadFloat(src, mk("_sz_y"), 100.0f) });
        entry.sprite->SetRotation(JsonHelper::ReadFloat(src, mk("_rot"), 0.0f));
        entry.sprite->SetColor({
            JsonHelper::ReadFloat(src, mk("_col_r"), 1.0f),
            JsonHelper::ReadFloat(src, mk("_col_g"), 1.0f),
            JsonHelper::ReadFloat(src, mk("_col_b"), 1.0f),
            JsonHelper::ReadFloat(src, mk("_col_a"), 1.0f) });

        uiElements_.push_back(std::move(entry));
    }
}

// ============================================================
// IEditorState 実装（各オブジェクト種別ごとの Inspector 描画）
// ============================================================

// 何も選んでいないときの Inspector 表示
void SceneEditor::NoneState::RenderInspector(const EditContext&, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextDisabled("(Nothing selected)");
    ImGui::TextDisabled("Hierarchy でオブジェクトを");
    ImGui::TextDisabled("クリックしてください");
#endif
}

// Camera を選んでいるときの Inspector 表示
void SceneEditor::CameraState::RenderInspector(const EditContext& ctx, SceneEditor& editor)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Camera]");
    ImGui::Separator();
    ImGui::DragFloat3("Position", &ctx.cameraTargetPos->x, 0.1f);  // ドラッグで位置を変更
    ImGui::DragFloat3("Rotation", &ctx.cameraTargetRot->x, 0.01f); // ドラッグで角度を変更
    // スムージングフレーム数を変えたら履歴をリセットしないと古い平均が残ってしまう
    if (ImGui::SliderInt("Smooth Frames", ctx.cameraSmoothFrames, 1, 60)) {
        ctx.cameraPosHistory->clear();
        ctx.cameraRotHistory->clear();
    }
    ImGui::Separator();
    if (ImGui::Button("Save##inspCam")) { editor.SaveCameraParams(ctx); }
#endif
}

// Ring を選んでいるときの Inspector 表示
void SceneEditor::RingState::RenderInspector(const EditContext& ctx, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1), "[Ring]");
    ImGui::Separator();
    // 値を変更した瞬間に Set～ メソッドを呼んでゲームに即時反映する
    if (ImGui::DragFloat3("Position", &ctx.ringPosition->x, 0.1f))  { ctx.ring->SetPosition(*ctx.ringPosition); }
    if (ImGui::DragFloat3("Rotation", &ctx.ringRotation->x, 0.01f)) { ctx.ring->SetRotation(*ctx.ringRotation); }
    if (ImGui::DragFloat("Scale", ctx.ringScale, 0.01f, 0.01f, 20.0f)) { ctx.ring->SetScale(*ctx.ringScale); }
    ImGui::Separator();
    // 内径は外径より必ず小さく、外径は内径より必ず大きくなるよう上限・下限を設ける
    if (ImGui::DragFloat("Inner Radius", ctx.ringInnerRadius, 0.05f, 0.01f, *ctx.ringOuterRadius - 0.01f)) {
        ctx.ring->SetInnerRadius(*ctx.ringInnerRadius);
    }
    if (ImGui::DragFloat("Outer Radius", ctx.ringOuterRadius, 0.05f, *ctx.ringInnerRadius + 0.01f, 50.0f)) {
        ctx.ring->SetOuterRadius(*ctx.ringOuterRadius);
    }
    ImGui::Separator();
    if (ImGui::ColorEdit4("Color", &ctx.ringColor->x)) { ctx.ring->SetColor(*ctx.ringColor); }
#endif
}

// Cylinder を選んでいるときの Inspector 表示
void SceneEditor::CylinderState::RenderInspector(const EditContext& ctx, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1), "[Cylinder]");
    ImGui::Separator();
    if (ImGui::DragFloat3("Position", &ctx.cylinderPosition->x, 0.1f))  { ctx.cylinder->SetPosition(*ctx.cylinderPosition); }
    if (ImGui::DragFloat3("Rotation", &ctx.cylinderRotation->x, 0.01f)) { ctx.cylinder->SetRotation(*ctx.cylinderRotation); }
    if (ImGui::DragFloat("Scale", ctx.cylinderScale, 0.01f, 0.01f, 20.0f)) { ctx.cylinder->SetScale(*ctx.cylinderScale); }
    ImGui::Separator();
    if (ImGui::DragFloat("Top Radius",    ctx.cylinderTopRadius,    0.05f, 0.01f, 50.0f)) { ctx.cylinder->SetTopRadius(*ctx.cylinderTopRadius); }
    if (ImGui::DragFloat("Bottom Radius", ctx.cylinderBottomRadius, 0.05f, 0.01f, 50.0f)) { ctx.cylinder->SetBottomRadius(*ctx.cylinderBottomRadius); }
    if (ImGui::DragFloat("Height",        ctx.cylinderHeight,       0.05f, 0.01f, 50.0f)) { ctx.cylinder->SetHeight(*ctx.cylinderHeight); }
    ImGui::Separator();
    if (ImGui::ColorEdit4("Color", &ctx.cylinderColor->x)) { ctx.cylinder->SetColor(*ctx.cylinderColor); }
    // AlphaReference = この値以下のピクセルを透明として描画しない（穴あきエフェクトなどに使う）
    if (ImGui::SliderFloat("Alpha Reference", ctx.cylinderAlphaRef, 0.0f, 1.0f)) { ctx.cylinder->SetAlphaReference(*ctx.cylinderAlphaRef); }
#endif
}

// Skydome を選んでいるときの Inspector 表示
void SceneEditor::SkydomeState::RenderInspector(const EditContext& ctx, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1), "[Skydome]");
    ImGui::Separator();
    if (ctx.skydome) {
        if (ImGui::ColorEdit4("Sky Color", &ctx.skyColor->x)) { ctx.skydome->SetSkyColor(*ctx.skyColor); }
        // 天球を Y 軸回転させることで、「朝」「夕方」「夜」のような方角の変化を演出できる
        if (ImGui::SliderFloat("Rotation Offset Y", ctx.skyRotOffsetY, -GameConstants::kPi, GameConstants::kPi)) {
            ctx.skydome->SetRotationOffsetY(*ctx.skyRotOffsetY);
        }
        if (ImGui::Button("Reset Offset")) {
            *ctx.skyRotOffsetY = 0.0f;
            ctx.skydome->SetRotationOffsetY(0.0f);
        }
    } else {
        ImGui::TextDisabled("Skydome is disabled.");
    }
#endif
}

// Human を選んでいるときの Inspector 表示
void SceneEditor::HumanState::RenderInspector(const EditContext& ctx, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1), "[Human]");
    ImGui::Separator();
    if (ImGui::DragFloat3("Position", &ctx.humanPosition->x, 0.05f)) { ctx.human->SetPosition(*ctx.humanPosition); }
    if (ImGui::DragFloat3("Rotation", &ctx.humanRotation->x, 0.01f)) { ctx.human->SetRotation(*ctx.humanRotation); }
    if (ImGui::DragFloat3("Scale",    &ctx.humanScale->x, 0.01f, 0.001f, 100.0f)) { ctx.human->SetScale(*ctx.humanScale); }
    ImGui::Separator();
    // アニメーション速度。0.0f で一時停止、2.0f で2倍速になる
    if (ImGui::SliderFloat("Anim Speed", ctx.humanAnimSpeed, 0.0f, 5.0f)) { ctx.human->SetAnimSpeed(*ctx.humanAnimSpeed); }
    ImGui::Separator();
    // Reset ボタンで初期値に戻す
    if (ImGui::Button("Reset")) {
        *ctx.humanPosition  = { 5.0f, 0.0f, 0.0f };
        *ctx.humanRotation  = {};
        *ctx.humanScale     = { 1.0f, 1.0f, 1.0f };
        *ctx.humanAnimSpeed = 1.0f;
        ctx.human->SetPosition(*ctx.humanPosition);
        ctx.human->SetRotation(*ctx.humanRotation);
        ctx.human->SetScale(*ctx.humanScale);
        ctx.human->SetAnimSpeed(*ctx.humanAnimSpeed);
    }
    ImGui::Separator();
    // ボーン（骨格）のデバッグ表示フラグ
    ImGui::Checkbox("Show Skeleton", ctx.showSkeleton);
#endif
}

// White Particles を選んでいるときの Inspector 表示
void SceneEditor::ParticlesState::RenderInspector(const EditContext& ctx, SceneEditor&)
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1), "[White Particles]");
    ImGui::Separator();
    ImGui::DragFloat3("Position", &ctx.whiteParticlePos->x, 0.1f);
    ImGui::ColorEdit4("Color",    &ctx.whiteParticleColor->x);
    ImGui::DragFloat("Scale",     ctx.whiteParticleScale, 0.01f, 0.01f, 10.0f);
    ImGui::SliderInt("Count",     ctx.whiteParticleCount, 1, 1024);
    ImGui::Separator();
    // Re-emit = 現在のパラメータで白パーティクルを撒き直す
    if (ImGui::Button("Re-emit", ImVec2(-1, 0))) {
        ParticleManager::GetInstance()->EmitScatterLoop(
            "white", *ctx.whiteParticlePos, 20.0f,
            static_cast<uint32_t>(*ctx.whiteParticleCount),
            *ctx.whiteParticleColor, 2.0f, 5.0f, *ctx.whiteParticleScale);
    }
#endif
}

// UI Element を選んでいるときの Inspector 表示
void SceneEditor::UIElementState::RenderInspector(const EditContext& ctx, SceneEditor& editor)
{
#ifdef USE_IMGUI
    int idx = editor.selectionIndex_;
    if (idx < 0 || idx >= (int)editor.uiElements_.size()) { return; } // 範囲外なら何もしない

    UIEntry& entry = editor.uiElements_[idx];
    Sprite*  sp    = entry.sprite.get();

    ImGui::TextColored(ImVec4(1, 0.5f, 1, 1), "[UI Element]");
    ImGui::Separator();

    // 名前編集用のバッファ。選択が変わったときだけバッファを更新する
    static int   lastIdx       = -2;
    static char  uiNameBuf[64] = {};
    if (lastIdx != idx) {
        lastIdx = idx;
        strncpy_s(uiNameBuf, entry.name.c_str(), sizeof(uiNameBuf) - 1);
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##uiname", uiNameBuf, sizeof(uiNameBuf))) { entry.name = uiNameBuf; }
    ImGui::Separator();

    // 位置・サイズ・回転・色を編集する
    Vector2 pos = sp->GetPosition();
    if (ImGui::DragFloat2("Position", &pos.x, 1.0f))            { sp->SetPosition(pos); }
    Vector2 sz  = sp->GetSize();
    if (ImGui::DragFloat2("Size", &sz.x, 1.0f, 1.0f, 4096.0f)) { sp->SetSize(sz); }
    float rot = sp->GetRotation();
    if (ImGui::DragFloat("Rotation", &rot, 0.01f))               { sp->SetRotation(rot); }
    Vector4 col = sp->GetColor();
    if (ImGui::ColorEdit4("Color", &col.x))                      { sp->SetColor(col); }
    ImGui::Separator();

    // テクスチャファイルのパスを表示し、Browse ボタンでファイル選択ダイアログを開く
    ImGui::TextDisabled("%.40s", entry.texPath.empty() ? "(no texture)" : entry.texPath.c_str());
    if (ImGui::Button("Browse##uitex")) {
        std::string p = OpenFileDialog("PNG Files\0*.png\0All Files\0*.*\0\0", "Resources");
        if (!p.empty()) { entry.texPath = p; sp->SetTexture(p); }
    }
    ImGui::Separator();

    // Delete で一覧から削除、Save/Load で JSON に保存・復元する
    if (ImGui::Button("Delete##ui")) {
        editor.uiElements_.erase(editor.uiElements_.begin() + idx);
        editor.ChangeState(Selection::None);
        lastIdx = -2;
        editor.SaveUILayout();
    } else {
        ImGui::SameLine();
        if (ImGui::Button("Save##inspUI"))  { editor.SaveUILayout();           }
        ImGui::SameLine();
        if (ImGui::Button("Load##inspUI"))  { editor.LoadUILayout(ctx);        }
    }
#endif
}
