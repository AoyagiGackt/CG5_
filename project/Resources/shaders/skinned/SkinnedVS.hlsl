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
    float4x4 skinMatrix =
        input.boneWeights.x * gMatrixPalette[input.boneIndices.x] +
        input.boneWeights.y * gMatrixPalette[input.boneIndices.y] +
        input.boneWeights.z * gMatrixPalette[input.boneIndices.z] +
        input.boneWeights.w * gMatrixPalette[input.boneIndices.w];

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
