#pragma once

#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class Buffer;
template<typename T>
class TBufferView;

using BufferSRV = TBufferView<RHI::BufferSRVPtr>;
using BufferUAV = TBufferView<RHI::BufferUAVPtr>;

class BufferManagerInterface;

namespace BufferImpl
{

    struct BufferData
    {
        size_t size_ = 0;
        RHI::BufferPtr rhiBuffer_;

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

    static RC<SubBuffer> GetSubRange(RC<SubBuffer> buffer, size_t offset, size_t size);
    static RC<SubBuffer> GetSubRange(RC<Buffer> buffer, size_t offset, size_t size);
};

class Buffer : protected BufferImpl::BufferData, public SubBuffer, public std::enable_shared_from_this<Buffer>
{
public:
    
    ~Buffer() override;

    const RHI::BufferPtr &GetRHIObject() const;
    void SetName(std::string name);

    size_t GetSize() const;
    const RHI::BufferDesc &GetDesc() const;

    RC<Buffer> GetFullBuffer() override;
    size_t GetSubBufferOffset() const override;
    size_t GetSubBufferSize() const override;

    void SetDefaultTexelFormat(RHI::Format format);
    void SetDefaultStructStride(size_t stride);

    void Upload(const void *data, size_t offset, size_t size);
    void Download(void *data, size_t offset, size_t size);

    BufferSRV GetSRV();
    BufferSRV GetSRV(RHI::Format texelFormat);
    BufferSRV GetSRV(size_t structStride);

    BufferUAV GetUAV();
    BufferUAV GetUAV(RHI::Format texelFormat);
    BufferUAV GetUAV(size_t structStride);

private:

    friend class BufferManagerInterface;
};

class BufferManager : public Uncopyable, public BufferManagerInterface
{
public:

    BufferManager(RHI::DevicePtr device, DeviceSynchronizer &sync);

    RC<Buffer> Create(const RHI::BufferDesc &desc);

    void _internalRelease(Buffer &buffer) override;

private:

    RHI::DevicePtr device_;
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

inline const RHI::BufferPtr &Buffer::GetRHIObject() const
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

inline void Buffer::SetDefaultTexelFormat(RHI::Format format)
{
    defaultViewTexelFormat_ = format;
}

inline void Buffer::SetDefaultStructStride(size_t stride)
{
    defaultViewStructStride_ = static_cast<uint32_t>(stride);
}

inline void Buffer::Upload(const void *data, size_t offset, size_t size)
{
    assert(offset + size <= size_);
    auto ptr = rhiBuffer_->Map(offset, size, false);
    std::memcpy(ptr, data, size);
    rhiBuffer_->Unmap(offset, size, true);
}

inline void Buffer::Download(void *data, size_t offset, size_t size)
{
    assert(offset + size <= size_);
    auto ptr = rhiBuffer_->Map(offset, size, true);
    std::memcpy(data, ptr, size);
    rhiBuffer_->Unmap(offset, size, false);
}

RTRC_END
