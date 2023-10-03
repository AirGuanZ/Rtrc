#if !RTRC_REFLECTION_TOOL

#include <Core/ReflectedStruct.h>
#include <Core/TypeList.h>

#include <Generated/Reflection.h>

RTRC_BEGIN

namespace ReflectedStruct
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
        float,    Vector2f, Vector3f, Vector4f,
        int32_t,  Vector2i, Vector3i, Vector4i,
        uint32_t, Vector2u, Vector3u, Vector4u,
        Matrix4x4f>;

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
            static_assert(std::is_class_v<T>, "Invalid constant buffer struct");
            size_t result = 0;
            Reflection<T>::ForEachNonStaticMemberVariable([&result]<typename M>(const M T::*, const char *) constexpr
            {
                constexpr size_t memSize = GetDeviceDWordCountImpl<M>();
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

    // f(name, hostOffset, deviceOffset)
    template<typename T, typename F>
    constexpr void ForEachFlattenMember(const char* name, const F &f, size_t hostDWordOffset, size_t deviceDWordOffset)
    {
        static_assert(sizeof(T) % 4 == 0);
        static_assert(alignof(T) <= 4);
        if constexpr(std::is_array_v<T>)
        {
            using Element = typename ArrayTrait<T>::Element;
            constexpr size_t elemSize = GetDeviceDWordCountImpl<Element>();
            constexpr size_t elemCount = ArrayTrait<T>::Size;
            for(size_t i = 0; i < elemCount; ++i)
            {
                ForEachFlattenMember<Element, F>(name, f, hostDWordOffset, deviceDWordOffset);
                hostDWordOffset += sizeof(Element) / 4;
                deviceDWordOffset += UpAlignTo4(elemSize);
            }
        }
        else if constexpr(BasicTypes::Contains<T>)
        {
            f.template operator()<T>(name, hostDWordOffset * 4, deviceDWordOffset * 4);
        }
        else
        {
            static_assert(std::is_class_v<T>, "Invalid constant buffer struct");
            Reflection<T>::ForEachNonStaticMemberVariable([&deviceDWordOffset, &hostDWordOffset, &f]
                <typename M>(const M T::*, const char *name) constexpr
            {
                const size_t memberSize = GetDeviceDWordCountImpl<M>();
                bool needNewLine;
                if constexpr(BasicTypes::Contains<M>)
                {
                    needNewLine = (deviceDWordOffset % 4) + memberSize > 4;
                }
                else
                {
                    needNewLine = true;
                }
                if(needNewLine)
                {
                    deviceDWordOffset = UpAlignTo4(deviceDWordOffset);
                }
                ForEachFlattenMember<M, F>(name, f, hostDWordOffset, deviceDWordOffset);
                hostDWordOffset += sizeof(M) / 4;
                deviceDWordOffset += memberSize;
            });
        }
    }

    template<typename T>
    void ToDeviceLayoutImpl(const void *hostData, void *deviceData, size_t initHostDWordOffset, size_t initDeviceDWordOffset)
    {
        auto f = [&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
        {
            auto src = static_cast<const unsigned char*>(hostData) + hostOffset;
            auto dst = static_cast<unsigned char*>(deviceData) + deviceOffset;
            std::memcpy(dst, src, sizeof(M));
        };
        ForEachFlattenMember<T>("root", f, initHostDWordOffset, initDeviceDWordOffset);
    }

    template<typename T>
    size_t ComputeHashImpl(const void* hostData)
    {
        std::vector<unsigned char> data;
        data.reserve(sizeof(T));
        auto f = [&]<typename M>(const char *, size_t hostOffset, size_t)
        {
            auto src = static_cast<const unsigned char *>(hostData) + hostOffset;
            const size_t dstOffset = data.size();
            data.resize(data.size() + sizeof(M));
            std::memcpy(data.data() + dstOffset, src, sizeof(M));
        };
        return Rtrc::HashMemory(data.data(), data.size());
    }

    template<typename T>
    void DumpDeviceLayoutImpl()
    {
        auto PrintFlattenMember = [&]<typename M>(const char *name, size_t hostOffset, size_t deviceOffset)
        {
            fmt::print("name = {}, hostOffset = {}, deviceOffset = {}\n", name, hostOffset, deviceOffset);
        };
        ForEachFlattenMember<T>("root", PrintFlattenMember, 0, 0);
    }

    template<typename T>
    size_t GetDeviceDWordCount()
    {
        return GetDeviceDWordCountImpl<MirrorType<T>>();
    }

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData)
    {
        ToDeviceLayoutImpl<MirrorType<T>>(hostData, deviceData, 0, 0);
    }

    template<typename T>
    void ToDeviceLayout(const void *hostData, void *deviceData, size_t initHostOffset, size_t initDeviceOffset)
    {
        assert(initHostOffset % 4 == 0);
        assert(initDeviceOffset % 4 == 0);
        ToDeviceLayoutImpl<MirrorType<T>>(hostData, deviceData, initHostOffset / 4, initDeviceOffset / 4);
    }

    template<typename T>
    void DumpDeviceLayout()
    {
        DumpDeviceLayoutImpl<MirrorType<T>>();
    }

    const char *GetGeneratedFilePath()
    {
        return
#include <Generated/ReflectionPath.inc>
            ;
    }

#define TRAVERSE_rtrc(TYPE)
#define TRAVERSE_shader(TYPE) template size_t GetDeviceDWordCount<TYPE>();
#include <Generated/ReflectionList.inc>
#undef TRAVERSE_rtrc
#undef TRAVERSE_shader

#define TRAVERSE_rtrc(TYPE)
#define TRAVERSE_shader(TYPE) template void ToDeviceLayout<TYPE>(const void *, void *); \
                              template void ToDeviceLayout<TYPE>(const void *, void *, size_t, size_t);
#include <Generated/ReflectionList.inc>
#undef TRAVERSE_rtrc
#undef TRAVERSE_shader

#define TRAVERSE_rtrc(TYPE)
#define TRAVERSE_shader(TYPE) template void DumpDeviceLayout<TYPE>();
#include <Generated/ReflectionList.inc>
#undef TRAVERSE_rtrc
#undef TRAVERSE_shader

} // namespace ReflectedStruct

RTRC_END

#endif // #if !RTRC_REFLECTION_TOOL
