#include "ParticleManager.h"
#include "TextureManager.h"
#include <algorithm>
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

void ParticleManager::Initialize(DirectXCommon* dxCommon)
{
    assert(dxCommon);
    dxCommon_ = dxCommon;

    CreateRootSignature();
    CreatePipelineState();
}

void ParticleManager::Finalize()
{
    particleGroups_.clear();
    graphicsPipelineState_.Reset();
    rootSignature_.Reset();
    dxCommon_ = nullptr;
}

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& textureFilePath)
{
    // 登録済みの名前かチェックしてassert
    assert(!particleGroups_.contains(name));

    // 新たな空のパーティクルグループを作成し、コンテナに登録
    ParticleGroup& group = particleGroups_[name];

    // マテリアルデータにテクスチャファイルパスを設定
    group.textureFilePath = textureFilePath;

    // テクスチャを読み込む
    TextureManager::GetInstance()->LoadTexture(textureFilePath);

    // インスタンシング用リソースの生成
    ID3D12Device* device = dxCommon_->GetDevice();
    size_t sizeInBytes = sizeof(ParticleForGPU) * group.kNumMaxInstance;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeInBytes;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&group.instancingResource));
    assert(SUCCEEDED(hr));

    // リソースをマッピング
    group.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&group.instancingData));

    // インスタンシング用にSRVを確保してSRVインデックスを記録
    group.srvIndex = SrvManager::GetInstance()->Allocate();
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = SrvManager::GetInstance()->GetCPUDescriptorHandle(group.srvIndex);

    // SRV生成 (StructuredBuffer用設定)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = group.kNumMaxInstance;
    srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(group.instancingResource.Get(), &srvDesc, cpuHandle);
}

void ParticleManager::Emit(const std::string& name, const Vector3& position, const Vector3& velocity)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    if (group.particles.size() >= group.kNumMaxInstance) {
        return;
    }

    // 新しいパーティクルを生成
    Particle newParticle;
    newParticle.transform.scale = { 1.0f, 1.0f, 1.0f };
    newParticle.transform.rotate = { 0.0f, 0.0f, 0.0f };
    newParticle.transform.translate = position;
    newParticle.velocity = velocity;
    newParticle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    newParticle.lifeTime = 1.0f; // 1秒生存
    newParticle.currentTime = 0.0f;

    // 指定グループのリストに追加
    particleGroups_[name].particles.push_back(newParticle);
}

void ParticleManager::EmitWithColor(const std::string& name, const Vector3& position,
    const Vector3& velocity, const Vector4& color,
    float lifeTime, float scale) // scaleを追加
{
    assert(particleGroups_.contains(name));

    Particle newParticle;
    newParticle.transform.scale = { scale, scale, scale }; // 引数の値をセット
    newParticle.transform.rotate = { 0.0f, 0.0f, 0.0f };
    newParticle.transform.translate = position;
    newParticle.velocity = velocity;
    newParticle.color = color;
    newParticle.lifeTime = lifeTime;
    newParticle.currentTime = 0.0f;

    particleGroups_[name].particles.push_back(newParticle);
}

void ParticleManager::EmitEllipse(const std::string& name, const Vector3& position,
    const Vector3& velocity, const Vector4& color,
    float lifeTime, float scaleX, float scaleY)
{
    assert(particleGroups_.contains(name));

    ParticleGroup& group = particleGroups_[name];
    if (group.particles.size() >= group.kNumMaxInstance) {
        return;
    }

    Particle newParticle;
    newParticle.transform.scale     = { scaleX, scaleY, 1.0f }; // 非均一スケールで楕円形に
    newParticle.transform.rotate    = { 0.0f, 0.0f, 0.0f };
    newParticle.transform.translate = position;
    newParticle.velocity            = velocity;
    newParticle.color               = color;
    newParticle.lifeTime            = lifeTime;
    newParticle.currentTime         = 0.0f;

    group.particles.push_back(newParticle);
}

void ParticleManager::EmitSlash(const std::string& name, const Vector3& position,
    float angle, const Vector4& color, float radius)
{
    // ヒットエフェクト：中心から斬撃線が放射状に飛び散る
    // angle を基準に ±60° の範囲に集中させて「斬られた方向感」を出す
    const int   kCount    = 6;
    const float kSpread   = 2.0f;   // 広がり角度（ラジアン全幅）
    const float kSpeed    = 6.0f;   // 飛散速度（単位/秒）
    const float kLifeTime = 0.15f;  // 非常に短命でパンチを出す

    for (int i = 0; i < kCount; ++i) {
        ParticleGroup& group = particleGroups_[name];
        if (group.particles.size() >= group.kNumMaxInstance) { break; }

        float t = static_cast<float>(i) / static_cast<float>(kCount - 1); // 0 → 1

        // angle を中心に kSpread の範囲で均等に散らす
        float a = angle - kSpread * 0.5f + kSpread * t;

        // 速度：各ラインの向きへ勢いよく飛ばす
        float speed = kSpeed * (0.7f + 0.3f * t) * (1.0f / 60.0f);
        Vector3 vel = { std::cos(a) * speed, std::sin(a) * speed, 0.0f };

        // 内側ほど明るい（中心が白く光るイメージ）
        float bright = 1.0f - t * 0.3f;
        Vector4 c = { color.x, color.y, color.z, color.w * bright };

        Particle p;
        p.transform.scale     = { radius * 0.5f, radius * 0.05f, 1.0f }; // 細い線
        p.transform.rotate    = { 0.0f, 0.0f, a };                        // 飛散方向に向ける
        p.transform.translate = position;                                  // 全部同じ中心から出る
        p.velocity            = vel;
        p.color               = c;
        p.lifeTime            = kLifeTime;
        p.currentTime         = 0.0f;

        group.particles.push_back(p);
    }
}

void ParticleManager::EmitHitStar(const std::string& name, const Vector3& position, const Vector4& color)
{
    assert(particleGroups_.contains(name));
    ParticleGroup& group = particleGroups_[name];

    // default_random_engine を static で保持し、起動時に乱数シードを設定する
    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> rotDist(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> scaleYDist(0.15f, 0.7f);   // 線香花火らしい短めの火花
    std::uniform_real_distribution<float> speedDist(0.5f, 2.5f);     // 速さにばらつき
    std::uniform_real_distribution<float> lifeDist(0.2f, 0.5f);      // 寿命にばらつき

    const int kCount = 8;

    for (int i = 0; i < kCount; ++i) {
        if (group.particles.size() >= group.kNumMaxInstance) { break; }

        // Z回転（視覚的な向き）と速度方向は独立させる ＝ 線香花火の火花がバラバラに散る
        float rotAngle = rotDist(engine);
        float velAngle = rotDist(engine);
        float scaleY   = scaleYDist(engine);
        float speed    = speedDist(engine) * (1.0f / 60.0f);
        float lifeTime = lifeDist(engine);

        Vector3 vel = { std::cos(velAngle) * speed, std::sin(velAngle) * speed, 0.0f };

        Particle p;
        p.transform.scale     = { 0.04f, scaleY, 1.0f }; // X固定・Y乱数で楕円形
        p.transform.rotate    = { 0.0f, 0.0f, rotAngle }; // Z軸ランダム（速度と独立）
        p.transform.translate = position;
        p.velocity            = vel;
        p.color               = color;
        p.lifeTime            = lifeTime;
        p.currentTime         = 0.0f;

        group.particles.push_back(p);
    }
}

void ParticleManager::CreateRootSignature()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    // t0 : スロット[2] — VS の StructuredBuffer<ParticleForGPU>
    D3D12_DESCRIPTOR_RANGE rangeT0[1] = {};
    rangeT0[0].BaseShaderRegister = 0; // t0
    rangeT0[0].NumDescriptors = 1;
    rangeT0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // t1 : スロット[4] — PS の Texture2D
    D3D12_DESCRIPTOR_RANGE rangeT1[1] = {};
    rangeT1[0].BaseShaderRegister = 1; // t1
    rangeT1[0].NumDescriptors = 1;
    rangeT1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeT1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // 3D/Sprite RS と同じ 5 パラメータレイアウト
    // [0]=CBV(b0,PS)  [1]=CBV(b0,VS)  [2]=DT(t0)  [3]=CBV(b1,PS)  [4]=DT(t1)
    D3D12_ROOT_PARAMETER rootParameters[5] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0 (Material — パーティクルシェーダーは未使用)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0; // b0 (Transform — パーティクルVSは未使用)
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = rangeT0;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1; // t0 : instancing SRV
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1; // b1 (Light — パーティクルPSは未使用)
    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].DescriptorTable.pDescriptorRanges = rangeT1;
    rootParameters[4].DescriptorTable.NumDescriptorRanges = 1; // t1 : texture SRV

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // 静的サンプラー（3D RS と同じ 2 つ）
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1; // s1
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr)) {
        assert(false);
    }

    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void ParticleManager::Update(Camera* camera)
{
    // ビルボード行列の計算
    Matrix4x4 billboardMatrix = MakeIdentity4x4();
    Matrix4x4 cameraView = camera->GetViewMatrix();
    // カメラの回転成分のみを抽出
    billboardMatrix.m[0][0] = cameraView.m[0][0];
    billboardMatrix.m[0][1] = cameraView.m[1][0];
    billboardMatrix.m[0][2] = cameraView.m[2][0];
    billboardMatrix.m[1][0] = cameraView.m[0][1];
    billboardMatrix.m[1][1] = cameraView.m[1][1];
    billboardMatrix.m[1][2] = cameraView.m[2][1];
    billboardMatrix.m[2][0] = cameraView.m[0][2];
    billboardMatrix.m[2][1] = cameraView.m[1][2];
    billboardMatrix.m[2][2] = cameraView.m[2][2];

    // ビュー行列とプロジェクション行列をカメラから取得
    Matrix4x4 viewProj = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());

    // 全てのパーティクルグループについて処理する
    for (auto& [name, group] : particleGroups_) {
        uint32_t instanceCount = 0;

        // グループ内の全てのパーティクルについて処理する
        for (auto it = group.particles.begin(); it != group.particles.end();) {

            if (it->currentTime >= it->lifeTime) {
                it = group.particles.erase(it);
                continue;
            }

            // enemyDeathグループの時のみ軌道を曲げる処理を行う
            if (name == "enemyDeath") {
                // 速度ベクトルを毎フレーム反時計回りに回転させて軌道を曲げる
                // 角度の数値を大きくするほど渦がキツくなります
                float curveAngle = 5.0f * (1.0f / 60.0f);
                float cosAngle = std::cosf(curveAngle);
                float sinAngle = std::sinf(curveAngle);
                float currentVx = it->velocity.x;
                float currentVy = it->velocity.y;

                it->velocity.x = currentVx * cosAngle - currentVy * sinAngle;
                it->velocity.y = currentVx * sinAngle + currentVy * cosAngle;
            }

            // 移動処理
            it->transform.translate.x += it->velocity.x * (1.0f / 60.0f);
            it->transform.translate.y += it->velocity.y * (1.0f / 60.0f);
            it->transform.translate.z += it->velocity.z * (1.0f / 60.0f);

            // 経過時間を加算
            it->currentTime += 1.0f / 60.0f;

            // インスタンシング用データの書き込み
            if (instanceCount < group.kNumMaxInstance) {
                // ワールド行列を計算
                Matrix4x4 scaleMatrix = MakeScaleMatrix(it->transform.scale);
                Matrix4x4 rotateMatrix = MakeRotateZMatrix(it->transform.rotate.z);
                Matrix4x4 translateMatrix = MakeTranslateMatrix(it->transform.translate);

                // ワールド行列
                Matrix4x4 worldMatrix = Multiply(scaleMatrix, rotateMatrix); // まず自転
                worldMatrix = Multiply(worldMatrix, billboardMatrix); // 次にビルボード
                worldMatrix = Multiply(worldMatrix, translateMatrix); // 最後に移動

                // ワールドビュープロジェクション行列を合成
                Matrix4x4 WVP = Multiply(worldMatrix, viewProj);

                group.instancingData[instanceCount].WVP = WVP;
                group.instancingData[instanceCount].World = worldMatrix;

                // enemyDeathグループの時のみ自転させる
                if (name == "enemyDeath") {
                    it->transform.rotate.z += 12.0f * (1.0f / 60.0f);
                }

                float alpha = 1.0f - (it->currentTime / it->lifeTime);
                group.instancingData[instanceCount].color = { it->color.x, it->color.y, it->color.z, alpha };

                instanceCount++;
            }

            ++it;
        }

        for (uint32_t i = instanceCount; i < group.kNumMaxInstance; ++i) {
            group.instancingData[i].WVP = MakeIdentity4x4(); // 行列を初期化
            group.instancingData[i].color.w = 0.0f; // 透明にする
        }
    }
}

void ParticleManager::Draw(Camera* camera)
{
    if (!model_) {
        return;
    }

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(graphicsPipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv = model_->GetVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv = model_->GetIndexBufferView();
    commandList->IASetIndexBuffer(&ibv);

    // SrvManagerのヒープをセット
    SrvManager::GetInstance()->PreDraw();

    // 全てのパーティクルグループについて処理する
    for (auto& [name, group] : particleGroups_) {
        UINT drawCount = (UINT)(std::min)((size_t)group.kNumMaxInstance, group.particles.size());
        if (drawCount == 0) {
            continue;
        }

        commandList->SetGraphicsRootDescriptorTable(2, SrvManager::GetInstance()->GetGPUDescriptorHandle(group.srvIndex));

        D3D12_GPU_DESCRIPTOR_HANDLE textureH = TextureManager::GetInstance()->GetSrvHandleGPU(group.textureFilePath);
        commandList->SetGraphicsRootDescriptorTable(4, textureH);
        commandList->DrawIndexedInstanced((UINT)model_->GetIndexCount(), drawCount, 0, 0, 0);
    }
}

void ParticleManager::CreatePipelineState()
{
    ID3D12Device* device = dxCommon_->GetDevice();

    // シェーダーコンパイル
    IDxcBlob* vsBlob = dxCommon_->CompileShader(L"Resources/shaders//Particle/Particle.VS.hlsl", L"vs_6_0");
    IDxcBlob* psBlob = dxCommon_->CompileShader(L"Resources/shaders//Particle/Particle.PS.hlsl", L"ps_6_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    // ブレンドステート
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // 加算ブレンド
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 両面描画
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.SampleDesc.Count = 1;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
    assert(SUCCEEDED(hr));
}

void ParticleManager::SetTexture(const std::string& groupName, const std::string& textureFilePath)
{
    // グループが存在するか確認
    if (particleGroups_.contains(groupName)) {
        ParticleGroup& group = particleGroups_[groupName];

        // パスを更新
        group.textureFilePath = textureFilePath;

        // テクスチャが未読み込みならロード
        TextureManager::GetInstance()->LoadTexture(textureFilePath);
    }
}