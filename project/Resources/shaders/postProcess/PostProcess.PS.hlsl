Texture2D<float4> sceneTex : register(t0);
Texture2D<float>  depthTex : register(t1);

SamplerState linearSamp : register(s0);
SamplerState pointSamp  : register(s1);

cbuffer PostProcessCB : register(b0)
{
    float grayscaleIntensity;     // [0,1]  グレースケール強度
    float vignetteIntensity;      // [0,1]  ビネット強度
    float vignetteRadius;         // [0,1]  ビネット開始半径
    float vignetteSoftness;       // [0,1]  ビネットぼかし幅

    float boxFilterIntensity;     // [0,1]  ボックスフィルタ強度
    float gaussianIntensity;      // [0,1]  ガウスフィルタ強度
    float lumOutlineIntensity;    // [0,1]  輝度アウトライン強度
    float lumOutlineThreshold;    // [0,1]  輝度アウトライン閾値

    float depthOutlineIntensity;  // [0,1]  深度アウトライン強度
    float depthOutlineThreshold;  // [0,1]  深度アウトライン閾値
    float radialBlurIntensity;    // [0,1]  ラジアルブラー強度
    float radialBlurWidth;        // [0,0.1] ラジアルブラー幅

    float dissolveThreshold;      // [0,1]  ディゾルブ閾値
    float randomIntensity;        // [0,1]  ランダムノイズ強度
    float randomTime;             // タイムカウンタ
    float texelSizeX;             // 1/screenWidth

    float texelSizeY;             // 1/screenHeight
    float pad0;
    float pad1;
    float pad2;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float Luminance(float3 c)
{
    return dot(c, float3(0.2126f, 0.7152f, 0.0722f));
}

float Hash(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv    = input.texcoord;
    float2 texel = float2(texelSizeX, texelSizeY);

    float4 base       = sceneTex.Sample(linearSamp, uv);
    float3 finalColor = base.rgb;

    // -------------------------------------------------------
    // Box Filter (3x3)
    // -------------------------------------------------------
    [branch]
    if (boxFilterIntensity > 0.001f)
    {
        float3 box = float3(0, 0, 0);
        [unroll]
        for (int by = -1; by <= 1; by++)
        [unroll]
        for (int bx = -1; bx <= 1; bx++)
            box += sceneTex.Sample(linearSamp, uv + float2((float)bx, (float)by) * texel).rgb;
        box /= 9.0f;
        finalColor = lerp(finalColor, box, boxFilterIntensity);
    }

    // -------------------------------------------------------
    // Gaussian Filter (3x3)
    // -------------------------------------------------------
    [branch]
    if (gaussianIntensity > 0.001f)
    {
        static const float kGW[3][3] = { {1,2,1},{2,4,2},{1,2,1} };
        float3 gauss = float3(0, 0, 0);
        [unroll]
        for (int gy = -1; gy <= 1; gy++)
        [unroll]
        for (int gx = -1; gx <= 1; gx++)
            gauss += sceneTex.Sample(linearSamp, uv + float2((float)gx, (float)gy) * texel).rgb
                     * kGW[gy + 1][gx + 1];
        gauss /= 16.0f;
        finalColor = lerp(finalColor, gauss, gaussianIntensity);
    }

    // -------------------------------------------------------
    // Radial Blur
    // -------------------------------------------------------
    [branch]
    if (radialBlurIntensity > 0.001f)
    {
        float2 dir = uv - float2(0.5f, 0.5f);
        float3 rad = float3(0, 0, 0);
        static const int kRS = 8;
        [unroll]
        for (int ri = 0; ri < kRS; ri++)
        {
            float t = (float)ri / (float)(kRS - 1);
            rad += sceneTex.Sample(linearSamp, uv - dir * radialBlurWidth * t).rgb;
        }
        rad /= (float)kRS;
        finalColor = lerp(finalColor, rad, radialBlurIntensity);
    }

    // -------------------------------------------------------
    // Luminance Based Outline (Sobel)
    // -------------------------------------------------------
    [branch]
    if (lumOutlineIntensity > 0.001f)
    {
        float lum[3][3];
        [unroll]
        for (int ly = -1; ly <= 1; ly++)
        [unroll]
        for (int lx = -1; lx <= 1; lx++)
            lum[ly + 1][lx + 1] = Luminance(
                sceneTex.Sample(linearSamp, uv + float2((float)lx, (float)ly) * texel).rgb);

        float gx = -lum[0][0] + lum[0][2] - 2*lum[1][0] + 2*lum[1][2] - lum[2][0] + lum[2][2];
        float gy = -lum[0][0] - 2*lum[0][1] - lum[0][2] + lum[2][0] + 2*lum[2][1] + lum[2][2];
        float edge = sqrt(gx * gx + gy * gy);
        float outline = saturate((edge - lumOutlineThreshold) * 50.0f);
        finalColor = lerp(finalColor, float3(0, 0, 0), outline * lumOutlineIntensity);
    }

    // -------------------------------------------------------
    // Depth Based Outline (Sobel on depth)
    // -------------------------------------------------------
    [branch]
    if (depthOutlineIntensity > 0.001f)
    {
        float d[3][3];
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        [unroll]
        for (int dx = -1; dx <= 1; dx++)
            d[dy + 1][dx + 1] = depthTex.Sample(pointSamp,
                uv + float2((float)dx, (float)dy) * texel);

        float gdx = -d[0][0] + d[0][2] - 2*d[1][0] + 2*d[1][2] - d[2][0] + d[2][2];
        float gdy = -d[0][0] - 2*d[0][1] - d[0][2] + d[2][0] + 2*d[2][1] + d[2][2];
        float depthEdge = sqrt(gdx * gdx + gdy * gdy);
        float depthOutline = saturate((depthEdge - depthOutlineThreshold) * 100.0f);
        finalColor = lerp(finalColor, float3(0, 0, 0), depthOutline * depthOutlineIntensity);
    }

    // -------------------------------------------------------
    // Grayscale
    // -------------------------------------------------------
    [branch]
    if (grayscaleIntensity > 0.001f)
    {
        float gray = Luminance(finalColor);
        finalColor = lerp(finalColor, float3(gray, gray, gray), grayscaleIntensity);
    }

    // -------------------------------------------------------
    // Vignetting
    // -------------------------------------------------------
    [branch]
    if (vignetteIntensity > 0.001f)
    {
        float2 centered = uv - float2(0.5f, 0.5f);
        float  dist     = length(centered * float2(1.0f, 1.7778f)); // 16:9 補正
        float  vign     = smoothstep(vignetteRadius, vignetteRadius - vignetteSoftness * 0.5f, dist);
        finalColor *= lerp(1.0f, vign, vignetteIntensity);
    }

    // -------------------------------------------------------
    // Dissolve
    // -------------------------------------------------------
    [branch]
    if (dissolveThreshold > 0.001f)
    {
        float noiseVal = Hash(uv * 123.456f + float2(randomTime * 0.01f, 0.0f));
        clip(noiseVal - dissolveThreshold);
    }

    // -------------------------------------------------------
    // Random Noise
    // -------------------------------------------------------
    [branch]
    if (randomIntensity > 0.001f)
    {
        float noise = Hash(uv * float2(1280.0f, 720.0f) + float2(randomTime, randomTime * 0.73f));
        finalColor  = lerp(finalColor, float3(noise, noise, noise), randomIntensity);
    }

    return float4(finalColor, base.a);
}
