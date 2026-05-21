struct PSInput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

cbuffer FilterParams : register(b0) {
    float2 texelSize;   // (1/width, 1/height)
    int    radius;      // 1D kernel radius
    float  pad0;
    float4 kernel[5];  // pre-computed 1D weights, packed (max 2*8+1 = 17 taps)
    float2 direction;  // (1,0)=horizontal pass, (0,1)=vertical pass
    float2 pad1;
};

Texture2D<float4> gTexture : register(t0);
SamplerState      gSampler : register(s0);

// float4 配列から任意インデックスの float を取り出す
float kernelAt(int i)
{
    float4 v = kernel[i >> 2];
    int    c = i & 3;
    if (c == 0) return v.x;
    if (c == 1) return v.y;
    if (c == 2) return v.z;
    return v.w;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 result = 0.0;
    [loop]
    for (int k = -radius; k <= radius; ++k) {
        float2 offset = float2(k, k) * direction * texelSize;
        result += gTexture.Sample(gSampler, input.uv + offset) * kernelAt(k + radius);
    }
    return result;
}
