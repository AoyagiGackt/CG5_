struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// フルスクリーン三角形を SV_VertexID から生成（頂点バッファ不要）
VSOutput main(uint vertexId : SV_VertexID)
{
    VSOutput output;
    output.texcoord  = float2((vertexId << 1) & 2, vertexId & 2);
    output.position  = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
