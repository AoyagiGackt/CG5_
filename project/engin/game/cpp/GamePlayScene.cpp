#include "GamePlayScene.h"
#include <algorithm>
#include <cmath>
#include <commdlg.h>
#include <fstream>
#include <sstream>
#pragma comment(lib, "comdlg32.lib")
#include "ImguiManager.h"
#include "ParticleManager.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include "TextureManager.h"

// =====================================================
// 初期化
// =====================================================

void GamePlayScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    dxCommon_ = dxCommon;
    input_ = input;
    audio_ = audio;

    // ----- 描画共通設定 -----
    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dxCommon_);

    modelCommon_ = std::make_unique<ModelCommon>();
    modelCommon_->Initialize(dxCommon_);

    objectCommon_ = std::make_unique<Object3dCommon>();
    objectCommon_->Initialize(dxCommon_);

    shadowManager_ = std::make_unique<ShadowManager>();
    shadowManager_->Initialize(dxCommon_, SrvManager::GetInstance());

    // ----- カメラ -----
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 14.5f, 6.0f, -30.0f });
    Object3d::SetCommonCamera(camera_.get());

    // ----- キャラクター・オブジェクト -----
    modelSkydome_ = std::make_unique<Model>();
    modelSkydome_->Initialize(modelCommon_.get(), "Resources/SkyDome/SkyDome.obj", "Resources/rostock_laage_airport_4k.dds");

    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize(modelCommon_.get(), modelSkydome_.get());

    auto hoge = std::make_unique<Hoge>();
    hoge->Initialize(modelCommon_.get(), dxCommon, input, audio);
    animCube_ = hoge.get();
    gameObjects_.push_back(std::move(hoge));

    // ----- スキンメッシュ（human）-----
    skinCommon_ = std::make_unique<SkinCommon>();
    skinCommon_->Initialize(dxCommon_);

    modelHuman_ = std::make_unique<SkinnedModel>();
    modelHuman_->Initialize(dxCommon_,
        "Resources/human/sneakWalk.gltf",
        "Resources/human/white.png");

    Node humanRootNode = LoadNodeHierarchyFromFile("Resources/human", "sneakWalk.gltf");
    Skeleton humanSkeleton = Skeleton::Create(humanRootNode);
    Animation humanAnimation = LoadAnimationFile("Resources/human", "sneakWalk.gltf");

    human_ = std::make_unique<SkinnedObject3d>();
    human_->Initialize(skinCommon_.get());
    human_->SetModel(modelHuman_.get());
    human_->SetSkeleton(std::move(humanSkeleton));
    human_->SetAnimation(std::move(humanAnimation));
    human_->SetPosition(humanPosition_);
    SkinnedObject3d::SetCommonCamera(camera_.get());
    SkinnedObject3d::SetCommonObjectCommon(objectCommon_.get());
    SkinnedObject3d::SetCommonShadowManager(shadowManager_.get());
    SkinnedObject3d::SetCommonModelCommon(modelCommon_.get());

    // ----- エフェクト・進行管理 -----
    // 楕円パーティクルグループ（circle2.png を使用）
    ParticleManager::GetInstance()->CreateParticleGroup("ellipse", "Resources/circle2.png");

    // 斬撃パーティクルグループ（gradationLine.png を使用）
    ParticleManager::GetInstance()->CreateParticleGroup("slash", "Resources/gradationLine.png");

    // Ring（gradationLine.png、AddressV=CLAMP）
    ring_ = std::make_unique<Ring>();
    ring_->Initialize(dxCommon_);
    ring_->SetPosition(ringPosition_);
    ring_->SetInnerRadius(ringInnerRadius_);
    ring_->SetOuterRadius(ringOuterRadius_);

    // Cylinder
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(dxCommon_);
    cylinder_->SetPosition(cylinderPosition_);
    cylinder_->SetTopRadius(cylinderTopRadius_);
    cylinder_->SetBottomRadius(cylinderBottomRadius_);
    cylinder_->SetHeight(cylinderHeight_);

    ScoreManager::GetInstance()->LoadScores();
    ScoreManager::GetInstance()->ResetCurrentScore();
    gameTime_.Initialize();

    // ----- デバッグパラメータ読み込み -----
    LoadCameraParams();
    LoadModelPaths();
    LoadUILayout();
}

/// <summary>
/// 更新処理
/// </summary>

void GamePlayScene::Update()
{
    // ゲーム内時刻の更新
    gameTime_.Update(1.0f);

    float timeRatio = gameTime_.GetElapsedMinutes() / GameTime::kTotalGameMinutes;

    // 天球の更新
    skydome_->Update(camera_.get(), timeRatio);

    // シャドウの更新
    shadowManager_->Update(objectCommon_->GetLightDirection());
    Object3d::SetLightViewProjection(shadowManager_->GetLightViewProjection());
    SkinnedObject3d::SetLightViewProjection(shadowManager_->GetLightViewProjection());

    // ゲームオブジェクトの更新
    for (auto& obj : gameObjects_) {
        obj->Update();
    }

    human_->Update();

    // UIと描画関連の更新
    UpdateDebugUI();

    // 楕円パーティクルをリング周回軌道で放出
    static constexpr float kOrbitSpeed = 3.14159265f * 2.0f / 3.0f; // 1周/3秒
    ringOrbitAngle_ += kOrbitSpeed / 60.0f;
    if (ringOrbitAngle_ > 3.14159265f * 2.0f) ringOrbitAngle_ -= 3.14159265f * 2.0f;

    ellipseParticleTimer_ += 1.0f / 60.0f;
    while (ellipseParticleTimer_ >= kEllipseEmitInterval) {
        ellipseParticleTimer_ -= kEllipseEmitInterval;

        float r = (ringInnerRadius_ + ringOuterRadius_) * 0.5f; // リングの中間半径
        Vector3 emitPos = {
            ringPosition_.x + std::cos(ringOrbitAngle_) * r,
            ringPosition_.y,
            ringPosition_.z + std::sin(ringOrbitAngle_) * r
        };
        // 接線方向 + わずかな上昇
        float tangentSpeed = 0.03f;
        Vector3 vel = {
            -std::sin(ringOrbitAngle_) * tangentSpeed,
            0.04f,
            std::cos(ringOrbitAngle_) * tangentSpeed
        };

        ParticleManager::GetInstance()->EmitEllipse(
            "ellipse",
            emitPos,
            vel,
            { 0.6f, 0.85f, 1.0f, 1.0f },
            1.5f,
            0.4f,
            0.2f
        );
    }

    ring_->Update(camera_.get());
    cylinder_->Update(camera_.get());
    ParticleManager::GetInstance()->Update(camera_.get());
}

// =====================================================
// ファイルダイアログヘルパー（デバッグ専用）
// =====================================================

#ifdef USE_IMGUI
/** @brief Windows標準のファイル選択ダイアログを開き、選択されたパスを返す
 *  @param filter  例: "PNG\0*.png\0OBJ\0*.obj\0All\0*.*\0\0"（\0区切り、末尾\0\0）
 *  @param initialDir  最初に開くフォルダ（nullptr で前回の場所）
 *  @return 選択されたファイルの絶対パス。キャンセル時は空文字列
 */
static std::string OpenFileDialog(const char* filter, const char* initialDir = nullptr)
{
    char szFile[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) ? std::string(szFile) : "";
}
#endif

// =====================================================
// デバッグ UI（ImGui）
// =====================================================

void GamePlayScene::UpdateDebugUI()
{
#ifdef USE_IMGUI
    if (!imguiManager_) {
        return;
    }

    // =====================================================
    // Hierarchy（左パネル）
    // =====================================================
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 400), ImGuiCond_Once);
    ImGui::Begin("Hierarchy");

    if (ImGui::Selectable("Camera", editorSelectedType_ == SelectedType::Camera)) {
        editorSelectedType_ = SelectedType::Camera;
        editorSelectedIndex_ = -1;
    }
    if (ImGui::Selectable("Skydome", editorSelectedType_ == SelectedType::Skydome)) {
        editorSelectedType_ = SelectedType::Skydome;
        editorSelectedIndex_ = -1;
    }
    if (ImGui::Selectable("AnimatedCube", editorSelectedType_ == SelectedType::AnimatedCube)) {
        editorSelectedType_ = SelectedType::AnimatedCube;
        editorSelectedIndex_ = -1;
    }
    if (ImGui::Selectable("Human", editorSelectedType_ == SelectedType::Human)) {
        editorSelectedType_ = SelectedType::Human;
        editorSelectedIndex_ = -1;
    }
    if (ImGui::Selectable("Ring", editorSelectedType_ == SelectedType::Ring)) {
        editorSelectedType_ = SelectedType::Ring;
        editorSelectedIndex_ = -1;
    }
    if (ImGui::Selectable("Cylinder", editorSelectedType_ == SelectedType::Cylinder)) {
        editorSelectedType_ = SelectedType::Cylinder;
        editorSelectedIndex_ = -1;
    }

    // ----- UI Elements -----
    char uiHeader[48];
    snprintf(uiHeader, sizeof(uiHeader), "UI Elements (%d)", (int)uiElements_.size());
    bool uiOpen = ImGui::TreeNodeEx(uiHeader);
    ImGui::SameLine();
    if (ImGui::SmallButton("+##addUI")) {
        UIEntry entry;
        entry.name = "UI Element " + std::to_string(uiElements_.size() + 1);
        entry.texPath = "";
        entry.sprite = std::make_unique<Sprite>();
        entry.sprite->Initialize(spriteCommon_.get(), entry.texPath);
        entry.sprite->SetPosition({ 640.0f, 360.0f });
        entry.sprite->SetSize({ 100.0f, 100.0f });
        uiElements_.push_back(std::move(entry));
    }
    if (uiOpen) {
        for (int i = 0; i < (int)uiElements_.size(); i++) {
            bool sel = (editorSelectedType_ == SelectedType::UIElement && editorSelectedIndex_ == i);
            char label[80];
            snprintf(label, sizeof(label), "  %s", uiElements_[i].name.c_str());
            if (ImGui::Selectable(label, sel)) {
                editorSelectedType_ = SelectedType::UIElement;
                editorSelectedIndex_ = i;
            }
        }
        ImGui::TreePop();
    }

    // ----- Save ボタン（選択中のオブジェクトに応じて保存先を切り替え） -----
    ImGui::Separator();
    static float savedTimer = 0.0f;
    if (savedTimer > 0.0f) {
        savedTimer -= 1.0f / 60.0f;
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Saved!");
    } else {
        if (ImGui::Button("Save", ImVec2(-1, 0))) {
            switch (editorSelectedType_) {
            case SelectedType::Camera:
                SaveCameraParams();
                savedTimer = 1.5f;
                break;
            case SelectedType::UIElement:
                SaveUILayout();
                savedTimer = 1.5f;
                break;
            default:
                break;
            }
        }
    }

    ImGui::End();

    // =====================================================
    // Inspector（右パネル）
    // =====================================================
    ImGui::SetNextWindowPos(ImVec2(1060, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 500), ImGuiCond_Once);
    ImGui::Begin("Inspector");

    // 選択インデックスが範囲外になった場合にリセット
    if (editorSelectedType_ == SelectedType::UIElement && (editorSelectedIndex_ < 0 || editorSelectedIndex_ >= (int)uiElements_.size())) {
        editorSelectedType_ = SelectedType::None;
    }

    switch (editorSelectedType_) {

    case SelectedType::Camera: {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Camera]");
        ImGui::Separator();
        Vector3 pos = camera_->GetTranslate();
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            camera_->SetTranslate(pos);
        }

        Vector3 rot = camera_->GetRotate();
        
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
            camera_->SetRotate(rot);
        }

        ImGui::Separator();
        
        if (ImGui::Button("Save##inspCam")) {
            SaveCameraParams();
        }

        break;
    }

    case SelectedType::Ring: {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1), "[Ring]");
        ImGui::Separator();

        if (ImGui::DragFloat3("Position", &ringPosition_.x, 0.1f)) {
            ring_->SetPosition(ringPosition_);
        }

        if (ImGui::DragFloat3("Rotation", &ringRotation_.x, 0.01f)) {
            ring_->SetRotation(ringRotation_);
        }

        if (ImGui::DragFloat("Scale", &ringScale_, 0.01f, 0.01f, 20.0f)) {
            ring_->SetScale(ringScale_);
        }

        ImGui::Separator();

        if (ImGui::DragFloat("Inner Radius", &ringInnerRadius_, 0.05f, 0.01f, ringOuterRadius_ - 0.01f)) {
            ring_->SetInnerRadius(ringInnerRadius_);
        }

        if (ImGui::DragFloat("Outer Radius", &ringOuterRadius_, 0.05f, ringInnerRadius_ + 0.01f, 50.0f)) {
            ring_->SetOuterRadius(ringOuterRadius_);
        }

        ImGui::Separator();

        if (ImGui::ColorEdit4("Color", &ringColor_.x)) {
            ring_->SetColor(ringColor_);
        }

        break;
    }

    case SelectedType::Cylinder: {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1), "[Cylinder]");
        ImGui::Separator();

        if (ImGui::DragFloat3("Position", &cylinderPosition_.x, 0.1f)) {
            cylinder_->SetPosition(cylinderPosition_);
        }
        if (ImGui::DragFloat3("Rotation", &cylinderRotation_.x, 0.01f)) {
            cylinder_->SetRotation(cylinderRotation_);
        }
        if (ImGui::DragFloat("Scale", &cylinderScale_, 0.01f, 0.01f, 20.0f)) {
            cylinder_->SetScale(cylinderScale_);
        }

        ImGui::Separator();

        if (ImGui::DragFloat("Top Radius", &cylinderTopRadius_, 0.05f, 0.01f, 50.0f)) {
            cylinder_->SetTopRadius(cylinderTopRadius_);
        }
        if (ImGui::DragFloat("Bottom Radius", &cylinderBottomRadius_, 0.05f, 0.01f, 50.0f)) {
            cylinder_->SetBottomRadius(cylinderBottomRadius_);
        }
        if (ImGui::DragFloat("Height", &cylinderHeight_, 0.05f, 0.01f, 50.0f)) {
            cylinder_->SetHeight(cylinderHeight_);
        }

        ImGui::Separator();

        if (ImGui::ColorEdit4("Color", &cylinderColor_.x)) {
            cylinder_->SetColor(cylinderColor_);
        }
        if (ImGui::SliderFloat("Alpha Reference", &cylinderAlphaRef_, 0.0f, 1.0f)) {
            cylinder_->SetAlphaReference(cylinderAlphaRef_);
        }

        break;
    }

    case SelectedType::Skydome: {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1), "[Skydome]");
        ImGui::Separator();

        if (ImGui::ColorEdit4("Sky Color", &skyColor_.x)) {
            skydome_->SetSkyColor(skyColor_);
        }

        ImGui::Separator();

        if (ImGui::SliderFloat("Rotation Offset Y", &skyRotOffsetY_, -3.14159265f, 3.14159265f)) {
            skydome_->SetRotationOffsetY(skyRotOffsetY_);
        }

        if (ImGui::Button("Reset Offset")) {
            skyRotOffsetY_ = 0.0f;
            skydome_->SetRotationOffsetY(0.0f);
        }

        break;
    }

    case SelectedType::UIElement: {
        UIEntry& entry = uiElements_[editorSelectedIndex_];
        Sprite* sp = entry.sprite.get();
        ImGui::TextColored(ImVec4(1, 0.5f, 1, 1), "[UI Element]");
        ImGui::Separator();

        // 名前変更
        static int lastUIIdx = -2;
        static char uiNameBuf[64] = {};
        
        if (lastUIIdx != editorSelectedIndex_) {
            lastUIIdx = editorSelectedIndex_;
            strncpy_s(uiNameBuf, entry.name.c_str(), sizeof(uiNameBuf) - 1);
        }

        ImGui::SetNextItemWidth(-1);
        
        if (ImGui::InputText("##uiname", uiNameBuf, sizeof(uiNameBuf))) {
            entry.name = uiNameBuf;
        }

        ImGui::Separator();

        // 位置・サイズ・回転
        Vector2 pos = sp->GetPosition();
        
        if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) {
            sp->SetPosition(pos);
        }

        Vector2 sz = sp->GetSize();
        
        if (ImGui::DragFloat2("Size", &sz.x, 1.0f, 1.0f, 4096.0f)) {
            sp->SetSize(sz);
        }

        float rot = sp->GetRotation();
        
        if (ImGui::DragFloat("Rotation", &rot, 0.01f)) {
            sp->SetRotation(rot);
        }

        // 色
        Vector4 col = sp->GetColor();
        
        if (ImGui::ColorEdit4("Color", &col.x)) {
            sp->SetColor(col);
        }

        ImGui::Separator();

        // テクスチャ
        ImGui::TextDisabled("%.40s", entry.texPath.empty() ? "(no texture)" : entry.texPath.c_str());
        
        if (ImGui::Button("Browse##uitex")) {
            std::string p = OpenFileDialog("PNG Files\0*.png\0All Files\0*.*\0\0", "Resources");
            
            if (!p.empty()) {
                entry.texPath = p;
                sp->SetTexture(p);
            }
        }

        ImGui::Separator();
        
        if (ImGui::Button("Delete##ui")) {
            uiElements_.erase(uiElements_.begin() + editorSelectedIndex_);
            editorSelectedType_ = SelectedType::None;
            editorSelectedIndex_ = -1;
            lastUIIdx = -2;
            SaveUILayout();
        } else {
            ImGui::SameLine();
            
            if (ImGui::Button("Save##inspUI")) {
                SaveUILayout();
            }

            ImGui::SameLine();
            
            if (ImGui::Button("Load##inspUI")) {
                LoadUILayout();
            }
        }

        break;
    }

    case SelectedType::AnimatedCube: {
        ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1), "[AnimatedCube]");
        ImGui::Separator();

        if (ImGui::DragFloat3("Position", &animCubePosition_.x, 0.1f)) {
            if (animCube_) { animCube_->SetWorldOffset(animCubePosition_); }
        }

        if (ImGui::SliderFloat("Anim Speed", &animCubeSpeed_, 0.0f, 5.0f)) {
            if (animCube_) { animCube_->SetAnimSpeed(animCubeSpeed_); }
        }

        if (ImGui::Button("Reset Position")) {
            animCubePosition_ = { 10.0f, 2.0f, 0.0f };
            if (animCube_) { animCube_->SetWorldOffset(animCubePosition_); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Speed")) {
            animCubeSpeed_ = 1.0f;
            if (animCube_) { animCube_->SetAnimSpeed(animCubeSpeed_); }
        }

        break;
    }

    case SelectedType::Human: {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1), "[Human]");
        ImGui::Separator();

        if (ImGui::DragFloat3("Position", &humanPosition_.x, 0.05f)) {
            if (human_) { human_->SetPosition(humanPosition_); }
        }
        if (ImGui::DragFloat3("Rotation", &humanRotation_.x, 0.01f)) {
            if (human_) { human_->SetRotation(humanRotation_); }
        }
        if (ImGui::DragFloat3("Scale", &humanScale_.x, 0.01f, 0.001f, 100.0f)) {
            if (human_) { human_->SetScale(humanScale_); }
        }

        ImGui::Separator();

        if (ImGui::SliderFloat("Anim Speed", &humanAnimSpeed_, 0.0f, 5.0f)) {
            if (human_) { human_->SetAnimSpeed(humanAnimSpeed_); }
        }

        ImGui::Separator();

        if (ImGui::Button("Reset")) {
            humanPosition_  = { 5.0f, 0.0f, 0.0f };
            humanRotation_  = { 0.0f, 0.0f, 0.0f };
            humanScale_     = { 1.0f, 1.0f, 1.0f };
            humanAnimSpeed_ = 1.0f;
            if (human_) {
                human_->SetPosition(humanPosition_);
                human_->SetRotation(humanRotation_);
                human_->SetScale(humanScale_);
                human_->SetAnimSpeed(humanAnimSpeed_);
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("Show Skeleton", &showSkeletonDebug_);

        break;
    }

    default:
        ImGui::TextDisabled("(Nothing selected)");
        ImGui::TextDisabled("Hierarchy でオブジェクトを");
        ImGui::TextDisabled("クリックしてください");
        break;
    }

    ImGui::End();

    // =====================================================
    // Scene Controls（左下パネル）
    // =====================================================
    ImGui::SetNextWindowPos(ImVec2(0, 400), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(220, 320), ImGuiCond_Once);
    ImGui::Begin("Scene Controls");

    // スコア
    if (ImGui::CollapsingHeader("Score")) {
        ImGui::Text("Current : %d", ScoreManager::GetInstance()->GetCurrentScore());
        const auto& ranking = ScoreManager::GetInstance()->GetRanking();
        
        if (ranking.empty()) {
            ImGui::TextDisabled("  (no records)");
        }

        for (int i = 0; i < (int)ranking.size(); ++i) {
            ImGui::Text("  %2d. %d", i + 1, ranking[i]);
        }

        if (ImGui::Button("Reset All Scores")) {
            ScoreManager::GetInstance()->ResetAllScores();
        }
    }

    // ゲーム時刻
    if (ImGui::CollapsingHeader("Game Time")) {
        ImGui::Text("Time : %02d:%02d", gameTime_.GetHour(), gameTime_.GetMinute());
        
        if (ImGui::Button("Skip 1 Hour")) {
            gameTime_.SkipMinutes(60.0f);
        }
    }

    // アクション
    if (ImGui::CollapsingHeader("Actions")) {
        if (ImGui::Button("Game Clear")) {
            SceneManager::GetInstance()->ChangeScene("CLEAR");
        }

        ImGui::SameLine();

        if (ImGui::Button("Game Over")) {
            SceneManager::GetInstance()->ChangeScene("GAMEOVER");
        }
    }

    ImGui::End();

    // =====================================================
    // Edit Mode（フローティングボタン）
    // =====================================================
    ImGui::SetNextWindowPos(ImVec2(230, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(160, 60), ImGuiCond_Once);
    ImGui::Begin("Edit Mode", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    
    if (debugEditMode_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.2f, 0.0f, 1.0f));
        
        if (ImGui::Button("EDIT MODE ON", ImVec2(-1, 0))) {
            debugEditMode_ = false;
        }

        ImGui::PopStyleColor(3);
    } else {
        if (ImGui::Button("EDIT MODE OFF", ImVec2(-1, 0))) {
            debugEditMode_ = true;
        }
    }

    ImGui::End();

    // =====================================================
    // Camera Control（常時表示・画面上部中央）
    // =====================================================
    ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 130), ImGuiCond_Once);
    ImGui::Begin("Camera Control");

    Vector3 camPos = camera_->GetTranslate();
    Vector3 camRot = camera_->GetRotate();

    if (ImGui::DragFloat3("Pos", &camPos.x, 0.1f)) {
        camera_->SetTranslate(camPos);
    }

    if (ImGui::DragFloat3("Rot", &camRot.x, 0.01f)) {
        camera_->SetRotate(camRot);
    }

    ImGui::Spacing();
    
    if (ImGui::Button("Center")) {
        camera_->SetTranslate({ 0.0f, 0.0f, 0.0f });
        camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Save##cam")) {
        SaveCameraParams();
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Load##cam")) {
        LoadCameraParams();
    }

    ImGui::End();
#endif
}

// =====================================================
// デバッグパラメータ JSON 保存 / 読み込み
// =====================================================

static float ReadJsonFloat(const std::string& src, const std::string& key, float def)
{
    std::string needle = "\"" + key + "\": ";
    auto pos = src.find(needle);
    
    if (pos == std::string::npos) {
        return def;
    }

    pos += needle.size();
    
    try {
        return std::stof(src.substr(pos));
    } catch (...) {
        return def;
    }
}

static int ReadJsonInt(const std::string& src, const std::string& key, int def)
{
    std::string needle = "\"" + key + "\": ";
    auto pos = src.find(needle);
    if (pos == std::string::npos) {
        return def;
    }

    pos += needle.size();
    
    try {
        return std::stoi(src.substr(pos));
    } catch (...) {
        return def;
    }
}

static std::string ReadJsonString(const std::string& src, const std::string& key, const std::string& def)
{
    std::string needle = "\"" + key + "\":";
    auto pos = src.find(needle);
    
    if (pos == std::string::npos) {
        return def;
    }

    pos += needle.size();
    
    while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) {
        pos++;
    }

    if (pos >= src.size() || src[pos] != '"') {
        return def;
    }

    pos++;
    auto end = src.find('"', pos);
    
    if (end == std::string::npos) {
        return def;
    }

    return src.substr(pos, end - pos);
}

// ---- カメラ ----
void GamePlayScene::SaveCameraParams()
{
    std::ofstream f("Resources/debug_camera.json");
    
    if (!f) {
        return;
    }

    Vector3 cp = camera_->GetTranslate();
    Vector3 cr = camera_->GetRotate();
    f << "{\n";
    f << "  \"camera_pos_x\": " << cp.x << ",\n";
    f << "  \"camera_pos_y\": " << cp.y << ",\n";
    f << "  \"camera_pos_z\": " << cp.z << ",\n";
    f << "  \"camera_rot_x\": " << cr.x << ",\n";
    f << "  \"camera_rot_y\": " << cr.y << ",\n";
    f << "  \"camera_rot_z\": " << cr.z << "\n";
    f << "}\n";
}

void GamePlayScene::LoadCameraParams()
{
    std::ifstream f("Resources/debug_camera.json");
    
    if (!f) {
        return;
    }

    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Vector3 cp = {
        ReadJsonFloat(src, "camera_pos_x", camera_->GetTranslate().x),
        ReadJsonFloat(src, "camera_pos_y", camera_->GetTranslate().y),
        ReadJsonFloat(src, "camera_pos_z", camera_->GetTranslate().z)
    };
    Vector3 cr = {
        ReadJsonFloat(src, "camera_rot_x", camera_->GetRotate().x),
        ReadJsonFloat(src, "camera_rot_y", camera_->GetRotate().y),
        ReadJsonFloat(src, "camera_rot_z", camera_->GetRotate().z)
    };
    camera_->SetTranslate(cp);
    camera_->SetRotate(cr);
}

// ---- モデルパス ----
void GamePlayScene::SaveModelPaths()
{
}

void GamePlayScene::LoadModelPaths()
{
}

// ---- UI レイアウト ----
void GamePlayScene::SaveUILayout()
{
    std::ofstream f("Resources/debug_ui.json");
    
    if (!f) {
        return;
    }

    f << "{\n";
    f << "  \"count\": " << uiElements_.size();
    
    for (int i = 0; i < (int)uiElements_.size(); i++) {
        const UIEntry& e = uiElements_[i];
        Sprite* sp = e.sprite.get();
        Vector2 pos = sp->GetPosition();
        Vector2 sz = sp->GetSize();
        Vector4 col = sp->GetColor();
        float rot = sp->GetRotation();
        f << ",\n";
        f << "  \"ui_" << i << "_name\":   \"" << e.name << "\",\n";
        f << "  \"ui_" << i << "_tex\":    \"" << e.texPath << "\",\n";
        f << "  \"ui_" << i << "_pos_x\":  " << pos.x << ",\n";
        f << "  \"ui_" << i << "_pos_y\":  " << pos.y << ",\n";
        f << "  \"ui_" << i << "_sz_x\":   " << sz.x << ",\n";
        f << "  \"ui_" << i << "_sz_y\":   " << sz.y << ",\n";
        f << "  \"ui_" << i << "_rot\":    " << rot << ",\n";
        f << "  \"ui_" << i << "_col_r\":  " << col.x << ",\n";
        f << "  \"ui_" << i << "_col_g\":  " << col.y << ",\n";
        f << "  \"ui_" << i << "_col_b\":  " << col.z << ",\n";
        f << "  \"ui_" << i << "_col_a\":  " << col.w;
    }

    f << "\n}\n";
}

void GamePlayScene::LoadUILayout()
{
    std::ifstream f("Resources/debug_ui.json");
    
    if (!f) {
        return;
    }

    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    int count = ReadJsonInt(src, "count", 0);
    uiElements_.clear();
    editorSelectedType_ = SelectedType::None;
    editorSelectedIndex_ = -1;

    for (int i = 0; i < count; i++) {
        auto mk = [&](const char* field) { return "ui_" + std::to_string(i) + field; };

        UIEntry entry;
        entry.name = ReadJsonString(src, mk("_name"), "UI Element");
        entry.texPath = ReadJsonString(src, mk("_tex"), "");
        entry.sprite = std::make_unique<Sprite>();
        entry.sprite->Initialize(spriteCommon_.get(), entry.texPath);
        entry.sprite->SetPosition({ ReadJsonFloat(src, mk("_pos_x"), 640.0f),
            ReadJsonFloat(src, mk("_pos_y"), 360.0f) });
        entry.sprite->SetSize({ ReadJsonFloat(src, mk("_sz_x"), 100.0f),
            ReadJsonFloat(src, mk("_sz_y"), 100.0f) });
        entry.sprite->SetRotation(ReadJsonFloat(src, mk("_rot"), 0.0f));
        entry.sprite->SetColor({ ReadJsonFloat(src, mk("_col_r"), 1.0f),
            ReadJsonFloat(src, mk("_col_g"), 1.0f),
            ReadJsonFloat(src, mk("_col_b"), 1.0f),
            ReadJsonFloat(src, mk("_col_a"), 1.0f) });
        uiElements_.push_back(std::move(entry));
    }
}

// =====================================================
// 描画
// =====================================================

void GamePlayScene::DrawShadowPass()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    shadowManager_->BeginShadowPass(commandList);
    modelCommon_->BeginShadowPass();

    shadowManager_->EndShadowPass(commandList);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = dxCommon_->GetCurrentBackBufferHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    D3D12_VIEWPORT vp = { 0, 0, 1280.0f, 720.0f, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, 1280, 720 };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);
}

void GamePlayScene::Draw()
{
    DrawShadowPass();

    // 3Dオブジェクト
    modelCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList());
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), SrvManager::GetInstance());

    // 天球（最初に描画して他のオブジェクトの背景とする）
    skydome_->Draw();

    for (auto& obj : gameObjects_) {
        obj->Draw();
    }

    // スキンメッシュ描画（PSO 切り替え後にライト・シャドウを再バインド）
    skinCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList());
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), SrvManager::GetInstance());
    human_->Draw();

    // スケルトンデバッグ描画（3Dワールド空間）
#ifdef USE_IMGUI
    if (showSkeletonDebug_ && human_) {
        human_->DebugDraw();
    }
#endif

    // PSO を通常に戻す
    modelCommon_->CommonDrawSettings();
    objectCommon_->SetDefaultLight(dxCommon_->GetCommandList());
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), SrvManager::GetInstance());

    ParticleManager::GetInstance()->Draw(camera_.get());
    ring_->Draw();
    cylinder_->Draw();

    // 2D UI（ImGuiで追加したスプライト要素）
    spriteCommon_->CommonDrawSettings();
    shadowManager_->SetShadowMap(dxCommon_->GetCommandList(), SrvManager::GetInstance());

    for (auto& e : uiElements_) {
        e.sprite->Update();
        e.sprite->Draw();
    }
}

// いつか使うかも
#if 0
	if(false){
		// --- 表示するスプライトと基準サイズの選択 ---
		Sprite* currentFlavor = nullptr;
		Sprite* currentGuide = nullptr;
		Vector2 flavorSize = {500.0f, 200.0f}; // デフォルト値
		Vector2 guideSize = {600.0f, 200.0f};  // デフォルト値
		// --- ステップごとの画像・サイズ切り替えロジック ---

		switch(tutorialStep_){
		case TutorialStep::Move:
			currentFlavor = moveUI_.get();
			currentGuide = moveGuide_.get();
			flavorSize = {500.0f, 200.0f};
			guideSize = {600.0f, 200.0f};
			break;

		case TutorialStep::Shoot:
			currentFlavor = shootUI_.get();
			currentGuide = shootGuide_.get();
			flavorSize = {828.0f, 252.0f};
			guideSize = {703.0f, 114.0f};
			break;

		case TutorialStep::ScrollStart:
			currentFlavor = scrollStartUI_.get();
			flavorSize = {855.0f, 124.0f};
			currentGuide = nullptr; // ガイドなし
			break;

		case TutorialStep::BattleTrain:
			currentFlavor = battleTrainUI_.get();
			currentGuide = battleTrainGuide_.get(); // ★追加
			flavorSize = {822.0f, 123.0f};
			guideSize = {590.0f, 109.0f};
			break;

		case TutorialStep::SwipeTrain:
			currentFlavor = swipeTrainUI_.get();
			currentGuide = swipeTrainGuide_.get(); // ★追加
			flavorSize = {771.0f, 122.0f};
			guideSize = {459.0f, 153.0f};
			break;

		case TutorialStep::SpecialMove:
			currentFlavor = specialMoveUI_.get();
			currentGuide = specialMoveGuide_.get(); // ★追加
			flavorSize = {714.0f, 221.0f};
			guideSize = {619.0f, 101.0f};
			break;

		case TutorialStep::Epilogue:
			currentFlavor = epilogueUI_.get();
			flavorSize = {717.0f, 213.0f};
			currentGuide = nullptr; // ガイドなし
			break;
		}

		// --- 選択されたスプライトがある場合のみ描画処理を実行 ---
		if(currentFlavor){
			// --- アニメーション用パラメータ ---
			Vector2 pos = {450.0f, 100.0f}; // 中央上部
			Vector2 size = flavorSize;
			Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};

			// --- 1枚目（Flavor: セリフ枠）のアニメーション ---
			if(displayTimer_ <= 1.0f){
				color.w = 0.0f; // 待機
			} else if(displayTimer_ <= 1.5f){
				// 1. 出現 (1.0s ～ 1.5s): 画面中央へズームイン
				float t = (displayTimer_ - 1.0f) / 0.5f;
				pos = {450.0f, 150.0f - (50.0f * t)};
				size = {flavorSize.x * t, flavorSize.y * t}; // flavorSize基準でズーム
				color.w = t;
			} else if(displayTimer_ <= 3.0f){
				// 2. 中央静止 (1.5s ～ 3.0s)
				pos = {450.0f, 100.0f};
				size = flavorSize;
			} else if(displayTimer_ <= 3.5f){
				// 3. 右上へ移動 (3.0s ～ 3.5f)
				float t = (displayTimer_ - 3.0f) / 0.5f;
				pos.x = 450.0f + (1050.0f - 450.0f) * t;
				pos.y = 100.0f + (120.0f - 100.0f) * t;
				// 右上の定位置（250x80）へ縮小
				size.x = flavorSize.x + (250.0f - flavorSize.x) * t;
				size.y = flavorSize.y + (80.0f - flavorSize.y) * t;
			} else{
				// 4. 右上で待機 ＆ 消失
				pos = {1050.0f, 120.0f};
				size = {250.0f, 80.0f};

				if(isExiting_){
					// エラー回避：マクロ競合を避けるため、std::maxの代わりに三項演算子を使用
					float rawT = (0.5f - exitTimer_) / 0.5f;
					float t = (rawT > 0.0f)?rawT:0.0f; // std::max(0.0f, rawT) と同等

					color.w = t;
					pos.x += (1.0f - t) * 30.0f; // 右へスライドして消える
				}
			}

			currentFlavor->SetPosition(pos);
			currentFlavor->SetSize(size);
			currentFlavor->SetColor(color);
			currentFlavor->Update();
			currentFlavor->Draw();

			// --- 2枚目（Guide: 操作説明）のアニメーション ---
			if(displayTimer_ > 3.5f && currentGuide){
				Vector2 posG = {640.0f, 360.0f}; // 画面中央
				Vector2 sizeG = guideSize;
				Vector4 colorG = {1.0f, 1.0f, 1.0f, 1.0f};

				// 出現フェードイン (0.3s)
				// エラー回避：std::minの代わりに三項演算子を使用
				float rawFade = (displayTimer_ - 3.5f) / 0.3f;
				float fadeIn = (rawFade < 1.0f)?rawFade:1.0f; // std::min(1.0f, rawFade) と同等
				colorG.w = fadeIn;

				if(isExiting_){
					// 消失フェードアウト（1枚目と同期）
					float rawT = (0.5f - exitTimer_) / 0.5f;
					float t = (rawT > 0.0f)?rawT:0.0f;

					colorG.w = t;
					posG.y -= (1.0f - t) * 20.0f; // 上へ浮きながら消える
				}

				currentGuide->SetPosition(posG);
				currentGuide->SetSize(sizeG);
				currentGuide->SetColor(colorG);
				currentGuide->Update();
				currentGuide->Draw();
			}
		}

		// スキップUI（リターンキー）は常に表示
		skipUI_->Update();
		skipUI_->Draw();
	} else{
		// ゲーム本編のUI表示（既存の処理）
		timeDisplay_.Draw();
		for(auto& e : uiElements_){
			e.sprite->Update();
			e.sprite->Draw();
		}
	}

	fade_.Draw();
}

#endif // end of removed Draw code

// =====================================================
// 終了
// =====================================================

void GamePlayScene::Finalize()
{
    if (audio_) {
        audio_->StopBGM();
        audio_->StopAllSE();
    }
    ParticleManager::GetInstance()->ClearAllGroups();
}
