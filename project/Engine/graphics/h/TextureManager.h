/**
 * @file TextureManager.h
 * @brief テクスチャの読み込み、GPUへの転送、およびSRV（シェーダーリソースビュー）の管理を行うファイル
 */
#pragma once
#include "DirectXCommon.h"
#include "DirectXTex.h"
#include <d3d12.h>
#include <map>
#include <string>
#include <wrl.h>

/**
 * @brief テクスチャを管理するシングルトンクラス
 * @note DirectXTexライブラリを使用して画像を読み込み、SrvManagerと連携して
 * 適切なディスクリプタを割り当てます。一度読み込んだパスの画像は内部でキャッシュされます
 */
class TextureManager {
public:
    /**
     * @brief TextureManagerの唯一のインスタンスを取得する
     * @return TextureManager* シングルトンインスタンスへのポインタ
     */
    static TextureManager* GetInstance();

    /**
     * @brief マネージャーの初期化
     * @param dxCommon DirectX基盤のポインタ（デバイス取得などに使用）
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief マネージャーの終了処理。保持している全テクスチャリソースを解放する
     */
    void Finalize();

    /**
     * @brief 画像ファイルを読み込み、GPUリソースを作成する
     * @param filePath 読み込む画像のパス（例: "Resources/uvChecker.png"）
     * @note すでに読み込み済みのパスが指定された場合は、新たにロードせず既存のデータを参照します
     */
    void LoadTexture(const std::string& filePath);

    /**
     * @brief 指定したファイルパスに対応するSRVインデックスを取得する
     * @param filePath 取得したいテクスチャのファイルパス
     * @return uint32_t SrvManagerで管理されているSRVのインデックス
     */
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);

    /**
     * @brief 指定したテクスチャのGPUハンドルを取得する
     * @param filePath 取得したいテクスチャのファイルパス
     * @return D3D12_GPU_DESCRIPTOR_HANDLE 描画コマンドに渡すためのGPUハンドル
     * @note 事前に LoadTexture() で読み込まれている必要あり
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

    /**
     * @brief 指定したテクスチャのメタデータ（幅、高さ、形式など）を取得する
     * @param filePath 取得したいテクスチャのファイルパス
     * @return const DirectX::TexMetadata& DirectXTexで定義されたメタデータ構造体
     */
    const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

private:
    /**
     * @brief 読み込み済みのテクスチャ1つ分のデータを保持する構造体
     */
    struct TextureData {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource; ///< GPU上のテクスチャリソース
        uint32_t srvIndex; ///< デスクリプタヒープ上のインデックス
        DirectX::TexMetadata metadata; ///< テクスチャのメタデータ（幅、高さ、形式等）
    };

    /** @brief DirectX基盤のポインタ */
    DirectXCommon* dxCommon_ = nullptr;

    /** * @brief 読み込み済みテクスチャの管理用マップ
     * @note キー：ファイルパス（std::string）、値：テクスチャデータ（TextureData）
     */
    std::map<std::string, TextureData> textureDatas_;
};