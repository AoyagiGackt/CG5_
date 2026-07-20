/**
 * @file MaterialManager.h
 * @brief 3Dモデルに適用するマテリアル（質感や色）のデータを管理するファイル
 */
#pragma once
#include "MakeAffine.h"
#include <vector>

/**
 * @brief 定数バッファとしてGPUに送るマテリアルのデータを保持する構造体
 */
struct Material {
    
    /** @brief マテリアルの基本色（RGBA） */
    Vector4 color;

    /** @brief ライティング計算を有効にするかどうかのフラグ（1: 有効, 0: 無効） */
    int enableLighting;

    /** * @brief HLSLの定数バッファにおける16バイトアライメント規則に合わせるためのパディング
     * @note int型(4バイト)のあとにMatrix4x4(64バイト)が続くため、float3つ分(12バイト)を空ける
     */
    float padding[3];

    /** @brief テクスチャのUV座標をスクロール・スケールさせるための変換行列 */
    Matrix4x4 uvTransform;
};

/**
 * @brief 複数のマテリアルを保持し、現在使用するマテリアルを切り替えるシングルトンクラス
 */
class MaterialManager {
public:

    /**
     * @brief コンストラクタ。内部でデフォルトのマテリアル群を初期化する
     */
    MaterialManager();

    /**
     * @brief 現在選択されているマテリアルのデータ（参照）を取得する
     * @return Material& 現在のマテリアルデータへの参照
     */
    Material& GetCurrentMaterial();

    /**
     * @brief 現在描画に使用するマテリアルのインデックスを設定する
     * @param index 設定するマテリアルのインデックス番号
     */
    void SetCurrentMaterialIndex(size_t index);

    /**
     * @brief 現在選択されているマテリアルのインデックス番号を取得する
     * @return size_t 現在のインデックス番号
     */
    size_t GetCurrentMaterialIndex() const;

    /** @brief マネージャーが保持している全マテリアルのリスト */
    std::vector<Material> materials;

    /**
     * @brief MaterialManagerの唯一のインスタンスを取得する
     * @return MaterialManager* シングルトンインスタンスへのポインタ
     */
    static MaterialManager* GetInstance();

    /**
     * @brief マネージャーの終了処理を行う（必要に応じてリソースを解放する）
     */
    void Finalize();

private:
    
    /** @brief 現在選択されているマテリアルのインデックス */
    size_t currentMaterialIndex_;

    /**
     * @brief 初期化時にいくつかのデフォルトマテリアル（赤、緑、青など）を生成してリストに登録する
     */
    void InitMaterials();
};