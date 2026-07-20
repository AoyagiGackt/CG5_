/**
 * @file Object3d.h
 * @brief 3D空間に配置される個々のオブジェクトを管理・描画するファイル
 */
#pragma once
#include "MakeAffine.h"
#include "GameObject.h"
#include "Model.h"
#include <string>
#include <wrl/client.h>

class ModelCommon;
class Camera;

/**
 * @brief 3D空間に配置されるオブジェクトを表すクラス
 * @note 1つの Model（形状データ）を複数の Object3d で共有し、
 * それぞれ異なる座標（Transform）やマテリアルを持たせて描画することができます。
 */
class Object3d : public GameObject {
public:
    /**
     * @brief 全てのObject3dで共通して使用するデフォルトカメラを設定する
     * @param camera 共通カメラのポインタ
     * @note 個別のカメラがセットされていないオブジェクトは、このカメラを使用して描画されます。
     */
    static void SetCommonCamera(Camera* camera);

    /**
     * @brief 全 Object3d で共通して使うライト空間行列を設定する（毎フレーム GamePlayScene から呼ぶ）
     * @param lvp ライトのビュー×プロジェクション行列
     */
    static void SetLightViewProjection(const Matrix4x4& lvp);

    /**
     * @brief オブジェクトの初期化。GPUに送るための定数バッファ（WVPやマテリアル用）を生成する
     * @param modelCommon 共通描画設定のポインタ
     */
    void Initialize(ModelCommon* modelCommon);

    /**
     * @brief 毎フレームの更新処理。トランスフォームからワールド行列とWVP行列を計算する
     */
    void Update() override;

    /**
     * @brief オブジェクトを描画するコマンドを積む
     * @note 事前に ModelCommon::CommonDrawSettings() が呼ばれている必要あり
     */
    void Draw() override;

    /**
     * @brief シャドウパス用の描画（テクスチャなし・深度のみ）
     * @note 事前に ModelCommon::BeginShadowPass() が呼ばれている必要あり
     */
    void DrawShadow();


    Model* GetModel() const{ return model_; }

    /**
     * @brief このオブジェクトに描画させるモデルの実体をセットする
     * @param model ModelManager等から取得したモデルのポインタ
     */
    void SetModel(Model* model) { model_ = model; }

    /**
     * @brief ファイルパスを指定して、ModelManagerからモデルを検索してセットする
     * @param filePath セットしたいモデルのファイルパス
     */
    void SetModel(const std::string& filePath);

    /**
     * @brief このオブジェクト専用のカメラをセットする（共通カメラを上書きする）
     * @param camera 個別に使用したいカメラのポインタ
     */
    void SetCamera(Camera* camera) { camera_ = camera; }

    /**
     * @brief オブジェクトの位置（座標）を設定する
     * @param position 新しいX, Y, Z座標
     */
    void SetPosition(const Vector3& position) { transform_.translate = position; }

    /**
     * @brief アニメーション用のローカル行列を直接セットする
     * @note セットすると Update() 内の通常のアフィン行列計算より優先される
     */
    void SetLocalMatrix(const Matrix4x4& mat)
    {
        localMatrix_ = mat;
        useLocalMatrix_ = true;
    }

    void ClearLocalMatrix() { useLocalMatrix_ = false; }

    /**
     * @brief オブジェクトの回転角を設定する
     * @param rotation X, Y, Z軸の回転角度（ラジアン）
     */
    void SetRotation(const Vector3& rotation) { transform_.rotate = rotation; }

    /**
     * @brief オブジェクトのスケール（拡大縮小）を設定する
     * @param scale X, Y, Z軸のスケール倍率
     */
    void SetScale(const Vector3& scale) { transform_.scale = scale; }

    /**
     * @brief 現在のトランスフォーム（座標・回転・スケール）を取得する
     * @return const Transform& トランスフォーム構造体への参照
     */
    const Transform& GetTransform() const { return transform_; }

    /**
     * @brief トランスフォーム（座標・回転・スケール）への参照を取得する（書き込み用）
     * @return Transform& トランスフォームへの参照
     */
    Transform& GetTransform() { return transform_; }

    /**
     * @brief ライティングの有効/無効を設定する
     * @param enable 有効にする場合はtrue、無効にする場合はfalse
     */
    void SetEnableLighting(bool enable) {
        if (materialData_) {
            materialData_->enableLighting = enable ? 1 : 0;
        }
    }

    /**
     * @brief マテリアルの基本色（RGBA）を設定する
     * @param color テクスチャ色と乗算される色（白なら変化なし）
     */
    void SetColor(const Vector4& color) {
        if (materialData_) { materialData_->color = color; }
    }

    /**
     * @brief スペキュラ（鏡面反射）の色を設定する
     * @param color 反射色（白ならハイライトが白くなる）
     */
    void SetSpecularColor(const Vector3& color) {
        if (materialData_) { materialData_->specularColor = color; }
    }

    /**
     * @brief 光沢度（スペキュラの鋭さ）を設定する
     * @param shininess 大きいほどシャープな光沢（8〜256程度が目安）
     */
    void SetShininess(float shininess) {
        if (materialData_) { materialData_->shininess = shininess; }
    }

    void SetUseCubemap(bool enable) {
        if (materialData_) { materialData_->useCubemap = enable ? 1 : 0; }
    }

    void SetUseTexture(bool enable) {
        if (materialData_) { materialData_->useTexture = enable ? 1 : 0; }
    }

    void SetEnvMapIntensity(float intensity) {
        if (materialData_) { materialData_->envMapIntensity = intensity; }
    }

private:
    /**
     * @brief GPUに送るための座標変換行列データ
     */
    struct TransformationMatrix {
        Matrix4x4 WVP;                  ///< ワールド・ビュー・プロジェクション行列
        Matrix4x4 World;                ///< ワールド行列
        Matrix4x4 WorldInverseTranspose; ///< World の逆転置行列（非均一スケール対応法線変換用）
        Matrix4x4 LightVP;              ///< ライト空間のビュープロジェクション行列（シャドウ用）
    };

    /**
     * @brief GPUに送るためのマテリアル（質感）データ
     * @note HLSL の Object3dPS.hlsl 内 Material 構造体と完全一致させること
     */
    struct Material {
        Vector4    color;          ///< 基本色（RGBA）
        int        enableLighting; ///< ライティング有効フラグ（1:有効, 0:無効）
        int        shadingType;    ///< シェーディング種類（0:Lambert, 1:HalfLambert）
        int        useCubemap;     ///< キューブマップサンプリング（1:有効）
        int        useTexture;     ///< テクスチャ色使用フラグ（0:白=色なし, 1:テクスチャあり）
        Matrix4x4  uvTransform;    ///< UV変換行列
        Vector3    specularColor;  ///< スペキュラ反射色
        float      shininess;      ///< 光沢度（大きいほどシャープ）
        Vector3    cameraWorldPos;    ///< カメラのワールド座標（Update()で自動書き込み）
        float      envMapIntensity;  ///< 環境マップ反射強度（0=なし, 1=フル反射）
    };

    /** @brief 全オブジェクトで共通して使うカメラのポインタ */
    static Camera* commonCamera_;

    /** @brief 全オブジェクトで共通して使うライト空間行列 */
    static Matrix4x4 commonLightVP_;

    /** @brief 共通描画設定のポインタ */
    ModelCommon* modelCommon_ = nullptr;

    /** @brief このオブジェクトが描画するモデルデータのポインタ */
    Model* model_ = nullptr;

    /** @brief このオブジェクト専用のカメラ（nullptrの場合は共通カメラを使う） */
    Camera* camera_ = nullptr;

    /** @brief オブジェクトのトランスフォーム（初期値はスケール1、原点） */
    Transform transform_ = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    // --- GPUリソース関連 ---

    /** @brief 座標変換行列用のGPUリソース（定数バッファ） */
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;

    /** @brief CPU側で書き込むための座標変換行列データのポインタ（マップ済み） */
    TransformationMatrix* transformationMatrixData_ = nullptr;

    /** @brief マテリアル用のGPUリソース（定数バッファ） */
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

    /** @brief CPU側で書き込むためのマテリアルデータのポインタ（マップ済み） */
    Material* materialData_ = nullptr;

    /** @brief アニメーション用ローカル行列 */
    Matrix4x4 localMatrix_ = MakeIdentity4x4();
    bool useLocalMatrix_ = false;
};