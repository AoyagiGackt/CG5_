struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer VignetteParams : register(b0) {
    float intensity;
    float radius;
    float softness;
    float pad;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 center = input.uv - float2(0.5f, 0.5f);
    float  dist   = length(center);
    float  vig    = smoothstep(radius, radius + softness, dist) * intensity;
    return float4(0.0f, 0.0f, 0.0f, vig);
}
