// ParticleUpdate.CS.hlsl
// パーティクルの物理演算と WVP 行列計算を GPU 上で行う Compute Shader

struct GPUParticleState
{
    float3 position;    // 12
    float  lifeTime;    //  4 -> 16
    float3 velocity;    // 12
    float  currentTime; //  4 -> 32
    float4 color;       // 16 -> 48
    float3 scale;       // 12
    float  rotateZ;     //  4 -> 64
    uint   alive;       //  4
    uint   curveFlag;   //  4  (1 = enemyDeath 螺旋回転)
    float2 pad;         //  8 -> 80
};

struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4   color;
};

cbuffer UpdateConstants : register(b0)
{
    float4x4 gBillboard;    // 64
    float4x4 gViewProj;     // 64
    float    gDeltaTime;    //  4
    uint     gMaxParticles; //  4
    float2   gPad;          //  8 -> 144
};

RWStructuredBuffer<GPUParticleState> gParticles  : register(u0);
RWStructuredBuffer<ParticleForGPU>   gInstancing : register(u1);

float4x4 MakeScale(float3 s)
{
    return float4x4(
        s.x, 0,   0,   0,
        0,   s.y, 0,   0,
        0,   0,   s.z, 0,
        0,   0,   0,   1
    );
}

float4x4 MakeRotateZ(float a)
{
    float c = cos(a), s = sin(a);
    return float4x4(
        c,  s, 0, 0,
        -s, c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    );
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.x;

    if (idx >= gMaxParticles)
    {
        return;
    }

    GPUParticleState p = gParticles[idx];

    if (p.alive == 0 || p.currentTime >= p.lifeTime)
    {
        p.alive = 0;
        gParticles[idx] = p;
        gInstancing[idx].color = float4(0, 0, 0, 0);
        return;
    }

    // enemyDeath: 速度を毎フレーム反時計回りに回転させて螺旋軌道
    if (p.curveFlag != 0)
    {
        float ca  = 5.0f * gDeltaTime;
        float cc  = cos(ca), cs = sin(ca);
        float vx  = p.velocity.x;
        float vy  = p.velocity.y;
        p.velocity.x = vx * cc - vy * cs;
        p.velocity.y = vx * cs + vy * cc;
        p.rotateZ   += 12.0f * gDeltaTime;
    }

    p.position    += p.velocity * gDeltaTime;
    p.currentTime += gDeltaTime;

    gParticles[idx] = p;

    // ワールド行列: scale * rotateZ * billboard、row3 に平行移動を直接書く
    float4x4 world = mul(mul(MakeScale(p.scale), MakeRotateZ(p.rotateZ)), gBillboard);
    world._m30 = p.position.x;
    world._m31 = p.position.y;
    world._m32 = p.position.z;
    world._m33 = 1.0f;

    float alpha = 1.0f - (p.currentTime / p.lifeTime);

    gInstancing[idx].WVP   = mul(world, gViewProj);
    gInstancing[idx].World = world;
    gInstancing[idx].color = float4(p.color.rgb, p.color.a * alpha);
}
