#pragma once
#include "Animation.h"
#include "MakeAffine.h"
#include "ModelCommon.h"
#include "Skeleton.h"
#include "SkinCS.h"
#include "SkinnedModel.h"
#include "SkinCommon.h"
#include <string>
#include <wrl/client.h>

#ifdef USE_IMGUI
#include "SkeletonDebugRenderer.h"
#include <memory>
#endif

class Camera;
class Object3dCommon;
class ShadowManager;

// スキニング（ボーンアニメーション）付き 3D オブジェクト。
// SkinnedModel + Skeleton + Animation を組み合わせて毎フレーム描画する。
class SkinnedObject3d {
public:
    static void SetCommonCamera(Camera* camera);
    static void SetLightViewProjection(const Matrix4x4& lvp);
    static void SetCommonObjectCommon(Object3dCommon* objectCommon);
    static void SetCommonShadowManager(ShadowManager* shadowManager);
    static void SetCommonModelCommon(ModelCommon* modelCommon);

    void Initialize(SkinCommon* skinCommon);

    void SetModel(SkinnedModel* model);  // SkinCS の初期化も行う
    void SetAnimation(Animation anim)         { animation_ = std::move(anim); }
    void SetSkeleton(Skeleton skeleton)       { skeleton_  = std::move(skeleton); }
    void SetEnvCubemapFilePath(const std::string& path) { envCubemapFilePath_ = path; }

    void SetPosition(const Vector3& pos) { transform_.translate = pos; }
    void SetRotation(const Vector3& rot) { transform_.rotate    = rot; }
    void SetScale(const Vector3& scale)  { transform_.scale     = scale; }
    void SetAnimSpeed(float s)           { animSpeed_ = s; }

    Vector3 GetPosition()  const { return transform_.translate; }
    Vector3 GetRotation()  const { return transform_.rotate; }
    Vector3 GetScale()     const { return transform_.scale; }
    float   GetAnimSpeed() const { return animSpeed_; }

    void Update();
    void Draw();
    void DebugDraw();

private:
    static const int kMaxJoints = 128;

    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 LightVP;
    };

    struct Material {
        Vector4   color;
        int       enableLighting;
        int       shadingType;
        int       useCubemap;
        int       useTexture;
        Matrix4x4 uvTransform;
        Vector3   specularColor;
        float     shininess;
        Vector3   cameraWorldPos;
        float     envMapIntensity;
    };

    static Camera*         commonCamera_;
    static Matrix4x4       commonLightVP_;
    static Object3dCommon* commonObjectCommon_;
    static ShadowManager*  commonShadowManager_;
    static ModelCommon*    commonModelCommon_;

    SkinCommon*   skinCommon_ = nullptr;
    SkinnedModel* model_      = nullptr;

    SkinCS skinCS_;
    bool   skinCSReady_ = false;

    void InitializeSkinCS();

    std::string envCubemapFilePath_;

    Skeleton  skeleton_;
    Animation animation_;
    float     animTime_  = 0.0f;
    float     animSpeed_ = 1.0f;

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Matrix4x4 worldMatrix_ = MakeIdentity4x4();

#ifdef USE_IMGUI
    std::unique_ptr<SkeletonDebugRenderer> skeletonDebugRenderer_;
#endif

    Microsoft::WRL::ComPtr<ID3D12Resource> transformCB_;
    TransformationMatrix* transformData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_;
    Material* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> paletteCB_;
    Matrix4x4* paletteData_ = nullptr;
};
