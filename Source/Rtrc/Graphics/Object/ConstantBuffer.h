#pragma once

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>
#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utils/SlotVector.h>
#include <Rtrc/Utils/Struct.h>
#include <Rtrc/Utils/TypeList.h>

RTRC_BEGIN

class ConstantBufferManager;
class HostSynchronizer;

namespace ConstantBufferDetail
{

    template<typename T>
    concept ConstantBufferStruct = requires { typename T::_rtrcCBufferTypeFlag; };

} // namespace ConstantBufferDetail

/*
    Usage:
    
        ConstantBuffer cb = mgr.Create();
        cb.SetData(...);
        // ...use cb
        cb.SetData(...);
        // ...use cb
*/
class ConstantBuffer : public Uncopyable
{
public:

    ConstantBuffer();
    ~ConstantBuffer();

    ConstantBuffer(ConstantBuffer &&other) noexcept;
    ConstantBuffer &operator=(ConstantBuffer &&other) noexcept;

    void Swap(ConstantBuffer &other) noexcept;

    template<ConstantBufferDetail::ConstantBufferStruct T>
    void SetData(const T &data);
    void SetData(const void *data, size_t size);

    const RHI::BufferPtr &GetBuffer() const;
    size_t GetOffset() const;
    size_t GetSize() const;

private:

    friend class ConstantBufferManager;

    ConstantBufferManager *manager_;

    RHI::BufferPtr rhiBuffer_;
    size_t offset_;
    size_t size_;
    int chunkIndex_;
    int slabIndex_;
};

/*
    Thread Safety:

        TS(1):
            Create
            ConstantBuffer::XXX
*/
class ConstantBufferManager : public Uncopyable
{
public:

    ConstantBufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device, size_t chunkSize = 4 * 1024 * 1024);
    ~ConstantBufferManager();

    RC<ConstantBuffer> Create();

    void _rtrcSetDataInternal(ConstantBuffer &buffer, const void *data, size_t size);
    void _rtrcFreeInternal(ConstantBuffer &buffer);

private:

    struct BatchReleaseData
    {
        int slabIndex;
        int chunkIndex;
        size_t offset;
    };

    struct Chunk
    {
        RHI::BufferPtr buffer;
        void *mappedPtr = nullptr;
    };

    struct Record
    {
        int chunkIndex = 0;
        size_t offset = 0;
    };

    struct Slab
    {
        std::vector<Record> freeBuffers;
        tbb::spin_mutex mutex;
    };

    void ComputeSlabIndex(size_t size, int &slabIndex, size_t &allocateSize) const;

    BatchReleaseHelper<BatchReleaseData> batchRelease_;
    RHI::DevicePtr device_;

    size_t chunkSize_;
    SlotVector<Chunk> chunks_;
    tbb::spin_rw_mutex chunkMutex_;

    // slabs_[i] stores records of size (cbufferAlignment_ << i)
    size_t cbufferAlignment_;
    std::unique_ptr<Slab[]> slabs_;
};

#define $cbuffer(NAME)                                                 \
    struct NAME;                                                       \
    struct _rtrcCBufferBase##NAME                                      \
    {                                                                  \
        using _rtrcSelf = NAME;                                        \
        static constexpr std::string_view _rtrcSelfName = #NAME;       \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...); \
        template<typename F>                                           \
        static constexpr void ForEachMember(const F &f)                \
        {                                                              \
            ::Rtrc::StructDetail::ForEachMember<_rtrcSelf>(f);         \
        }                                                              \
    };                                                                 \
    struct NAME : _rtrcCBufferBase##NAME

#define $var(TYPE, NAME)                                \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                   \
        f.template operator()(&_rtrcSelf::NAME, #NAME); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                  \
    using _rtrcMemberType##NAME = TYPE;                 \
    _rtrcMemberType##NAME NAME;

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
            using Elem = typename ArrayTrait<T>::Element;
            constexpr size_t Size = ArrayTrait<T>::Size;
            constexpr size_t elemSize = GetConstantBufferDWordCount<Elem>();
            return Size * UpAlignTo4(elemSize);
        }
        else if constexpr(ConstantBufferStruct<T>)
        {
            size_t result = 0;
            T::ForEachMember([&result]<typename M>(const M T::*, const char *) constexpr
            {
                constexpr size_t memSize = GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || ConstantBufferStruct<M>)
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
                uint32_t, Vector2u, Vector3u, Vector4u>;
            static_assert(ValidTypeList::Contains<T>, "Invalid value type in constant buffer struct");
            return sizeof(T) / 4;
        }
    }

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
                    needNewLine = (deviceOffset % 4) + memberSize > 4;
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
            static_assert(ValidTypeList::Contains<T>, "Invalid value type in constant buffer struct");
            f.template operator() < T > (name, hostOffset * 4, deviceOffset * 4);
        }
    }

    template<ConstantBufferStruct T, typename F>
    void ForEachFlattenMember(const F &f)
    {
        ForEachFlattenMember<T>("struct", f, 0, 0);
    }

} // namespace ConstantBufferDetail

template<ConstantBufferDetail::ConstantBufferStruct T>
void ConstantBuffer::SetData(const T &data)
{
    constexpr size_t deviceSize = ConstantBufferDetail::GetConstantBufferDWordCount<T>() * 4;
    assert(manager_);
    std::vector<unsigned char> deviceData(deviceSize);
    ConstantBufferDetail::ForEachFlattenMember<T>([&]<typename M>(const char *, size_t hostOffset, size_t deviceOffset)
    {
        auto dst = deviceData.data() + deviceOffset;
        auto src = reinterpret_cast<const unsigned char *>(&data) + hostOffset;
        std::memcpy(dst, src, sizeof(M));
    });
    manager_->_rtrcSetDataInternal(*this, deviceData.data(), deviceSize);
}

RTRC_END
