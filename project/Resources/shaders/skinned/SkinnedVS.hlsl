#include "../object3d/Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose; // 非均一スケール対応（法線変換用）
    float4x4 LightVP;
};
cbuffer gTransformationMatrixCB : register(b0)
{
    TransformationMatrix gTransformationMatrix;
};

#define MAX_JOINTS 128
cbuffer SkinningPalette : register(b1)
{
    float4x4 gMatrixPalette[MAX_JOINTS];
};

struct VertexShaderInput
{
    float4 position    : POSITION0;
    float2 texcoord    : TEXCOORD0;
    float3 normal      : NORMAL0;
    uint4  boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    // ウェイト合計が 1.0 でない場合に縮小・拡大が起きないよう正規化する
    float totalWeight = input.boneWeights.x + input.boneWeights.y +
                        input.boneWeights.z + input.boneWeights.w;
    float4 weights = (totalWeight > 0.0f) ? input.boneWeights / totalWeight : float4(1, 0, 0, 0);

    float w0 = weights.x, w1 = weights.y, w2 = weights.z, w3 = weights.w;
    float4x4 skinMatrix =
        w0 * gMatrixPalette[input.boneIndices.x] +
        w1 * gMatrixPalette[input.boneIndices.y] +
        w2 * gMatrixPalette[input.boneIndices.z] +
        w3 * gMatrixPalette[input.boneIndices.w];

    float4 skinnedPos    = mul(input.position, skinMatrix);
    float3 skinnedNormal = mul(input.normal, (float3x3)skinMatrix);

    VertexShaderOutput output;
    output.position      = mul(skinnedPos, gTransformationMatrix.WVP);
    output.texcoord      = input.texcoord;
    output.normal        = normalize(mul(skinnedNormal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
    output.worldPos      = mul(skinnedPos, gTransformationMatrix.World).xyz;
    output.lightSpacePos = mul(skinnedPos, gTransformationMatrix.LightVP);
    return output;
}
