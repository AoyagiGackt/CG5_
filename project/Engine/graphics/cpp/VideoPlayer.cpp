/**
 * @file VideoPlayer.cpp
 * @brief 動画ファイルをリアルタイムで再生してスプライトとして表示するクラス
 *
 * Media Foundation（Windows の音声・動画デコードライブラリ）で動画をフレームごとに読み取り、
 * DirectX 12 のテクスチャとして GPU に転送して、スプライトとして画面に表示する。
 *
 * 動画の1フレームを表示するステップ:
 *   1. Media Foundation でフレームの RGB ピクセルデータを取得する
 *   2. アップロードバッファ（CPU から書き込めるメモリ）にコピーする
 *   3. コマンドリストでアップロードバッファ → GPU テクスチャに転送する
 *   4. スプライトがそのテクスチャを参照して描画する
 */
#include "VideoPlayer.h"
#include "GameConstants.h"
#include "Logger.h"
#include "SrvManager.h"
#include "StringUtility.h"
#include <cassert>

using namespace Microsoft::WRL;

// デストラクタ: オブジェクトが破棄されるときに自動でリソースを解放する
VideoPlayer::~VideoPlayer()
{
    Finalize();
}

// 動画を読み込んで再生準備をする。
// dxCommon: DirectX12 の中核（GPU コマンド発行に使う）
// spriteCommon: 2D描画の共通設定
// filePath: 再生する動画ファイルのパス（例: "Resources/movie.mp4"）
void VideoPlayer::Initialize(DirectXCommon* dxCommon, SpriteCommon* spriteCommon, const std::string& filePath)
{
    dxCommon_    = dxCommon;
    spriteCommon_ = spriteCommon;

    HRESULT hr;

    // Media Foundation を初期化する（動画デコードの準備）
    hr = MFStartup(MF_VERSION);
    assert(SUCCEEDED(hr));

    // 属性オブジェクトを作成し、「ビデオ処理を有効にする」フラグを立てる
    // → これにより RGB32 形式への自動変換が有効になる
    ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 1);
    assert(SUCCEEDED(hr));
    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    assert(SUCCEEDED(hr));

    // ファイルパスを string から wstring に変換する（Windows API が wstring を要求するため）
    std::wstring wFilePath = StringUtility::ConvertString(filePath);

    // 動画ファイルを開く
    hr = MFCreateSourceReaderFromURL(wFilePath.c_str(), attributes.Get(), &pSourceReader_);
    if (FAILED(hr)) {
        Logger::Log("Failed to load video: " + filePath);
        assert(false);
    }

    // デコード先のフォーマットを「RGB32」に設定する
    // RGB32 = 1ピクセルを R・G・B・X の 4バイトで表す形式（DirectX で扱いやすい）
    ComPtr<IMFMediaType> pMediaType;
    MFCreateMediaType(&pMediaType);
    pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = pSourceReader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType.Get());
    assert(SUCCEEDED(hr));

    // デコード後の解像度（横幅 × 縦幅）を取得する
    ComPtr<IMFMediaType> pOutputMediaType;
    hr = pSourceReader_->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pOutputMediaType);
    assert(SUCCEEDED(hr));

    UINT32 width = 0, height = 0;
    MFGetAttributeSize(pOutputMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);
    videoWidth_  = width;
    videoHeight_ = height;

    // ----- GPU テクスチャの作成 -----

    // GPU 側のテクスチャ（DEFAULT ヒープ: GPU だけが読み書きできる高速なメモリ）
    D3D12_HEAP_PROPERTIES defaultHeapProps {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc {};
    resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width            = videoWidth_;
    resDesc.Height           = videoHeight_;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels        = 1; // ミップマップなし（1解像度のみ）
    resDesc.Format           = DXGI_FORMAT_B8G8R8X8_UNORM; // RGB32 に対応するフォーマット
    resDesc.SampleDesc.Count = 1;

    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, // 最初はシェーダーから読める状態
        IID_PPV_ARGS(&textureResource_));
    assert(SUCCEEDED(hr));

    // CPU 書き込み用の中間バッファ（UPLOAD ヒープ: CPU が書き込める、GPUへの転送用）
    // 毎フレームここにピクセルデータを書いて、GPU テクスチャにコピーする
    D3D12_HEAP_PROPERTIES uploadHeapProps {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // 必要なバッファサイズを GPU に問い合わせる（行ごとのパディングがあるため計算が必要）
    UINT64 uploadBufferSize = 0;
    dxCommon_->GetDevice()->GetCopyableFootprints(&resDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    D3D12_RESOURCE_DESC uploadBufferDesc {};
    uploadBufferDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadBufferDesc.Width            = uploadBufferSize;
    uploadBufferDesc.Height           = 1;
    uploadBufferDesc.DepthOrArraySize = 1;
    uploadBufferDesc.MipLevels        = 1;
    uploadBufferDesc.SampleDesc.Count = 1;
    uploadBufferDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // バッファは行優先レイアウト

    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, // CPU が読み書きできる状態
        IID_PPV_ARGS(&uploadBuffer_));
    assert(SUCCEEDED(hr));

    // テクスチャを GPU のシェーダーから参照できるよう SRV（シェーダーリソースビュー）を作成する
    srvIndex_ = SrvManager::GetInstance()->Allocate();
    SrvManager::GetInstance()->CreateSRVforTexture2D(
        srvIndex_, textureResource_.Get(), resDesc.Format, 1);

    // ----- 描画用スプライトを作成する -----
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(spriteCommon_, "Resources/uvChecker.png"); // テクスチャは後で差し替える

    sprite_->SetPosition({ 960.0f, 15.0f }); // 画面右上あたりに配置
    sprite_->SetSize({ 300.0f, 450.0f });     // 動画の表示サイズ

    // スプライトが参照するテクスチャを、動画用 SRV に差し替える
    sprite_->SetExternalTexture(srvIndex_);
}

// 毎フレーム呼ぶ。動画の次のフレームを取り出して GPU テクスチャに転送する。
void VideoPlayer::Update()
{
    // 動画のフレームレートより速く読み取らないよう、タイマーで間引きを行う
    // frameDuration_ = 動画の 1フレームあたりの時間（例: 30FPS なら約0.0333秒）
    timeCount_ += GameConstants::kFrameDeltaTime;

    if (timeCount_ < frameDuration_) {
        // まだ次のフレームに進む時間でなければスプライトだけ更新して終わる
        if (sprite_) {
            sprite_->Update();
        }
        return;
    }

    timeCount_ -= frameDuration_;

    if (!pSourceReader_) {
        return;
    }

    // 動画から次のフレームのサンプルデータを読み取る
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    ComPtr<IMFSample> pSample;
    HRESULT hr = pSourceReader_->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, &timestamp, &pSample);

    if (SUCCEEDED(hr) && pSample) {
        // サンプルをひとつの連続したメモリバッファに変換する
        ComPtr<IMFMediaBuffer> pBuffer;
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);

        if (SUCCEEDED(hr)) {
            // バッファをロックしてピクセルデータへのポインタを取得する
            BYTE* pData = nullptr;
            DWORD maxLength = 0, currentLength = 0;
            pBuffer->Lock(&pData, &maxLength, &currentLength);

            // GPU へのアップロードバッファをマップ（CPU から書き込めるようにする）
            BYTE* pMappedData = nullptr;
            uploadBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));

            // GPU が期待するテクスチャのレイアウト情報を取得する
            // RowPitch = 1行あたりのバイト数（GPU のアライメントでパディングが入ることがある）
            D3D12_RESOURCE_DESC desc = textureResource_->GetDesc();
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
            UINT numRows;
            UINT64 rowSizeInBytes, totalBytes;
            dxCommon_->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

            // 動画の 1行あたりのバイト数（パディングなし: 幅 × 4バイト/ピクセル）
            LONG mfStride = videoWidth_ * 4;

            // 1行ずつコピーする（GPU の RowPitch に合わせてアドレスを計算）
            for (int y = 0; y < videoHeight_; ++y) {
                memcpy(
                    pMappedData + y * footprint.Footprint.RowPitch, // コピー先（GPU 行境界に合わせる）
                    pData + y * mfStride,                            // コピー元（動画データ）
                    videoWidth_ * 4);                                // コピーするバイト数
            }

            uploadBuffer_->Unmap(0, nullptr); // マップを解除
            pBuffer->Unlock();

            isNewFrame_       = true;       // 新しいフレームが準備できた
            currentFootprint_ = footprint;  // Draw() でも使うので保存しておく

            // アップロードバッファの内容を GPU テクスチャにコピーする
            // コピー中はテクスチャのリソースバリアを「コピー先」に切り替える必要がある
            ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

            // リソースバリア: シェーダー参照可能 → コピー先に遷移
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource   = textureResource_.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
            commandList->ResourceBarrier(1, &barrier);

            // コピー先（GPU テクスチャ）と コピー元（アップロードバッファ）を指定してコピー実行
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource        = textureResource_.Get();
            dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource       = uploadBuffer_.Get();
            src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = footprint;

            commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

            // リソースバリア: コピー先 → シェーダー参照可能に戻す
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            commandList->ResourceBarrier(1, &barrier);
        }
    } else if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        // 動画の最後まで再生したら先頭に戻してループ再生する
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt             = VT_I8;
        var.hVal.QuadPart  = 0; // シーク位置 = 0（先頭）
        pSourceReader_->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
    }

    if (sprite_) {
        sprite_->Update();
    }
}

// 毎フレーム呼ぶ。スプライトとしてスクリーンに動画フレームを描画する。
void VideoPlayer::Draw()
{
    if (isNewFrame_) {
        // 新しいフレームがあれば GPU テクスチャにコピーしてから描画する
        // ※ Update() でコピーが完了しているので、ここでは描画だけ行う場合もある
        //    （現実装では Update() と Draw() 両方にコピー処理が残っているが、
        //     実際には Update() 側でのコピーで十分）
        ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = textureResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource        = textureResource_.Get();
        dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource       = uploadBuffer_.Get();
        src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = currentFootprint_;

        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);

        isNewFrame_ = false; // フラグをリセットして次のフレームを待つ
    }

    if (sprite_) {
        sprite_->Draw();
    }
}

// リソースをすべて解放する。デストラクタから自動的に呼ばれる。
void VideoPlayer::Finalize()
{
    pSourceReader_.Reset();    // Media Foundation のリーダーを解放
    textureResource_.Reset();  // GPU テクスチャを解放
    uploadBuffer_.Reset();     // アップロードバッファを解放
    sprite_.reset();           // スプライトを解放
}
