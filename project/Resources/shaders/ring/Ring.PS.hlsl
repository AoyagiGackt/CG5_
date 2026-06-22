Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
};

PSOutput main(PSInput input)
{
    PSOutput output;
    float4 tex = gTexture.Sample(gSampler, input.texcoord);
    output.color = input.color * tex;
    
    if (output.color.a <= 0.0f)
    {
        discard;
    }
    
    return output;
}
