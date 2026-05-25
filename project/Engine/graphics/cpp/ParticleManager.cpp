#include "ParticleManager.h"
#include "GameConstants.h"
#include "TextureManager.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <random>
#include <d3dx12.h>

using namespace Microsoft::WRL;

ParticleManager* ParticleManager::GetInstance()
{
    static ParticleManager instance;
    return &instance;
}

// ============================================================
//  初期化 / 終了
// ============================================================

void ParticleManager::Initialize(DirectXCommon* dxCommon)
{
    assert(dxCommon);
    dxCommon_ = dxCommon;

    CreateRootSignature();
    CreatePipelineState();
    CreateCSRootSignature();
    CreateCSPipelineState();
    CreateCSEmitRootSignature();
    CreateCSEmitPipelineState();

    // CS 定数バッファ (UPLOAD heap, 256 bytes, 常時マップ)
    csConstantsBuffer_ = dxCommon_->CreateBufferResource(256);
    csConstantsBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&csConstantsData_));

    CreateQuadGeometry();
}

void ParticleManager::Finalize()
{
    particleGroups_.clear();

    if (csConstantsBuffer_) {
        csConstantsBuffer_->Unmap(0, nullptr);
        csConstantsData_ = nullptr;
    }

    quadIndexBuffer_.Reset();
    quadVertexBuffer_.Reset();
    csEmitPipelineState_.Reset();
    csEmitRootSignature_.Reset();
    csPipelineState_.Reset();
    csRootSignature_.Reset();
    graphicsPipelineStateAlpha_.Reset();
    graphicsPipelineState_.Reset();
    rootSignature_.Reset();
    dxCommon_ = nullptr;
}

// ============================================================
//  グループ生成
// ============================================================

void ParticleManager::CreateParticleGroup(const std::string& name,
                                          const std::string& textureFilePath)
{
    assert(!particleGroups_.contains(name));

    ParticleGroup& group = particleGroups_[name];
    group.textureFilePath = textureFilePath;

    TextureManager::GetInstance()->LoadTexture(textureFilePath);

    ID3D12Device* device = dxCommon_->GetDevice();
    const UINT64 stateSize   = sizeof(GPUParticleState) * ParticleGroup::kNumMaxInstance;
    const UINT64 instancSize = sizeof(ParticleForGPU)   * ParticleGroup::kNumMaxInstance;

    // ---- particleStateBuffer: DEFAULT heap, UAV ----
    {
        D3D12_HEAP_PROPERTIES hp{ D3D12_HEAP_TYPE_DEFAULT };
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = stateSize;
        rd.Height           = rd.DepthOrArraySize = rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        HRESULT hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
            IID_PPV_ARGS(&group.particleStateBuffer));
        assert(SUCCEEDED(hr));
    }

    // ---- particleUploadBuffer: UPLOAD heap, 常時マップ ----
    {
        D3D12_HEAP_PROPERTIES hp{ D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = stateSize;
        rd.Height           = rd.DepthOrArraySize = rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        HRESULT hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&group.particleUploadBuffer));
        assert(SUCCEEDED(hr));
        group.particleUploadBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&group.particleUploadData));
        memset(group.particleUploadData, 0, static_cast<size_t>(stateSize));
    }

    // ---- instancingResource: DEFAULT heap, UAV (CS が書く) ----
    {
        D3D12_HEAP_PROPERTIES hp{ D3D12_HEAP_TYPE_DEFAULT };
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width            = instancSize;
        rd.Height           = rd.DepthOrArraySize = rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        HRESULT hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
            IID_PPV_ARGS(&group.instancingResource));
        assert(SUCCEEDED(hr));
    }

    // ---- SRV for instancingResource (VS が t0 として読む) ----
    group.srvIndex = SrvManager::GetInstance()->Allocate();
    D3D12_CPU_DESCRIPTOR_HANDLE cpuH =
        SrvManager::GetInstance()->GetCPUDescriptorHandle(group.srvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements         = ParticleGroup::kNumMaxInstance;
    srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
    device->CreateShaderResourceView(group.instancingResource.Get(), &srvDesc, cpuH);

    group.slotExpiry.fill(0.0f);
    group.groupTime      = 0.0f;
    group.needsInit      = true;
    group.instancingInSRV = false;

    group.freeList.clear();
    group.freeList.reserve(ParticleGroup::kNumMaxInstance);
    for (uint32_t i = ParticleGroup::kNumMaxInstance; i-- > 0; )
        group.freeList.push_back(i);

    // ---- エミッターバッファ (UPLOAD heap, 256 bytes, 常時マップ) ----
    group.emitterBuffer = dxCommon_->CreateBufferResource(256);
    group.emitterBuffer->Map(0, nullptr, reinterpret_cast<void**>(&group.emitterData));
    memset(group.emitterData, 0, sizeof(Emitter));
    group.emitterData->lifeTime = 1.0f; // デフォルト 1 秒
}

// ============================================================
//  スロット管理
// ============================================================

uint32_t ParticleManager::AllocateSlot(ParticleGroup& group)
{
    if (group.freeList.empty()) { return UINT32_MAX; }
    uint32_t slot = group.freeList.back();
    group.freeList.pop_back();
    return slot;
}

// ============================================================
//  Emit 系
// ============================================================

void ParticleManager::Emit(const std::string& name,
                           const Vector3& position,
                           const Vector3& velocity)
{
    EmitWithColor(name, position, velocity, {1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 1.0f);
}

void ParticleManager::EmitWithColor(const std::string& name,
                                    const Vector3& position,
                                    const Vector3& velocity,
                                    const Vector4& color,
                                    float lifeTime,
                                    float scale,
                                    bool flicker)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    uint32_t slot = AllocateSlot(group);

    if (slot == UINT32_MAX) {
        return;
    }

    GPUParticleState& p = group.particleUploadData[slot];
    p.position    = position;
    p.lifeTime    = lifeTime;
    p.velocity    = velocity;
    p.currentTime = 0.0f;
    p.color       = color;
    p.scale       = { scale, scale, scale };
    p.rotateZ     = 0.0f;
    p.alive       = 1;
    p.curveFlag   = flicker ? 2u : 0u;

    group.slotExpiry[slot] = group.groupTime + lifeTime + 0.1f;
    group.pendingSlots.push_back(slot);
}

void ParticleManager::EmitEllipse(const std::string& name,
                                  const Vector3& position,
                                  const Vector3& velocity,
                                  const Vector4& color,
                                  float lifeTime,
                                  float scaleX,
                                  float scaleY)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    uint32_t slot = AllocateSlot(group);

    if (slot == UINT32_MAX) {
        return;
    }

    GPUParticleState& p = group.particleUploadData[slot];
    p.position    = position;
    p.lifeTime    = lifeTime;
    p.velocity    = velocity;
    p.currentTime = 0.0f;
    p.color       = color;
    p.scale       = { scaleX, scaleY, 1.0f };
    p.rotateZ     = 0.0f;
    p.alive       = 1;
    p.curveFlag   = 0;

    group.slotExpiry[slot] = group.groupTime + lifeTime + 0.1f;
    group.pendingSlots.push_back(slot);
}

void ParticleManager::EmitSlash(const std::string& name,
                                const Vector3& position,
                                float angle,
                                const Vector4& color,
                                float radius)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    const int   kCount    = 6;
    const float kSpread   = 2.0f;
    const float kSpeed    = 6.0f;
    const float kLifeTime = 0.15f;

    for (int i = 0; i < kCount; ++i) {
        uint32_t slot = AllocateSlot(group);

        if (slot == UINT32_MAX) {
            break;
        }

        float t = static_cast<float>(i) / static_cast<float>(kCount - 1);
        float a = angle - kSpread * 0.5f + kSpread * t;

        float speed = kSpeed * (0.7f + 0.3f * t) * GameConstants::kFrameDeltaTime;
        Vector3 vel = { std::cos(a) * speed, std::sin(a) * speed, 0.0f };

        float bright = 1.0f - t * 0.3f;
        Vector4 c    = { color.x, color.y, color.z, color.w * bright };

        GPUParticleState& p = group.particleUploadData[slot];
        p.position    = position;
        p.lifeTime    = kLifeTime;
        p.velocity    = vel;
        p.currentTime = 0.0f;
        p.color       = c;
        p.scale       = { radius * 0.5f, radius * 0.05f, 1.0f };
        p.rotateZ     = a;
        p.alive       = 1;
        p.curveFlag   = 0;

        group.slotExpiry[slot] = group.groupTime + kLifeTime + 0.1f;
        group.pendingSlots.push_back(slot);
    }
}

void ParticleManager::EmitScatterLoop(const std::string& name,
                                      const Vector3& center, float radius,
                                      uint32_t count, const Vector4& color,
                                      float lifeTimeMin, float lifeTimeMax, float scale)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];
    count = (std::min)(count, ParticleGroup::kNumMaxInstance);

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> xzDist(-radius, radius);
    std::uniform_real_distribution<float> yDist(0.0f, 12.0f);
    std::uniform_real_distribution<float> lifeDist(lifeTimeMin, lifeTimeMax);
    std::uniform_real_distribution<float> velXZDist(-0.3f, 0.3f);
    std::uniform_real_distribution<float> velYDist(0.1f, 0.6f);

    memset(group.particleUploadData, 0, sizeof(GPUParticleState) * ParticleGroup::kNumMaxInstance);
    group.slotExpiry.fill(0.0f);
    group.freeList.clear();
    for (uint32_t i = ParticleGroup::kNumMaxInstance; i-- > count; )
        group.freeList.push_back(i);
    group.pendingSlots.clear();

    for (uint32_t i = 0; i < count; ++i) {
        float lifeTime = lifeDist(rng);
        std::uniform_real_distribution<float> timeDist(0.0f, lifeTime);
        float currentTime = timeDist(rng);

        GPUParticleState& p = group.particleUploadData[i];
        p.position    = { center.x + xzDist(rng), center.y + yDist(rng), center.z + xzDist(rng) };
        p.lifeTime    = lifeTime;
        p.velocity    = { velXZDist(rng), velYDist(rng), velXZDist(rng) };
        p.currentTime = currentTime;
        p.color       = color;
        p.scale       = { scale, scale, scale };
        p.rotateZ     = 0.0f;
        p.alive       = 1;
        p.curveFlag   = 0;

        group.slotExpiry[i] = group.groupTime + (lifeTime - currentTime) + 0.1f;
    }

    group.needsInit = true;

    group.autoRespawn       = true;
    group.respawnConfig     = { center, radius, lifeTimeMin, lifeTimeMax, color, scale, count };
}

void ParticleManager::EmitBurst(const std::string& name,
                                const Vector3& position,
                                const Vector4& color,
                                uint32_t count,
                                float lifeTime,
                                float scale,
                                bool flicker)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    count = (std::min)(count, ParticleGroup::kNumMaxInstance);

    // 全スロットをリセット（re-emit 時に前の状態を消す）
    memset(group.particleUploadData, 0,
        sizeof(GPUParticleState) * ParticleGroup::kNumMaxInstance);
    group.slotExpiry.fill(0.0f);
    group.freeList.clear();
    for (uint32_t i = ParticleGroup::kNumMaxInstance; i-- > count; )
        group.freeList.push_back(i);

    for (uint32_t i = 0; i < count; ++i) {
        GPUParticleState& p = group.particleUploadData[i];
        // ランダムにばら撒く（初期化時と同様の範囲）
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> distXZ(-20.0f, 20.0f);
        std::uniform_real_distribution<float> distY(0.0f, 12.0f);
        Vector3 randOffset = { distXZ(rng), distY(rng), distXZ(rng) };

        p.position    = { position.x + randOffset.x, position.y + randOffset.y, position.z + randOffset.z };
        p.lifeTime    = lifeTime;
        p.velocity    = { 0.0f, 0.0f, 0.0f };
        p.currentTime = 0.0f;
        p.color       = color;
        p.scale       = { scale, scale, scale };
        p.rotateZ     = 0.0f;
        p.alive       = 1;
        p.curveFlag   = flicker ? 2u : 0u;
        group.slotExpiry[i] = group.groupTime + lifeTime + 0.1f;
    }

    group.needsInit = true;
    group.pendingSlots.clear();
    for (uint32_t i = 0; i < count; ++i) {
        group.pendingSlots.push_back(i);
    }
}

void ParticleManager::EmitHitStar(const std::string& name,
                                  const Vector3& position,
                                  const Vector4& color)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> rotDist   (-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> scaleYDist(0.15f, 0.7f);
    std::uniform_real_distribution<float> speedDist (0.5f, 2.5f);
    std::uniform_real_distribution<float> lifeDist  (0.2f, 0.5f);

    const int kCount = 8;

    for (int i = 0; i < kCount; ++i) {
        uint32_t slot = AllocateSlot(group);

        if (slot == UINT32_MAX) {
            break;
        }

        float rotAngle = rotDist(engine);
        float velAngle = rotDist(engine);
        float scaleY   = scaleYDist(engine);
        float speed    = speedDist(engine) * GameConstants::kFrameDeltaTime;
        float lifeTime = lifeDist(engine);

        Vector3 vel = { std::cos(velAngle) * speed, std::sin(velAngle) * speed, 0.0f };

        GPUParticleState& p = group.particleUploadData[slot];
        p.position    = position;
        p.lifeTime    = lifeTime;
        p.velocity    = vel;
        p.currentTime = 0.0f;
        p.color       = color;
        p.scale       = { 0.04f, scaleY, 1.0f };
        p.rotateZ     = rotAngle;
        p.alive       = 1;
        p.curveFlag   = 0;

        group.slotExpiry[slot] = group.groupTime + lifeTime + 0.1f;
        group.pendingSlots.push_back(slot);
    }
}

// ============================================================
//  Update: CS ディスパッチ
// ============================================================

void ParticleManager::Update(Camera* camera)
{
    constexpr float dt = GameConstants::kFrameDeltaTime;

    // ---- ビルボード行列とビュープロジェクション行列を計算 ----
    Matrix4x4 billboard  = MakeIdentity4x4();
    Matrix4x4 cameraView = camera->GetViewMatrix();
    billboard.m[0][0] = cameraView.m[0][0];
    billboard.m[0][1] = cameraView.m[1][0];
    billboard.m[0][2] = cameraView.m[2][0];
    billboard.m[1][0] = cameraView.m[0][1];
    billboard.m[1][1] = cameraView.m[1][1];
    billboard.m[1][2] = cameraView.m[2][1];
    billboard.m[2][0] = cameraView.m[0][2];
    billboard.m[2][1] = cameraView.m[1][2];
    billboard.m[2][2] = cameraView.m[2][2];

    Matrix4x4 viewProj = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

    csConstantsData_->billboard    = billboard;
    csConstantsData_->viewProj     = viewProj;
    csConstantsData_->deltaTime    = dt;
    csConstantsData_->maxParticles = ParticleGroup::kNumMaxInstance;

    auto* cmd = dxCommon_->GetCommandList();

    for (auto& [name, group] : particleGroups_) {
        group.groupTime += dt;

        // ---- instancingResource: 前フレームの SRV 状態 → UAV に戻す ----
        if (group.instancingInSRV) {
            D3D12_RESOURCE_BARRIER b{};
            b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = group.instancingResource.Get();
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            b.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmd->ResourceBarrier(1, &b);
            group.instancingInSRV = false;
        }

        // ---- 期限切れスロットを freeList へ返却 ----
        if (!group.autoRespawn) {
            for (uint32_t i = 0; i < ParticleGroup::kNumMaxInstance; ++i) {
                if (group.slotExpiry[i] > 0.0f && group.groupTime >= group.slotExpiry[i]) {
                    group.freeList.push_back(i);
                    group.slotExpiry[i] = 0.0f;
                }
            }
        }

        // ---- 自動再配置: 期限切れスロットを一つずつ再配置 ----
        if (group.autoRespawn) {
            static std::mt19937 rng{ std::random_device{}() };
            const ParticleGroup::RespawnConfig& cfg = group.respawnConfig;
            std::uniform_real_distribution<float> xzDist(-cfg.radius, cfg.radius);
            std::uniform_real_distribution<float> yDist(0.0f, 12.0f);
            std::uniform_real_distribution<float> lifeDist(cfg.lifeTimeMin, cfg.lifeTimeMax);
            std::uniform_real_distribution<float> velXZDist(-0.3f, 0.3f);
            std::uniform_real_distribution<float> velYDist(0.1f, 0.6f);

            for (uint32_t i = 0; i < cfg.count; ++i) {
                if (group.slotExpiry[i] > 0.0f && group.groupTime >= group.slotExpiry[i]) {
                    float lifeTime = lifeDist(rng);
                    GPUParticleState& p = group.particleUploadData[i];
                    p.position    = { cfg.center.x + xzDist(rng),
                                      cfg.center.y + yDist(rng),
                                      cfg.center.z + xzDist(rng) };
                    p.lifeTime    = lifeTime;
                    p.velocity    = { velXZDist(rng), velYDist(rng), velXZDist(rng) };
                    p.currentTime = 0.0f;
                    p.color       = cfg.color;
                    p.scale       = { cfg.scale, cfg.scale, cfg.scale };
                    p.rotateZ     = 0.0f;
                    p.alive       = 1;
                    p.curveFlag   = 0;
                    group.slotExpiry[i] = group.groupTime + lifeTime + 0.1f;
                    group.pendingSlots.push_back(i);
                }
            }
        }

        // ---- 新パーティクルを GPU ステートバッファへコピー ----
        if (group.needsInit || !group.pendingSlots.empty()) {
            D3D12_RESOURCE_BARRIER cb{};
            cb.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            cb.Transition.pResource   = group.particleStateBuffer.Get();
            cb.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            cb.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
            cb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmd->ResourceBarrier(1, &cb);

            if (group.needsInit) {
                UINT64 fullSize = sizeof(GPUParticleState) * ParticleGroup::kNumMaxInstance;
                cmd->CopyBufferRegion(
                    group.particleStateBuffer.Get(), 0,
                    group.particleUploadBuffer.Get(), 0,
                    fullSize);
                group.needsInit = false;
            }

            for (uint32_t slot : group.pendingSlots) {
                UINT64 offset = static_cast<UINT64>(slot) * sizeof(GPUParticleState);
                cmd->CopyBufferRegion(
                    group.particleStateBuffer.Get(), offset,
                    group.particleUploadBuffer.Get(), offset,
                    sizeof(GPUParticleState));
            }

            group.pendingSlots.clear();

            cb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            cb.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            cmd->ResourceBarrier(1, &cb);
        }

        // ---- エミッター: frequency 更新 → emit フラグ管理 ----
        {
            Emitter* e = group.emitterData;
            e->time = *reinterpret_cast<uint32_t*>(&group.groupTime); // float ビット列をそのまま uint で渡す
            e->seed++;
            if (e->frequency > 0.0f) {
                e->frequencyTime += dt;
                if (e->frequency <= e->frequencyTime) {
                    e->frequencyTime -= e->frequency;
                    e->emit = 1;
                } else {
                    e->emit = 0;
                }
            }

            if (e->emit != 0) {
                uint32_t count = (std::min)(e->count, ParticleGroup::kNumMaxInstance);
                float maxExpiry = group.groupTime + e->lifeTime + 0.1f;
                for (uint32_t i = 0; i < count; ++i) {
                    group.slotExpiry[i] = maxExpiry;
                }

                // 発射のたびにステートバッファをゼロ初期化してキャッシュをフラッシュ
                {
                    D3D12_RESOURCE_BARRIER bIn{};
                    bIn.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    bIn.Transition.pResource   = group.particleStateBuffer.Get();
                    bIn.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    bIn.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
                    bIn.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    cmd->ResourceBarrier(1, &bIn);

                    UINT64 fullSize = sizeof(GPUParticleState) * ParticleGroup::kNumMaxInstance;
                    cmd->CopyBufferRegion(group.particleStateBuffer.Get(), 0,
                                         group.particleUploadBuffer.Get(), 0, fullSize);

                    D3D12_RESOURCE_BARRIER bOut{};
                    bOut.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    bOut.Transition.pResource   = group.particleStateBuffer.Get();
                    bOut.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                    bOut.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    bOut.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    cmd->ResourceBarrier(1, &bOut);
                }

                cmd->SetComputeRootSignature(csEmitRootSignature_.Get());
                cmd->SetPipelineState(csEmitPipelineState_.Get());
                cmd->SetComputeRootConstantBufferView(0, group.emitterBuffer->GetGPUVirtualAddress());
                cmd->SetComputeRootUnorderedAccessView(1, group.particleStateBuffer->GetGPUVirtualAddress());
                cmd->Dispatch(1, 1, 1);

                D3D12_RESOURCE_BARRIER emitUavB{};
                emitUavB.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                emitUavB.UAV.pResource = group.particleStateBuffer.Get();
                cmd->ResourceBarrier(1, &emitUavB);

                // one-shot (frequency == 0) はディスパッチ後にリセット
                if (e->frequency == 0.0f) {
                    e->emit = 0;
                }
            }
        }

        // ---- CS ディスパッチ ----
        cmd->SetComputeRootSignature(csRootSignature_.Get());
        cmd->SetPipelineState(csPipelineState_.Get());
        cmd->SetComputeRootConstantBufferView(0, csConstantsBuffer_->GetGPUVirtualAddress());
        cmd->SetComputeRootUnorderedAccessView(1, group.particleStateBuffer->GetGPUVirtualAddress());
        cmd->SetComputeRootUnorderedAccessView(2, group.instancingResource->GetGPUVirtualAddress());

        UINT threadGroups = (ParticleGroup::kNumMaxInstance + 63) / 64;
        cmd->Dispatch(threadGroups, 1, 1);

        // UAV バリア: particleStateBuffer（次フレームの CS 読み込み前に書き込み完了を保証）
        //            instancingResource（VS が SRV として読む前に書き込み完了を保証）
        D3D12_RESOURCE_BARRIER uavBs[2]{};
        uavBs[0].Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBs[0].UAV.pResource = group.particleStateBuffer.Get();
        uavBs[1].Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBs[1].UAV.pResource = group.instancingResource.Get();
        cmd->ResourceBarrier(2, uavBs);

        // instancingResource: UAV → NON_PIXEL_SHADER_RESOURCE (VS が SRV として読む)
        D3D12_RESOURCE_BARRIER srvB{};
        srvB.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        srvB.Transition.pResource   = group.instancingResource.Get();
        srvB.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        srvB.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        srvB.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &srvB);
        group.instancingInSRV = true;
    }
}

// ============================================================
//  Draw: インスタンシング描画
// ============================================================

void ParticleManager::Draw(Camera* camera)
{
    (void)camera;

    auto* cmd = dxCommon_->GetCommandList();

    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &quadVBV_);
    cmd->IASetIndexBuffer(&quadIBV_);

    SrvManager::GetInstance()->PreDraw();

    ID3D12PipelineState* activePSO = nullptr;

    for (auto& [name, group] : particleGroups_) {
        bool hasAlive = false;

        for (uint32_t i = 0; i < ParticleGroup::kNumMaxInstance; ++i) {
            if (group.groupTime < group.slotExpiry[i]) {
                hasAlive = true;
                break;
            }
        }

        if (!hasAlive) {
            continue;
        }

        ID3D12PipelineState* pso = group.additiveBlend
            ? graphicsPipelineState_.Get()
            : graphicsPipelineStateAlpha_.Get();
        if (pso != activePSO) {
            cmd->SetPipelineState(pso);
            activePSO = pso;
        }

        cmd->SetGraphicsRootDescriptorTable(
            2, SrvManager::GetInstance()->GetGPUDescriptorHandle(group.srvIndex));

        D3D12_GPU_DESCRIPTOR_HANDLE texH =
            TextureManager::GetInstance()->GetSrvHandleGPU(group.textureFilePath);
        cmd->SetGraphicsRootDescriptorTable(4, texH);

        cmd->DrawIndexedInstanced(6, ParticleGroup::kNumMaxInstance, 0, 0, 0);
    }
}

// ============================================================
//  テクスチャ変更
// ============================================================

void ParticleManager::SetTexture(const std::string& groupName,
                                 const std::string& textureFilePath)
{
    if (!particleGroups_.contains(groupName)) {
        return;
    }

    ParticleGroup& group  = particleGroups_[groupName];
    group.textureFilePath = textureFilePath;
    TextureManager::GetInstance()->LoadTexture(textureFilePath);
}

void ParticleManager::SetAdditiveBlend(const std::string& name, bool additive)
{
    if (particleGroups_.contains(name)) {
        particleGroups_[name].additiveBlend = additive;
    }
}

bool ParticleManager::IsGroupAlive(const std::string& name) const
{
    auto it = particleGroups_.find(name);
    if (it == particleGroups_.end()) { return false; }
    const ParticleGroup& group = it->second;
    for (uint32_t i = 0; i < ParticleGroup::kNumMaxInstance; ++i) {
        if (group.groupTime < group.slotExpiry[i]) { return true; }
    }
    return false;
}

// ============================================================
//  グラフィックスルートシグネチャ
// ============================================================

void ParticleManager::CreateRootSignature()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    D3D12_DESCRIPTOR_RANGE rangeT0[1]{};
    rangeT0[0].BaseShaderRegister                = 0;
    rangeT0[0].NumDescriptors                    = 1;
    rangeT0[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE rangeT1[1]{};
    rangeT1[0].BaseShaderRegister                = 1;
    rangeT1[0].NumDescriptors                    = 1;
    rangeT1[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeT1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[5]{};
    rootParameters[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility           = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister  = 0;
    rootParameters[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility           = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister  = 0;
    rootParameters[2].ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility                     = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[2].DescriptorTable.pDescriptorRanges   = rangeT0;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility           = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister  = 1;
    rootParameters[4].ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].ShaderVisibility                     = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].DescriptorTable.pDescriptorRanges   = rangeT1;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_STATIC_SAMPLER_DESC samplers[2]{};
    samplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW
                         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].MaxLOD           = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister   = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    samplers[1].Filter           = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW
                         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].MaxLOD           = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister   = 1;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    desc.pParameters       = rootParameters;
    desc.NumParameters     = _countof(rootParameters);
    desc.pStaticSamplers   = samplers;
    desc.NumStaticSamplers = _countof(samplers);

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
                                sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
}

// ============================================================
//  CS ルートシグネチャ
// ============================================================

void ParticleManager::CreateCSRootSignature()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0; // b0
    params[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[1].Descriptor.ShaderRegister = 0; // u0
    params[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[2].Descriptor.ShaderRegister = 1; // u1

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    desc.pParameters   = params;
    desc.NumParameters = _countof(params);

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
                                sigBlob->GetBufferSize(), IID_PPV_ARGS(&csRootSignature_));
}

// ============================================================
//  CS パイプラインステート
// ============================================================

void ParticleManager::CreateQuadGeometry()
{
    struct Vertex {
        float pos[4];
        float uv[2];
        float nrm[3];
    };

    Vertex verts[4] = {
        { {-0.5f,  0.5f, 0.f, 1.f}, {0.f, 0.f}, {0.f, 0.f, -1.f} },
        { { 0.5f,  0.5f, 0.f, 1.f}, {1.f, 0.f}, {0.f, 0.f, -1.f} },
        { { 0.5f, -0.5f, 0.f, 1.f}, {1.f, 1.f}, {0.f, 0.f, -1.f} },
        { {-0.5f, -0.5f, 0.f, 1.f}, {0.f, 1.f}, {0.f, 0.f, -1.f} },
    };
    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    quadVertexBuffer_ = dxCommon_->CreateBufferResource(sizeof(verts));
    void* mapped = nullptr;
    quadVertexBuffer_->Map(0, nullptr, &mapped);
    memcpy(mapped, verts, sizeof(verts));
    quadVertexBuffer_->Unmap(0, nullptr);

    quadVBV_.BufferLocation = quadVertexBuffer_->GetGPUVirtualAddress();
    quadVBV_.SizeInBytes    = sizeof(verts);
    quadVBV_.StrideInBytes  = sizeof(Vertex);

    quadIndexBuffer_ = dxCommon_->CreateBufferResource(sizeof(indices));
    quadIndexBuffer_->Map(0, nullptr, &mapped);
    memcpy(mapped, indices, sizeof(indices));
    quadIndexBuffer_->Unmap(0, nullptr);

    quadIBV_.BufferLocation = quadIndexBuffer_->GetGPUVirtualAddress();
    quadIBV_.SizeInBytes    = sizeof(indices);
    quadIBV_.Format         = DXGI_FORMAT_R32_UINT;
}

void ParticleManager::CreateCSPipelineState()
{
    IDxcBlob* csBlob = dxCommon_->CompileShader(
        L"Resources/shaders/Particle/ParticleUpdate.CS.hlsl", L"cs_6_0");
    assert(csBlob);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = csRootSignature_.Get();
    psoDesc.CS             = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    HRESULT hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &psoDesc, IID_PPV_ARGS(&csPipelineState_));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  EmitParticle CS ルートシグネチャ / パイプライン
// ============================================================

void ParticleManager::CreateCSEmitRootSignature()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0; // b0 : EmitConstants
    params[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[1].Descriptor.ShaderRegister = 0; // u0 : gParticles

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    desc.pParameters   = params;
    desc.NumParameters = _countof(params);

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
                                sigBlob->GetBufferSize(), IID_PPV_ARGS(&csEmitRootSignature_));
}

void ParticleManager::CreateCSEmitPipelineState()
{
    IDxcBlob* csBlob = dxCommon_->CompileShader(
        L"Resources/shaders/Particle/EmitParticle.CS.hlsl", L"cs_6_0");
    assert(csBlob);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = csEmitRootSignature_.Get();
    psoDesc.CS             = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    HRESULT hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &psoDesc, IID_PPV_ARGS(&csEmitPipelineState_));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  GetEmitter
// ============================================================

Emitter* ParticleManager::GetEmitter(const std::string& name)
{
    assert(particleGroups_.contains(name));
    return particleGroups_[name].emitterData;
}

// ============================================================
//  グラフィックスパイプラインステート
// ============================================================

void ParticleManager::CreatePipelineState()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    IDxcBlob* vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/Particle/Particle.VS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/Particle/Particle.PS.hlsl", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout    = { inputLayout, _countof(inputLayout) };
    psoDesc.VS             = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS             = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState.RenderTarget[0].BlendEnable           = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
    // RT のアルファをパーティクルのフェードアウトで上書きしない（黒くなる原因の修正）
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;

    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.DepthStencilState.DepthEnable    = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    psoDesc.DSVFormat             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask            = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.SampleDesc.Count      = 1;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
    assert(SUCCEEDED(hr));

    // Alpha blend variant: DestBlend を INV_SRC_ALPHA に変えるだけ
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineStateAlpha_));
    assert(SUCCEEDED(hr));
}
