#pragma once

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>

RTRC_BEGIN

template<typename T>
class TBufferView
{
public:

    TBufferView() = default;
    TBufferView(RC<Buffer> buffer, size_t byteOffset, RHI::Format format);
    TBufferView(RC<Buffer> buffer, size_t byteOffset, size_t structStride);
    TBufferView(RC<Buffer> buffer, size_t byteOffset, RHI::Format format, size_t structStride);

    const RC<Buffer> &GetBuffer() const;

    const T &GetRHIObject() const;

    RHI::Format GetFinalFormat() const;

private:

    static_assert(std::is_same_v<T, RHI::BufferSrvPtr> || std::is_same_v<T, RHI::BufferUavPtr>);

    RC<Buffer> buffer_;
    T view_;
};

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, size_t byteOffset, RHI::Format format)
    : TBufferView(std::move(buffer), 0, format, 0)
{
    
}

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, size_t byteOffset, size_t structStride)
    : TBufferView(std::move(buffer), 0, RHI::Format::Unknown, structStride)
{
    
}

template<typename T>
TBufferView<T>::TBufferView(RC<Buffer> buffer, size_t byteOffset, RHI::Format format, size_t structStride)
    : buffer_(std::move(buffer))
{
    assert(format != RHI::Format::Unknown || structStride != 0);
    const RHI::BufferSrvDesc desc =
    {
        .format = format,
        .offset = static_cast<uint32_t>(byteOffset),
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

template<typename T>
RHI::Format TBufferView<T>::GetFinalFormat() const
{
    return GetRHIObject().GetDesc().format;
}

template<typename T>
const RC<Buffer> &TBufferView<T>::GetBuffer() const
{
    return buffer_;
}

inline BufferSrv SubBuffer::GetStructuredSrv()
{
    auto fullBuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    const size_t stride = fullBuffer->GetDefaultStructStride();
    assert(stride > 0);
    return BufferSrv(std::move(fullBuffer), offset, stride);
}

inline BufferSrv SubBuffer::GetStructuredSrv(size_t structStride)
{
    auto fullBuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    return BufferSrv(std::move(fullBuffer), offset, structStride);
}

inline BufferSrv SubBuffer::GetStructuredSrv(size_t byteOffset, size_t structStride)
{
    auto fullBuffer = GetFullBuffer();
    return BufferSrv(std::move(fullBuffer), GetSubBufferOffset() + byteOffset, structStride);
}

inline BufferSrv SubBuffer::GetTexelSrv()
{
    auto fullbuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    const RHI::Format format = fullbuffer->GetDefaultTexelFormat();
    assert(format != RHI::Format::Unknown);
    return BufferSrv(std::move(fullbuffer), offset, format);
}

inline BufferSrv SubBuffer::GetTexelSrv(RHI::Format texelFormat)
{
    auto fullbuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    return BufferSrv(std::move(fullbuffer), offset, texelFormat);
}

inline BufferSrv SubBuffer::GetTexelSrv(size_t byteOffset, RHI::Format texelFormat)
{
    auto fullbuffer = GetFullBuffer();
    return BufferSrv(std::move(fullbuffer), GetSubBufferOffset() + byteOffset, texelFormat);
}

inline BufferUav SubBuffer::GetStructuredUav()
{
    auto fullBuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    const size_t stride = fullBuffer->GetDefaultStructStride();
    assert(stride > 0);
    return BufferUav(std::move(fullBuffer), offset, stride);
}

inline BufferUav SubBuffer::GetStructuredUav(size_t structStride)
{
    auto fullBuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    return BufferUav(std::move(fullBuffer), offset, structStride);
}

inline BufferUav SubBuffer::GetStructuredUav(size_t byteOffset, size_t structStride)
{
    auto fullBuffer = GetFullBuffer();
    return BufferUav(std::move(fullBuffer), GetSubBufferOffset() + byteOffset, structStride);
}

inline BufferUav SubBuffer::GetTexelUav()
{
    auto fullbuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    const RHI::Format format = fullbuffer->GetDefaultTexelFormat();
    assert(format != RHI::Format::Unknown);
    return BufferUav(std::move(fullbuffer), offset, format);
}

inline BufferUav SubBuffer::GetTexelUav(RHI::Format texelFormat)
{
    auto fullbuffer = GetFullBuffer();
    const size_t offset = GetSubBufferOffset();
    return BufferUav(std::move(fullbuffer), offset, texelFormat);
}

inline BufferUav SubBuffer::GetTexelUav(size_t byteOffset, RHI::Format texelFormat)
{
    auto fullbuffer = GetFullBuffer();
    return BufferUav(std::move(fullbuffer), GetSubBufferOffset() + byteOffset, texelFormat);
}

RTRC_END
