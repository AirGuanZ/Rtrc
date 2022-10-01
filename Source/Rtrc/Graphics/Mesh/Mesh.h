#pragma once

#include <Rtrc/Graphics/Mesh/MeshLayout.h>

RTRC_BEGIN

class Mesh : public Uncopyable
{
public:

    Mesh();

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    void Swap(Mesh &other) noexcept;

    const RC<MeshLayout> &GetLayout() const;
    const RHI::BufferPtr &GetVertexBuffer(int index) const;

    const RHI::BufferPtr &GetIndexBuffer() const;
    RHI::IndexBufferFormat GetIndexBufferFormat() const;

private:

    friend class MeshBuilder;

    RC<MeshLayout> layout_;
    std::vector<RHI::BufferPtr> vertexBuffers_;

    RHI::IndexBufferFormat indexBufferFormat_;
    RHI::BufferPtr indexBuffer_;
};

class MeshBuilder
{
public:

    MeshBuilder &SetLayout(RC<MeshLayout> layout);
    MeshBuilder &SetVertexBuffer(int index, RHI::BufferPtr buffer);
    MeshBuilder &SetIndexBuffer(RHI::BufferPtr buffer, RHI::IndexBufferFormat format);

    Mesh CreateMesh();

private:

    RC<MeshLayout>              layout_;
    std::vector<RHI::BufferPtr> vertexBuffers_;

    RHI::BufferPtr         indexBuffer_;
    RHI::IndexBufferFormat indexFormat_ = RHI::IndexBufferFormat::UInt16;
};

RTRC_END
