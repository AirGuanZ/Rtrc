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

    static_assert(std::is_same_v<T, RHI::BufferSrvPtr> || std::is_same_v<T, RHI::BufferUavPtr>);

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
    const RHI::BufferSrvDesc desc =
    {
        .format = format,
        .offset = 0,
        .range = static_cast<uint32_t>(buffer_->GetSize()),
        .stride = static_cast<uint32_t>(structStride)
    };
    if constexpr(std::is_same_v<T, RHI::BufferSrvPtr>)
    {
        view_ = buffer_->GetRHIObject()->CreateSrv(desc);
    }
    else
    {
        view_ = buffer_->GetRHIObject()->CreateUav(desc);
    }
}

template<typename T>
const T &TBufferView<T>::GetRHIObject() const
{
    return view_;
}

inline BufferSrv Buffer::GetSrv()
{
    return BufferSrv(shared_from_this(), defaultViewTexelFormat_, defaultViewStructStride_);
}

inline TBufferView<RHI::BufferSrvPtr> Buffer::GetSrv(RHI::Format texelFormat)
{
    return BufferSrv(shared_from_this(), texelFormat);
}

inline TBufferView<RHI::BufferSrvPtr> Buffer::GetSrv(size_t structStride)
{
    return BufferSrv(shared_from_this(), structStride);
}

inline BufferUav Buffer::GetUav()
{
    return BufferUav(shared_from_this(), defaultViewTexelFormat_, defaultViewStructStride_);
}

inline TBufferView<RHI::BufferUavPtr> Buffer::GetUav(RHI::Format texelFormat)
{
    return BufferUav(shared_from_this(), texelFormat);
}

inline TBufferView<RHI::BufferUavPtr> Buffer::GetUav(size_t structStride)
{
    return BufferUav(shared_from_this(), structStride);
}

RTRC_END
