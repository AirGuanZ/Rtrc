#pragma once

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Container/Cache/SharedObjectCache.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/MeshLayout.h>

RTRC_BEGIN

class CommandBuffer;
class Blas;
class DynamicBuffer;

class Mesh : public InSharedObjectCache, public WithUniqueObjectID
{
public:

    Mesh() = default;

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    void Swap(Mesh &other) noexcept;

    const MeshLayout *GetLayout() const { return layout_; }

    uint32_t             GetVertexCount()           const { return vertexCount_; }
    const RC<SubBuffer> &GetVertexBuffer(int index) const { return vertexBuffers_[index]; }
    
    uint32_t             GetIndexCount()  const { return indexCount_; }
    const RC<SubBuffer> &GetIndexBuffer() const { return indexBuffer_; }
    RHI::IndexFormat     GetIndexFormat() const { return indexFormat_; }

    uint32_t GetPrimitiveCount() const { return (indexCount_ ? indexCount_ : vertexCount_) / 3; }

    AABB3f GetBoundingBox() const { return bound_; }

    void Bind(CommandBuffer &commandBuffer) const;

private:

    friend class MeshBuilder;

    const MeshLayout *layout_;

    uint32_t                   vertexCount_ = 0;
    std::vector<RC<SubBuffer>> vertexBuffers_;
    std::vector<size_t>        vertexStrides_;

    uint32_t         indexCount_ = 0;
    RHI::IndexFormat indexFormat_;
    RC<SubBuffer>    indexBuffer_;

    AABB3f bound_;
};

class MeshBuilder
{
public:

    MeshBuilder &SetLayout(const MeshLayout *layout);
    
    MeshBuilder &SetVertexBuffer(int index, const RC<SubBuffer> &buffer);
    MeshBuilder &SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format);

    MeshBuilder &SetVertexCount(uint32_t count);
    MeshBuilder &SetIndexCount(uint32_t count);

    MeshBuilder &SetBoundingBox(const AABB3f &bound);

    Mesh CreateMesh();

private:

    const MeshLayout *layout_ = nullptr;

    uint32_t                   vertexCount_ = 0;
    std::vector<RC<SubBuffer>> vertexBuffers_;

    uint32_t         indexCount_ = 0;
    RHI::IndexFormat indexFormat_ = RHI::IndexFormat::UInt16;
    RC<SubBuffer>    indexBuffer_;

    AABB3f bound_;
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
        layout_, vertexCount_, vertexBuffers_, vertexStrides_, indexCount_, indexFormat_, indexBuffer_, bound_);
}

inline MeshBuilder &MeshBuilder::SetLayout(const MeshLayout *layout)
{
    layout_ = layout;
    vertexBuffers_.resize(layout_->GetVertexBufferLayouts().size());
    return *this;
}

inline MeshBuilder &MeshBuilder::SetVertexBuffer(int index, const RC<SubBuffer> &buffer)
{
    assert(layout_);
    vertexBuffers_[index] = buffer;
    return *this;
}

inline MeshBuilder &MeshBuilder::SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format)
{
    indexBuffer_ = buffer;
    indexFormat_ = format;
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

inline MeshBuilder &MeshBuilder::SetBoundingBox(const AABB3f &bound)
{
    bound_ = bound;
    return *this;
}

inline Mesh MeshBuilder::CreateMesh()
{
    Mesh ret;
    ret.layout_        = layout_;
    ret.vertexBuffers_ = std::move(vertexBuffers_);
    ret.indexFormat_   = indexFormat_;
    ret.indexBuffer_   = std::move(indexBuffer_);
    ret.vertexCount_   = vertexCount_;
    ret.indexCount_    = indexCount_;
    ret.bound_         = bound_;
    ret.vertexStrides_.resize(ret.vertexBuffers_.size());
    for(size_t i = 0; i < ret.layout_->GetVertexBufferLayouts().size(); ++i)
    {
        ret.vertexStrides_[i] = ret.layout_->GetVertexBufferLayouts()[i]->GetElementStride();
    }
    return ret;
}

RTRC_END
