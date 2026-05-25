#include "ImguiControl.h"
#include "ImGuiManager.h"
#include "LightingMode.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "LightManager.h"
#include "GrayscaleEffect.h"
#include "ImageFilter.h"
#include "VignetteEffect.h"
#include "HsvFilter.h"
#include "TextureManager.h"

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
        const char* lightItems[] = { "なし", "Lambert", "Half Lambert", "Lambert+Phong", "HalfLambert+Phong" };
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
            const char* modeItems[] = { "Box", "Linear (Gaussian)", "Prewitt エッジ", "深度アウトライン", "ラジアルブラー", "ディゾルブ", "GPU ノイズ" };
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

        ImGui::Separator();

        // ----- HSV フィルター -----
        auto* hsv = HsvFilter::GetInstance();

        bool hsvEnabled = hsv->IsEnabled();
        if (ImGui::Checkbox("HSV フィルター", &hsvEnabled)) {
            hsv->SetEnabled(hsvEnabled);
        }

        if (hsvEnabled) {
            float hueShift = hsv->GetHueShift();
            if (ImGui::SliderFloat("色相シフト", &hueShift, -180.0f, 180.0f, "%.1f°")) {
                hsv->SetHueShift(hueShift);
            }
            if (ImGui::Button("色相リセット")) {
                hsv->SetHueShift(0.0f);
            }

            float sat = hsv->GetSaturation();
            if (ImGui::SliderFloat("彩度", &sat, 0.0f, 2.0f, "%.2f")) {
                hsv->SetSaturation(sat);
            }

            float val = hsv->GetValue();
            if (ImGui::SliderFloat("明度", &val, 0.0f, 2.0f, "%.2f")) {
                hsv->SetValue(val);
            }

            if (ImGui::Button("全パラメーターリセット")) {
                hsv->SetHueShift(0.0f);
                hsv->SetSaturation(1.0f);
                hsv->SetValue(1.0f);
            }

            ImGui::TextDisabled("※ イメージフィルター・グレースケールと同時使用不可");
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

    // ----- ディゾルブ設定ウィンドウ -----
    {
        auto* imgFilter  = ImageFilter::GetInstance();
        bool  isDissolve = (imgFilter->GetMode() == ImageFilter::Mode::Dissolve);

        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
        ImGui::Begin("ディゾルブ設定");

        if (!imgFilter->IsEnabled() || !isDissolve) {
            ImGui::TextDisabled("イメージフィルターで\n「ディゾルブ」を選択してください");
        } else {
            ImGui::TextDisabled("ノイズマスクで画面を溶かします");
            ImGui::Separator();

            // マスク選択
            const char* maskItems[] = { "noise0.png", "noise1.png" };
            int maskIdx = imgFilter->GetDissolveMaskIndex();
            if (ImGui::Combo("マスク", &maskIdx, maskItems, IM_ARRAYSIZE(maskItems))) {
                imgFilter->SetDissolveMaskIndex(maskIdx);
            }

            ImGui::Separator();

            float threshold = imgFilter->GetDissolveThreshold();
            if (ImGui::SliderFloat("進行度", &threshold, 0.0f, 1.0f, "%.3f")) {
                imgFilter->SetDissolveThreshold(threshold);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0=なし / 1=完全消滅");
            }

            float edgeWidth = imgFilter->GetDissolveEdgeWidth();
            if (ImGui::SliderFloat("エッジ幅", &edgeWidth, 0.0f, 0.3f, "%.3f")) {
                imgFilter->SetDissolveEdgeWidth(edgeWidth);
            }

            float edgeCol[4];
            imgFilter->GetDissolveEdgeColor(edgeCol);
            if (ImGui::ColorEdit4("エッジ色", edgeCol)) {
                imgFilter->SetDissolveEdgeColor(edgeCol[0], edgeCol[1], edgeCol[2], edgeCol[3]);
            }
        }

        ImGui::End();
    }

    // ----- GPU ノイズ設定ウィンドウ -----
    {
        auto* imgFilter = ImageFilter::GetInstance();
        bool  isNoise   = (imgFilter->GetMode() == ImageFilter::Mode::NoiseGen);

        ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Once);
        ImGui::Begin("GPU ノイズ設定");

        if (!imgFilter->IsEnabled() || !isNoise) {
            ImGui::TextDisabled("イメージフィルターで\n「GPU ノイズ」を選択してください");
        } else {
            // アニメーション
            ImGui::TextDisabled("--- アニメーション ---");
            bool anim = imgFilter->GetNoiseAnimate();
            if (ImGui::Checkbox("自動更新（毎フレーム seed を変化）", &anim)) {
                imgFilter->SetNoiseAnimate(anim);
            }
            if (anim) {
                float speed = imgFilter->GetNoiseSpeed();
                if (ImGui::SliderFloat("速度", &speed, 0.0005f, 0.05f, "%.4f")) {
                    imgFilter->SetNoiseSpeed(speed);
                }
            }
            if (ImGui::Button("リセット")) {
                imgFilter->ResetNoiseTime();
            }

            ImGui::Separator();

            // 透過合成
            float opacity = imgFilter->GetNoiseOpacity();
            if (ImGui::SliderFloat("不透明度", &opacity, 0.0f, 1.0f, "%.2f")) {
                imgFilter->SetNoiseOpacity(opacity);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0=シーンのみ / 1=ノイズのみ\n中間値でシーンが透けて見えます");
            }

            ImGui::Separator();

            // スケール
            ImGui::TextDisabled("--- スケール ---");
            float sx = imgFilter->GetNoiseScaleX();
            float sy = imgFilter->GetNoiseScaleY();
            if (ImGui::DragFloat("スケール X", &sx, 0.05f, 0.1f, 50.0f, "%.2f")) {
                imgFilter->SetNoiseScale(sx, sy);
            }
            if (ImGui::DragFloat("スケール Y", &sy, 0.05f, 0.1f, 50.0f, "%.2f")) {
                imgFilter->SetNoiseScale(sx, sy);
            }
            if (ImGui::Button("均一にする")) {
                imgFilter->SetNoiseScale(sx, sx);
            }

            ImGui::Separator();

            // シード（アニメーション OFF 時に手動調整）
            float seed = imgFilter->GetNoiseSeed();
            if (ImGui::DragFloat("シード（手動）", &seed, 0.01f, -100.0f, 100.0f, "%.3f")) {
                imgFilter->SetNoiseSeed(seed);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("アニメーション OFF 時にパターンを手動で変えられます");
            }

            ImGui::Separator();

            // fBm パラメータ
            ImGui::TextDisabled("--- fBm（フラクタル重ね合わせ）---");
            int octaves = imgFilter->GetNoiseOctaves();
            if (ImGui::SliderInt("オクターブ数", &octaves, 1, 8)) {
                imgFilter->SetNoiseOctaves(octaves);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("重ね合わせ回数。多いほど細かいディテールが加わる。");
            }

            float persistence = imgFilter->GetNoisePersistence();
            if (ImGui::SliderFloat("パーシステンス", &persistence, 0.1f, 1.0f, "%.2f")) {
                imgFilter->SetNoisePersistence(persistence);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("オクターブごとの振幅減衰率。\n低いほど細かいディテールが弱くなる。");
            }

            float lacunarity = imgFilter->GetNoiseLacunarity();
            if (ImGui::SliderFloat("ラクナリティ", &lacunarity, 1.0f, 4.0f, "%.2f")) {
                imgFilter->SetNoiseLacunarity(lacunarity);
            }
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("オクターブごとの周波数倍率。\n大きいほど細部が急激に細かくなる。");
            }

            ImGui::Separator();

            // カラーモード
            const char* colorItems[] = { "グレースケール", "カラー" };
            int colorMode = imgFilter->GetNoiseColorMode();
            if (ImGui::Combo("カラーモード", &colorMode, colorItems, IM_ARRAYSIZE(colorItems))) {
                imgFilter->SetNoiseColorMode(colorMode);
            }
        }

        ImGui::End();
    }

#endif
}