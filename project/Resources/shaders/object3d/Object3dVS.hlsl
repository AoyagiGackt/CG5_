#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose; // 非均一スケール対応（法線変換用）
    float4x4 LightVP;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position      = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord      = input.texcoord;
    output.normal        = normalize(mul(input.normal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
    output.worldPos      = mul(input.position, gTransformationMatrix.World).xyz;
    output.lightSpacePos = mul(input.position, gTransformationMatrix.LightVP);
    return output;
}
