#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/Utility/ClearBufferUtils.h>
#include <Rtrc/Graphics/Shader/StandaloneCompiler.h>

RTRC_BEGIN

namespace ClearBufferUtilsDetail
{

    const char *SHADER_SOURCE_RWBUFFER = R"___(
rtrc_comp(CSMain)
rtrc_group(Pass)
{
    rtrc_define(RWBuffer<uint>, Out)
    rtrc_uniform(uint, firstElement)
    rtrc_uniform(uint, threadCount)
    rtrc_uniform(uint, value)
};
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid < Pass.threadCount)
        Out[Pass.firstElement + tid] = Pass.value;
}
)___";

    rtrc_group(BindingGroup_RWBuffer_Pass)
    {
        rtrc_define(RWBuffer, Out);
        rtrc_uniform(uint, firstElement);
        rtrc_uniform(uint, threadCount);
        rtrc_uniform(uint, value);
    };

    const char *SHADER_SOURCE_RWSTRUCTURED_BUFFER = R"___(
rtrc_comp(CSMain)
rtrc_group(Pass)
{
    rtrc_define(RWStructuredBuffer<uint>, Out)
    rtrc_uniform(uint, firstElement)
    rtrc_uniform(uint, threadCount)
    rtrc_uniform(uint, value)
};
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid < Pass.threadCount)
        Out[Pass.firstElement + tid] = Pass.value;
}
)___";

    rtrc_group(BindingGroup_RWStructuredBuffer_Pass)
    {
        rtrc_define(RWStructuredBuffer, Out);
        rtrc_uniform(uint, firstElement);
        rtrc_uniform(uint, threadCount);
        rtrc_uniform(uint, value);
    };

} // namespace ClearBufferUtilsDetail

ClearBufferUtils::ClearBufferUtils(Ref<Device> device)
    : device_(device)
{
    StandaloneShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device);
    
    clearRWBufferShader_ = shaderCompiler.Compile(
        ClearBufferUtilsDetail::SHADER_SOURCE_RWBUFFER, {}, RTRC_DEBUG);
    clearRWBufferBindingGroupLayout_ = clearRWBufferShader_->GetBindingGroupLayoutByIndex(0);
    
    clearRWStructuredBufferShader_ = shaderCompiler.Compile(
        ClearBufferUtilsDetail::SHADER_SOURCE_RWSTRUCTURED_BUFFER, {}, RTRC_DEBUG);
    clearRWStructuredBufferBindingGroupLayout_ = clearRWStructuredBufferShader_->GetBindingGroupLayoutByIndex(0);
}

void ClearBufferUtils::ClearRWBuffer(CommandBuffer &commandBuffer, const RC<SubBuffer> &buffer, uint32_t value)
{
    using namespace ClearBufferUtilsDetail;
    assert(buffer->GetSubBufferSize() % sizeof(uint32_t) == 0);
    assert(buffer->GetFullBuffer()->GetDesc().usage.Contains(RHI::BufferUsage::ShaderRWBuffer));

    commandBuffer.BindComputePipeline(clearRWBufferShader_->GetComputePipeline());

    auto uav = buffer->GetTexelUav(RHI::Format::R32_UInt);

    const uint32_t maxGroupCount = device_->GetRawDevice()->GetComputeShaderDispatchLimit().maxThreadGroupCountX;
    const uint32_t maxThreadCountPerDispatch = maxGroupCount * clearRWBufferShader_->GetThreadGroupSize().x;
    const uint32_t totalElementCount = buffer->GetSubBufferSize() / sizeof(uint32_t);
    
    uint32_t startElement = 0;
    while(startElement < totalElementCount)
    {
        const uint32_t endElement = (std::min)(totalElementCount, startElement + maxThreadCountPerDispatch);
        
        BindingGroup_RWBuffer_Pass passData;
        passData.Out          = uav;
        passData.firstElement = startElement;
        passData.threadCount  = endElement - startElement;
        passData.value        = value;
        auto passGroup = device_->CreateBindingGroup(passData, clearRWBufferBindingGroupLayout_);

        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(passData.threadCount);

        startElement = endElement;
    }
}

void ClearBufferUtils::ClearRWStructuredBuffer(CommandBuffer &commandBuffer, const RC<SubBuffer> &buffer, uint32_t value)
{
    using namespace ClearBufferUtilsDetail;
    assert(buffer->GetSubBufferSize() % sizeof(uint32_t) == 0);
    assert(buffer->GetFullBuffer()->GetDesc().usage.Contains(RHI::BufferUsage::ShaderRWStructuredBuffer));

    commandBuffer.BindComputePipeline(clearRWStructuredBufferShader_->GetComputePipeline());

    auto uav = buffer->GetStructuredUav(sizeof(uint32_t));

    const uint32_t maxGroupCount = device_->GetRawDevice()->GetComputeShaderDispatchLimit().maxThreadGroupCountX;
    const uint32_t maxThreadCountPerDispatch = maxGroupCount * clearRWBufferShader_->GetThreadGroupSize().x;
    const uint32_t totalElementCount = buffer->GetSubBufferSize() / sizeof(uint32_t);

    uint32_t startElement = 0;
    while(startElement < totalElementCount)
    {
        const uint32_t endElement = (std::min)(totalElementCount, startElement + maxThreadCountPerDispatch);

        BindingGroup_RWStructuredBuffer_Pass passData;
        passData.Out          = uav;
        passData.firstElement = startElement;
        passData.threadCount  = endElement - startElement;
        passData.value        = value;
        auto passGroup = device_->CreateBindingGroup(passData, clearRWStructuredBufferBindingGroupLayout_);

        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(passData.threadCount);

        startElement = endElement;
    }
}

RTRC_END
