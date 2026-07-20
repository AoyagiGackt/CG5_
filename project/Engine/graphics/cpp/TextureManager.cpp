#include "TextureManager.h"
#include "DirectXTex.h"
#include "SrvManager.h"
#include "StringUtility.h"
#include <vector>
#include <cassert>
#include <algorithm>
#include <cctype>

using namespace Microsoft::WRL;

TextureManager* TextureManager::GetInstance()
{
    static TextureManager instance;
    return &instance;
}

void TextureManager::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    textureDatas_.clear();
}

void TextureManager::LoadTexture(const std::string& filePath)
{
    // 読み込み済みなら早期リターン
    if (textureDatas_.contains(filePath)) {
        return;
    }

    assert(dxCommon_);
    ID3D12Device* device = dxCommon_->GetDevice();

    // 画像ファイルを読み込む
    std::wstring filePathW = StringUtility::ConvertString(filePath);

    // 拡張子を取得して小文字化
    std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    // 最終的な画像データを格納するScratchImage
    DirectX::ScratchImage finalImage {};
    HRESULT hr;

    if (ext == "dds") {
        // DDSファイルはミップマップが埋め込み済みのためそのまま読み込む
        hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, finalImage);
        assert(SUCCEEDED(hr));
    } else {
        // WICファイル（PNG/JPG等）はミップマップを生成する
        DirectX::ScratchImage image {};
        // GIFはパレット形式のためFORCE_SRGBが使えない
        DirectX::WIC_FLAGS wicFlags = (ext == "gif") ? DirectX::WIC_FLAGS_DEFAULT_SRGB : DirectX::WIC_FLAGS_FORCE_SRGB;
        hr = DirectX::LoadFromWICFile(filePathW.c_str(), wicFlags, nullptr, image);
        assert(SUCCEEDED(hr));

        // ミップマップ生成。失敗した場合（非対応フォーマット・奇数サイズ等）は元画像をミップレベル1として使用する
        hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_BOX, 0, finalImage);
        if (FAILED(hr)) {
            hr = finalImage.InitializeFromImage(*image.GetImages());
            assert(SUCCEEDED(hr));
        }
    }

    // テクスチャリソース記述子
    const DirectX::TexMetadata& metadata = finalImage.GetMetadata();
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width            = UINT(metadata.width);
    resourceDesc.Height           = UINT(metadata.height);
    resourceDesc.MipLevels        = UINT16(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    resourceDesc.Format           = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    // VRAM（DEFAULT heap）にテクスチャリソースを作成
    D3D12_HEAP_PROPERTIES defaultHeap {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> resource;
    hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // 転送先として開始
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    // アップロードバッファのサイズを算出して作成
    const UINT subresourceCount = UINT(metadata.mipLevels * metadata.arraySize);
    const UINT64 uploadSize = GetRequiredIntermediateSize(resource.Get(), 0, subresourceCount);

    D3D12_HEAP_PROPERTIES uploadHeap {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc {};
    uploadDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width            = uploadSize;
    uploadDesc.Height           = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels        = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer));
    assert(SUCCEEDED(hr));

    // サブリソースデータを構築（mip×array の全スライス）
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    subresources.reserve(subresourceCount);
    for (size_t arrayIndex = 0; arrayIndex < metadata.arraySize; ++arrayIndex) {
        for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
            const DirectX::Image* img = finalImage.GetImage(mipLevel, arrayIndex, 0);
            D3D12_SUBRESOURCE_DATA sub {};
            sub.pData      = img->pixels;
            sub.RowPitch   = LONG_PTR(img->rowPitch);
            sub.SlicePitch = LONG_PTR(img->slicePitch);
            subresources.push_back(sub);
        }
    }

    // 転送専用コマンドリストを作成して記録
    ComPtr<ID3D12CommandAllocator> uploadAlloc;
    ComPtr<ID3D12GraphicsCommandList> uploadCmdList;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAlloc));
    assert(SUCCEEDED(hr));
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadAlloc.Get(), nullptr, IID_PPV_ARGS(&uploadCmdList));
    assert(SUCCEEDED(hr));

    // アップロードバッファ → VRAM へコピーコマンドを積む
    UpdateSubresources(uploadCmdList.Get(), resource.Get(), uploadBuffer.Get(),
        0, 0, subresourceCount, subresources.data());

    // バリア: COPY_DEST → PIXEL_SHADER_RESOURCE（全サブリソース＝全ミップレベルを一括遷移）
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type                        = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource        = resource.Get();
    barrier.Transition.Subresource      = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore      = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter       = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    uploadCmdList->ResourceBarrier(1, &barrier);

    // コマンドリストを閉じてキューに投入
    hr = uploadCmdList->Close();
    assert(SUCCEEDED(hr));
    ID3D12CommandList* cmdLists[] = { uploadCmdList.Get() };
    dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);

    // GPU完了を待つ（一時フェンス）
    ComPtr<ID3D12Fence> uploadFence;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));
    assert(SUCCEEDED(hr));
    dxCommon_->GetCommandQueue()->Signal(uploadFence.Get(), 1);
    if (uploadFence->GetCompletedValue() < 1) {
        HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(evt != nullptr);
        uploadFence->SetEventOnCompletion(1, evt);
        WaitForSingleObject(evt, INFINITE);
        CloseHandle(evt);
    }
    // uploadBuffer はここでスコープ外になり自動解放される

    // SRV作成（キューブマップか2Dテクスチャかで種別を分ける）
    uint32_t srvIndex = SrvManager::GetInstance()->Allocate();

    if (metadata.IsCubemap()) {
        SrvManager::GetInstance()->CreateSRVforTextureCube(
            srvIndex, resource.Get(), metadata.format, UINT(metadata.mipLevels));
    } else {
        SrvManager::GetInstance()->CreateSRVforTexture2D(
            srvIndex, resource.Get(), metadata.format, UINT(metadata.mipLevels));
    }

    // データ保存
    TextureData& data = textureDatas_[filePath];
    data.resource = resource;
    data.srvIndex = srvIndex; // インデックスを保存
    data.metadata = metadata;
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
    assert(textureDatas_.contains(filePath));
    return textureDatas_[filePath].srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
    assert(textureDatas_.contains(filePath));
    return SrvManager::GetInstance()->GetGPUDescriptorHandle(textureDatas_[filePath].srvIndex);
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
    assert(textureDatas_.contains(filePath));
    return textureDatas_[filePath].metadata;
}

void TextureManager::Finalize()
{
    textureDatas_.clear();
}