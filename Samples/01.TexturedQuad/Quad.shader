rtrc_shader("Sample01/Quad")
{
    rtrc_vertex(VSMain)
    rtrc_fragment(FSMain)
    rtrc_keyword(DADADA)
			
    rtrc_group(Material)
    {
        rtrc_define(Texture2D<float4>, MainTexture)
        rtrc_define(SamplerState, MainSampler)
        rtrc_uniform(float, scale)
        rtrc_uniform(float, mipLevel)
    };

    struct VSInput
    {
        float3 position : POSITION;
        float2 texCoord : TEXCOORD;
    };

    struct VSOutput
    {
        float4 position : SV_Position;
        float2 texCoord : TexCoord;
    };

    VSOutput VSMain(VSInput input)
    {
        VSOutput output;
        output.position = float4(input.position, 1);
        output.texCoord = input.texCoord;
        return output;
    }

    struct FSOutput
    {
        float4 color : SV_Target;
    };

    FSOutput FSMain(VSOutput input)
    {
        FSOutput output;
#if DADADA
        output.color = float4(MainTexture.SampleLevel(MainSampler, input.texCoord, Material.mipLevel).rgb, 1.0);
#else
        output.color = float4(2 * MainTexture.SampleLevel(MainSampler, input.texCoord, Material.mipLevel).rgb, 1.0);
#endif
        output.color *= Material.scale;
        return output;
    }
} // rtrc_shader("Sample01/Quad")
