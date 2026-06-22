// ParticleUpdate.CS.hlsl
// パーティクルの物理演算と WVP 行列計算を GPU 上で行う Compute Shader

struct GPUParticleState
{
    float3 position; // 12
    float lifeTime; //  4 -> 16
    float3 velocity; // 12
    float currentTime; //  4 -> 32
    float4 color; // 16 -> 48
    float3 scale; // 12
    float rotateZ; //  4 -> 64
    uint alive; //  4
    uint curveFlag; //  4  (1 = enemyDeath 螺旋回転, 2 = flicker)
    float2 pad; //  8 -> 80
};

struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
};

cbuffer UpdateConstants : register(b0)
{
    float4x4 gBillboard; // 64
    float4x4 gViewProj; // 64
    float gDeltaTime; //  4
    uint gMaxParticles; //  4
    float2 gPad; //  8 -> 144
};

RWStructuredBuffer<GPUParticleState> gParticles : register(u0);
RWStructuredBuffer<ParticleForGPU> gInstancing : register(u1);

// -------------------------------------------------------
//  xoroshiro128+
// -------------------------------------------------------
uint rotl32u(uint x, int k)
{
    return (x << k) | (x >> (32 - k));
}

uint4 xoshiroSeed(uint seed, uint time, uint idx)
{
    uint a = seed ^ (idx * 2654435761u);
    uint b = time ^ (idx * 2246822519u);
    a ^= a >> 16;
    a *= 0x45d9f3bu;
    a ^= a >> 16;
    b ^= b >> 16;
    b *= 0x45d9f3bu;
    b ^= b >> 16;
    return uint4(a, b, a ^ 0x9e3779b9u, b ^ 0x6c62272eu);
}

uint xoshiroNext(inout uint4 s)
{
    uint result = s.x + s.w;
    uint t = s.y << 9;
    s.z ^= s.x;
    s.w ^= s.y;
    s.y ^= s.z;
    s.x ^= s.w;
    s.z ^= t;
    s.w = rotl32u(s.w, 11);
    return result;
}

float rand01(inout uint4 s)
{
    return float(xoshiroNext(s) >> 8) * (1.0f / 16777216.0f);
}

// -------------------------------------------------------
//  行列ヘルパー
// -------------------------------------------------------
float4x4 MakeScale(float3 s)
{
    return float4x4(
        s.x, 0, 0, 0,
        0, s.y, 0, 0,
        0, 0, s.z, 0,
        0, 0, 0, 1
    );
}

float4x4 MakeRotateZ(float a)
{
    float c = cos(a), s = sin(a);
    return float4x4(
        c, s, 0, 0,
        -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}

// -------------------------------------------------------
//  groupshared: スレッドグループ内の生存パーティクル数
//  GroupMemoryBarrierWithGroupSync を使うには全スレッドが
//  必ず同じ回数バリアを通過しなければならないため、
//  early return を避けたフラグベースの構造にしている
// -------------------------------------------------------
groupshared uint gs_aliveCount;

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    // --- Sync 1: 共有変数をスレッド 0 が初期化し、全スレッドに同期 ---
    if (GTid.x == 0)
    {
        gs_aliveCount = 0u;
    }
    
    GroupMemoryBarrierWithGroupSync();

    uint idx = DTid.x;
    bool active = (idx < gMaxParticles);

    GPUParticleState p = (GPUParticleState) 0;
    bool alive = false;

    if (active)
    {
        p = gParticles[idx];
        alive = (p.alive != 0 && p.currentTime < p.lifeTime);

        if (!alive)
        {
            p.alive = 0;
            gParticles[idx] = p;
            gInstancing[idx].color = float4(0, 0, 0, 0);
        }
    }

    // --- 不可分操作: 生存パーティクルをグループ単位でカウント ---
    if (alive)
    {
        InterlockedAdd(gs_aliveCount, 1u);
    }

    // --- Sync 2: 全スレッドの InterlockedAdd 完了を待つ ---
    GroupMemoryBarrierWithGroupSync();

    // gs_aliveCount 確定（このスレッドグループ内の生存数）
    // 現状は使用しないが、将来的な LOD / カリング等に活用可能

    // dead / 範囲外スレッドはここで終了（2 つのバリアはすでに通過済み）
    if (!active || !alive)
    {
        return;
    }

    // --- 生存パーティクルのみ以下を処理 ---

    // curveFlag == 1: enemyDeath 螺旋軌道
    if (p.curveFlag == 1)
    {
        float ca = 5.0f * gDeltaTime;
        float cc = cos(ca), cs = sin(ca);
        float vx = p.velocity.x;
        float vy = p.velocity.y;
        p.velocity.x = vx * cc - vy * cs;
        p.velocity.y = vx * cs + vy * cc;
        p.rotateZ += 12.0f * gDeltaTime;
    }

    p.position += p.velocity * gDeltaTime;
    p.currentTime += gDeltaTime;
    gParticles[idx] = p;

    // ワールド行列: scale * rotateZ * billboard、row3 に平行移動を直接書く
    float4x4 world = mul(mul(MakeScale(p.scale), MakeRotateZ(p.rotateZ)), gBillboard);
    world._m30 = p.position.x;
    world._m31 = p.position.y;
    world._m32 = p.position.z;
    world._m33 = 1.0f;

    gInstancing[idx].WVP = mul(world, gViewProj);
    gInstancing[idx].World = world;

    // curveFlag == 2: xoroshiro128+ でパーティクルごとに独立した周期でランダム点滅
    if (p.curveFlag == 2)
    {
        float flickerHz = 2.0f + float(idx % 7u) * 0.857142857f; // 2〜8 Hz でばらつかせる
        uint timeSlot = (uint) (p.currentTime * flickerHz);
        uint4 rng = xoshiroSeed(timeSlot, idx * 3141592653u, idx);
        float r = rand01(rng);
        gInstancing[idx].color = float4(p.color.rgb, p.color.a * (r < 0.75f ? 1.0f : 0.0f));
    }
    else
    {
        float alpha = 1.0f - (p.currentTime / p.lifeTime);
        gInstancing[idx].color = float4(p.color.rgb, p.color.a * alpha);
    }
}
