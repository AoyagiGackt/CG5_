/**
 * @file VideoPlayer.h
 * @brief Media Foundationを使用して動画を読み込み、テクスチャとして毎フレーム更新・描画するクラス
 */
#pragma once
#include "DirectXCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <wrl/client.h>

/**
 * @brief 動画再生・描画管理クラス
 * @note Media Foundation でデコードした映像フレームを動的にテクスチャへ書き込み、
 * Sprite クラスを通じて画面に表示します。
 */
class VideoPlayer {
public:
    /**
     * @brief コンストラクタ
     */
    VideoPlayer() = default;

    /**
     * @brief デストラクタ
     */
    ~VideoPlayer();

    /**
     * @brief 動画プレイヤーの初期化
     * @param dxCommon DirectX基盤のポインタ
     * @param spriteCommon スプライト共通設定のポインタ
     * @param filePath 読み込む動画ファイルのパス（例: "Resources/movie.mp4"）
     */
    void Initialize(DirectXCommon* dxCommon, SpriteCommon* spriteCommon, const std::string& filePath);

    /**
     * @brief 毎フレームの更新処理
     * @note 動画のコマ送りタイマーを更新し、新しいフレームがあればテクスチャへの書き込み準備を行います。
     */
    void Update();

    /**
     * @brief 描画処理
     * @note コマンドリストが開いているこのタイミングでGPUへデータを転送し、スプライトを描画します。
     */
    void Draw();

    /**
     * @brief 終了処理
     * @note 使用していた各種リソース（Media Foundation関連、GPUバッファ）を解放します。
     */
    void Finalize();

    // --- スプライト操作の中継 ---

    /**
     * @brief 動画の表示座標を取得
     * @return const Vector2& 座標(x, y)
     */
    const Vector2& GetPosition() const { return sprite_->GetPosition(); }

    /**
     * @brief 動画の表示座標を設定
     * @param pos 座標(x, y)
     */
    void SetPosition(const Vector2& pos)
    {
        if (sprite_) {
            sprite_->SetPosition(pos);
        }
    }

    /**
     * @brief 動画の表示サイズを取得
     * @return const Vector2& サイズ(width, height)
     */
    const Vector2& GetSize() const { return sprite_->GetSize(); }

    /**
     * @brief 動画の表示サイズを設定
     * @param size サイズ(width, height)
     */
    void SetSize(const Vector2& size)
    {
        if (sprite_) {
            sprite_->SetSize(size);
        }
    }

    /**
     * @brief 動画の回転角を取得
     * @return float 回転角（ラジアン）
     */
    float GetRotation() const { return sprite_->GetRotation(); }

    /**
     * @brief 動画の回転角を設定
     * @param rot 回転角（ラジアン）
     */
    void SetRotation(float rot)
    {
        if (sprite_) {
            sprite_->SetRotation(rot);
        }
    }

    // --- 再生速度の操作 ---

    /**
     * @brief 1フレームあたりの再生時間を取得
     * @return float 秒数
     */
    float GetFrameDuration() const { return frameDuration_; }

    /**
     * @brief 1フレームあたりの再生時間を設定（再生速度の変更）
     * @param duration 秒数（例: 30FPSなら 1.0f/30.0f）
     */
    void SetFrameDuration(float duration) { frameDuration_ = duration; }

private:
    /** @brief DirectX基盤のポインタ（借り物） */
    DirectXCommon* dxCommon_ = nullptr;

    /** @brief スプライト共通設定のポインタ（借り物） */
    SpriteCommon* spriteCommon_ = nullptr;

    // --- Media Foundation 関連 ---
    /** @brief 動画ファイルの読み込みを管理するインターフェース */
    Microsoft::WRL::ComPtr<IMFSourceReader> pSourceReader_;

    // --- DirectX12 GPUリソース ---
    /** @brief 動画フレームを書き込むテクスチャ本体 */
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;

    /** @brief CPUからテクスチャデータを送り込むためのアップロードバッファ */
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;

    /** @brief 割り当てられたSRV（シェーダーリソースビュー）のインデックス */
    uint32_t srvIndex_ = 0;

    // --- 内部描画用 ---
    /** @brief 映像を貼り付けて表示するためのスプライト */
    std::unique_ptr<Sprite> sprite_;

    /** @brief 動画の解像度（幅） */
    int videoWidth_ = 0;

    /** @brief 動画の解像度（高さ） */
    int videoHeight_ = 0;

    // --- フレームレート制御用 ---
    /** @brief 経過時間計測用タイマー */
    float timeCount_ = 0.0f;

    /** @brief 次のフレームに進むための間隔（秒） */
    float frameDuration_ = 1.0f / 30.0f;

    /** @brief 今のフレームでテクスチャ更新が必要かどうかのフラグ */
    bool isNewFrame_ = false;

    /** @brief 更新フレームのフットプリント情報 */
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT currentFootprint_ {};
};