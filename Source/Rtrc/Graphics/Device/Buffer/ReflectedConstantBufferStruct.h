#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

namespace ReflectedConstantBufferStruct
{

    template<typename T>
    size_t GetDeviceDWordCount();

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData);

} // namespace ReflectedConstantBufferStruct

RTRC_END
