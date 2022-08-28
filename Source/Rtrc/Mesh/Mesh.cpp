#include <Rtrc/Mesh/Mesh.h>

RTRC_BEGIN

Mesh::Mesh()
    : indexBufferFormat_(RHI::IndexBufferFormat::UInt16)
{
    
}

Mesh::Mesh(Mesh &&other) noexcept
    : Mesh()
{
    Swap(other);
}

Mesh &Mesh::operator=(Mesh &&other) noexcept
{
    Swap(other);
    return *this;
}

void Mesh::Swap(Mesh &other) noexcept
{
    std::swap(layout_, other.layout_);
    std::swap(vertexBuffers_, other.vertexBuffers_);
    std::swap(indexBufferFormat_, other.indexBufferFormat_);
    std::swap(indexBuffer_, other.indexBuffer_);
}

const RC<MeshLayout> &Mesh::GetLayout() const
{
    return layout_;
}

const RHI::BufferPtr &Mesh::GetVertexBuffer(int index) const
{
    return vertexBuffers_[index];
}

const RHI::BufferPtr &Mesh::GetIndexBuffer() const
{
    return indexBuffer_;
}

RHI::IndexBufferFormat Mesh::GetIndexBufferFormat() const
{
    return indexBufferFormat_;
}

MeshBuilder &MeshBuilder::SetLayout(RC<MeshLayout> layout)
{
    layout_ = std::move(layout);
    vertexBuffers_.resize(layout_->GetVertexBufferLayouts().size());
    return *this;
}

MeshBuilder &MeshBuilder::SetVertexBuffer(int index, RHI::BufferPtr buffer)
{
    assert(layout_);
    vertexBuffers_[index] = std::move(buffer);
    return *this;
}

MeshBuilder &MeshBuilder::SetIndexBuffer(RHI::BufferPtr buffer, RHI::IndexBufferFormat format)
{
    indexBuffer_ = std::move(buffer);
    indexFormat_ = format;
    return *this;
}

Mesh MeshBuilder::CreateMesh()
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
