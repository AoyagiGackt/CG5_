/**
 * @file Skydome.h
 * @brief 背景となる巨大な空（天球）を管理するファイル
 */
#pragma once
#include "Camera.h"
#include "Model.h"
#include "Object3d.h"
#include <memory>

class ModelCommon;

/**
 * @brief 背景の空を常にカメラと同じ位置に配置し、無限の広がりを表現するクラス
 */
class Skydome {
public:
    /**
     * @brief 天球オブジェクトの生成と設定
     * @param modelCommon 描画設定
     * @param model 天球用のモデルデータ
     */
    void Initialize(ModelCommon* modelCommon, Model* model);

    /**
     * @brief カメラの位置を取得し、天球をその場所に移動させる
     * @param camera 追従対象のカメラポインタ
     * @param timeRatio ゲーム内時刻の進行率（0.0=18:00 〜 1.0=翌6:00）。Y軸回転でテクスチャをスクロール
     */
    void Update(Camera* camera, float timeRatio = 0.0f);

    /**
     * @brief 天球を描画
     */
    void Draw();

    /**
     * @brief 天球全体に色味を乗算する（デフォルトは白 = 無変化）
     * @param color RGBA。夕焼けや夜空の演出などに使用
     */
    void SetSkyColor(const Vector4& color);

    /**
     * @brief 時刻ベースのY回転に加算するオフセット（ラジアン）を設定する
     */
    void SetRotationOffsetY(float offsetY) { rotationOffsetY_ = offsetY; }

private:
    std::unique_ptr<Object3d> object_;
    float rotationOffsetY_ = 0.0f;
};