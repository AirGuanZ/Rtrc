#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

namespace ReflectedConstantBufferStruct
{

    template<typename T>
    size_t GetDeviceDWordCount();

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData);

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData, size_t initHostOffset, size_t initDeviceOffset);
    
    template<typename T>
    void DumpDeviceLayout();

} // namespace ReflectedConstantBufferStruct

RTRC_END
