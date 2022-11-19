#pragma once

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>

RTRC_BEGIN

template<typename T>
class TBufferView
{
public:

    TBufferView() = default;
    TBufferView(RC<Buffer> buffer, RHI::Format format);
    TBufferView(RC<Buffer> buffer, size_t structStride);
    TBufferView(RC<Buffer> buffer, RHI::Format format, size_t structStride);

    const T &GetRHIObject() const;

private:

    static_assert(std::is_same_v<T, RHI::BufferSRVPtr> || std::is_same_v<T, RHI::BufferUAVPtr>);

    RC<Buffer> buffer_;
    T view_;
};

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, RHI::Format format)
    : TBufferView(std::move(buffer), format, 0)
{
    
}

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, size_t structStride)
    : TBufferView(std::move(buffer), RHI::Format::Unknown, structStride)
{
    
}

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, RHI::Format format, size_t structStride)
    : buffer_(std::move(buffer))
{
    assert(format != RHI::Format::Unknown || structStride != 0);
    const RHI::BufferSRVDesc desc =
    {
        .format = format,
        .offset = 0,
        .range = static_cast<uint32_t>(buffer_->GetSize()),
        .stride = static_cast<uint32_t>(structStride)
    };
    if constexpr(std::is_same_v<T, RHI::BufferSRVPtr>)
    {
        view_ = buffer_->GetRHIObject()->CreateSRV(desc);
    }
    else
    {
        view_ = buffer_->GetRHIObject()->CreateUAV(desc);
    }
}

template<typename T>
const T &TBufferView<T>::GetRHIObject() const
{
    return view_;
}

inline BufferSRV Buffer::GetSRV()
{
    return BufferSRV(shared_from_this(), defaultViewTexelFormat_, defaultViewStructStride_);
}

inline TBufferView<RHI::BufferSRVPtr> Buffer::GetSRV(RHI::Format texelFormat)
{
    return BufferSRV(shared_from_this(), texelFormat);
}

inline TBufferView<RHI::BufferSRVPtr> Buffer::GetSRV(size_t structStride)
{
    return BufferSRV(shared_from_this(), structStride);
}

inline BufferUAV Buffer::GetUAV()
{
    return BufferUAV(shared_from_this(), defaultViewTexelFormat_, defaultViewStructStride_);
}

inline TBufferView<RHI::BufferUAVPtr> Buffer::GetUAV(RHI::Format texelFormat)
{
    return BufferUAV(shared_from_this(), texelFormat);
}

inline TBufferView<RHI::BufferUAVPtr> Buffer::GetUAV(size_t structStride)
{
    return BufferUAV(shared_from_this(), structStride);
}

RTRC_END
