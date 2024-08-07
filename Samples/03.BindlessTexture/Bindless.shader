rtrc_shader("Bindless")
{
    rtrc_vert(VSMain)
    rtrc_frag(FSMain)

    rtrc_group(Pass)
    {
        rtrc_uniform(float, alpha)
        rtrc_bindless_varsize(Texture2D<float3>, Textures, [2048])	
    };

    rtrc_push_constant(Main, FS)
    {
        uint4 slots;
    };

    rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)

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
        int xi = min(int(input.texCoord.x * 2), 1);
        int yi = min(int(input.texCoord.y * 2), 1);
        float xf = frac(2 * input.texCoord.x);
        float yf = frac(2 * input.texCoord.y);
        int textureIndex = rtrc_get_push_constant(Main, slots)[yi * 2 + xi];
        output.color.rgb = Textures[NonUniformResourceIndex(textureIndex)].SampleLevel(Sampler, float2(xf, yf), 0);
        output.color.a = Pass.alpha;
        return output;
    }
}
