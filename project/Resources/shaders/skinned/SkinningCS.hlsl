// SkinningCS.hlsl
// SkinnedVS.hlsl で頂点ごとに行っていたスキニング計算を事前にGPUで一括処理する。
// 入力: 元の頂点バッファ (SRV, t0) + スキニングパレット (CBV, b0)
// 出力: 計算済み頂点バッファ (UAV, u0) → 描画パスで頂点バッファとして使用

struct InputVertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
    uint4  boneIndices;
    float4 boneWeights;
};

// ModelCommon の入力レイアウト (POSITION/TEXCOORD/NORMAL) と完全一致させること
struct OutputVertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

#define MAX_JOINTS 128
cbuffer SkinningPalette : register(b0)
{
    float4x4 gMatrixPalette[MAX_JOINTS];
};

StructuredBuffer<InputVertex>    gInput  : register(t0);
RWStructuredBuffer<OutputVertex> gOutput : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint vertexCount, stride;
    gInput.GetDimensions(vertexCount, stride);
    if (dtid.x >= vertexCount)
        return;

    InputVertex input = gInput[dtid.x];

    float totalWeight = input.boneWeights.x + input.boneWeights.y
                      + input.boneWeights.z + input.boneWeights.w;
    float4 weights = (totalWeight > 0.0f) ? input.boneWeights / totalWeight
                                          : float4(1, 0, 0, 0);

    float4x4 skinMatrix =
        weights.x * gMatrixPalette[input.boneIndices.x] +
        weights.y * gMatrixPalette[input.boneIndices.y] +
        weights.z * gMatrixPalette[input.boneIndices.z] +
        weights.w * gMatrixPalette[input.boneIndices.w];

    OutputVertex output;
    output.position = mul(input.position, skinMatrix);
    output.texcoord = input.texcoord;
    output.normal   = normalize(mul(input.normal, (float3x3)skinMatrix));

    gOutput[dtid.x] = output;
}
