#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ComputeEntry.h>

RTRC_EDSL_BEGIN

namespace ComputeEntryDetail
{

    RC<Sampler> CreateSampler(Ref<Device> device, const RHI::SamplerDesc &desc)
    {
        return device->CreateSampler(desc);
    }

    RHI::BackendType GetBackendType(Ref<Device> device)
    {
        return device->GetBackendType();
    }

} // namespace ComputeEntryDetail

RTRC_EDSL_END
