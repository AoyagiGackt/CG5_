#include "ImguiControl.h"
#include "ImGuiManager.h"
#include "LightingMode.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "LightManager.h"
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

    ImGui::End();

#endif
}