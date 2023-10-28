#pragma once

#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"

rtrc_group(Pass)
{
    rtrc_define(ConstantBuffer<CameraData>, PrevCamera)
    rtrc_define(ConstantBuffer<CameraData>, CurrCamera)

    rtrc_define(Texture2D<float>, PrevDepth)
    rtrc_define(Texture2D<float>, CurrDepth)

    rtrc_define(Texture2D<float3>, PrevNormal)
    rtrc_define(Texture2D<float3>, CurrNormal)

    rtrc_define(Texture2D<float3>, PrevColor)
    rtrc_define(Texture2D<float3>, CurrColor)
    rtrc_define(RWTexture2D<float4>, ResolvedColor)

    rtrc_define(Texture2D<float2>, PrevMoments)
    rtrc_define(RWTexture2D<float2>, ResolvedMoments)

    rtrc_uniform(uint2, resolution)
    rtrc_uniform(float, alpha)
};

rtrc_sampler(BilinearSampler, filter = linear, address = clamp)
rtrc_sampler(PointSampler, filter = point, address = clamp)

float ComputePointPlaneDistance(float3 o, float3 d, float3 p)
{
    const float3 op = p - o;
    return dot(op, d);
}

void SampleFourTexels(Texture2D<float3> tex, float2 coord, out float3 a, out float3 b, out float3 c, out float3 d)
{
    const int2 center = int2(floor(coord + 0.5));
    a = tex[clamp(center - int2(0, 0), int2(0, 0), int2(Pass.resolution - 1))];
    b = tex[clamp(center - int2(0, 1), int2(0, 0), int2(Pass.resolution - 1))];
    c = tex[clamp(center - int2(1, 0), int2(0, 0), int2(Pass.resolution - 1))];
    d = tex[clamp(center - int2(1, 1), int2(0, 0), int2(Pass.resolution - 1))];
}

void SampleFourTexels(Texture2D<float> tex, float2 coord, out float a, out float b, out float c, out float d)
{
    const int2 center = int2(floor(coord + 0.5));
    a = tex[clamp(center - int2(0, 0), int2(0, 0), int2(Pass.resolution - 1))];
    b = tex[clamp(center - int2(0, 1), int2(0, 0), int2(Pass.resolution - 1))];
    c = tex[clamp(center - int2(1, 0), int2(0, 0), int2(Pass.resolution - 1))];
    d = tex[clamp(center - int2(1, 1), int2(0, 0), int2(Pass.resolution - 1))];
}

void SampleFourTexels(Texture2D<float2> tex, float2 coord, out float2 a, out float2 b, out float2 c, out float2 d)
{
    const int2 center = int2(floor(coord + 0.5));
    a = tex[clamp(center - int2(0, 0), int2(0, 0), int2(Pass.resolution - 1))];
    b = tex[clamp(center - int2(0, 1), int2(0, 0), int2(Pass.resolution - 1))];
    c = tex[clamp(center - int2(1, 0), int2(0, 0), int2(Pass.resolution - 1))];
    d = tex[clamp(center - int2(1, 1), int2(0, 0), int2(Pass.resolution - 1))];
}

bool Reproject(float viewZ, float3 worldPos, float3 worldNormal, out float3 prevColor, out float2 prevMoments)
{
    const float4 prevClip = mul(PrevCamera.worldToClip, float4(worldPos, 1));
    const float2 prevScr = prevClip.xy / prevClip.w;
    if(prevClip.w <= 0 || any(abs(prevScr) >= 1))
        return false;
    
    const float2 prevUV = 0.5 + float2(0.5, -0.5) * prevScr;
    const float2 prevCoord = prevUV * Pass.resolution;
    float4 prevDepth;
    SampleFourTexels(PrevDepth, prevCoord, prevDepth.x, prevDepth.y, prevDepth.z, prevDepth.w);
    //const float4 prevDepth = PrevDepth.GatherRed(BilinearSampler, prevUV);
    if(all(prevDepth >= 1))
        return false;

    const float2 prevCenter = floor(prevCoord + 0.5);
    const float2 distA = prevCoord - (prevCenter - float2(0, 0) + 0.5);
    const float2 distB = prevCoord - (prevCenter - float2(0, 1) + 0.5);
    const float2 distC = prevCoord - (prevCenter - float2(1, 0) + 0.5);
    const float2 distD = prevCoord - (prevCenter - float2(1, 1) + 0.5);
    const float2 invDistA = max(1.2 - abs(distA), 0);
    const float2 invDistB = max(1.2 - abs(distB), 0);
    const float2 invDistC = max(1.2 - abs(distC), 0);
    const float2 invDistD = max(1.2 - abs(distD), 0);
    const float4 prevDistWeights = pow(float4(
        invDistA.x * invDistA.y,
        invDistB.x * invDistB.y,
        invDistC.x * invDistC.y,
        invDistD.x * invDistD.y), 0.5);
    //const float2 prevCoordT = frac(prevCoord + 0.5);
    //const float2 distA = float2(1 - prevCoordT.x, 0 + prevCoordT.y);
    //const float2 distB = float2(0 + prevCoordT.x, 0 + prevCoordT.y);
    //const float2 distC = float2(0 + prevCoordT.x, 1 - prevCoordT.y);
    //const float2 distD = float2(1 - prevCoordT.x, 1 - prevCoordT.y);
    //const float4 prevDistWeights = pow(float4(distA.x * distA.y, distB.x * distB.y, distC.x * distC.y, distD.x * distD.y), 0.4);

    const bool4 prevValidMask = prevDepth < 1;
    const float4 prevViewZ = float4(
        prevValidMask[0] ? CameraUtils::DeviceZToViewZ(PrevCamera, prevDepth.x) : -100,
        prevValidMask[1] ? CameraUtils::DeviceZToViewZ(PrevCamera, prevDepth.y) : -100,
        prevValidMask[2] ? CameraUtils::DeviceZToViewZ(PrevCamera, prevDepth.z) : -100,
        prevValidMask[3] ? CameraUtils::DeviceZToViewZ(PrevCamera, prevDepth.w) : -100);
    const float4 absErr = abs(prevViewZ - viewZ);
    const float4 relErr = absErr / max(viewZ, 0.2);
    if(all(absErr >= 0.2) || all(relErr >= 0.1))
        return false;

    float3 normalA, normalB, normalC, normalD;
    SampleFourTexels(PrevNormal, prevCoord, normalA, normalB, normalC, normalD);
    normalA = 2 * normalA - 1;
    normalB = 2 * normalB - 1;
    normalC = 2 * normalC - 1;
    normalD = 2 * normalD - 1;
    if(!prevValidMask[0]) normalA = 0;
    if(!prevValidMask[1]) normalB = 0;
    if(!prevValidMask[2]) normalC = 0;
    if(!prevValidMask[3]) normalD = 0;

    //const float4 normalX = 2 * PrevNormal.GatherRed(BilinearSampler, prevUV) - 1;
    //const float4 normalY = 2 * PrevNormal.GatherGreen(BilinearSampler, prevUV) - 1;
    //const float4 normalZ = 2 * PrevNormal.GatherBlue(BilinearSampler, prevUV) - 1;
    //const float3 normalA = prevValidMask[0] ? float3(normalX[0], normalY[0], normalZ[0]) : 0;
    //const float3 normalB = prevValidMask[1] ? float3(normalX[1], normalY[1], normalZ[1]) : 0;
    //const float3 normalC = prevValidMask[2] ? float3(normalX[2], normalY[2], normalZ[2]) : 0;
    //const float3 normalD = prevValidMask[3] ? float3(normalX[3], normalY[3], normalZ[3]) : 0;
    const float4 norDot = float4(
        dot(normalA, worldNormal),
        dot(normalB, worldNormal),
        dot(normalC, worldNormal),
        dot(normalD, worldNormal));
    if(all(norDot < 0.95))
        return false;
    
    const float4 weightMask = step(relErr, 0.1) * step(0.95, norDot);
    float4 unnormalizedWeights = prevDistWeights * 1.0 / max(relErr, 1e-3) * (norDot - 0.9) * weightMask;
    const float maxUW = max(max(unnormalizedWeights.x, unnormalizedWeights.y), max(unnormalizedWeights.z, unnormalizedWeights.w));
    if(unnormalizedWeights.x == maxUW)
        unnormalizedWeights.yzw = 0;
    else if(unnormalizedWeights.y == maxUW)
        unnormalizedWeights.xzw = 0;
    else if(unnormalizedWeights.z == maxUW)
        unnormalizedWeights.xyw = 0;
    else
        unnormalizedWeights.xyz = 0;
    const float4 weights = unnormalizedWeights / max(dot(float4(1, 1, 1, 1), unnormalizedWeights), 1e-4);

    float3 colorA, colorB, colorC, colorD;
    SampleFourTexels(PrevColor, prevCoord, colorA, colorB, colorC, colorD);

    prevColor = weights[0] * colorA + weights[1] * colorB + weights[2] * colorC + weights[3] * colorD;

    //const float4 colorR = PrevColor.GatherRed(BilinearSampler, prevUV);
    //const float4 colorG = PrevColor.GatherGreen(BilinearSampler, prevUV);
    //const float4 colorB = PrevColor.GatherBlue(BilinearSampler, prevUV);
    //prevColor.r = dot(weights, colorR);
    //prevColor.g = dot(weights, colorG);
    //prevColor.b = dot(weights, colorB);

    float2 momA, momB, momC, momD;
    SampleFourTexels(PrevMoments, prevCoord, momA, momB, momC, momD);
    prevMoments= weights[0] * momA + weights[1] * momB + weights[2] * momC + weights[3] * momD;

    //const float4 momX = PrevMoments.GatherRed(BilinearSampler, prevUV);
    //const float4 momY = PrevMoments.GatherGreen(BilinearSampler, prevUV);
    //prevMoments.x = dot(weights, momX);
    //prevMoments.y = dot(weights, momY);

    return true;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;

    const float currZ = CurrDepth[tid];
    if(currZ >= 1)
        return;
    const float2 uv = (tid + 0.5) / Pass.resolution;
    const float viewZ = CameraUtils::DeviceZToViewZ(CurrCamera, currZ);
    const float3 worldPos = CameraUtils::GetWorldPosition(CurrCamera, uv, currZ);

    const float3 currNormal = 2 * CurrNormal[tid] - 1;
    const float3 currColor = CurrColor[tid];
    const float currLum = RelativeLuminance(currColor);
    const float2 currMoments = float2(currLum, currLum * currLum);

    float3 prevColor; float2 prevMoments;
    const bool hasPrev = Reproject(viewZ, worldPos, currNormal, prevColor, prevMoments);
    if(hasPrev)
    {
        const float alpha = Pass.alpha;
        const float3 resolvedColor = lerp(prevColor, currColor, alpha);
        const float2 resolvedMoments = lerp(prevMoments, currMoments, alpha);
        ResolvedColor[tid] = float4(resolvedColor, 1);
        ResolvedMoments[tid] = resolvedMoments;
        return;
    }

    ResolvedColor[tid] = float4(currColor, 1);
    ResolvedMoments[tid] = currMoments;
}
