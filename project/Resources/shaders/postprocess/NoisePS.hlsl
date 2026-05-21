cbuffer NoiseParams : register(b0)
{
    float2 scale;       // UV スケール（ノイズの細かさ）
    float  seed;        // シード（CPU が毎フレーム更新してアニメーション）
    int    octaves;     // オクターブ数
    float  persistence; // 振幅減衰率
    float  lacunarity;  // 周波数倍率
    int    colorMode;   // 0=グレー, 1=カラー
    float  opacity;     // ノイズ不透明度（0=シーンのみ, 1=ノイズのみ）
};

Texture2D<float4> gTexture : register(t0); // シーン（背景として透過合成）
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

float hash21(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float valueNoise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    float2 u = f * f * (3.0 - 2.0 * f);

    float a = hash21(i + float2(0.0, 0.0));
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm(float2 uv, int oct, float pers, float lacu)
{
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float norm      = 0.0;

    for (int i = 0; i < oct; ++i)
    {
        value     += valueNoise(uv * frequency) * amplitude;
        norm      += amplitude;
        amplitude *= pers;
        frequency *= lacu;
    }
    return value / norm;
}

float4 main(PSInput input) : SV_Target
{
    float4 scene = gTexture.Sample(gSampler, input.uv);
    float2 uv    = input.uv * scale + seed;

    float4 noiseColor;
    if (colorMode == 0)
    {
        float n = fbm(uv, octaves, persistence, lacunarity);
        noiseColor = float4(n, n, n, 1.0);
    }
    else
    {
        float r = fbm(uv + float2(  0.0,  0.0), octaves, persistence, lacunarity);
        float g = fbm(uv + float2(100.3, 17.5), octaves, persistence, lacunarity);
        float b = fbm(uv + float2(200.7, 53.1), octaves, persistence, lacunarity);
        noiseColor = float4(r, g, b, 1.0);
    }

    return lerp(scene, noiseColor, opacity);
}
