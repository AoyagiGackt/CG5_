/**
 * @file SceneFactory.h
 * @brief AbstractSceneFactoryを継承し、具体的なシーンの生成を管理するファイル
 */
#pragma once
#include "AbstractSceneFactory.h"

/**
 * @brief ゲーム固有のシーン生成工場クラス
 * @note このクラスは、文字列（シーン名）を受け取って対応するシーンクラスの
 * インスタンスを生成します。新しいシーンを追加した際は、このクラスの
 * CreateScene 内に生成ロジックを追記すること
 */
class SceneFactory : public AbstractSceneFactory {
public:
    /**
     * @brief 指定されたシーン名に対応するシーンを生成する
     * @param sceneName 生成したいシーンの名前（例: "TITLE", "GAMEPLAY"）
     * @return std::unique_ptr<BaseScene> 生成されたシーンのスマートポインタ
     * @note 内部で if文 や switch文 を使用し、名前とクラスを紐づけます
     * 未知のシーン名が渡された場合は nullptr を返します
     */
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;
};