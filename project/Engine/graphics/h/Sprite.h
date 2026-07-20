/**
 * @file Sprite.h
 * @brief 2D画像（スプライト）の描画、座標変換、テクスチャ切り出しなどを管理するファイル
 */
#pragma once
#include "MakeAffine.h"
#include "SpriteCommon.h"
#include <string>

/**
 * @brief スプライトを描画するための1頂点のデータ
 */
struct VertexDataSprite {
    Vector4 position; /// 頂点座標（2D空間）
    Vector2 texcoord; /// UV座標（テクスチャの切り出し位置）
    Vector3 normal; /// 法線ベクトル（ライティング用）
};

/**
 * @brief スプライトの質感（色やライティング設定）用データ
 */
struct MaterialSprite {
    Vector4 color; /// スプライトの色・透明度（RGBA）
    int enableLighting; /// ライティングを有効にするか（1:有効, 0:無効）
    int shadingType; /// シェーディングの種類
    int useCubemap; /// キューブマップを使用するか（スプライトは常に0）
    int useTexture; /// テクスチャをサンプリングするか（1:有効, 0:白固定）
    Matrix4x4 uvTransform; /// UVアニメーション用の変換行列
};

/**
 * @brief スプライトの座標変換用データ
 * Object3dVS.hlsl が WVP/World/WorldInverseTranspose/LightVP の 4 行列すべてを読むため、
 * スプライトで不要な後半 2 行列もバッファサイズ確保のために定義する（値は単位行列固定）。
 */
struct TransformationMatrixSprite {
    Matrix4x4 WVP;                   /// ワールド・ビュー・プロジェクション行列（2D正投影）
    Matrix4x4 World;                 /// ワールド行列
    Matrix4x4 WorldInverseTranspose; /// VS が読むため確保（スプライトは enableLighting=false なので未使用）
    Matrix4x4 LightVP;               /// VS が読むため確保（スプライトは影を受けないので未使用）
};

/**
 * @brief 2Dスプライトのインスタンスを管理・描画するクラス
 */
class Sprite {
public:
    /**
     * @brief スプライトの初期化とバッファの生成
     * @param spriteCommon 2D描画の共通設定オブジェクトのポインタ
     * @param textureFilePath 貼り付けるテクスチャ画像のパス（例: "Resources/uvChecker.png"）
     */
    void Initialize(SpriteCommon* spriteCommon, std::string textureFilePath);

    /**
     * @brief 毎フレームの更新処理。座標やサイズから行列を再計算する
     */
    void Update();

    /**
     * @brief スプライトを描画するコマンドを積む
     * @note 事前に SpriteCommon::CommonDrawSettings() が呼ばれている必要があり
     */
    void Draw();

    // --- セッター・ゲッター ---

    /** @brief 画面上の描画座標（X, Y）を取得 */
    const Vector2& GetPosition() const { return position_; }

    /** @brief 画面上の描画座標（X, Y）を設定 */
    void SetPosition(const Vector2& position) { position_ = position; }

    /** @brief スプライトの回転角（Z軸回転）を取得 */
    float GetRotation() const { return rotation_; }

    /** @brief スプライトの回転角（ラジアン）を設定 */
    void SetRotation(float rotation) { rotation_ = rotation; }

    /** @brief スプライトの描画サイズ（幅、高さ）を取得 */
    const Vector2& GetSize() const { return size_; }

    /** @brief スプライトの描画サイズ（幅、高さ）を設定 */
    void SetSize(const Vector2& size) { size_ = size; }

    /** @brief 画像の基準点（0.0～1.0）を取得 */
    const Vector2& GetAnchorPoint() const { return anchorPoint_; }

    /** @brief 画像の基準点を設定（例: {0.5f, 0.5f} で中心基準） */
    void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }

    /** @brief スプライトの色（RGBA）を取得 */
    const Vector4& GetColor() const { return materialData_->color; }

    /** @brief スプライトの色・透明度を設定（デフォは {1,1,1,1}） */
    void SetColor(const Vector4& color) { materialData_->color = color; }

    /** @brief 左右反転フラグを設定 */
    void SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }

    /** @brief 上下反転フラグを設定 */
    void SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }

    // --- テクスチャ切り出し機能 ---

    /** @brief テクスチャの切り出し開始位置（左上）を取得 */
    const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }

    /** @brief テクスチャの切り出し開始位置（左上X, 左上Y）を設定 */
    void SetTextureLeftTop(const Vector2& textureLeftTop) { textureLeftTop_ = textureLeftTop; }

    /** @brief テクスチャの切り出しサイズを取得 */
    const Vector2& GetTextureSize() const { return textureSize_; }

    /** @brief テクスチャの切り出しサイズ（幅, 高さ）を設定 */
    void SetTextureSize(const Vector2& textureSize) { textureSize_ = textureSize; }

    /**
     * @brief 描画するテクスチャを別の画像に変更する
     * @param textureFilePath 新しいテクスチャ画像のパス
     */
    void SetTexture(std::string textureFilePath);

    /** @brief 外部で作成したSRVインデックスを直接指定する（動画用） */
    void SetExternalTexture(uint32_t srvIndex)
    {
        textureIndex_ = srvIndex;
        useExternalTexture_ = true;
    }

private:
    /**
     * @brief テクスチャサイズと実際の描画サイズを調整・反映する内部処理
     */
    void AdjustTextureSize();

private:
    /** @brief スプライト描画の共通設定ポインタ */
    SpriteCommon* spriteCommon_ = nullptr;

    // --- GPUリソース関連 ---
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    MaterialSprite* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrixSprite* transformationMatrixData_ = nullptr;

    // --- スプライト固有データ ---
    std::string textureFilePath_; /// 使用中のテクスチャパス

    // スプライトパラメータ
    Vector2 position_ = { 250.0f, 250.0f }; /// 座標
    float rotation_ = 0.0f; /// 回転角（Z軸）
    Vector2 size_ = { 640.0f, 360.0f }; /// 描画サイズ
    Vector2 anchorPoint_ = { 0.0f, 0.0f }; /// アンカーポイント（デフォルトは左上）
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; /// 色・アルファ値
    uint32_t srvIndex_ = 0;

    bool isFlipX_ = false; /// 左右反転フラグ
    bool isFlipY_ = false; /// 上下反転フラグ

    // テクスチャ切り出し用
    Vector2 textureLeftTop_ = { 0.0f, 0.0f }; /// 切り出し開始座標
    Vector2 textureSize_ = { 100.0f, 100.0f }; /// 切り出しサイズ
    uint32_t textureIndex_ = 0;
    bool useExternalTexture_ = false;
};
