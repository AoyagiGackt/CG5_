/**
 * @file MapChipField.h
 * @brief CSVマップの読み込みとブロックの自動スクロールを管理するファイル
 */
#pragma once
#include "Object3d.h"
#include <memory>
#include <string>
#include <vector>
#include <random>

class ModelCommon;
class Model;

/**
 * @brief マップチップの種類を定義
 */
enum class MapChipType {
    None = 0, ///< 空白
    Block = 1, ///< 通常ブロック
    Obstacle1 = 2, ///< 障害物その1
    Obstacle2 = 3, ///< 障害物その2
};

/**
 * @brief フィールド全体の構築と無限ループ制御を行うクラス
 */
class MapChipField {
public:
    /**
     * @brief CSVファイルからマップデータを数値配列としてロード
     * @param filePath 読み込むCSVのパス
     */
    void LoadMapFromCSV(const std::string& filePath);

    /**
     * @brief 読み込んだデータに基づきObject3dを生成・配置
     * @param modelCommon 共通描画設定
     * @param models 使用するモデルのリスト
     */
    void Initialize(ModelCommon* modelCommon, const std::vector<Model*>& models);

    /**
     * @brief 全ブロックをスクロールさせ、画面外に出たら右端へループさせる
     * @param waveStrength ウェーブ強度（0.0=なし, 1.0=最大）
     */
    void Update(float speedMultiplier, float waveStrength = 0.0f);

    /** @brief 絶不調フラグをセット（障害物生成を止める） */
    void SetTerribleMode(bool terrible) { terribleMode_ = terrible; }

    /**
     * @brief 全ブロックを描画
     */
    void Draw();

    /**
     * @brief シャドウパス用描画（深度のみ）
     */
    void DrawShadow();

    /**
     * @brief 当たり判定用：配置されている全チップのリストを取得
     * @return Object3dのリスト
     */
    const std::vector<std::unique_ptr<Object3d>>& GetMapChips() const { return mapChips_; }

    size_t GetRow() const { return mapData_.size(); }
    
    size_t GetCol() const
    {
        if (mapData_.empty()) {
            return 0;
        }
        return mapData_[0].size();
    }

    /* @brief 指定したマスのチップの種類を取得
     * @param x マスのX座標（列番号）
     * @param y マスのY座標（行番号）
     * @return MapChipType チップの種類（None, Blockなど）
     */
    MapChipType GetMapType(uint32_t x, uint32_t y) const;

    /* @brief 指定したマスのワールド座標を取得
     * @param x マスのX座標（列番号）
     * @param y マスのY座標（行番号）
     * @return Vector3 ワールド空間での座標
     */
    Vector3 GetWorldPos(uint32_t x, uint32_t y) const;

private:
    /** @brief 1マスのサイズ（1.0f = 立方体1個分） */
    const float kTileSize = 1.0f;

    /** @brief 左方向へのスクロール速度 */
    float scrollSpeed_ = 0.1f;

    /** @brief CSVデータの横幅（ループ位置の計算に使用） */
    uint32_t mapWidth_ = 0;

    /** @brief CSVから読み取った生の数値データ */
    std::vector<std::vector<MapChipType>> mapData_;

    /** @brief 実際に描画される3Dオブジェクトのリスト */
    std::vector<std::unique_ptr<Object3d>> mapChips_;

    /** @brief 各チップの初期Y座標（ウェーブの基準） */
    std::vector<float> chipBaseY_;

    /** @brief 1列分の障害物プロフィール（地面からのオフセットと高さ） */
    struct ColumnProfile {
        int offsetY; ///< 地面(y=1)からの追加オフセット
        int height;  ///< 積み上げるブロック数
    };

    /** @brief ブロックの自動生成に関する制御変数 **/
    float nextGenerateX_ = 0.0f;
    /** @brief 連続生成を防ぐための空白期間 */
    int coolTime_ = 0;
    /** @brief 現在のスポーン確率（0〜99、調子で変動） */
    int spawnChance_ = 8;

    /** @brief 現在生成中の形状（列ごとのプロフィール列） */
    std::vector<ColumnProfile> currentShape_;
    /** @brief 現在の形状の何列目を生成中か */
    int shapeStep_ = 0;

    ModelCommon* modelCommon_ = nullptr;
    std::vector<Model*> models_;

    /** 乱数関連 **/
    std::mt19937 gen_;
    std::uniform_int_distribution<int> distChance_;
    std::uniform_int_distribution<int> distShape_;

    // 1列分のブロックを新しく生成する関数
    void GenerateColumn(float x);

    // ランダムな形状を返す
    std::vector<ColumnProfile> PickRandomShape();

    /** @brief 絶不調フラグ（trueのとき障害物を出さない） */
    bool terribleMode_ = false;

    /** @brief 生成済み列数カウンタ（最初はフラット地帯） */
    int totalColumnsGenerated_ = 0;

    /** @brief この列数まで障害物を出さないフラット地帯の長さ */
    static constexpr int kFlatZoneColumns = 80;

    /** @brief ウェーブアニメーション用の時間カウンタ */
    float waveTime_ = 0.0f;

    /** @brief 現在のウェーブ強度（スムージング後） */
    float waveStrength_ = 0.0f;
};