#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class ClearBufferUtils
{
public:

    explicit ClearBufferUtils(Ref<Device> device);

    void ClearRWBuffer(
        CommandBuffer       &commandBuffer,
        const RC<SubBuffer> &buffer,
        uint32_t             value);

    void ClearRWStructuredBuffer(
        CommandBuffer       &commandBuffer,
        const RC<SubBuffer> &buffer,
        uint32_t             value);

private:

    Ref<Device> device_;

    RC<Shader>             clearRWBufferShader_;
    RC<BindingGroupLayout> clearRWBufferBindingGroupLayout_;

    RC<Shader>             clearRWStructuredBufferShader_;
    RC<BindingGroupLayout> clearRWStructuredBufferBindingGroupLayout_;
};

RTRC_END
