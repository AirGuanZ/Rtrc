#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/RayTracing/TlasPrebuildInfo.h>

RTRC_RHI_D3D12_BEGIN

DirectX12TlasPrebuildInfo::DirectX12TlasPrebuildInfo(
    DirectX12Device                          *device,
    const RayTracingInstanceArrayDesc        &instances,
    RayTracingAccelerationStructureBuildFlags flags)
    : device_(device), prebuildInfo_{}, buildInputs_{}
{
#if RTRC_DEBUG
    instances_ = instances;
#endif

    buildInputs_.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildInputs_.Flags         = TranslateRayTracingAccelerationStructureBuildFlags(flags);
    buildInputs_.NumDescs      = instances.instanceCount;
    buildInputs_.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildInputs_.InstanceDescs = 1;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
    device_->_internalGetNativeDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs_, &prebuildInfo);

    prebuildInfo_.accelerationStructureSize = prebuildInfo.ResultDataMaxSizeInBytes;
    prebuildInfo_.buildScratchSize          = prebuildInfo.ScratchDataSizeInBytes;
    prebuildInfo_.updateScratchSize         = prebuildInfo.UpdateScratchDataSizeInBytes;
}

const RayTracingAccelerationStructurePrebuildInfo &DirectX12TlasPrebuildInfo::GetPrebuildInfo() const
{
    return prebuildInfo_;
}

void DirectX12TlasPrebuildInfo::_internalBuildTlas(
    ID3D12GraphicsCommandList7        *commandList,
    const RayTracingInstanceArrayDesc &instances,
    const TlasOPtr                    &tlas,
    BufferDeviceAddress                scratchBuffer)
{
#if RTRC_DEBUG
    if(instances.instanceCount > instances_.instanceCount)
    {
        throw Exception("DirectX12TlasPrebuildInfo is not compatible with given instance count");
    }
#endif
    
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc;
    buildDesc.DestAccelerationStructureData    = tlas->GetDeviceAddress().address;
    buildDesc.Inputs                           = buildInputs_;
    buildDesc.Inputs.InstanceDescs             = instances.instanceData.address;
    buildDesc.SourceAccelerationStructureData  = 0;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer.address;
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

RTRC_RHI_D3D12_END
