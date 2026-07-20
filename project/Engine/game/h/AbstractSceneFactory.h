/**
 * @file AbstractSceneFactory.h
 * @brief シーンを生成するための抽象ファクトリクラスを定義するファイル
 */
#pragma once
#include "BaseScene.h"
#include <string>
#include <memory>

/**
 * @brief シーン生成のインターフェース（抽象ファクトリ）
 * @note このクラスを継承した具体的なファクトリ（SceneFactoryなど）を作ることで、
 * シーンマネージャーが個別のシーンクラスを知らなくてもシーンを生成できるようになる
 */
class AbstractSceneFactory {
public:

    /**
     * @brief 仮想デストラクタ
     */
    virtual ~AbstractSceneFactory() = default;

    /**
     * @brief シーンを生成する純粋仮想関数
     * @param sceneName 生成したいシーンの名前（例: "TITLE", "GAMEPLAY" など）
     * @return BaseScene* 生成されたシーンのポインタ（失敗した場合は nullptr）
     * @note 継承先のクラスで、名前文字列に応じた new処理を記述してください
     */
    virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};