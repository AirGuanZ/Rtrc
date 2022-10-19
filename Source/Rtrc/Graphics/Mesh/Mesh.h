#pragma once

#include <Rtrc/Graphics/Mesh/MeshLayout.h>
#include <Rtrc/Graphics/Object/Buffer.h>

RTRC_BEGIN

class Mesh : public Uncopyable
{
public:

    Mesh() = default;

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    void Swap(Mesh &other) noexcept;

    const MeshLayout *GetLayout() const;
    const RC<Buffer> &GetVertexBuffer(int index) const;

    const RC<Buffer> &GetIndexBuffer() const;
    RHI::IndexBufferFormat GetIndexBufferFormat() const;

private:

    friend class MeshBuilder;

    const MeshLayout *layout_ = nullptr;
    std::vector<RC<Buffer>> vertexBuffers_;

    RHI::IndexBufferFormat indexBufferFormat_ = RHI::IndexBufferFormat::UInt16;
    RC<Buffer> indexBuffer_;
};

class MeshBuilder
{
public:

    MeshBuilder &SetLayout(const MeshLayout *layout);
    MeshBuilder &SetVertexBuffer(int index, RC<Buffer> buffer);
    MeshBuilder &SetIndexBuffer(RC<Buffer> buffer, RHI::IndexBufferFormat format);

    Mesh CreateMesh();

private:

    const MeshLayout *layout_ = nullptr;
    std::vector<RC<Buffer>> vertexBuffers_;

    RC<Buffer> indexBuffer_;
    RHI::IndexBufferFormat indexFormat_ = RHI::IndexBufferFormat::UInt16;
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
    std::swap(layout_, other.layout_);
    std::swap(vertexBuffers_, other.vertexBuffers_);
    std::swap(indexBufferFormat_, other.indexBufferFormat_);
    std::swap(indexBuffer_, other.indexBuffer_);
}

inline const MeshLayout *Mesh::GetLayout() const
{
    return layout_;
}

inline const RC<Buffer> &Mesh::GetVertexBuffer(int index) const
{
    return vertexBuffers_[index];
}

inline const RC<Buffer> &Mesh::GetIndexBuffer() const
{
    return indexBuffer_;
}

inline RHI::IndexBufferFormat Mesh::GetIndexBufferFormat() const
{
    return indexBufferFormat_;
}

inline MeshBuilder &MeshBuilder::SetLayout(const MeshLayout *layout)
{
    layout_ = std::move(layout);
    vertexBuffers_.resize(layout_->GetVertexBufferLayouts().size());
    return *this;
}

inline MeshBuilder &MeshBuilder::SetVertexBuffer(int index, RC<Buffer> buffer)
{
    assert(layout_);
    vertexBuffers_[index] = std::move(buffer);
    return *this;
}

inline MeshBuilder &MeshBuilder::SetIndexBuffer(RC<Buffer> buffer, RHI::IndexBufferFormat format)
{
    indexBuffer_ = std::move(buffer);
    indexFormat_ = format;
    return *this;
}

inline Mesh MeshBuilder::CreateMesh()
{
    Mesh ret;
    ret.layout_ = std::move(layout_);
    ret.vertexBuffers_ = std::move(vertexBuffers_);
    ret.indexBufferFormat_ = indexFormat_;
    ret.indexBuffer_ = std::move(indexBuffer_);
    *this = MeshBuilder();
    return ret;
}

RTRC_END
