#pragma once

#include <Graphics/Device/Buffer.h>
#include <Graphics/Device/GeneralGPUObject.h>

RTRC_BEGIN

class AccelerationStructureManager;

class Blas : public GeneralGPUObject<RHI::BlasPtr>
{
public:
    
    void SetBuffer(RC<SubBuffer> buffer);
    const RC<SubBuffer> &GetBuffer() const;
    RHI::BufferDeviceAddress GetDeviceAddress() const;

private:

    friend class AccelerationStructureManager;

    Blas() = default;
    
    RC<SubBuffer> buffer_;
};

class Tlas : public GeneralGPUObject<RHI::TlasPtr>
{
public:

    void SetBuffer(RC<SubBuffer> buffer);
    const RC<SubBuffer> &GetBuffer() const;

private:

    friend class AccelerationStructureManager;

    Tlas() = default;
    
    RC<SubBuffer> buffer_;
};

class BlasPrebuildInfo
{
public:

    const RHI::BlasPrebuildInfoPtr &GetRHIObject() const { return info_; }

    size_t GetAccelerationStructureBufferSize() const { return info_->GetPrebuildInfo().accelerationStructureSize; }
    size_t GetUpdateScratchBufferSize()         const { return info_->GetPrebuildInfo().updateScratchSize; }
    size_t GetBuildScratchBufferSize()          const { return info_->GetPrebuildInfo().buildScratchSize; }

private:

    friend class AccelerationStructureManager;

    RHI::BlasPrebuildInfoPtr info_;
};

class TlasPrebuildInfo
{
public:

    const RHI::TlasPrebuildInfoPtr &GetRHIObject() const { return info_; }

    size_t GetAccelerationStructureBufferSize() const { return info_->GetPrebuildInfo().accelerationStructureSize; }
    size_t GetUpdateScratchBufferSize()         const { return info_->GetPrebuildInfo().updateScratchSize; }
    size_t GetBuildScratchBufferSize()          const { return info_->GetPrebuildInfo().buildScratchSize; }

private:

    friend class AccelerationStructureManager;

    RHI::TlasPrebuildInfoPtr info_;
};

class AccelerationStructureManager : public GeneralGPUObjectManager
{
public:

    AccelerationStructureManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    BlasPrebuildInfo CreateBlasPrebuildinfo(
        Span<RHI::RayTracingGeometryDesc>              geometries,
        RHI::RayTracingAccelerationStructureBuildFlags flags);
    TlasPrebuildInfo CreateTlasPrebuildInfo(
        const RHI::RayTracingInstanceArrayDesc        &instances,
        RHI::RayTracingAccelerationStructureBuildFlags flags);

    RC<Blas> CreateBlas(RC<SubBuffer> buffer = nullptr);
    RC<Tlas> CreateTlas(RC<SubBuffer> buffer = nullptr);

    void _internalCreate(Blas &blas);
    void _internalCreate(Tlas &tlas);

private:

    RHI::DeviceOPtr rhiDevice_;
};

RTRC_END