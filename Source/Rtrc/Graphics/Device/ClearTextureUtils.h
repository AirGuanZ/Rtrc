#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class ClearTextureUtils
{
public:

    explicit ClearTextureUtils(ObserverPtr<Device> device);

    void ClearRWTexture2D(CommandBuffer &commandBuffer, const RC<Texture> &texture, const Vector4f &value);

private:

    ObserverPtr<Device> device_;
    RC<Shader> clearShader_;
};

RTRC_END
