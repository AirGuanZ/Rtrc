#pragma once

#include <Graphics/Device/DeviceSynchronizer.h>
#include <RHI/RHI.h>

RTRC_BEGIN

class Buffer;
template<typename T>
class TBufferView;

using BufferSrv = TBufferView<RHI::BufferSrvRPtr>;
using BufferUav = TBufferView<RHI::BufferUavRPtr>;

class BufferManagerInterface;

namespace BufferImpl
{

    struct BufferData
    {
        size_t size_ = 0;
        RHI::BufferRPtr rhiBuffer_;

        RHI::Format defaultViewTexelFormat_ = RHI::Format::Unknown;
        uint32_t defaultViewStructStride_ = 0;

        BufferManagerInterface *manager_ = nullptr;
        void *managerSpecificData_ = nullptr;
    };

} // namespace BufferImpl

class BufferManagerInterface
{
public:

    virtual ~BufferManagerInterface() = default;
    virtual void _internalRelease(Buffer &buffer) = 0;

protected:

    static BufferImpl::BufferData &GetBufferData(Buffer &buffer);
};

class SubBuffer
{
public:

    virtual ~SubBuffer() = default;

    virtual RC<Buffer> GetFullBuffer() = 0;
    virtual size_t GetSubBufferOffset() const = 0;
    virtual size_t GetSubBufferSize() const = 0;

    virtual const RHI::BufferRPtr &GetFullBufferRHIObject();
    RHI::BufferDeviceAddress GetDeviceAddress();

    static RC<SubBuffer> GetSubRange(RC<SubBuffer> buffer, size_t offset, size_t size);
    static RC<SubBuffer> GetSubRange(RC<Buffer> buffer, size_t offset, size_t size);

    BufferSrv GetStructuredSrv();
    BufferSrv GetStructuredSrv(size_t structStride);
    BufferSrv GetStructuredSrv(size_t byteOffset, size_t structStride);

    BufferSrv GetTexelSrv();
    BufferSrv GetTexelSrv(RHI::Format texelFormat);
    BufferSrv GetTexelSrv(size_t byteOffset, RHI::Format texelFormat);

    BufferUav GetStructuredUav();
    BufferUav GetStructuredUav(size_t structStride);
    BufferUav GetStructuredUav(size_t byteOffset, size_t structStride);

    BufferUav GetTexelUav();
    BufferUav GetTexelUav(RHI::Format texelFormat);
    BufferUav GetTexelUav(size_t byteOffset, RHI::Format texelFormat);
};

class Buffer : protected BufferImpl::BufferData, public SubBuffer, public std::enable_shared_from_this<Buffer>
{
public:
    
    ~Buffer() override;

    const RHI::BufferRPtr &GetRHIObject() const;
    void SetName(std::string name);

    size_t GetSize() const;
    const RHI::BufferDesc &GetDesc() const;

    RC<Buffer> GetFullBuffer() override;
    size_t GetSubBufferOffset() const override;
    size_t GetSubBufferSize() const override;

    const RHI::BufferRPtr &GetFullBufferRHIObject() override;

    void SetDefaultTexelFormat(RHI::Format format);
    void SetDefaultStructStride(size_t stride);

    RHI::Format GetDefaultTexelFormat() const;
    size_t GetDefaultStructStride() const;

    void Upload(const void *data, size_t offset, size_t size);
    void Download(void *data, size_t offset, size_t size);

private:

    friend class BufferManagerInterface;
};

class BufferManager : public Uncopyable, public BufferManagerInterface
{
public:

    BufferManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<Buffer> Create(const RHI::BufferDesc &desc);

    void _internalRelease(Buffer &buffer) override;

private:

    RHI::DeviceOPtr device_;
    DeviceSynchronizer &sync_;
};

inline BufferImpl::BufferData &BufferManagerInterface::GetBufferData(Buffer &buffer)
{
    return buffer;
}

inline Buffer::~Buffer()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

inline const RHI::BufferRPtr &Buffer::GetRHIObject() const
{
    return rhiBuffer_;
}

inline void Buffer::SetName(std::string name)
{
    rhiBuffer_->SetName(std::move(name));
}

inline size_t Buffer::GetSize() const
{
    return size_;
}

inline const RHI::BufferDesc &Buffer::GetDesc() const
{
    return rhiBuffer_->GetDesc();
}

inline RC<Buffer> Buffer::GetFullBuffer()
{
    return shared_from_this();
}

inline size_t Buffer::GetSubBufferOffset() const
{
    return 0;
}

inline size_t Buffer::GetSubBufferSize() const
{
    return size_;
}

inline const RHI::BufferRPtr &Buffer::GetFullBufferRHIObject()
{
    return rhiBuffer_;
}

inline void Buffer::SetDefaultTexelFormat(RHI::Format format)
{
    defaultViewTexelFormat_ = format;
}

inline void Buffer::SetDefaultStructStride(size_t stride)
{
    defaultViewStructStride_ = static_cast<uint32_t>(stride);
}

inline RHI::Format Buffer::GetDefaultTexelFormat() const
{
    return defaultViewTexelFormat_;
}

inline size_t Buffer::GetDefaultStructStride() const
{
    return defaultViewStructStride_;
}

inline void Buffer::Upload(const void *data, size_t offset, size_t size)
{
    assert(GetDesc().hostAccessType == RHI::BufferHostAccessType::Upload);
    if(!size)
    {
        return;
    }
    assert(offset + size <= size_);
    void *ptr = rhiBuffer_->Map(offset, size, {}, false);
    std::memcpy(ptr, data, size);
    rhiBuffer_->Unmap(offset, size, true);
}

inline void Buffer::Download(void *data, size_t offset, size_t size)
{
    assert(GetDesc().hostAccessType == RHI::BufferHostAccessType::Readback);
    if(!size)
    {
        return;
    }
    assert(offset + size <= size_);
    const void *ptr = rhiBuffer_->Map(offset, size, { offset, size }, true);
    std::memcpy(data, ptr, size);
    rhiBuffer_->Unmap(offset, size, false);
}

RTRC_END
