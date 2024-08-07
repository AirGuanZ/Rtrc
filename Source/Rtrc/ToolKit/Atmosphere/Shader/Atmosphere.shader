rtrc_shader("Builtin/Atmosphere/GenerateTransmittanceLut")
{

    rtrc_comp(CSMain)

    #include "Atmosphere.hlsl"

    #define SAMPLE_COUNT 256

    rtrc_group(Pass, CS)
    {
        rtrc_define(RWTexture2D<float4>,     TransmittanceTextureRW)
        rtrc_uniform(uint2,                  outputResolution)
        rtrc_uniform(Atmosphere::Properties, atmosphere)
    };

    [numthreads(16, 16, 1)]
    void CSMain(uint2 tid : SV_DispatchThreadID)
    {
        if(any(tid >= Pass.outputResolution))
            return;

        float2 t01 = (tid + 0.5) / Pass.outputResolution;
        float theta = asin(lerp(-1.0, 1.0, t01.y));
        float h = lerp(0.0, Pass.atmosphere.atmosphereRadius - Pass.atmosphere.planetRadius, t01.x);

        float2 o = float2(0, h + Pass.atmosphere.planetRadius);
        float2 d = float2(cos(theta), sin(theta));

        float t = 0;
        if(!Atmosphere::IntersectRayCircle(o, d, Pass.atmosphere.planetRadius, t))
            Atmosphere::IntersectRayCircle(o, d, Pass.atmosphere.atmosphereRadius, t);
        float2 end = o + t * d;

        float3 sum = 0;
        for(int i = 0; i < SAMPLE_COUNT; ++i)
        {
            float2 pi = lerp(o, end, (i + 0.5) / SAMPLE_COUNT);
            float hi = length(pi) - Pass.atmosphere.planetRadius;
            float3 sigma = Atmosphere::GetSigmaT(Pass.atmosphere, hi);
            sum += sigma;
        }

        float3 result = exp(-sum * (t / SAMPLE_COUNT));
        TransmittanceTextureRW[tid] = float4(result, 1);
    }

} // Shader "Atmosphere/GenerateTransmittanceLut"

rtrc_shader("Builtin/Atmosphere/GenerateMultiScatterLut")
{

    rtrc_comp(CSMain)

    #include "Atmosphere.hlsl"

    rtrc_group(Pass, CS)
    {
        rtrc_define(RWTexture2D<float4>,      MultiScatterTextureRW)
        rtrc_define(StructuredBuffer<float2>, RawDirSamples)
        rtrc_define(Texture2D<float3>,        TransmittanceLutTexture)
        rtrc_uniform(uint2,                   outputResolution)
        rtrc_uniform(uint,                    dirSampleCount)
        rtrc_uniform(uint,                    rayMarchStepCount)
        rtrc_uniform(Atmosphere::Properties,  atmosphere)
    };

    rtrc_sampler(TransmittanceLutSampler, filter = linear, address = clamp)

    static float PI = 3.14159265;

    float3 UniformOnSphere(float2 u)
    {
        float z = 1 - 2 * u.x;
        float r = sqrt(max(0, 1 - z * z));
        float phi = 2 * PI * u.y;
        return float3(r * cos(phi), r * sin(phi), z);
    }

    void Integrate(float3 worldOri, float3 worldDir, float sunTheta, float3 toSunDir, out float3 innerL2, out float3 innerF)
    {
        float u = dot(worldDir, toSunDir);

        float endT = 0;
        bool groundInct = Atmosphere::IntersectRaySphere(worldOri, worldDir, Pass.atmosphere.planetRadius, endT);
        if(!groundInct)
        {
            if(!Atmosphere::IntersectRaySphere(worldOri, worldDir, Pass.atmosphere.atmosphereRadius, endT))
            {
                innerL2 = 0;
                innerF = 0;
                return;
            }
        }

        float dt = endT / Pass.rayMarchStepCount;
        float halfDt = 0.5 * dt;
        float t = 0;

        float3 sumSigmaT = float3(0, 0, 0);
        float3 sumL2 = float3(0, 0, 0), sumF = float3(0, 0, 0);
        for(uint i = 0; i < Pass.rayMarchStepCount; ++i)
        {
            float midT = t + halfDt;
            t += dt;

            float3 worldPos = worldOri + midT * worldDir;
            float h = length(worldPos) - Pass.atmosphere.planetRadius;

            float3 sigmaS = 0, sigmaT = 0;
            Atmosphere::GetSigmaST(Pass.atmosphere, h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

            if(!Atmosphere::TestRaySphere(worldPos, toSunDir, Pass.atmosphere.planetRadius))
            {
                float3 rho = Atmosphere::EvaluatePhaseFunction(Pass.atmosphere, h, u);
                float3 sunTransmittance = Atmosphere::SampleTransmittanceLut(
                    TransmittanceLutTexture, TransmittanceLutSampler, Pass.atmosphere, h, sunTheta);
                sumL2 += dt * transmittance * sunTransmittance * sigmaS * rho;
            }

            sumF += dt * transmittance * sigmaS;
            sumSigmaT += deltaSumSigmaT;
        }

        if(groundInct)
        {
            float3 transmittance = exp(-sumSigmaT);
            float3 sunTransmittance = Atmosphere::SampleTransmittanceLut(
                    TransmittanceLutTexture, TransmittanceLutSampler, Pass.atmosphere, 0, sunTheta);
            sumL2 += transmittance * sunTransmittance * max(0.0, toSunDir.y) * (Pass.atmosphere.terrainAlbedo / PI);
        }

        innerL2 = sumL2;
        innerF = sumF;
    }

    float3 ComputeM(float h, float sunTheta)
    {    
        float3 worldOri = { 0, h + Pass.atmosphere.planetRadius, 0 };
        float3 toSunDir = { cos(sunTheta), sin(sunTheta), 0 };

        float3 sumL2 = float3(0, 0, 0), sumF = float3(0, 0, 0);
        for(int i = 0; i < Pass.dirSampleCount; ++i)
        {
            float2 rawSample = RawDirSamples[i];
            float3 worldDir = UniformOnSphere(rawSample);

            float3 innerL2, innerF;
            Integrate(worldOri, worldDir, sunTheta, toSunDir, innerL2, innerF);

            // phase function is canceled by pdf

            sumL2 += innerL2;
            sumF  += innerF;
        }

        float3 l2 = sumL2 / Pass.dirSampleCount;
        float3 f  = sumF  / Pass.dirSampleCount;
        return l2 / (1 - f);
    }

    [numthreads(16, 16, 1)]
    void CSMain(uint2 tid : SV_DispatchThreadID)
    {
        if(any(tid >= Pass.outputResolution))
            return;
        
        float sinSunTheta = lerp(-1.0, 1.0, (tid.y + 0.5) / Pass.outputResolution.y);
        float sunTheta = asin(sinSunTheta);
        float h = lerp(0.0, Pass.atmosphere.planetRadius, (tid.x + 0.5) / Pass.outputResolution.x);

        float3 M = ComputeM(h, sunTheta);
        MultiScatterTextureRW[tid] = float4(M, 1);
    }

} // Shader "Atmosphere/GenerateMultiScatterLut"

rtrc_shader("Builtin/Atmosphere/GenerateSkyLut")
{

    rtrc_comp(CSMain)

    #include "Atmosphere.hlsl"

    rtrc_group(Pass, CS)
    {
        rtrc_define(RWTexture2D<float4>,     SkyLutTextureRW)
        rtrc_define(Texture2D<float3>,       TransmittanceLutTexture)
        rtrc_define(Texture2D<float3>,       MultiScatterLutTexture)
        rtrc_uniform(Atmosphere::Properties, atmosphere)
        rtrc_uniform(uint2,                  outputResolution)
        rtrc_uniform(uint,                   rayMarchStepCount)
        rtrc_uniform(float,                  frameRandom01)
        rtrc_uniform(float,                  eyePosY)
        rtrc_uniform(float3,                 sunDirection)
        rtrc_uniform(float,                  lerpFactor)
        rtrc_uniform(float3,                 sunIntensity)
    };

    rtrc_sampler(LutSampler, filter = linear, address = clamp)

    static float PI = 3.14159265;

    void RayMarchStep(
        float phaseU, float oriY, float3 dir, float t, float dt,
        inout float3 sumSigmaT, inout float3 inScatter)
    {
        float3 posR = float3(0, oriY + Pass.atmosphere.planetRadius, 0) + dir * t;
        float h = length(posR) - Pass.atmosphere.planetRadius;

        float3 sigmaS, sigmaT;
        Atmosphere::GetSigmaST(Pass.atmosphere, h, sigmaS, sigmaT);

        float3 deltaSumSigmaT = dt * sigmaT;
        float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);
        float sunTheta = PI / 2 - acos(dot(-Pass.sunDirection, normalize(posR)));

        if(!Atmosphere::TestRaySphere(posR, -Pass.sunDirection, Pass.atmosphere.planetRadius))
        {
            float3 rho = Atmosphere::EvaluatePhaseFunction(Pass.atmosphere, h, phaseU);
            float3 sunTrans = Atmosphere::SampleTransmittanceLut(
                    TransmittanceLutTexture, LutSampler, Pass.atmosphere, h, sunTheta);
            inScatter += dt * eyeTrans * sigmaS * rho * sunTrans;
        }

        {
            float tx = h / (Pass.atmosphere.atmosphereRadius - Pass.atmosphere.planetRadius);
            float ty = 0.5 + 0.5 * sin(sunTheta);
            float3 ms = MultiScatterLutTexture.SampleLevel(LutSampler, float2(tx, ty), 0);
            inScatter += dt * eyeTrans * sigmaS * ms;
        }

        sumSigmaT += deltaSumSigmaT;
    }

    [numthreads(16, 16, 1)]
    void CSMain(uint2 tid : SV_DispatchThreadID)
    {
        if(any(tid >= Pass.outputResolution))
            return;
        
        float phi = 2 * PI * (tid.x + 0.5) / Pass.outputResolution.x;
        float vm = 2 * (tid.y + 0.5) / Pass.outputResolution.y - 1;
        float theta = sign(vm) * (PI / 2) * vm * vm;
        float sinTheta = sin(theta), cosTheta = cos(theta);

        float oriY = Pass.eyePosY;
        float3 dir = float3(cos(phi) * cosTheta, sinTheta, sin(phi) * cosTheta);
        float2 planetOri = float2(0, oriY + Pass.atmosphere.planetRadius);
        float2 planetDir = float2(cosTheta, sinTheta);

        float begT = 0, endT = 0;
        if(planetOri.y < Pass.atmosphere.planetRadius)
        {
            Atmosphere::IntersectRayCircle(planetOri, planetDir, Pass.atmosphere.planetRadius, begT);
            Atmosphere::IntersectRayCircle(planetOri, planetDir, Pass.atmosphere.atmosphereRadius, endT);
        }
        else if(planetOri.y > Pass.atmosphere.atmosphereRadius)
        {
            if(!Atmosphere::IntersectRayCircle2(planetOri, planetDir, Pass.atmosphere.atmosphereRadius, begT, endT))
            {
                SkyLutTextureRW[tid] = float4(0, 0, 0, 1);
                return;
            }
            float innerEndT = 0;
            if(Atmosphere::IntersectRayCircle(planetOri, planetDir, Pass.atmosphere.planetRadius, innerEndT))
                endT = innerEndT;
        }
        else
        {
            if(!Atmosphere::IntersectRayCircle(planetOri, planetDir, Pass.atmosphere.planetRadius, endT))
                Atmosphere::IntersectRayCircle(planetOri, planetDir, Pass.atmosphere.atmosphereRadius, endT);
        }

        float phaseU = dot(Pass.sunDirection, -dir);
        float3 inScatter = 0, sumSigmaT = 0;
        float dt = (endT - begT) / Pass.rayMarchStepCount;
        float t = begT + Pass.frameRandom01 * dt;
        for(uint i = 0; i < Pass.rayMarchStepCount; ++i)
        {
            RayMarchStep(phaseU, oriY, dir, t, dt, sumSigmaT, inScatter);
            t += dt;
        }

        float3 curr = inScatter * Pass.sunIntensity;
        float3 prev = SkyLutTextureRW[tid].xyz;
        SkyLutTextureRW[tid] = float4(lerp(prev, curr, Pass.lerpFactor), 1);
    }

} // Shader "Atmosphere/GenerateSkyLut"
