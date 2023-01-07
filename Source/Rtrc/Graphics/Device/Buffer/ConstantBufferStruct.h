#pragma once

#include <Rtrc/Math/Matrix4x4.h>
#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utility/Struct.h>
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

    template<typename T>
    consteval size_t GetConstantBufferDWordCount()
    {
        if constexpr(std::is_array_v<T>)
        {
            using Element = typename ArrayTrait<T>::Element;
            constexpr size_t Size = ArrayTrait<T>::Size;
            constexpr size_t elemSize = GetConstantBufferDWordCount<Element>();
            return Size * UpAlignTo4(elemSize);
        }
        else if constexpr(RtrcStruct<T>)
        {
            size_t result = 0;
            T::ForEachMember([&result]<typename M>(const M T::*, const char *) constexpr
            {
                constexpr size_t memSize = GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (result % 4) + memSize > 4;
                }
                if(needNewLine)
                {
                    result = UpAlignTo4(result);
                }
                result += memSize;
            });
            return UpAlignTo4(result);
        }
        else
        {
            using ValidTypeList = TypeList<
                float, Vector2f, Vector3f, Vector4f,
                int32_t, Vector2i, Vector3i, Vector4i,
                uint32_t, Vector2u, Vector3u, Vector4u,
                Matrix4x4f>;
            static_assert(ValidTypeList::Contains<T>, "Invalid value type in constant buffer struct");
            return sizeof(T) / 4;
        }
    }

    template<typename T, typename F>
    constexpr void ForEachFlattenMember(const char *name, const F &f, size_t hostDWordOffset, size_t deviceDWordOffset)
    {
        if constexpr(std::is_array_v<T>)
        {
            // assert(deviceDWordOffset % 4 == 0);
            using Element = typename ArrayTrait<T>::Element;
            static_assert(sizeof(Element) % 4 == 0);
            static_assert(alignof(Element) <= 4);
            const size_t elemSize = UpAlignTo4(GetConstantBufferDWordCount<Element>());
            const size_t elemCount = ArrayTrait<T>::Size;
            for(size_t i = 0; i < elemCount; ++i)
            {
                ForEachFlattenMember<Element, F>(name, f, hostDWordOffset, deviceDWordOffset);
                hostDWordOffset += sizeof(Element) / 4;
                deviceDWordOffset += elemSize;
            }
        }
        else if constexpr(RtrcStruct<T>)
        {
            // assert(deviceDWordOffset % 4 == 0)
            T::ForEachMember([&deviceDWordOffset, &hostDWordOffset, &f]<typename M>(
                const M T::*, const char *memberName) constexpr
            {
                const size_t memberSize = GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (deviceDWordOffset % 4) + memberSize > 4;
                }
                if(needNewLine)
                {
                    deviceDWordOffset = UpAlignTo4(deviceDWordOffset);
                }
                ForEachFlattenMember<M, F>(memberName, f, hostDWordOffset, deviceDWordOffset);
                static_assert(sizeof(M) % 4 == 0);
                static_assert(alignof(M) <= 4);
                hostDWordOffset += sizeof(M) / 4;
                deviceDWordOffset += memberSize;
            });
        }
        else
        {
            using ValidTypeList = TypeList<
                float, Vector2f, Vector3f, Vector4f,
                int32_t, Vector2i, Vector3i, Vector4i,
                uint32_t, Vector2u, Vector3u, Vector4u,
                Matrix4x4f>;
            static_assert(ValidTypeList::Contains<T>, "Invalid value type in constant buffer struct");
            f.template operator()<T>(name, hostDWordOffset * 4, deviceDWordOffset * 4);
        }
    }

    template<RtrcStruct T, typename F>
    void ForEachFlattenMember(const F &f)
    {
        ForEachFlattenMember<T>("struct", f, 0, 0);
    }

} // namespace ConstantBufferDetail

template<RtrcStruct T>
constexpr size_t ConstantBufferSize = ConstantBufferDetail::GetConstantBufferDWordCount<T>() * 4;

RTRC_END
