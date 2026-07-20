/**
 * @file WorldTransform.h
 * @brief 3Dオブジェクトの座標変換（SRT）とワールド行列を管理するファイル
 */
#pragma once
#include "MakeAffine.h"
#include <d3d12.h>
#include <wrl.h>

/**
 * @brief GPUへ転送するワールド行列の構造体
 */
struct ConstBufferDataWorld {
    Matrix4x4 matWorld; ///< 最終的なワールド行列
};

/**
 * @brief 3Dオブジェクトの位置・回転・スケールと行列計算を担うクラス
 */
class WorldTransform {
public:
    // --- メンバ変数 ---

    /** @brief ローカル座標（位置・回転・スケール）のデータ */
    Transform transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    /** @brief 計算済みのワールド行列 */
    Matrix4x4 matWorld;

    /** @brief 親となるWorldTransformのポインタ（親子構造・追従用） */
    const WorldTransform* parent = nullptr;

    // --- メンバ関数 ---

    /**
     * @brief 定数バッファの生成と初期化
     * @param device DirectX12デバイスのポインタ
     */
    void Initialize(ID3D12Device* device);

    /**
     * @brief SRTから行列を計算し、親があれば合成する
     * @note 座標を変更した後は必ず呼び出す必要があります
     */
    void UpdateMatrix();

    /**
     * @brief 計算した行列を定数バッファ（GPU）へ書き込む
     */
    void TransferMatrix();

    /**
     * @brief 描画コマンドに使用する定数バッファリソースを取得
     * @return ID3D12Resource* バッファリソースのポインタ
     */
    ID3D12Resource* GetConstantBuffer() const { return constBuffer_.Get(); }

private:
    /** @brief GPU側の定数バッファリソース */
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

    /** @brief 書き込み用にマッピングされたポインタ */
    ConstBufferDataWorld* mappedData_ = nullptr;
};