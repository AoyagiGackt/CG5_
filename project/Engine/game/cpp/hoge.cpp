#include "hoge.h"

// コンストラクタ
Hoge::Hoge()
{
    // メンバ変数の初期化などが必要ならここに
}

// デストラクタ
Hoge::~Hoge()
{
    // 終了処理
    Finalize();
}

// 初期化処理
void Hoge::Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, Input* input, Audio* audio)
{
    dxCommon_ = dxCommon;
    input_    = input;
    audio_    = audio;

    model_ = std::make_unique<Model>();
    model_->Initialize(modelCommon,
        "Resources/AnimatedCube/AnimatedCube.gltf",
        "Resources/AnimatedCube/AnimatedCube_BaseColor.png");

    animation_ = LoadAnimationFile("Resources/AnimatedCube", "AnimatedCube.gltf");

    object_ = std::make_unique<Object3d>();
    object_->Initialize(modelCommon);
    object_->SetModel(model_.get());
}

// 更新処理
void Hoge::Update()
{
    if (!object_ || animation_.nodeAnimations.empty()) {
        return;
    }

    animTime_ = std::fmod(animTime_ + animSpeed_ / 60.0f, animation_.duration);

    auto& nodeAnim = animation_.nodeAnimations.begin()->second;
    Vector3    t = CalculateValue(nodeAnim.translate, animTime_);
    Quaternion r = CalculateValue(nodeAnim.rotate,    animTime_);
    Vector3    s = CalculateValue(nodeAnim.scale,     animTime_);

    t = t + worldOffset_;

    object_->SetLocalMatrix(MakeAffineMatrix(s, r, t));
    object_->Update();
}

// 描画処理
void Hoge::Draw()
{
    if (object_) {
        object_->Draw();
    }
}

// 終了処理
void Hoge::Finalize()
{
    
}