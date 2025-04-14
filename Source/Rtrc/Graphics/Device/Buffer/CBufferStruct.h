#pragma once

#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/Math/Matrix4x4.h>
#include <Rtrc/Core/Struct.h>
#include <Rtrc/Core/TypeList.h>

RTRC_BEGIN

namespace CBufferStructDetail
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

    using BasicTypes = TypeList<
        float, Vector2f, Vector3f, Vector4f,
        int32_t, Vector2i, Vector3i, Vector4i,
        uint32_t, Vector2u, Vector3u, Vector4u,
        Matrix4x4f>;

    template<typename T, typename F>
    constexpr void ForEachMember(const F &f)
    {
        // call f(member_pointer, member_name)
        StructDetail::ForEachMember<T>(f);
    }

    template<typename T>
    consteval size_t GetDeviceDWordCountImpl()
    {
        if constexpr(std::is_array_v<T>)
        {
            using Element = typename ArrayTrait<T>::Element;
            constexpr size_t Size = ArrayTrait<T>::Size;
            constexpr size_t elemSize = GetDeviceDWordCountImpl<Element>();
            return Size * UpAlignTo4(elemSize);
        }
        else if constexpr(BasicTypes::Contains<T>)
        {
            return sizeof(T) / 4;
        }
        else
        {
            static_assert(RtrcStruct<T>, "Invalid constant buffer struct");
            size_t result = 0;
            CBufferStructDetail::ForEachMember<T>([&result]<typename M>(const M T::*, const char *) constexpr
            {
                constexpr size_t memSize = CBufferStructDetail::GetDeviceDWordCountImpl<M>();
                bool needNewLine;
                if constexpr(BasicTypes::Contains<M>)
                {
                    needNewLine = (result % 4) + memSize > 4;
                }
                else
                {
                    needNewLine = true;
                }
                if(needNewLine)
                {
                    result = UpAlignTo4(result);
                }
                result += memSize;
            });
            return UpAlignTo4(result);
        }
    }

    // call f(name, hostOffset, deviceOffset)
    template<typename T, typename F>
    constexpr void ForEachFlattenMember(const char *name, const F &f, size_t hostByteOffset, size_t deviceByteOffset)
    {
        static_assert(sizeof(T) % 4 == 0);
        static_assert(alignof(T) <= 4);
        if constexpr(std::is_array_v<T>)
        {
            using Element = typename ArrayTrait<T>::Element;
            constexpr size_t elemDWordSize = CBufferStructDetail::GetDeviceDWordCountImpl<Element>();
            constexpr size_t elemCount = ArrayTrait<T>::Size;
            for(size_t i = 0; i < elemCount; ++i)
            {
                CBufferStructDetail::ForEachFlattenMember<Element, F>(name, f, hostByteOffset, deviceByteOffset);
                hostByteOffset += sizeof(Element);
                deviceByteOffset += 4 * UpAlignTo4(elemDWordSize);
            }
        }
        else if constexpr(BasicTypes::Contains<T>)
        {
            f.template operator()<T>(name, hostByteOffset, deviceByteOffset);
        }
        else
        {
            static_assert(RtrcStruct<T>, "Invalid constant buffer struct");
            CBufferStructDetail::ForEachMember<T>([&hostByteOffset, &deviceByteOffset, &f]
                                                  <typename M>(const M T::* ptr, const char *name) constexpr
            {
                const size_t localHostOffset = GetMemberOffset(ptr);
                const size_t memberDWordSize = CBufferStructDetail::GetDeviceDWordCountImpl<M>();
                bool needNewLine;
                if constexpr(BasicTypes::Contains<M>)
                {
                    needNewLine = ((deviceByteOffset / 4) % 4) + memberDWordSize > 4;
                }
                else
                {
                    needNewLine = true;
                }
                if(needNewLine)
                {
                    deviceByteOffset = 4 * UpAlignTo4(deviceByteOffset / 4);
                }
                CBufferStructDetail::ForEachFlattenMember<M, F>(
                    name, f, hostByteOffset + localHostOffset, deviceByteOffset);
                //hostDWordOffset += sizeof(M) / 4;
                deviceByteOffset += 4 * memberDWordSize;
            });
        }
    }

    template<typename T>
    void ToDeviceLayoutImpl(
        const void *hostData, void *deviceData, size_t initHostByteOffset, size_t initDeviceByteOffset)
    {
        auto f = [&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            auto src = static_cast<const unsigned char *>(hostData) + hostOffset;
            auto dst = static_cast<unsigned char *>(deviceData) + deviceOffset;
            std::memcpy(dst, src, sizeof(M));
        };
        CBufferStructDetail::ForEachFlattenMember<T>("root", f, initHostByteOffset, initDeviceByteOffset);
    }

    template<typename T>
    void DumpDeviceLayoutImpl(std::ostream &ostream)
    {
        auto PrintFlattenMember = [&]<typename M>(const char *name, size_t hostOffset, size_t deviceOffset)
        {
            ostream << std::format("name = {}, hostOffset = {}, deviceOffset = {}\n", name, hostOffset, deviceOffset);
        };
        CBufferStructDetail::ForEachFlattenMember<T>("root", PrintFlattenMember, 0, 0);
    }

} // namespace CBufferStructDetail

template<typename T>
consteval size_t GetDeviceDWordCount()
{
    return CBufferStructDetail::GetDeviceDWordCountImpl<T>();
}

template<typename T>
void ToDeviceLayout(const void *hostData, void *deviceData)
{
    return CBufferStructDetail::ToDeviceLayoutImpl<T>(hostData, deviceData, 0, 0);
}

template<typename T>
void ToDeviceLayout(const void *hostData, void *deviceData, size_t initHostOffset, size_t initDeviceOffset)
{
    CBufferStructDetail::ToDeviceLayoutImpl<T>(hostData, deviceData, initHostOffset, initDeviceOffset);
}

template<typename T>
void DumpDeviceLayout(std::ostream &ostream)
{
    CBufferStructDetail::DumpDeviceLayoutImpl<T>(ostream);
}

RTRC_END
