#include "WorldTransform.h"
#include <cassert>

void WorldTransform::Initialize(ID3D12Device* device)
{
    // 定数バッファの生成
    // 生成したリソースをマッピングしておく
    constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
}

void WorldTransform::UpdateMatrix()
{
    // SRTから行列を作成 (スケール * 回転 * 平行移動)
    matWorld = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // 親がいれば、親の行列を掛け算して座標系を合成する
    if (parent) {
        matWorld = Multiply(matWorld, parent->matWorld);
    }

    // GPUへ送る準備
    TransferMatrix();
}

void WorldTransform::TransferMatrix()
{
    // マッピングされたメモリに行列をコピー
    mappedData_->matWorld = matWorld;
}