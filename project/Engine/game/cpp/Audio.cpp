/**
 * @file Audio.cpp
 * @brief BGM・効果音（SE）の読み込みと再生を管理するクラス
 *
 * XAudio2 を使って音声を再生する。
 * Media Foundation（MF）で音声ファイルをデコードし（MP3・WAV など対応）、
 * XAudio2 の SourceVoice に渡して再生する。
 *
 * 【BGM】: 1曲だけ再生。ループ可能。Stop で停止、別のを再生するときは自動で前の曲を停止する。
 * 【SE】 : 複数同時再生可能。1つのSEが複数の SourceVoice を持てる。再生終了したものは自動削除する。
 */
#include "Audio.h"
#include "Logger.h"
#include "StringUtility.h"
#include <algorithm>
#include <cassert>

using namespace Microsoft::WRL;

// =====================================================
// 初期化 / 終了
// =====================================================

// 音声システムを初期化する。ゲーム起動時に1度だけ呼ぶ。
void Audio::Initialize()
{
    HRESULT hr;

    // Media Foundation を初期化する（音声・動画のデコードに必要な Windows の仕組み）
    // MFSTARTUP_NOSOCKET = ソケット通信機能なしで起動（軽量化）
    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    assert(SUCCEEDED(hr));

    // XAudio2 エンジンを作成する（音声の生成・加工・出力を担う中核）
    hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));

    // マスタリングボイスを作成する（最終的な音声出力先。スピーカーやヘッドホンにつながる）
    hr = xAudio2_->CreateMasteringVoice(&masteringVoice_);
    assert(SUCCEEDED(hr));

    Logger::Log("Audio System Initialized.\n");
}

// 音声システムを終了する。ゲーム終了時に1度だけ呼ぶ。
void Audio::Finalize()
{
    if (!xAudio2_) {
        return; // 初期化されていなければ何もしない
    }

    // 再生中の音声をすべて停止する
    StopBGM();
    StopAllSE();

    // マスタリングボイスを破棄する
    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
    }

    // XAudio2 エンジンを解放する
    xAudio2_.Reset();

    // Media Foundation を終了する
    MFShutdown();
}

// =====================================================
// ロード
// =====================================================

// 音声ファイル（MP3・WAV など）を読み込んでメモリに展開する。
// 戻り値の SoundData を PlayBGM / PlaySE に渡して再生する。
SoundData Audio::LoadAudio(const std::string& filename)
{
    HRESULT hr;
    SoundData soundData = {};

    // ファイルパスを string から wstring に変換する（Windows API が wstring を要求するため）
    std::wstring wFilename = StringUtility::ConvertString(filename);

    // Media Foundation でファイルを開く
    ComPtr<IMFSourceReader> pSourceReader;
    hr = MFCreateSourceReaderFromURL(wFilename.c_str(), nullptr, &pSourceReader);
    if (FAILED(hr)) {
        Logger::Log("Error: Failed to open audio file: " + filename + "\n");
        assert(false);
        return soundData;
    }

    // デコード先のフォーマットを「PCM 形式（無圧縮音声）」に指定する
    // XAudio2 が直接扱えるのは PCM のみなので、MP3 などは自動的に変換される
    ComPtr<IMFMediaType> pMediaType;
    MFCreateMediaType(&pMediaType);
    pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    hr = pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pMediaType.Get());
    if (FAILED(hr)) {
        Logger::Log("Error: Failed to set media type for: " + filename + "\n");
        assert(false);
        return soundData;
    }

    // デコード後のフォーマット情報（チャンネル数・サンプルレートなど）を取得する
    ComPtr<IMFMediaType> pOutputMediaType;
    hr = pSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutputMediaType);
    assert(SUCCEEDED(hr));

    // WAVEFORMATEX 構造体に音声フォーマット情報を書き込む（XAudio2 に渡す形式）
    WAVEFORMATEX* wfex = &soundData.wfex;
    wfex->wFormatTag = WAVE_FORMAT_PCM;

    UINT32 temp = 0;
    pOutputMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &temp);
    wfex->nChannels = (WORD)temp;          // チャンネル数（1 = モノラル、2 = ステレオ）
    pOutputMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &temp);
    wfex->nSamplesPerSec = temp;           // サンプルレート（例: 44100 = 1秒間に44100個のサンプル）
    pOutputMediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &temp);
    wfex->wBitsPerSample = (WORD)temp;     // サンプルあたりのビット数（例: 16ビット）
    pOutputMediaType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &temp);
    wfex->nBlockAlign = (WORD)temp;        // 1サンプルブロックのバイト数
    pOutputMediaType->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &temp);
    wfex->nAvgBytesPerSec = temp;          // 1秒あたりの平均バイト数
    wfex->cbSize = 0;

    // ファイルの終端まで PCM データをチャンクごとに読み取ってバッファに貯める
    std::vector<byte> rawData;
    while (true) {
        DWORD flags = 0;
        ComPtr<IMFSample> pSample;
        hr = pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &pSample);

        // エラー or ファイル終端に達したらループを抜ける
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            break;
        }

        if (!pSample) {
            continue; // サンプルが取れなかった場合はスキップ
        }

        // サンプルのメモリを連続したバッファに変換して rawData に追記する
        ComPtr<IMFMediaBuffer> pBuffer;
        pSample->ConvertToContiguousBuffer(&pBuffer);
        BYTE* pBufferPtr = nullptr;
        DWORD currentLength = 0;
        pBuffer->Lock(&pBufferPtr, nullptr, &currentLength);
        size_t oldSize = rawData.size();
        rawData.resize(oldSize + currentLength);
        memcpy(rawData.data() + oldSize, pBufferPtr, currentLength);
        pBuffer->Unlock();
    }

    // 読み込んだ PCM データを SoundData に格納して返す
    soundData.pBuffer    = std::move(rawData);
    soundData.bufferSize = static_cast<unsigned int>(soundData.pBuffer.size());
    return soundData;
}

// =====================================================
// ヘルパー
// =====================================================

// SoundData の WAVEFORMATEX から XAudio2 の SourceVoice（音声出力チャンネル）を作成して返す。
// SourceVoice = 音声1系統の出力口。同じ SE を複数同時再生するには SourceVoice を複数作る。
IXAudio2SourceVoice* Audio::CreateSourceVoice(const SoundData& soundData)
{
    IXAudio2SourceVoice* voice = nullptr;
    HRESULT hr = xAudio2_->CreateSourceVoice(&voice, &soundData.wfex);
    assert(SUCCEEDED(hr));
    return voice;
}

// 再生が終わった SE の SourceVoice を削除してメモリを解放する。
// seVoices_ には複数の SE が溜まっていくので、終わったものを定期的に掃除する必要がある。
void Audio::CleanupFinishedSE()
{
    seVoices_.erase(
        std::remove_if(seVoices_.begin(), seVoices_.end(), [](IXAudio2SourceVoice* v) {
            XAUDIO2_VOICE_STATE state;
            v->GetState(&state);

            // BuffersQueued == 0 = 再生待ちのバッファがない = 再生が終わった
            if (state.BuffersQueued == 0) {
                v->DestroyVoice(); // SourceVoice を破棄してメモリを解放
                return true;       // リストから除去する
            }

            return false; // まだ再生中なので残す
        }),
        seVoices_.end());
}

// =====================================================
// BGM
// =====================================================

// BGM を再生する。すでに別の BGM が再生中なら先に停止してから新しいものを再生する。
// soundData: LoadAudio で読み込んだ音声データ
// loop: true なら曲が終わったら自動的に最初から再生する
void Audio::PlayBGM(const SoundData& soundData, bool loop)
{
    // 前の BGM を停止してから新しいものを再生する
    StopBGM();

    bgmVoice_ = CreateSourceVoice(soundData);

    // 音声バッファを設定する
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = soundData.pBuffer.data(); // PCM データの先頭ポインタ
    buffer.AudioBytes = soundData.bufferSize;      // データのバイト数
    buffer.Flags      = XAUDIO2_END_OF_STREAM;    // これがバッファの終端であることを示す
    buffer.LoopCount  = loop ? XAUDIO2_LOOP_INFINITE : 0; // ループ回数（無限 or 0 = ループなし）

    HRESULT hr = bgmVoice_->SubmitSourceBuffer(&buffer);
    assert(SUCCEEDED(hr));

    bgmVoice_->Start(0); // 再生開始
}

// BGM を停止して SourceVoice を解放する。
void Audio::StopBGM()
{
    if (bgmVoice_) {
        bgmVoice_->Stop();
        bgmVoice_->DestroyVoice();
        bgmVoice_ = nullptr;
    }
}

// BGM の音量を変更する（0.0f = 無音、1.0f = 標準、それ以上 = 増幅）
void Audio::SetBGMVolume(float volume)
{
    if (bgmVoice_) {
        bgmVoice_->SetVolume(volume);
    }
}

// BGM の再生速度を変更する（1.0f = 等速、0.5f = 半速、2.0f = 倍速）
void Audio::SetBGMSpeed(float speed)
{
    if (bgmVoice_) {
        bgmVoice_->SetFrequencyRatio(speed);
    }
}

// =====================================================
// SE
// =====================================================

// SE を再生する。同じ SE を複数同時に鳴らすことができる。
// soundData: LoadAudio で読み込んだ音声データ
// volume: 音量（0.0f = 無音、1.0f = 標準）
void Audio::PlaySE(const SoundData& soundData, float volume)
{
    // 終了済みの SE の SourceVoice を先に片付けてメモリの無駄遣いを防ぐ
    CleanupFinishedSE();

    IXAudio2SourceVoice* voice = CreateSourceVoice(soundData);
    voice->SetVolume(volume);

    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = soundData.pBuffer.data();
    buffer.AudioBytes = soundData.bufferSize;
    buffer.Flags      = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount  = 0; // SE はループしない

    HRESULT hr = voice->SubmitSourceBuffer(&buffer);
    assert(SUCCEEDED(hr));

    voice->Start(0); // 再生開始

    // 後でクリーンアップできるようにリストに追加しておく
    seVoices_.push_back(voice);
}

// 再生中のすべての SE を停止して SourceVoice を解放する。
void Audio::StopAllSE()
{
    for (auto* v : seVoices_) {
        v->Stop();
        v->DestroyVoice();
    }
    seVoices_.clear();
}
