Buffer<float2> VertexPositionBuffer;
Buffer<float2> VertexTexCoordBuffer;
Texture2D<float4> MainTexture;
SamplerState MainSampler;

group TestGroup
{
    VertexPositionBuffer, VertexTexCoordBuffer, MainTexture, MainSampler
}

struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(VertexPositionBuffer[input.vertexID], 0.0, 1.0f);
    output.texCoord = VertexTexCoordBuffer[input.vertexID];
    return output;
}

struct FSOutput
{
    float4 color : SV_Target;
};

FSOutput FSMain(VSOutput input)
{
    FSOutput output;
    output.color = float4(MainTexture.Sample(MainSampler, input.texCoord).rgb, 1);
    return output;
}
