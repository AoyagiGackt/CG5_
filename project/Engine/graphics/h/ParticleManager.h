/**
 * @file ParticleManager.h
 * @brief Compute Shader でパーティクルを GPU 完結シミュレーションし、インスタンシング描画するファイル
 */
#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "MakeAffine.h"
#include "Model.h"
#include "SrvManager.h"
#include <array>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

/**
 * @brief GPU 側で保持するパーティクル 1 粒のシミュレーションデータ (CS が読み書き)
 */
struct GPUParticleState {
    Vector3  position;    // 12
    float    lifeTime;    //  4 -> 16
    Vector3  velocity;    // 12
    float    currentTime; //  4 -> 32
    Vector4  color;       // 16 -> 48
    Vector3  scale;       // 12
    float    rotateZ;     //  4 -> 64
    uint32_t alive;       //  4
    uint32_t curveFlag;   //  4  (1 = enemyDeath 螺旋)
    float    pad[2];      //  8 -> 80
};

/**
 * @brief 描画のために VS が読むインスタンシングデータ (CS が書き込み)
 */
struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4   color;
};

/**
 * @brief CS に渡す定数バッファの内容
 */
struct CSConstants {
    Matrix4x4 billboard;    // 64
    Matrix4x4 viewProj;     // 64
    float     deltaTime;    //  4
    uint32_t  maxParticles; //  4
    float     pad[2];       //  8 -> 144
};

/**
 * @brief EmitParticle.CS に渡すエミッター定数バッファ（UPLOAD heap 256 bytes に格納）
 *        CPU 側で frequencyTime / emit を毎フレーム更新する
 */
struct Emitter {
    Vector3  translate;     // 12
    float    radius;        //  4 -> 16
    uint32_t count;         //  4
    float    frequency;     //  4
    float    frequencyTime; //  4
    uint32_t emit;          //  4 -> 32
    float    lifeTime;      //  4
    uint32_t seed;          //  4
    uint32_t time;          //  4  (groupTime の float ビット列、Update が自動設定)
    float    pad;           //  4 -> 48
    Vector4  color;         // 16 -> 64
};

/**
 * @brief 同じテクスチャを共有するパーティクルの集まり（グループ）
 */
struct ParticleGroup {
    std::string textureFilePath;

    // GPU シミュレーションステートバッファ (DEFAULT heap, UAV)
    Microsoft::WRL::ComPtr<ID3D12Resource> particleStateBuffer;

    // CPU→GPU コピー用ステージングバッファ (UPLOAD heap, 常時マップ済み)
    Microsoft::WRL::ComPtr<ID3D12Resource> particleUploadBuffer;
    GPUParticleState* particleUploadData = nullptr;

    // インスタンシング描画バッファ (DEFAULT heap, CS が UAV として書き、VS が SRV として読む)
    uint32_t srvIndex = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;

    static constexpr uint32_t kNumMaxInstance = 1024;

    // CPU 側スロット管理 (GPU readback 不要)
    std::array<float, kNumMaxInstance> slotExpiry = {};
    float groupTime = 0.0f;

    // 空きスロットのスタック（O(1) Allocate）
    std::vector<uint32_t> freeList;

    // このフレームに新規発生したスロット一覧 (次の Update で GPU にコピーする)
    std::vector<uint32_t> pendingSlots;

    // instancingResource の現在状態
    bool instancingInSRV = false;   // false=UAV  true=NON_PIXEL_SHADER_RESOURCE
    bool needsInit       = true;    // 初回 Update で全スロットをゼロ初期化する
    bool additiveBlend   = true;    // false = alpha blend (SRC_ALPHA / INV_SRC_ALPHA)

    // デフォルト寿命（EmitBurst で設定される）
    float defaultLifeTime = 1.0f;

    // GPU エミッター (EmitParticle.CS)
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterBuffer;
    Emitter* emitterData = nullptr;

    // CPU 自動再配置（EmitScatterLoop 用）
    struct RespawnConfig {
        Vector3  center      = {};
        float    radius      = 0.0f;
        float    lifeTimeMin = 1.0f;
        float    lifeTimeMax = 1.0f;
        Vector4  color       = { 1.0f, 1.0f, 1.0f, 1.0f };
        float    scale       = 1.0f;
        uint32_t count       = 0;
    };
    bool          autoRespawn  = false;
    RespawnConfig respawnConfig;
};

/**
 * @brief パーティクル全体を管理し、CS でシミュレーション・インスタンシング描画を行うシングルトン
 */
class ParticleManager {
public:
    static ParticleManager* GetInstance();

    void Initialize(DirectXCommon* dxCommon);
    void Finalize();

    void Update(Camera* camera);
    void Draw(Camera* camera);

    void Emit(const std::string& name, const Vector3& position, const Vector3& velocity);
    void EmitWithColor(const std::string& name, const Vector3& position,
        const Vector3& velocity, const Vector4& color,
        float lifeTime = 1.0f, float scale = 1.0f, bool flicker = false);
    void EmitEllipse(const std::string& name, const Vector3& position,
        const Vector3& velocity, const Vector4& color,
        float lifeTime = 1.0f, float scaleX = 2.0f, float scaleY = 1.0f);
    void EmitSlash(const std::string& name, const Vector3& position,
        float angle, const Vector4& color, float radius = 1.0f);
    void EmitHitStar(const std::string& name, const Vector3& position, const Vector4& color);
    void EmitBurst(const std::string& name, const Vector3& position, const Vector4& color,
        uint32_t count = ParticleGroup::kNumMaxInstance,
        float lifeTime = 100000.0f, float scale = 1.0f, bool flicker = false);
    void EmitScatterLoop(const std::string& name, const Vector3& center, float radius,
        uint32_t count, const Vector4& color,
        float lifeTimeMin, float lifeTimeMax, float scale);

    void SetTexture(const std::string& groupName, const std::string& textureFilePath);
    void CreateParticleGroup(const std::string& name, const std::string& textureFilePath);
    void ClearAllGroups()       { particleGroups_.clear(); }
    void SetAdditiveBlend(const std::string& name, bool additive);

    // 指定グループに生存中のパーティクルがあるかを返す
    bool IsGroupAlive(const std::string& name) const;

    // エミッターのポインタを返す。game 側で translate/radius/count/frequency/lifeTime を設定する
    Emitter* GetEmitter(const std::string& name);

private:
    ParticleManager() = default;
    ~ParticleManager() = default;
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    void CreateRootSignature();
    void CreatePipelineState();
    void CreateCSRootSignature();
    void CreateCSPipelineState();
    void CreateCSEmitRootSignature();
    void CreateCSEmitPipelineState();
    void CreateQuadGeometry();

    // 空きスロットを返す。なければ UINT32_MAX
    uint32_t AllocateSlot(ParticleGroup& group);

private:
    DirectXCommon* dxCommon_ = nullptr;

    // quad ジオメトリ（全グループ共有）
    Microsoft::WRL::ComPtr<ID3D12Resource> quadVertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> quadIndexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW quadVBV_{};
    D3D12_INDEX_BUFFER_VIEW  quadIBV_{};

    // グラフィックスパイプライン
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;        // additive blend
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateAlpha_;   // alpha blend

    // Compute パイプライン (Update)
    Microsoft::WRL::ComPtr<ID3D12RootSignature> csRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csPipelineState_;

    // Compute パイプライン (Emit)
    Microsoft::WRL::ComPtr<ID3D12RootSignature> csEmitRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> csEmitPipelineState_;

    // フレームごとの CS 定数バッファ (UPLOAD heap, 常時マップ済み)
    Microsoft::WRL::ComPtr<ID3D12Resource> csConstantsBuffer_;
    CSConstants* csConstantsData_ = nullptr;

    std::unordered_map<std::string, ParticleGroup> particleGroups_;
};
