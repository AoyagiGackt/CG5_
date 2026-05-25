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

// インラインルートSRV では GetDimensions が未定義動作になるため、
// 頂点数はルート32bitコンスタント (b1) で受け取る
cbuffer DispatchParams : register(b1)
{
    uint gNumVertices;
};

StructuredBuffer<InputVertex>    gInput  : register(t0);
RWStructuredBuffer<OutputVertex> gOutput : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= gNumVertices)
        return;

    InputVertex input = gInput[dtid.x];

    float totalWeight = input.boneWeights.x + input.boneWeights.y
                      + input.boneWeights.z + input.boneWeights.w;
    float4 weights = (totalWeight > 0.0f) ? input.boneWeights / totalWeight
                                          : float4(1, 0, 0, 0);

    float w0 = weights.x, w1 = weights.y, w2 = weights.z, w3 = weights.w;
    float4x4 skinMatrix =
        w0 * gMatrixPalette[input.boneIndices.x] +
        w1 * gMatrixPalette[input.boneIndices.y] +
        w2 * gMatrixPalette[input.boneIndices.z] +
        w3 * gMatrixPalette[input.boneIndices.w];

    OutputVertex output;
    output.position = mul(input.position, skinMatrix);
    output.texcoord = input.texcoord;
    output.normal   = normalize(mul(input.normal, (float3x3)skinMatrix));

    gOutput[dtid.x] = output;
}
