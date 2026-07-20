/**
 * @file ResourceObject.h
 * @brief DirectX12のリソース（ID3D12Resource）の寿命を管理し、解放忘れを防ぐためのラッパークラス
 */
#pragma once
#include <d3d12.h>

/**
 * @brief COMオブジェクト（ID3D12Resource）の解放処理を自動化するRAIIクラス
 * @note ComPtrの簡易版のような役割を果たし変数のスコープを抜けた際（寿命が尽きた際）に
 * 自動で Release() が呼ばれるため、メモリリークを安全に防ぐことができるはず
 */
class ResourceObject {
public:

    /**
     * @brief コンストラクタ。管理対象となるDirectX12リソースのポインタを受け取る
     * @param resource 管理対象とする ID3D12Resource の生ポインタ
     */
    ResourceObject(ID3D12Resource* resource)
        : resource_(resource)
    {
    }

    /**
     * @brief デストラクタ。保持しているリソースが有効であれば、自動的に解放（Release）する
     */
    ~ResourceObject()
    {
        if (resource_) {
            resource_->Release();
        }
    }

    /**
     * @brief 管理しているリソースの生ポインタを取得する
     * @return ID3D12Resource* DirectX12リソースの生ポインタ
     * @note DirectXのAPI（各種ビューの作成やコマンドへのセットなど）に直接ポインタを渡す際に使用
     */
    ID3D12Resource* Get() { return resource_; }

private:

    /** @brief 管理対象となっているDirectX12リソースのポインタ */
    ID3D12Resource* resource_;
};
