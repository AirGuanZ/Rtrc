rtrc_shader("FADM/RenderPlanarParameterizedInputMesh")
{

    rtrc_vert(VSMain)
    rtrc_frag(FSMain)

    rtrc_group(Pass)
    {
        rtrc_define(StructuredBuffer<float2>, ParameterSpacePositionBuffer)
        rtrc_uniform(float3, Color)
        rtrc_uniform(float2, ClipLower)
        rtrc_uniform(float2, ClipUpper)
    };

    struct VS2FS
    {
        float4 clipPosition : SV_Position;
    };

    VS2FS VSMain(uint vid : SV_VertexID)
    {
        const float2 uv = ParameterSpacePositionBuffer[vid];
        const float2 pos = lerp(Pass.ClipLower, Pass.ClipUpper, uv);
        VS2FS output;
        output.clipPosition = float4(pos, 0.5, 1.0);
        return output;
    }

    float4 FSMain(VS2FS input) : SV_Target
    {
        return float4(Pass.Color, 1);
    }

} // rtrc_shader("FADM/RenderPlanarParameterizedInputMesh")

rtrc_shader("FADM/VisualizeAlignedGrid")
{

    rtrc_vert(VSMain)
    rtrc_frag(FSMain)

    rtrc_group(Pass)
    {
        rtrc_define(Texture2D<float2>, GridUVTexture)
        rtrc_uniform(float3, color)
        rtrc_uniform(float2, clipLower)
        rtrc_uniform(float2, clipUpper)
    };
    
    struct VSInput
    {
        uint2 position : POSITION;
    };

    struct VS2FS
    {
        float4 clipPosition : SV_Position;
    };

    VS2FS VSMain(VSInput input)
    {
        const float2 uv = GridUVTexture[input.position];
        const float2 pos = lerp(Pass.clipLower, Pass.clipUpper, uv);
        VS2FS output;
        output.clipPosition = float4(pos, 0.5, 1.0);
        return output;
    }

    float4 FSMain(VS2FS input) : SV_Target
    {
        return float4(Pass.color, 1);
    }

} // rtrc_shader("FADM/VisualizeAlignedGrid")

rtrc_shader("FADM/BakeUniformVDM")
{

    rtrc_vert(VSMain)
    rtrc_frag(FSMain)

    rtrc_group(Pass)
    {
        rtrc_define(StructuredBuffer<float2>, ParameterSpacePositionBuffer)
        rtrc_define(StructuredBuffer<float3>, PositionBuffer)
        rtrc_uniform(float2, gridLower)
        rtrc_uniform(float2, gridUpper)
        rtrc_uniform(float2, res)
    };

    struct VS2FS
    {
        float4 clipPosition : SV_Position;
        float3 position : POSITION;
        float2 uv : UV;
    };

    VS2FS VSMain(uint vid : SV_VertexID)
    {
        const float2 uv = ParameterSpacePositionBuffer[vid];
        const float2 scale = (Pass.res - 1) / Pass.res;
        const float2 bias = 0.5 / Pass.res;
        VS2FS output;
        output.clipPosition = float4(lerp(-1, 1, scale * float2(uv.x, 1 - uv.y) + bias), 0.5, 1.0);
        output.position		= PositionBuffer[vid];
        output.uv			= uv;
        return output;
    }

    float4 FSMain(VS2FS input) : SV_Target
    {
        const float2 originalHorizontalPosition = lerp(Pass.gridLower, Pass.gridUpper, input.uv);
        const float3 originalPosition = float3(originalHorizontalPosition, 0).xzy;
        const float3 displacement = input.position - originalPosition;
        return float4(displacement, 0);
    }

} // rtrc_shader("FADM/BakeUniformVDM")

rtrc_shader("FADM/BakeAlignedVDM")
{

    rtrc_comp(CSMain)

    rtrc_group(Pass)
    {
        rtrc_define(Texture2D<float2>, GridUVTexture)
        rtrc_define(Texture2D<float3>, HighResUniformVDM)
        rtrc_define(RWTexture2D<float4>, VDM)
        rtrc_uniform(float2, uniformRes)
        rtrc_uniform(uint2, res)
        rtrc_uniform(float2, gridLower)
        rtrc_uniform(float2, gridUpper)
    };
    
    rtrc_sampler(Sampler, filter = linear, address_u = clamp, address_v = clamp)

    [numthreads(8, 8, 1)]
    void CSMain(uint2 tid : SV_DispatchThreadID)
    {
        if(any(tid >= Pass.res))
            return;

        const float2 rawUV = tid / (Pass.res - 1.0);
        const float2 alignedUV = GridUVTexture[tid];
        const float2 horizontalCorrection = (alignedUV - rawUV) * (Pass.gridUpper - Pass.gridLower);
        const float2 srcUV = (Pass.uniformRes - 1.0) / Pass.uniformRes * alignedUV + 0.5 / Pass.uniformRes;
        const float3 displacement = HighResUniformVDM.SampleLevel(Sampler, srcUV, 0);
        VDM[tid] = float4(displacement + float3(horizontalCorrection.x, 0, horizontalCorrection.y), 0);
    }

} // rtrc_shader("FADM/BakeAlignedVDM")

rtrc_shader("FADM/RenderVDM")
{

    rtrc_vert(VSMain)
    rtrc_frag(FSMain)

    rtrc_group(Pass)
    {
        rtrc_define(Texture2D<float3>, VDM)
        rtrc_uniform(float4x4, WorldToClip)
        rtrc_uniform(float2, gridLower)
        rtrc_uniform(float2, gridUpper)
        rtrc_uniform(float2, res)
    };
    
    rtrc_sampler(Sampler, filter = linear, address_u = clamp, address_v = clamp)

    struct VSInput
    {
        uint2 position : POSITION;
    };

    struct VS2FS
    {
        float4 clipPosition : SV_Position;
        float2 uv : UV;
    };

    VS2FS VSMain(VSInput input)
    {
        const float2 uv = input.position / Pass.res;
        const float2 originalHorizontalPosition = lerp(Pass.gridLower, Pass.gridUpper, uv);
        const float3 originalPosition = float3(originalHorizontalPosition, 0).xzy;
        const float3 displacement = VDM[uint2(input.position.x, input.position.y)];
        VS2FS output;
        output.clipPosition = mul(Pass.WorldToClip, float4(originalPosition + displacement, 1));
        output.uv = uv;
        return output;
    }

    float4 FSMain(VS2FS input) : SV_Target
    {
        return float4(0.7 * input.uv, 0, 1);
    }

} // rtrc_shader("FADM/RenderVDM")
