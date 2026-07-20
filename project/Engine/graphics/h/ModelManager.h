/**
 * @file ModelManager.h
 * @brief 3Dモデルデータの読み込み・保持を一元管理するファイル
 */
#pragma once
#include "Model.h"
#include "ModelCommon.h"
#include <map>
#include <memory>
#include <string>

/**
 * @brief 3Dモデルを管理するシングルトンクラス
 * @note 同じファイルパスのモデルが複数回要求された場合でも、
 * 一度だけメモリに読み込んで共有する仕組み（Flyweightパターン）になっており、
 * メモリ使用量とロード時間を最適化します。
 */
class ModelManager {
public:
    /**
     * @brief ModelManagerの唯一のインスタンスを取得する
     * @return ModelManager* シングルトンインスタンスへのポインタ
     */
    static ModelManager* GetInstance();

    /**
     * @brief マネージャーの初期化。モデル生成に必要な共通設定を登録する
     * @param modelCommon モデルの共通描画設定を持つオブジェクトのポインタ
     */
    void Initialize(ModelCommon* modelCommon);

    /**
     * @brief 指定したファイルパスのモデルを読み込み、メモリに保持する
     * @param filePath 読み込むOBJファイルのパス（例: "Resources/model.obj"）
     * @param textureFilePath モデルに適用するテクスチャのパス（例: "Resources/texture.png"）
     * @note すでに同じ filePath のモデルが読み込まれている場合は、新規ロードをスキップします
     */
    void LoadModel(const std::string& filePath, const std::string& textureFilePath);

    /**
     * @brief 読み込み済みのモデルを検索して取得する
     * @param filePath 取得したいモデルのファイルパス
     * @return Model* 見つかったモデルのポインタ（未ロードの場合は nullptr を返す）
     */
    Model* FindModel(const std::string& filePath);

    /**
     * @brief マネージャーの終了処理。保持しているすべてのモデルデータを破棄する
     */
    void Finalize();

private:
    // シングルトンパターンのためのコンストラクタ・コピー禁止設定
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    /** @brief モデル生成時に渡す共通描画設定のポインタ */
    ModelCommon* modelCommon_ = nullptr;

    /** * @brief 読み込み済みモデルのリスト（連想配列）
     * @note キーにファイルパス（std::string）、値にモデルの実体（std::unique_ptr<Model>）を保持
     */
    std::map<std::string, std::unique_ptr<Model>> models_;
};