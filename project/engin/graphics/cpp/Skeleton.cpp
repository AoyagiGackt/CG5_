#include "Skeleton.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cassert>
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

// =================================================================
// NodeHierarchy 読み込み（assimp）
// =================================================================

static Node ConvertAiNode(const aiNode* ainode)
{
    Node node;
    node.name = ainode->mName.C_Str();

    // assimpの行列を分解（右手系）
    aiVector3D aiScale, aiTranslate;
    aiQuaternion aiRotate;
    ainode->mTransformation.Decompose(aiScale, aiRotate, aiTranslate);

    // 右手系→左手系変換：X軸を反転、クォータニオンはY・Zを反転
    node.transform.scale     = { aiScale.x,       aiScale.y,      aiScale.z };
    node.transform.rotate    = { aiRotate.x,  -aiRotate.y, -aiRotate.z, aiRotate.w };
    node.transform.translate = { -aiTranslate.x, aiTranslate.y, aiTranslate.z };
    node.localMatrix = MakeAffineMatrix(node.transform.scale, node.transform.rotate, node.transform.translate);

    for (uint32_t i = 0; i < ainode->mNumChildren; ++i) {
        node.children.push_back(ConvertAiNode(ainode->mChildren[i]));
    }
    return node;
}

Node LoadNodeHierarchyFromFile(const std::string& directoryPath, const std::string& filename)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile((directoryPath + "/" + filename).c_str(), 0);
    assert(scene && scene->mRootNode);
    return ConvertAiNode(scene->mRootNode);
}

// =================================================================
// Skeleton 生成
// =================================================================

int32_t Skeleton::CreateJoint(
    const Node& node,
    const std::optional<int32_t>& parent,
    std::vector<Joint>& joints,
    std::map<std::string, int32_t>& jointMap)
{
    Joint joint;
    joint.name              = node.name;
    joint.localMatrix       = node.localMatrix;
    joint.skeletonSpaceMatrix = MakeIdentity4x4();
    joint.transform         = node.transform;
    joint.parent            = parent;
    joint.index             = static_cast<int32_t>(joints.size());

    joints.push_back(joint);
    jointMap[joint.name] = joint.index;

    for (const Node& child : node.children) {
        CreateJoint(child, joint.index, joints, jointMap);
    }
    return joint.index;
}

Skeleton Skeleton::Create(const Node& rootNode)
{
    Skeleton skeleton;
    skeleton.root = CreateJoint(rootNode, std::nullopt, skeleton.joints, skeleton.jointMap);
    return skeleton;
}

// =================================================================
// 更新（DFS順なので前から順に計算するだけでよい）
// =================================================================

void Skeleton::Update()
{
    for (Joint& joint : joints) {
        joint.localMatrix = MakeAffineMatrix(
            joint.transform.scale,
            joint.transform.rotate,
            joint.transform.translate);

        if (joint.parent) {
            joint.skeletonSpaceMatrix = Multiply(joint.localMatrix, joints[*joint.parent].skeletonSpaceMatrix);
        } else {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}

// =================================================================
// デバッグ描画（ImGui）
// =================================================================

#ifdef USE_IMGUI

static void DrawJointTree(const std::vector<Joint>& joints, int32_t index)
{
    const Joint& joint = joints[index];
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    bool open = ImGui::TreeNodeEx(joint.name.c_str(), flags);
    if (open) {
        const auto& m = joint.skeletonSpaceMatrix;
        ImGui::Text("pos: (%.1f, %.1f, %.1f)", m.m[3][0], m.m[3][1], m.m[3][2]);
        for (const Joint& other : joints) {
            if (other.parent && *other.parent == index) {
                DrawJointTree(joints, other.index);
            }
        }
        ImGui::TreePop();
    }
}

#endif // USE_IMGUI

void Skeleton::DebugDraw()
{
#ifdef USE_IMGUI
    ImGui::SetNextWindowSize(ImVec2(440.0f, 600.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Skeleton Debug")) {
        ImGui::End();
        return;
    }

    // ---- キャンバスサイズと描画領域 ----
    const ImVec2 canvasSize = { 400.0f, 360.0f };
    const ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
    ImDrawList*  drawList   = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        canvasPos,
        { canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y },
        IM_COL32(20, 20, 30, 220));
    drawList->AddRect(
        canvasPos,
        { canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y },
        IM_COL32(80, 80, 100, 255));

    // ---- 全ジョイントのXY範囲を自動計算してスケールを決める ----
    float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    bool first = true;
    for (const Joint& joint : joints) {
        float x = joint.skeletonSpaceMatrix.m[3][0];
        float y = joint.skeletonSpaceMatrix.m[3][1];
        if (first) { minX = maxX = x; minY = maxY = y; first = false; }
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    }

    float rangeX = std::max(maxX - minX, 1.0f);
    float rangeY = std::max(maxY - minY, 1.0f);
    float margin = 0.85f;
    float scale  = std::min(canvasSize.x * margin / rangeX, canvasSize.y * margin / rangeY);

    // キャンバス中央にルートを合わせる原点
    float cx    = (minX + maxX) * 0.5f;
    ImVec2 origin = {
        canvasPos.x + canvasSize.x * 0.5f - cx * scale,
        canvasPos.y + canvasSize.y * 0.92f - minY * scale   // 下揃え（Y=上）
    };

    // ガイドライン
    drawList->AddLine(
        { canvasPos.x + 10.0f, origin.y },
        { canvasPos.x + canvasSize.x - 10.0f, origin.y },
        IM_COL32(55, 55, 55, 180), 1.0f);

    // 骨の線（親→子）
    for (const Joint& joint : joints) {
        if (!joint.parent) { continue; }
        const Joint& p = joints[*joint.parent];
        ImVec2 p0 = {
            origin.x + p.skeletonSpaceMatrix.m[3][0] * scale,
            origin.y - p.skeletonSpaceMatrix.m[3][1] * scale
        };
        ImVec2 p1 = {
            origin.x + joint.skeletonSpaceMatrix.m[3][0] * scale,
            origin.y - joint.skeletonSpaceMatrix.m[3][1] * scale
        };
        drawList->AddLine(p0, p1, IM_COL32(80, 200, 120, 220), 2.0f);
    }

    // ジョイントの点
    for (const Joint& joint : joints) {
        ImVec2 p = {
            origin.x + joint.skeletonSpaceMatrix.m[3][0] * scale,
            origin.y - joint.skeletonSpaceMatrix.m[3][1] * scale
        };
        ImU32 col = (joint.index == root)
            ? IM_COL32(255, 80, 80, 255)
            : IM_COL32(255, 200, 50, 255);
        drawList->AddCircleFilled(p, 3.5f, col);
    }

    // マウスホバーでジョイント名ツールチップ
    ImVec2 mousePos = ImGui::GetMousePos();
    for (const Joint& joint : joints) {
        ImVec2 p = {
            origin.x + joint.skeletonSpaceMatrix.m[3][0] * scale,
            origin.y - joint.skeletonSpaceMatrix.m[3][1] * scale
        };
        float dx = mousePos.x - p.x, dy = mousePos.y - p.y;
        if (dx * dx + dy * dy < 10.0f * 10.0f) {
            ImGui::SetNextWindowBgAlpha(0.8f);
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(joint.name.c_str());
            const auto& m = joint.skeletonSpaceMatrix;
            ImGui::Text("(%.1f, %.1f, %.1f)", m.m[3][0], m.m[3][1], m.m[3][2]);
            ImGui::EndTooltip();
        }
    }

    ImGui::Dummy(canvasSize);

    // ---- ジョイントツリービュー ----
    ImGui::Separator();
    ImGui::Text("Joints: %d", static_cast<int>(joints.size()));
    if (ImGui::BeginChild("##jointTree", ImVec2(0.0f, 0.0f), false)) {
        DrawJointTree(joints, root);
    }
    ImGui::EndChild();

    ImGui::End();
#endif // USE_IMGUI
}
