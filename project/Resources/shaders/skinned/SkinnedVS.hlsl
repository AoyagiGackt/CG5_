#include "../object3d/Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 LightVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

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

    float4x4 skinMatrix =
        weights.x * gMatrixPalette[input.boneIndices.x] +
        weights.y * gMatrixPalette[input.boneIndices.y] +
        weights.z * gMatrixPalette[input.boneIndices.z] +
        weights.w * gMatrixPalette[input.boneIndices.w];

    float4 skinnedPos    = mul(input.position, skinMatrix);
    float3 skinnedNormal = mul(input.normal, (float3x3)skinMatrix);

    VertexShaderOutput output;
    output.position      = mul(skinnedPos, gTransformationMatrix.WVP);
    output.texcoord      = input.texcoord;
    output.normal        = normalize(mul(skinnedNormal, (float3x3)gTransformationMatrix.World));
    output.worldPos      = mul(skinnedPos, gTransformationMatrix.World).xyz;
    output.lightSpacePos = mul(skinnedPos, gTransformationMatrix.LightVP);
    return output;
}
