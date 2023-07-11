#pragma once

#include <Rtrc/Graphics/Mesh/MeshLayout.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Math/AABB.h>
#include <Rtrc/Utility/Container/ObjectCache.h>
#include <Rtrc/Utility/SmartPointer/CopyOnWritePtr.h>
#include <Rtrc/Utility/Thread.h>

RTRC_BEGIN

class CommandBuffer;
class Blas;
class DynamicBuffer;

class Mesh : public InObjectCache
{
public:

    struct BuiltinVertexStruct_Default
    {
        Vector3f position;
        Vector3f normal;
        Vector2f texCoord;
        Vector3f tangent;
    };

    struct BuiltinVertexStruct_Simplified
    {
        Vector3f position;
        Vector3f normal;
        Vector2f texCoord;
    };

    struct BuiltinVertexBufferLayout
    {
        static const VertexBufferLayout *Standard;
    };

    // Acquired SharedRenderingData will never be modified by original Mesh,
    // and can be safely used/modified in render thread
    class SharedRenderingData : public ReferenceCounted, public WithUniqueObjectID
    {
    public:

        const MeshLayout *GetLayout() const { return layout_; }

        uint32_t             GetVertexCount() const           { return vertexCount_; }
        const RC<SubBuffer> &GetVertexBuffer(int index) const { return vertexBuffers_[index]; }

        uint32_t             GetIndexCount() const  { return indexCount_ ; }
        const RC<SubBuffer> &GetIndexBuffer() const { return indexBuffer_; }
        RHI::IndexFormat     GetIndexFormat() const { return indexFormat_; }

        uint32_t GetPrimitiveCount() const { return (indexCount_ ? indexCount_ : vertexCount_) / 3; }

        const AABB3f &GetBoundingBox() const { return bound_; }
        
        void BindVertexAndIndexBuffers(CommandBuffer &commandBuffer) const;
        
    private:

        friend class Mesh;
        friend class MeshBuilder;

        const MeshLayout *layout_ = nullptr;

        uint32_t                   vertexCount_ = 0;
        std::vector<RC<SubBuffer>> vertexBuffers_;
        std::vector<size_t>        vertexStrides_;

        uint32_t         indexCount_ = 0;
        RHI::IndexFormat indexFormat_ = RHI::IndexFormat::UInt16;
        RC<SubBuffer>    indexBuffer_;

        AABB3f bound_;
    };

    Mesh() = default;

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    void Swap(Mesh &other) noexcept;

    const MeshLayout *GetLayout() const { return sharedData_->GetLayout(); }

    uint32_t             GetVertexCount() const           { return sharedData_->GetVertexCount(); }
    const RC<SubBuffer> &GetVertexBuffer(int index) const { return sharedData_->GetVertexBuffer(index); }
    
    uint32_t             GetIndexCount()  const { return sharedData_->GetIndexCount();  }
    const RC<SubBuffer> &GetIndexBuffer() const { return sharedData_->GetIndexBuffer(); }
    RHI::IndexFormat     GetIndexFormat() const { return sharedData_->GetIndexFormat(); }

    uint32_t GetPrimitiveCount() const { return sharedData_->GetPrimitiveCount(); }

    AABB3f GetBoundingBox() const { return sharedData_->GetBoundingBox(); }

    ReferenceCountedPtr<SharedRenderingData> GetRenderingData() const  { return sharedData_; }
    SharedRenderingData                     *GetMutableRenderingData() { return sharedData_.Unshare(); }

private:

    CopyOnWritePtr<SharedRenderingData> sharedData_ = MakeReferenceCountedPtr<SharedRenderingData>();
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

void BindMesh(CommandBuffer &commandBuffer, const Mesh::SharedRenderingData &mesh);
void BindMesh(CommandBuffer &commandBuffer, const Mesh &mesh);

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
    RTRC_SWAP_MEMBERS(*this, other, sharedData_);
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
    Mesh::SharedRenderingData &data = *ret.GetMutableRenderingData();
    data.layout_        = layout_;
    data.vertexBuffers_ = std::move(vertexBuffers_);
    data.indexFormat_   = indexFormat_;
    data.indexBuffer_   = std::move(indexBuffer_);
    data.vertexCount_   = vertexCount_;
    data.indexCount_    = indexCount_;
    data.bound_         = bound_;
    data.vertexStrides_.resize(data.vertexBuffers_.size());
    for(size_t i = 0; i < data.layout_->GetVertexBufferLayouts().size(); ++i)
    {
        data.vertexStrides_[i] = data.layout_->GetVertexBufferLayouts()[i]->GetElementStride();
    }
    return ret;
}

inline void BindMesh(CommandBuffer &commandBuffer, const Mesh::SharedRenderingData &mesh)
{
    mesh.BindVertexAndIndexBuffers(commandBuffer);
}

inline void BindMesh(CommandBuffer &commandBuffer, const Mesh &mesh)
{
    BindMesh(commandBuffer, *mesh.GetRenderingData());
}

RTRC_END
