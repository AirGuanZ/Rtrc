#pragma once

#include <Rtrc/Graphics/Mesh/MeshLayout.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Utility/ObjectCache.h>

RTRC_BEGIN

class CommandBuffer;
class DynamicBuffer;

class Mesh : public InObjectCache
{
public:

    Mesh() = default;

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    void Swap(Mesh &other) noexcept;

    const MeshLayout *GetLayout() const;
    const RC<Buffer> &GetVertexBuffer(int index) const;
    size_t GetVertexBufferByteOffset(int index) const;

    bool HasIndexBuffer() const { return indexBuffer_ != nullptr; }
    uint32_t GetIndexCount() const { return indexCount_; }
    uint32_t GetVertexCount() const { return vertexCount_; }

    const RC<Buffer> &GetIndexBuffer() const;
    size_t GetIndexBufferByteOffset() const;
    RHI::IndexFormat GetIndexBufferFormat() const;

    void Bind(CommandBuffer &commandBuffer) const;

private:

    friend class MeshBuilder;

    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;

    const MeshLayout *layout_ = nullptr;

    std::vector<RC<Buffer>> vertexBuffers_;
    std::vector<size_t> vertexBufferByteOffsets_;

    RHI::IndexFormat indexFormat_ = RHI::IndexFormat::UInt16;
    RC<Buffer> indexBuffer_;
    size_t indexBufferByteOffset_ = 0;
};

class MeshBuilder
{
public:

    MeshBuilder &SetLayout(const MeshLayout *layout);
    
    MeshBuilder &SetVertexBuffer(int index, const RC<SubBuffer> &buffer);
    MeshBuilder &SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format);

    MeshBuilder &SetVertexCount(uint32_t count);
    MeshBuilder &SetIndexCount(uint32_t count);

    Mesh CreateMesh();

private:

    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;

    const MeshLayout *layout_ = nullptr;

    std::vector<RC<Buffer>> vertexBuffers_;
    std::vector<size_t> vertexBufferByteOffsets_;

    RHI::IndexFormat indexBufferFormat_ = RHI::IndexFormat::UInt16;
    RC<Buffer> indexBuffer_;
    size_t indexBufferByteOffset_ = 0;
};

inline Mesh::Mesh(Mesh &&other) noexcept
    : Mesh()
{
    Swap(other);
}

inline Mesh &Mesh::operator=(Mesh &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void Mesh::Swap(Mesh &other) noexcept
{
    RTRC_SWAP_MEMBERS(
        *this, other,
        vertexCount_, indexCount_, layout_, vertexBuffers_, vertexBufferByteOffsets_,
        indexFormat_, indexBuffer_, indexBufferByteOffset_);
}

inline const MeshLayout *Mesh::GetLayout() const
{
    return layout_;
}

inline const RC<Buffer> &Mesh::GetVertexBuffer(int index) const
{
    return vertexBuffers_[index];
}

inline size_t Mesh::GetVertexBufferByteOffset(int index) const
{
    return vertexBufferByteOffsets_[index];
}

inline const RC<Buffer> &Mesh::GetIndexBuffer() const
{
    return indexBuffer_;
}

inline size_t Mesh::GetIndexBufferByteOffset() const
{
    return indexBufferByteOffset_;
}

inline RHI::IndexFormat Mesh::GetIndexBufferFormat() const
{
    return indexFormat_;
}

inline MeshBuilder &MeshBuilder::SetLayout(const MeshLayout *layout)
{
    layout_ = layout;
    vertexBuffers_.resize(layout_->GetVertexBufferLayouts().size());
    vertexBufferByteOffsets_.resize(vertexBuffers_.size());
    return *this;
}

inline MeshBuilder &MeshBuilder::SetVertexBuffer(int index, const RC<SubBuffer> &buffer)
{
    assert(layout_);
    vertexBuffers_[index] = buffer->GetFullBuffer();
    vertexBufferByteOffsets_[index] = buffer->GetSubBufferOffset();
    return *this;
}

inline MeshBuilder &MeshBuilder::SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format)
{
    indexBuffer_ = buffer->GetFullBuffer();
    indexBufferByteOffset_ = buffer->GetSubBufferOffset();
    indexBufferFormat_ = format;
    return *this;
}

inline MeshBuilder &MeshBuilder::SetVertexCount(uint32_t count)
{
    vertexCount_ = count;
    return *this;
}

inline MeshBuilder &MeshBuilder::SetIndexCount(uint32_t count)
{
    indexCount_ = count;
    return *this;
}

inline Mesh MeshBuilder::CreateMesh()
{
    Mesh ret;
    ret.layout_ = layout_;
    ret.vertexBuffers_ = std::move(vertexBuffers_);
    ret.vertexBufferByteOffsets_ = std::move(vertexBufferByteOffsets_);
    ret.indexFormat_ = indexBufferFormat_;
    ret.indexBuffer_ = std::move(indexBuffer_);
    ret.indexBufferByteOffset_ = indexBufferByteOffset_;
    ret.vertexCount_ = vertexCount_;
    ret.indexCount_ = indexCount_;
    *this = MeshBuilder();
    return ret;
}

RTRC_END
