#include <Rtrc/Graphics/Device/ClearBufferUtils.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/ShaderCompiler.h>

RTRC_BEGIN

namespace ClearBufferUtilsDetail
{

    const char *SHADER_SOURCE_RWBUFFER = R"___(
#comp CSMain
rtrc_group(Pass)
{
    rtrc_define(RWBuffer<uint>, Out)
    rtrc_uniform(uint, threadCount)
    rtrc_uniform(uint, value)
};
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid < Pass.threadCount)
        Out[tid] = Pass.value;
}
)___";

    rtrc_group(BindingGroup_RWBuffer_Pass)
    {
        rtrc_define(RWBuffer, Out);
        rtrc_uniform(uint, threadCount);
        rtrc_uniform(uint, value);
    };

    const char *SHADER_SOURCE_RWSTRUCTURED_BUFFER = R"___(
#comp CSMain
rtrc_group(Pass)
{
    rtrc_define(RWStructuredBuffer<uint>, Out)
    rtrc_uniform(uint, threadCount)
    rtrc_uniform(uint, value)
};
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid < Pass.threadCount)
        Out[tid] = Pass.value;
}
)___";

    rtrc_group(BindingGroup_RWStructuredBuffer_Pass)
    {
        rtrc_define(RWStructuredBuffer, Out);
        rtrc_uniform(uint, threadCount);
        rtrc_uniform(uint, value);
    };

} // namespace ClearBufferUtilsDetail

ClearBufferUtils::ClearBufferUtils(ObserverPtr<Device> device)
    : device_(device)
{
    ShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device);

    ShaderCompiler::ShaderSource source =
    {
        .source = ClearBufferUtilsDetail::SHADER_SOURCE_RWBUFFER
    };
    clearRWBufferShader_ = shaderCompiler.Compile(source, {}, RTRC_DEBUG);
    clearRWBufferBindingGroupLayout_ = clearRWBufferShader_->GetBindingGroupLayoutByIndex(0);

    source =
    {
        .source = ClearBufferUtilsDetail::SHADER_SOURCE_RWSTRUCTURED_BUFFER
    };
    clearRWStructuredBufferShader_ = shaderCompiler.Compile(source, {}, RTRC_DEBUG);
    clearRWStructuredBufferBindingGroupLayout_ = clearRWStructuredBufferShader_->GetBindingGroupLayoutByIndex(0);
}

void ClearBufferUtils::ClearRWBuffer(CommandBuffer &commandBuffer, const RC<SubBuffer> &buffer, uint32_t value)
{
    using namespace ClearBufferUtilsDetail;
    assert(buffer->GetSubBufferSize() % sizeof(uint32_t) == 0);
    assert(buffer->GetFullBuffer()->GetDesc().usage.Contains(RHI::BufferUsage::ShaderRWBuffer));

    BindingGroup_RWBuffer_Pass passData;
    passData.Out         = buffer->GetTexelUav(RHI::Format::R32_UInt);
    passData.threadCount = buffer->GetSubBufferSize() / sizeof(uint32_t);
    passData.value       = value;
    auto passGroup = device_->CreateBindingGroup(passData, clearRWBufferBindingGroupLayout_);

    commandBuffer.BindComputePipeline(clearRWBufferShader_->GetComputePipeline());
    commandBuffer.BindComputeGroup(0, passGroup);
    commandBuffer.DispatchWithThreadCount(passData.threadCount, 1, 1);
}

void ClearBufferUtils::ClearRWStructuredBuffer(CommandBuffer &commandBuffer, const RC<SubBuffer> &buffer, uint32_t value)
{
    using namespace ClearBufferUtilsDetail;
    assert(buffer->GetSubBufferSize() % sizeof(uint32_t) == 0);
    assert(buffer->GetFullBuffer()->GetDesc().usage.Contains(RHI::BufferUsage::ShaderRWStructuredBuffer));

    BindingGroup_RWStructuredBuffer_Pass passData;
    passData.Out         = buffer->GetStructuredUav(sizeof(uint32_t));
    passData.threadCount = buffer->GetSubBufferSize() / sizeof(uint32_t);
    passData.value       = value;
    auto passGroup = device_->CreateBindingGroup(passData, clearRWStructuredBufferBindingGroupLayout_);

    commandBuffer.BindComputePipeline(clearRWStructuredBufferShader_->GetComputePipeline());
    commandBuffer.BindComputeGroup(0, passGroup);
    commandBuffer.DispatchWithThreadCount(passData.threadCount, 1, 1);
}

RTRC_END
