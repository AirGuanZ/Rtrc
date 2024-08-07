#include <Rtrc/Graphics/Device/AccelerationStructure.h>

RTRC_BEGIN

void Blas::SetBuffer(RC<SubBuffer> buffer)
{
    buffer_ = std::move(buffer);
    assert(manager_);
    static_cast<AccelerationStructureManager *>(manager_)->_internalCreate(*this);
}

const RC<SubBuffer> &Blas::GetBuffer() const
{
    return buffer_;
}

RHI::BufferDeviceAddress Blas::GetDeviceAddress() const
{
    return GetRHIObject()->GetDeviceAddress();
}

void Tlas::SetBuffer(RC<SubBuffer> buffer)
{
    buffer_ = std::move(buffer);
    assert(manager_);
    static_cast<AccelerationStructureManager *>(manager_)->_internalCreate(*this);
}

const RC<SubBuffer> &Tlas::GetBuffer() const
{
    return buffer_;
}

AccelerationStructureManager::AccelerationStructureManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync)
    : GeneralGPUObjectManager(sync), rhiDevice_(std::move(device))
{
    
}

Box<BlasPrebuildInfo> AccelerationStructureManager::CreateBlasPrebuildinfo(
    Span<RHI::RayTracingGeometryDesc>              geometries,
    RHI::RayTracingAccelerationStructureBuildFlags flags)
{
    auto info = MakeBox<BlasPrebuildInfo>();
    info->info_ = rhiDevice_->CreateBlasPrebuildInfo(geometries, flags);
    return info;
}

Box<TlasPrebuildInfo> AccelerationStructureManager::CreateTlasPrebuildInfo(
    const RHI::RayTracingInstanceArrayDesc        &instances,
    RHI::RayTracingAccelerationStructureBuildFlags flags)
{
    auto info = MakeBox<TlasPrebuildInfo>();
    info->info_ = rhiDevice_->CreateTlasPrebuildInfo(instances, flags);
    return info;
}

RC<Blas> AccelerationStructureManager::CreateBlas(RC<SubBuffer> buffer)
{
    auto blas = new Blas;
    blas->manager_ = this;
    blas->SetBuffer(std::move(buffer));
    return RC<Blas>(blas);
}

RC<Tlas> AccelerationStructureManager::CreateTlas(RC<SubBuffer> buffer)
{
    auto tlas = new Tlas;
    tlas->manager_ = this;
    tlas->SetBuffer(std::move(buffer));
    return RC<Tlas>(tlas);
}

void AccelerationStructureManager::_internalCreate(Blas &blas)
{
    if(blas.rhiObject_)
    {
        sync_.OnFrameComplete([o = std::move(blas.rhiObject_)] {});
    }

    if(blas.buffer_)
    {
        blas.rhiObject_ = rhiDevice_->CreateBlas(
            blas.buffer_->GetFullBufferRHIObject(),
            blas.buffer_->GetSubBufferOffset(),
            blas.buffer_->GetSubBufferSize());
    }
    else
    {
        blas.rhiObject_ = {};
    }
}

void AccelerationStructureManager::_internalCreate(Tlas &tlas)
{
    if(tlas.rhiObject_)
    {
        sync_.OnFrameComplete([o = std::move(tlas.rhiObject_)] {});
    }

    if(tlas.buffer_)
    {
        tlas.rhiObject_ = rhiDevice_->CreateTlas(
            tlas.buffer_->GetFullBufferRHIObject(),
            tlas.buffer_->GetSubBufferOffset(),
            tlas.buffer_->GetSubBufferSize());
    }
    else
    {
        tlas.rhiObject_ = {};
    }
}

RTRC_END
