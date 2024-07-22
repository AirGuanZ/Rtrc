rtrc_shader("RayTracingTriangle")
{
    rtrc_raytracing()

    rtrc_rt_group(RayGen)
    rtrc_rt_group(Miss)
    rtrc_rt_group(Hit)

    rtrc_group(Pass)
    {
        rtrc_define(RWTexture2D<float4>, OutputTextureRW)
        rtrc_define(RaytracingAccelerationStructure, Scene)
    };

    struct [raypayload] Payload
    {
        float3 value : read(caller) : write(miss, closesthit);
    };

    struct Attribute
    {
        float2 bary;
    };

    [shader("raygeneration")]
    void RayGen()
    {
        uint2 texelXY = DispatchRaysIndex().xy;
        uint2 imageSize = DispatchRaysDimensions().xy;
        float2 uv = float2(texelXY) / imageSize;

        RayDesc ray;
        ray.Origin    = float3(2 * (2 * uv.x - 1), 2 * (1 - 2 * uv.y), -1);
        ray.Direction = float3(0, 0, 1);
        ray.TMin      = 0;
        ray.TMax      = 100;

        Payload payload;
        TraceRay(Scene, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        OutputTextureRW[texelXY] = float4(payload.value, 1);
    }

    [shader("miss")]
    void Miss(inout Payload payload)
    {
        payload.value = 0;
    }

    [shader("closesthit")]
    void Hit(inout Payload payload, in Attribute attrib)
    {
        payload.value = float3(1, 0, 0) + attrib.bary.x * float3(-1, 1, 0) + attrib.bary.y * float3(-1, 0, 1);
    }

} // rtrc_shader("RayTracingTriangle")