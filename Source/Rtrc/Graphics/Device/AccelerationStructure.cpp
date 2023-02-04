#include <Rtrc/Graphics/Device/AccelerationStructure.h>

RTRC_BEGIN

void Blas::SetBuffer(RC<SubBuffer> buffer)
{
    buffer_ = std::move(buffer);
}

const RC<SubBuffer> &Blas::GetBuffer() const
{
    return buffer_;
}

void Tlas::SetBuffer(RC<SubBuffer> buffer)
{
    buffer_ = std::move(buffer);
}

const RC<SubBuffer> &Tlas::GetBuffer() const
{
    return buffer_;
}

AccelerationStructureManager::AccelerationStructureManager(RHI::DevicePtr device, DeviceSynchronizer &sync)
    : GeneralGPUObjectManager(sync), rhiDevice_(std::move(device))
{
    
}

BlasPrebuildInfo AccelerationStructureManager::CreateBlasPrebuildinfo(
    Span<RHI::RayTracingGeometryDesc>             geometries,
    RHI::RayTracingAccelerationStructureBuildFlag flags)
{
    BlasPrebuildInfo info;
    info.info_ = rhiDevice_->CreateBlasPrebuildInfo(geometries, flags);
    return info;
}

TlasPrebuildInfo AccelerationStructureManager::CreateTlasPrebuildInfo(
    Span<RHI::RayTracingInstanceArrayDesc>        instanceArrays,
    RHI::RayTracingAccelerationStructureBuildFlag flags)
{
    TlasPrebuildInfo info;
    info.info_ = rhiDevice_->CreateTlasPrebuildInfo(instanceArrays, flags);
    return info;
}

RC<Blas> AccelerationStructureManager::CreateBlas(RC<SubBuffer> buffer)
{
    auto blas = new Blas;
    blas->SetBuffer(std::move(buffer));
    blas->manager_ = this;
    return RC<Blas>(blas);
}

RC<Tlas> AccelerationStructureManager::CreateTlas(RC<SubBuffer> buffer)
{
    auto tlas = new Tlas;
    tlas->SetBuffer(std::move(buffer));
    tlas->manager_ = this;
    return RC<Tlas>(tlas);
}

RTRC_END
