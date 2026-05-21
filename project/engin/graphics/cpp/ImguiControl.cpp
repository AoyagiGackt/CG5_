#include "ImguiControl.h"
#include "ImGuiManager.h"
#include "LightingMode.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "LightManager.h"
#include "GrayscaleEffect.h"
#include "ImageFilter.h"
#include "VignetteEffect.h"

void ShowControls()
{
#ifdef USE_IMGUI

    ImGui::Begin("コントロール");

    if (ImGui::CollapsingHeader("メッシュ設定", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* meshItems[] = { "球体", "立方体", "平面" };
        int currentMesh = (int)MeshManager::GetInstance()->GetCurrentMeshType();

        if (ImGui::Combo("メッシュ種類", &currentMesh, meshItems, IM_ARRAYSIZE(meshItems))) {
            MeshManager::GetInstance()->SetCurrentMeshType((MeshType)currentMesh);
        }

        // 各メッシュのトランスフォーム制御
        const char* meshNames[] = { "Sphere", "Cube", "Plane" };
        for (int i = 0; i < MeshType_Count; ++i) {
            ImGui::PushID(i);

            if (ImGui::TreeNode(meshNames[i])) {
                ImGui::DragFloat3("スケール",     &MeshManager::GetInstance()->meshes[i].transform.scale.x, 0.01f);
                ImGui::DragFloat3("回転",         &MeshManager::GetInstance()->meshes[i].transform.rotate.x, 0.01f);
                ImGui::DragFloat3("移動",         &MeshManager::GetInstance()->meshes[i].transform.translate.x, 0.01f);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    // マテリアル切り替え
    if (ImGui::CollapsingHeader("マテリアル設定", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* matItems[] = { "赤", "緑", "青", "白" };
        int currentMat = (int)MaterialManager::GetInstance()->GetCurrentMaterialIndex();

        if (ImGui::Combo("マテリアルカラー", &currentMat, matItems, IM_ARRAYSIZE(matItems))) {
            MaterialManager::GetInstance()->SetCurrentMaterialIndex(currentMat);
        }
    }

    // ライティング切り替え
    if (ImGui::CollapsingHeader("ライティング設定", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* lightItems[] = { "なし", "Lambert", "Half Lambert" };
        int currentMode = LightManager::GetInstance()->GetLightingMode();
        if (ImGui::Combo("ライティングモード", &currentMode, lightItems, IM_ARRAYSIZE(lightItems))) {
            LightManager::GetInstance()->SetLightingMode(currentMode);
        }
    }

    // ポストプロセス
    if (ImGui::CollapsingHeader("ポストプロセス", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* gs = GrayscaleEffect::GetInstance();

        bool enabled = gs->IsEnabled();
        if (ImGui::Checkbox("グレースケール", &enabled)) {
            gs->SetEnabled(enabled);
        }
        if (enabled) {
            float amount = gs->GetAmount();
            if (ImGui::SliderFloat("適用量", &amount, 0.0f, 1.0f)) {
                gs->SetAmount(amount);
            }
        }

        ImGui::Separator();

        // ----- イメージフィルター -----
        auto* imgFilter = ImageFilter::GetInstance();

        bool filterEnabled = imgFilter->IsEnabled();
        if (ImGui::Checkbox("イメージフィルター", &filterEnabled)) {
            imgFilter->SetEnabled(filterEnabled);
        }

        if (filterEnabled) {
            const char* modeItems[] = { "Box", "Linear (Gaussian)", "Prewitt エッジ", "深度アウトライン", "ラジアルブラー" };
            int currentMode = (int)imgFilter->GetMode();
            if (ImGui::Combo("フィルターモード", &currentMode, modeItems, IM_ARRAYSIZE(modeItems))) {
                imgFilter->SetMode((ImageFilter::Mode)currentMode);
            }

            auto mode = imgFilter->GetMode();
            if (mode == ImageFilter::Mode::Box) {
                int r = imgFilter->GetRadius();
                if (ImGui::SliderInt("半径", &r, 0, 8)) {
                    imgFilter->SetRadius(r);
                }
                ImGui::TextDisabled("タップ数: %d x %d", 2*r+1, 2*r+1);
            } else if (mode == ImageFilter::Mode::Gaussian) {
                float sigma = imgFilter->GetSigma();
                if (ImGui::SliderFloat("シグマ", &sigma, 0.5f, 8.0f, "%.2f")) {
                    imgFilter->SetSigma(sigma);
                }
                int r = (int)(sigma * 3.0f);
                if (r > 8) r = 8;
                ImGui::TextDisabled("半径: %d, タップ数: %d x %d", r, 2*r+1, 2*r+1);
            }
        }

        ImGui::Separator();

        auto* vg = VignetteEffect::GetInstance();

        bool vigEnabled = vg->IsEnabled();
        if (ImGui::Checkbox("ビネット", &vigEnabled)) {
            vg->SetEnabled(vigEnabled);
        }

        if (vigEnabled) {
            float intensity = vg->GetIntensity();
            if (ImGui::SliderFloat("強度", &intensity, 0.0f, 2.0f)) {
                vg->SetIntensity(intensity);
            }

            float radius = vg->GetRadius();
            if (ImGui::SliderFloat("半径", &radius, 0.0f, 1.0f)) {
                vg->SetRadius(radius);
            }

            float softness = vg->GetSoftness();
            if (ImGui::SliderFloat("ソフトネス", &softness, 0.0f, 1.0f)) {
                vg->SetSoftness(softness);
            }
        }
    }

    ImGui::End();

    // ----- アウトライン設定ウィンドウ -----
    {
        auto* imgFilter = ImageFilter::GetInstance();
        auto  mode      = imgFilter->GetMode();
        bool  isOutline = (mode == ImageFilter::Mode::PrewittEdge ||
                           mode == ImageFilter::Mode::DepthOutline);

        ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Once);
        ImGui::Begin("アウトライン設定");

        if (!imgFilter->IsEnabled() || !isOutline) {
            ImGui::TextDisabled("イメージフィルターで\n「Prewitt エッジ」か\n「深度アウトライン」を選択してください");
        } else {
            ImGui::Text("モード: %s",
                mode == ImageFilter::Mode::PrewittEdge ? "Prewitt エッジ（輝度ベース）" : "深度アウトライン");

            ImGui::Separator();

            float threshold = imgFilter->GetOutlineThreshold();
            if (ImGui::SliderFloat("検出閾値", &threshold, 0.0f, 0.5f, "%.3f")) {
                imgFilter->SetOutlineThreshold(threshold);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("エッジとみなす勾配の最小値。\n小さいほど細かいエッジも検出される。");
            }

            float strength = imgFilter->GetOutlineStrength();
            if (ImGui::SliderFloat("エッジ強度", &strength, 0.1f, 30.0f, "%.1f")) {
                imgFilter->SetOutlineStrength(strength);
            }

            float color[4];
            imgFilter->GetOutlineColor(color);
            if (ImGui::ColorEdit4("アウトライン色", color)) {
                imgFilter->SetOutlineColor(color[0], color[1], color[2], color[3]);
            }

            if (mode == ImageFilter::Mode::DepthOutline) {
                ImGui::Separator();
                ImGui::TextDisabled("--- 深度アウトライン専用 ---");
                float ds = imgFilter->GetDepthScale();
                if (ImGui::SliderFloat("深度スケール", &ds, 1.0f, 500.0f, "%.1f")) {
                    imgFilter->SetDepthScale(ds);
                }
                ImGui::SameLine(); ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("深度差を増幅する倍率。\n大きいほど奥行き差のあるエッジが強調される。");
                }
            }
        }

        ImGui::End();
    }

    // ----- ラジアルブラー設定ウィンドウ -----
    {
        auto* imgFilter  = ImageFilter::GetInstance();
        bool  isRadial   = (imgFilter->GetMode() == ImageFilter::Mode::RadialBlur);

        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
        ImGui::Begin("ラジアルブラー設定");

        if (!imgFilter->IsEnabled() || !isRadial) {
            ImGui::TextDisabled("イメージフィルターで\n「ラジアルブラー」を選択してください");
        } else {
            ImGui::TextDisabled("中心から放射状にブラーをかけます");
            ImGui::Separator();

            // 中心座標（UV空間）
            float cx = imgFilter->GetRadialCenterX();
            float cy = imgFilter->GetRadialCenterY();
            if (ImGui::SliderFloat("中心 X", &cx, 0.0f, 1.0f, "%.2f")) {
                imgFilter->SetRadialCenter(cx, cy);
            }
            if (ImGui::SliderFloat("中心 Y", &cy, 0.0f, 1.0f, "%.2f")) {
                imgFilter->SetRadialCenter(cx, cy);
            }
            if (ImGui::Button("中心にリセット")) {
                imgFilter->SetRadialCenter(0.5f, 0.5f);
            }

            ImGui::Separator();

            float strength = imgFilter->GetRadialStrength();
            if (ImGui::SliderFloat("強度", &strength, 0.0f, 1.0f, "%.3f")) {
                imgFilter->SetRadialStrength(strength);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("ブラーの広がり具合。\n大きいほど中心から遠くまでサンプルが伸びる。");
            }

            int sampleCount = imgFilter->GetRadialSampleCount();
            if (ImGui::SliderInt("サンプル数", &sampleCount, 2, 32)) {
                imgFilter->SetRadialSampleCount(sampleCount);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("多いほど滑らかになるが処理負荷が上がる。");
            }
        }

        ImGui::End();
    }

#endif
}