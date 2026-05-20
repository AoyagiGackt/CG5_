#include "ImguiControl.h"
#include "ImGuiManager.h"
#include "LightingMode.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "LightManager.h"
#include "GrayscaleEffect.h"

void ShowControls()
{
#ifdef USE_IMGUI

    ImGui::Begin("Controls");

    if (ImGui::CollapsingHeader("Mesh Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* meshItems[] = { "Sphere", "Cube", "Plane" };
        int currentMesh = (int)MeshManager::GetInstance()->GetCurrentMeshType();
        
        if (ImGui::Combo("Mesh Type", &currentMesh, meshItems, IM_ARRAYSIZE(meshItems))) {
            MeshManager::GetInstance()->SetCurrentMeshType((MeshType)currentMesh);
        }

        // 各メッシュのトランスフォーム制御
        for (int i = 0; i < MeshType_Count; ++i) {
            ImGui::PushID(i);
            
            if (ImGui::TreeNode(meshItems[i])) {
                ImGui::DragFloat3("Scale", &MeshManager::GetInstance()->meshes[i].transform.scale.x, 0.01f);
                ImGui::DragFloat3("Rotation", &MeshManager::GetInstance()->meshes[i].transform.rotate.x, 0.01f);
                ImGui::DragFloat3("Translate", &MeshManager::GetInstance()->meshes[i].transform.translate.x, 0.01f);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    // マテリアル切り替え
    if (ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* matItems[] = { "Red", "Green", "Blue", "White" };
        int currentMat = (int)MaterialManager::GetInstance()->GetCurrentMaterialIndex();
        
        if (ImGui::Combo("Material Color", &currentMat, matItems, IM_ARRAYSIZE(matItems))) {
            MaterialManager::GetInstance()->SetCurrentMaterialIndex(currentMat);
        }
    }

    // ライティング切り替え
    if (ImGui::CollapsingHeader("Lighting Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* lightItems[] = { "None", "Lambert", "Half Lambert" };

        // マネージャーから現在の値を取得
        int currentMode = LightManager::GetInstance()->GetLightingMode();

        // ImGuiで値が変更されたら、マネージャーにセットし直す
        if (ImGui::Combo("Lighting Mode", &currentMode, lightItems, IM_ARRAYSIZE(lightItems))) {
            LightManager::GetInstance()->SetLightingMode(currentMode);
        }
    }

    // グレースケール ポストエフェクト
    if (ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* gs = GrayscaleEffect::GetInstance();

        bool enabled = gs->IsEnabled();
        if (ImGui::Checkbox("Grayscale", &enabled)) {
            gs->SetEnabled(enabled);
        }

        if (enabled) {
            float amount = gs->GetAmount();
            if (ImGui::SliderFloat("Amount", &amount, 0.0f, 1.0f)) {
                gs->SetAmount(amount);
            }
        }
    }

    ImGui::End();

#endif
}