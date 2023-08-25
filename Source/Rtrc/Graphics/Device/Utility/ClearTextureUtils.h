#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class ClearTextureUtils
{
public:

    explicit ClearTextureUtils(ObserverPtr<Device> device);

    void ClearRWTexture2D(CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4f &value) const;
    void ClearRWTexture2D(CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4u &value) const;
    void ClearRWTexture2D(CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4i &value) const;

private:

    template<typename ValueType>
    void ClearRWTexture2DImpl(
        const RC<Shader> &shader,
        CommandBuffer    &commandBuffer,
        const TextureUav &uav,
        const ValueType  &value) const;

    ObserverPtr<Device> device_;
    RC<Shader> clearFloat4Shader_;
    RC<Shader> clearFloat2Shader_;
    RC<Shader> clearFloatShader_;
    RC<Shader> clearUIntShader_;
    RC<Shader> clearUNorm4Shader_;
    RC<Shader> clearUNormShader_;
};

RTRC_END
