struct PSInput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

cbuffer RadialBlurParams : register(b0) {
    float2 center;      // ブラー中心（UV空間 0～1、デフォルト 0.5, 0.5）
    float  strength;    // ブラー強度（大きいほど広範囲にぼける）
    int    sampleCount; // サンプリング回数（多いほど滑らかだが重い）
};

Texture2D<float4> gTexture : register(t0);
SamplerState      gSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float2 uv  = input.uv;
    float2 dir = uv - center; // 中心からの方向ベクトル

    float4 result = 0.0;
    float  denom  = max(float(sampleCount - 1), 1.0); // ゼロ除算防止

    // 現在の UV から中心方向へ向かって等間隔にサンプリングして加算平均
    [loop]
    for (int i = 0; i < sampleCount; ++i) {
        float  t        = float(i) / denom;
        float2 sampleUV = uv - dir * (strength * t);
        result += gTexture.Sample(gSampler, sampleUV);
    }
    return result / float(sampleCount);
}
