#pragma once

#include "Scene.hlsl"

struct PerObjectData
{
    float4x4 localToWorld;
    float4x4 worldToLocal;
    float4x4 localToCamera;
    float4x4 localToClip;
};

rtrc_group(Pass)
{
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    rtrc_define(StructuredBuffer<PerObjectData>, PerObjectDataBuffer)
};

rtrc_push_constant(All)
{
    uint perObjectDataOffset;
};

#if ENABLE_INSTANCE
#define DefineInstanceID()            uint instanceID : SV_InstanceID
#define TransferInstanceID(IN, OUT)   OUT.instanceID = IN.instanceID
#define GetInstancedPerObjectData(IN) (PerObjectDataBuffer[PushConstant.perObjectDataOffset + IN.instanceID])
#else
#define DefineInstanceID()
#define TransferInstanceID(IN, OUT)
#define GetInstancedPerObjectData(IN) (PerObjectDataBuffer[PushConstant.perObjectDataOffset])
#define GetPerObjectData()            (PerObjectDataBuffer[PushConstant.perObjectDataOffset])
#endif
