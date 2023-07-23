#pragma once

#include "../Common/Scene.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH
#include "../Common/GBufferRead.hlsl"

// External macros:
//     ENABLE_SHADOW_SOFTNESS
//     RAY_COUNT
//     ENABLE_LOW_RES_MASK
//     ALWAYS_WRITE_OUTPUT
//     UNALIGNED_OUTPUT_TEXEL

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(RaytracingAccelerationStructure, Scene)
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)

    rtrc_define(RWTexture2D<unorm float>, OutputTextureRW)

    rtrc_define(Texture2D<float>, BlueNoise256)
    rtrc_define(Texture2D<float>, LowResMask)

    rtrc_uniform(uint2,  outputResolution)
    rtrc_uniform(float2, rcpOutputResolution)
    rtrc_uniform(uint,   lightType)     // 0: point; 1: directional
    rtrc_uniform(float3, lightGeometry) // position or normalized direction
    rtrc_uniform(float,  shadowSoftness)
};

#if ENABLE_LOW_RES_MASK
rtrc_sampler(LowResMaskSampler, filter = point, address = clamp)
#endif

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

float3 GetWorldRay(float2 uv)
{
    float3 rayAB = lerp(Camera.worldRays[0], Camera.worldRays[1], uv.x);
    float3 rayCD = lerp(Camera.worldRays[2], Camera.worldRays[3], uv.x);
    float3 ray = lerp(rayAB, rayCD, uv.y);
    return ray;
}

void GenerateLocalFrameXY(float3 z, out float3 x, out float3 y)
{
    z = normalize(z);
    if(abs(abs(z.x) - 1) < 0.1)
        x = normalize(cross(float3(0, 1, 0), z));
    else
        x = normalize(cross(float3(1, 0, 0), z));
    y = normalize(cross(z, x));
}

float EvaluateShadowFactor(float3 origin, float3 direction, float t)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.TMin      = 0;
    ray.Direction = direction;
    ray.TMax      = t;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();
    return rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;

    float2 uv = (tid + 0.5) * Pass.rcpOutputResolution;
#if UNALIGNED_OUTPUT_TEXEL
    float2 gbufferResolution = float2(GetGBufferResolution());
    uv = floor(min(uv * gbufferResolution, gbufferResolution - float2(1, 1)));
    uv = (uv + 0.5) / gbufferResolution;
#endif

    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
    {
#if ALWAYS_WRITE_OUTPUT
        OutputTextureRW[tid] = 1;
#endif
        return;
    }

    float3 worldRay = GetWorldRay(uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPosition = viewZ * worldRay + Camera.worldPosition;
    float3 worldNormal = gpixel.normal;
    float3 origin = worldPosition + 1e-3 * worldNormal;
 
#if ENABLE_LOW_RES_MASK
    float lowResMaskValue = LowResMask.SampleLevel(LowResMaskSampler, uv, 0);
    if(lowResMaskValue <= 0 || lowResMaskValue >= 1)
    {
        OutputTextureRW[tid] = lowResMaskValue;
        return;
    }
#endif

#if RAY_COUNT != 1 && RAY_COUNT != 8 && RAY_COUNT != 12 && RAY_COUNT != 16 && RAY_COUNT != 24 && RAY_COUNT != 32 && RAY_COUNT != 48 && RAY_COUNT != 64
#error "Invalid RAY_COUNT"
#endif

    float2 poissonDisk[64] = {
        float2(0.511749, 0.547686), float2(0.58929, 0.257224), float2(0.165018, 0.57663), float2(0.407692, 0.742285),
        float2(0.707012, 0.646523), float2(0.31463, 0.466825), float2(0.801257, 0.485186), float2(0.418136, 0.146517),
        float2(0.579889, 0.0368284), float2(0.79801, 0.140114), float2(-0.0413185, 0.371455), float2(-0.0529108, 0.627352),
        float2(0.0821375, 0.882071), float2(0.17308, 0.301207), float2(-0.120452, 0.867216), float2(0.371096, 0.916454),
        float2(-0.178381, 0.146101), float2(-0.276489, 0.550525), float2(0.12542, 0.126643), float2(-0.296654, 0.286879),
        float2(0.261744, -0.00604975), float2(-0.213417, 0.715776), float2(0.425684, -0.153211), float2(-0.480054, 0.321357),
        float2(-0.0717878, -0.0250567), float2(-0.328775, -0.169666), float2(-0.394923, 0.130802), float2(-0.553681, -0.176777),
        float2(-0.722615, 0.120616), float2(-0.693065, 0.309017), float2(0.603193, 0.791471), float2(-0.0754941, -0.297988),
        float2(0.109303, -0.156472), float2(0.260605, -0.280111), float2(0.129731, -0.487954), float2(-0.537315, 0.520494),
        float2(-0.42758, 0.800607), float2(0.77309, -0.0728102), float2(0.908777, 0.328356), float2(0.985341, 0.0759158),
        float2(0.947536, -0.11837), float2(-0.103315, -0.610747), float2(0.337171, -0.584), float2(0.210919, -0.720055),
        float2(0.41894, -0.36769), float2(-0.254228, -0.49368), float2(-0.428562, -0.404037), float2(-0.831732, -0.189615),
        float2(-0.922642, 0.0888026), float2(-0.865914, 0.427795), float2(0.706117, -0.311662), float2(0.545465, -0.520942),
        float2(-0.695738, 0.664492), float2(0.389421, -0.899007), float2(0.48842, -0.708054), float2(0.760298, -0.62735),
        float2(-0.390788, -0.707388), float2(-0.591046, -0.686721), float2(-0.769903, -0.413775), float2(-0.604457, -0.502571),
        float2(-0.557234, 0.00451362), float2(0.147572, -0.924353), float2(-0.0662488, -0.892081), float2(0.863832, -0.407206)
    };

/*#if RAY_COUNT == 16
    const float2 poissonDisk[16] =
    {
        float2(-0.94201624,  -0.39906216),
        float2(0.94558609,   -0.76890725),
        float2(-0.094184101, -0.92938870),
        float2(0.34495938,    0.29387760),
        float2(-0.91588581,   0.45771432),
        float2(-0.81544232,  -0.87912464),
        float2(-0.38277543,   0.27676845),
        float2(0.97484398,    0.75648379),
        float2(0.44323325,   -0.97511554),
        float2(0.53742981,   -0.47373420),
        float2(-0.26496911,  -0.41893023),
        float2(0.79197514,    0.19090188),
        float2(-0.24188840,   0.99706507),
        float2(-0.81409955,   0.91437590),
        float2(0.19984126,    0.78641367),
        float2(0.14383161,    -0.14100790)
    };
#endif*/

#if ENABLE_SHADOW_SOFTNESS
    float blueNoise = BlueNoise256[tid % 256];
    float diskRotRad = 2 * 3.1415926535 * blueNoise;
    float rotSin, rotCos;
    sincos(diskRotRad, rotSin, rotCos);
    float2x2 diskRot = float2x2(rotCos, -rotSin, rotSin, rotCos);
#endif

    float shadowFactor = 0;
    [branch]
    if(Pass.lightType == LIGHT_TYPE_POINT)
    {
        float t = 1.0;
#if !ENABLE_SHADOW_SOFTNESS
        float3 direction = Pass.lightGeometry - origin;
        shadowFactor = EvaluateShadowFactor(origin, direction, t);
#else
        float3 localX, localY;
        GenerateLocalFrameXY(Pass.lightGeometry - origin, localX, localY);
        for(int i = 0; i < RAY_COUNT; ++i)
        {
            float2 u = poissonDisk[i];
            u = mul(u, diskRot);
            float3 direction = Pass.lightGeometry - origin + Pass.shadowSoftness * (u.x * localX + u.y * localY);
            shadowFactor += EvaluateShadowFactor(origin, direction, t);
        }
        shadowFactor *= 1.0 / RAY_COUNT;
#endif
    }
    else
    {
        float t = 1e5;
#if !ENABLE_SHADOW_SOFTNESS
        float3 direction = -Pass.lightGeometry;
        shadowFactor = EvaluateShadowFactor(origin, direction, t);
#else
        float3 localX, localY;
        GenerateLocalFrameXY(-Pass.lightGeometry, localX, localY);
        for(int i = 0; i < RAY_COUNT; ++i)
        {
            float2 u = poissonDisk[i];
            float3 direction = normalize(-Pass.lightGeometry + Pass.shadowSoftness * (u.x * localX + u.y * localY));
            shadowFactor += EvaluateShadowFactor(origin, direction, t);
        }
        shadowFactor *= 1.0 / RAY_COUNT;
#endif
    }

    OutputTextureRW[tid] = shadowFactor;
}
