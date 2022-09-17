#pragma once

#include <map>

#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utils/Struct.h>
#include <Rtrc/Utils/TypeList.h>

RTRC_BEGIN

namespace CBDetail
{

    template<typename T>
    concept ConstantBufferStruct = requires { typename T::_rtrcCBufferTypeFlag; };

} // namespace CBDetail

#define cbuffer_begin(NAME) RTRC_META_STRUCT_BEGIN(NAME) struct _rtrcCBufferTypeFlag{};
#define cbuffer_end() RTRC_META_STRUCT_END()
#define cbuffer_var(TYPE, NAME)                        \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                   \
        f.template operator()(&_rtrcSelf::NAME, #NAME); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                  \
    using _rtrcMemberType##NAME = TYPE;                 \
    _rtrcMemberType##NAME NAME;

class ConstantBuffer : public Uncopyable
{
public:

    template<CBDetail::ConstantBufferStruct T>
    static constexpr size_t CalculateSize();
    template<CBDetail::ConstantBufferStruct T, typename F>
    static constexpr void ForEachFlattenMember(const F &f);

    ConstantBuffer();
    ConstantBuffer(RHI::BufferPtr buffer, unsigned char *mappedPtr, size_t offset, size_t size);

    void SetData(const void *data, size_t size);

    template<CBDetail::ConstantBufferStruct T>
    void SetData(const T &data);

    const RHI::BufferPtr &GetBuffer() const;
    size_t GetOffset() const;
    size_t GetSize() const;

private:

    RHI::BufferPtr buffer_;
    unsigned char *mappedPtr_;
    size_t offset_;
    size_t size_;
};

class FrameConstantBufferAllocator : public Uncopyable
{
public:

    FrameConstantBufferAllocator(RHI::DevicePtr device, int frameCount, size_t chunkSize = 2 * 1024 * 1024);
    ~FrameConstantBufferAllocator();

    void BeginFrame();

    Box<ConstantBuffer> AllocateConstantBuffer(size_t size);

    template<CBDetail::ConstantBufferStruct T>
    Box<ConstantBuffer> AllocateConstantBuffer();

private:

    struct FrameRecord
    {
        struct BufferRecord
        {
            RHI::BufferPtr buffer;
            size_t nextOffset = 0;
            unsigned char *mappedPtr = nullptr;
        };
        std::multimap<size_t, BufferRecord> availableSizeToBufferRecord;
    };

    RHI::DevicePtr device_;
    size_t chunkSize_;
    size_t alignment_;
    int frameCount_;
    int frameIndex_;
    std::vector<FrameRecord> frameRecords_;
};

template <CBDetail::ConstantBufferStruct T>
void ConstantBuffer::SetData(const T &data)
{
    constexpr size_t deviceSize = ConstantBuffer::CalculateSize<T>();
    assert(deviceSize <= size_);
    ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
    {
        std::memcpy(mappedPtr_ + deviceOffset, reinterpret_cast<const unsigned char *>(&data) + hostOffset, sizeof(M));
    });
    buffer_->FlushAfterWrite(offset_, deviceSize);
}

template <CBDetail::ConstantBufferStruct T>
Box<ConstantBuffer> FrameConstantBufferAllocator::AllocateConstantBuffer()
{
    return this->AllocateConstantBuffer(ConstantBuffer::CalculateSize<T>());
}

namespace CBDetail
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
        return (x + 3) / 4 * 4;
    }

    template<typename T>
    consteval size_t GetConstantBufferDWordCount()
    {
        if constexpr(std::is_array_v<T>)
        {
            return ArrayTrait<T>::Size * UpAlignTo4(GetConstantBufferDWordCount<typename ArrayTrait<T>::Element>());
        }
        else if constexpr(ConstantBufferStruct<T>)
        {
            size_t result = 0;
            T::ForEachMember([&result]<typename M>(const M T::*, const char *) constexpr
            {
                const size_t memberSize = GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || ConstantBufferStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = memberSize >= 4 || (result % 4) + memberSize > 4;
                }
                if(needNewLine)
                {
                    result = UpAlignTo4(result);
                }
                result += memberSize;
            });
            return UpAlignTo4(result);
        }
        else
        {
            using ValidTypeList = TypeList<
                float,    Vector2f, Vector3f, Vector4f,
                int32_t,  Vector2i, Vector3i, Vector4i,
                uint32_t, Vector2u, Vector3u, Vector4u>;
            static_assert(ValidTypeList::Contains<T>);
            return sizeof(T) / 4;
        }
    }

    // f.template operator()<MemberType>(name, byteOffsetInHost, byteOffsetInDevice)
    template<typename T, typename F>
    constexpr void ForEachFlattenMember(const char *name, const F &f, size_t hostOffset, size_t deviceOffset)
    {
        if constexpr(std::is_array_v<T>)
        {
            // assert(deviceOffset % 4 == 0);
            using Element = typename ArrayTrait<T>::Element;
            static_assert(sizeof(Element) % 4 == 0);
            static_assert(alignof(Element) <= 4);
            const size_t elemSize = UpAlignTo4(GetConstantBufferDWordCount<Element>());
            const size_t elemCount = ArrayTrait<T>::Size;
            for(size_t i = 0; i < elemCount; ++i)
            {
                ForEachFlattenMember<Element, F>(name, f, hostOffset, deviceOffset);
                hostOffset += sizeof(Element) / 4;
                deviceOffset += elemSize;
            }
        }
        else if constexpr(ConstantBufferStruct<T>)
        {
            // assert(deviceOffset % 4 == 0)
            T::ForEachMember([&deviceOffset, &hostOffset, &f]<typename M>(
                const M T::*, const char *memberName) constexpr
            {
                const size_t memberSize = GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || ConstantBufferStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = memberSize >= 4 || (deviceOffset % 4) + memberSize > 4;
                }
                if(needNewLine)
                {
                    deviceOffset = UpAlignTo4(deviceOffset);
                }
                ForEachFlattenMember<M, F>(memberName, f, hostOffset, deviceOffset);
                static_assert(sizeof(M) % 4 == 0);
                static_assert(alignof(M) <= 4);
                hostOffset += sizeof(M) / 4;
                deviceOffset += memberSize;
            });
        }
        else
        {
            using ValidTypeList = TypeList<
                float, Vector2f, Vector3f, Vector4f,
                int32_t, Vector2i, Vector3i, Vector4i,
                uint32_t, Vector2u, Vector3u, Vector4u>;
            static_assert(ValidTypeList::Contains<T>);
            f.template operator()<T>(name, hostOffset * 4, deviceOffset * 4);
        }
    }

} // namespace CBDetail

template<CBDetail::ConstantBufferStruct T>
constexpr size_t ConstantBuffer::CalculateSize()
{
    return CBDetail::GetConstantBufferDWordCount<T>() * 4;
}

template<CBDetail::ConstantBufferStruct T, typename F>
constexpr void ConstantBuffer::ForEachFlattenMember(const F &f)
{
    CBDetail::ForEachFlattenMember<T>("struct", f, 0, 0);
}

RTRC_END
