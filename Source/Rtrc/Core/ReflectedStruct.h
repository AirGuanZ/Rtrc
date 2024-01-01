#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

namespace ReflectedStruct
{

    template<typename T>
    size_t GetDeviceDWordCount();

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData);

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData, size_t initHostOffset, size_t initDeviceOffset);
    
    template<typename T>
    void DumpDeviceLayout();

    const char *GetGeneratedFilePath();

} // namespace ReflectedStruct

RTRC_END
