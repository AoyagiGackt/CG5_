/**
 * @file LightManager.h
 * @brief ゲーム全体のライティング（陰影処理）の設定を一元管理するファイル
 */
#pragma once
#include "LightingMode.h"

/**
 * @brief ライティングのモードを管理するシングルトンクラス
 * @note どこからでも LightManager::GetInstance() を通じてアクセスし、
 * 現在のライティング設定を取得・変更することができる
 */
class LightManager {
public:
    
    /**
     * @brief LightManagerの唯一のインスタンスを取得する
     * @return LightManager* シングルトンインスタンスへのポインタ
     */
    static LightManager* GetInstance()
    {
        static LightManager instance;
        return &instance;
    }

    /**
     * @brief 現在のライティングモードを設定する
     * @param mode 設定するライティングモード（LightingMode 列挙型の値）
     * @note 例: LightingMode::Lighting_Lambert や LightingMode::Lighting_HalfLambert を指定する
     */
    void SetLightingMode(int mode) { lightingMode_ = mode; }

    /**
     * @brief 現在設定されているライティングモードを取得する
     * @return int 現在のライティングモード（LightingMode 列挙型の値）
     */
    int GetLightingMode() const { return lightingMode_; }

private:
    // シングルトンパターンのためのコンストラクタ・コピー禁止設定
    LightManager() = default;
    ~LightManager() = default;
    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    /** @brief 現在のライティングモード（初期値はハーフランバート） */
    int lightingMode_ = LightingMode::Lighting_HalfLambert;
};