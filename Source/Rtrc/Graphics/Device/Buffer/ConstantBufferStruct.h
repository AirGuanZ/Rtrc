#pragma once

#include <Rtrc/Graphics/Device/Buffer/ReflectedConstantBufferStruct.h>
#include <Rtrc/Math/Matrix4x4.h>
#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utility/TypeList.h>

RTRC_BEGIN

namespace ConstantBufferDetail
{

    template<typename T>
    struct ArrayTrait;

    template<typename T, size_t N>
    struct ArrayTrait<T[N]>
    {
        using Element = T;
        static constexpr size_t Size = N;
    };

    constexpr size_t UpAlignTo4(size_t x)
    {
        return ((x + 3) >> 2) << 2;
    }

    using BasicTypeList = TypeList<
        float, Vector2f, Vector3f, Vector4f,
        int32_t, Vector2i, Vector3i, Vector4i,
        uint32_t, Vector2u, Vector3u, Vector4u,
        Matrix4x4f>;

    template<typename T>
    size_t GetConstantBufferDWordCount()
    {
        if constexpr(std::is_array_v<T>)
        {
            using Element = typename ArrayTrait<T>::Element;
            constexpr size_t Size = ArrayTrait<T>::Size;
            constexpr size_t elemSize = GetConstantBufferDWordCount<Element>();
            return Size * UpAlignTo4(elemSize);
        }
        else if constexpr(RtrcReflStruct<T>)
        {
            return ReflectedConstantBufferStruct::GetDeviceDWordCount<T>();
        }
        else
        {
            static_assert(BasicTypeList::Contains<T>, "Invalid value type in constant buffer struct");
            return sizeof(T) / 4;
        }
    }
    
    template<typename T>
    void FlattenToConstantBufferData(const void *input, void *output, size_t hostDWordOffset, size_t deviceDWordOffset)
    {
        if constexpr(std::is_array_v<T>)
        {
            assert(deviceDWordOffset % 4 == 0);
            using Element = typename ArrayTrait<T>::Element;
            const size_t elemSize = ConstantBufferDetail::UpAlignTo4(
                ConstantBufferDetail::GetConstantBufferDWordCount<Element>());
            constexpr size_t elemCount = ArrayTrait<T>::Size;
            for(size_t i = 0; i < elemCount; ++i)
            {
                FlattenToConstantBufferData<Element>(input, output, hostDWordOffset, deviceDWordOffset);
                hostDWordOffset += sizeof(Element) / 4;
                deviceDWordOffset += elemSize;
            }
        }
        else if constexpr(RtrcReflStruct<T>)
        {
            ReflectedConstantBufferStruct::ToDeviceLayout<T>(
                input, output, hostDWordOffset * 4, deviceDWordOffset * 4);
        }
        else
        {
            static_assert(BasicTypeList::Contains<T>);
            std::memcpy(
                static_cast<char *>(output) + deviceDWordOffset * 4,
                static_cast<const char *>(input) + hostDWordOffset * 4,
                sizeof(T));
        }
    }
    
} // namespace ConstantBufferDetail

RTRC_END
