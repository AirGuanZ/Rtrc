#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Graphics/Device/MeshLayout.h>

RTRC_BEGIN

namespace MeshLayoutDetail
{
    int VertexAttributeTypeToByteSize(RHI::VertexAttributeType type)
    {
        using enum RHI::VertexAttributeType;
        switch(type)
        {
        case Int:
        case UInt:
        case Float:
            return 4;
        case Int2:
        case UInt2:
        case Float2:
            return 8;
        case Int3:
        case UInt3:
        case Float3:
            return 12;
        case Int4:
        case UInt4:
        case Float4:
            return 16;
        case UChar4Norm:
            return 4;
        }
        Unreachable();
    }

    void Regularize(MeshLayoutDSL::LayoutDesc &desc)
    {
        std::ranges::sort(desc.buffers, [](const MeshLayoutDSL::Buffer &a, const MeshLayoutDSL::Buffer &b)
        {
            return a.attributes.front().semantic < b.attributes.front().semantic;
        });
    }

} // namespace MeshLayoutDetail

const VertexBufferLayout *VertexBufferLayout::Create(const MeshLayoutDSL::Buffer &desc)
{
    using namespace MeshLayoutDetail;

    static std::map<MeshLayoutDSL::Buffer, Box<VertexBufferLayout>> vertexBufferLayoutCache;
    static tbb::spin_rw_mutex vertexBufferLayoutCacheMutex;

    {
        std::shared_lock readLock(vertexBufferLayoutCacheMutex);
        if(auto it = vertexBufferLayoutCache.find(desc); it != vertexBufferLayoutCache.end())
        {
            return it->second.get();
        }
    }

    std::unique_lock writeLock(vertexBufferLayoutCacheMutex);
    if(auto it = vertexBufferLayoutCache.find(desc); it != vertexBufferLayoutCache.end())
    {
        return it->second.get();
    }

    auto newLayout = MakeBox<VertexBufferLayout>();
    int stride = 0;
    for(size_t i = 0; i < desc.attributes.size(); ++i)
    {
        newLayout->attribs_.push_back(VertexAttribute
            {
                .semantic = desc.attributes[i].semantic,
                .type = desc.attributes[i].type,
                .byteOffsetInBuffer = stride
            });
        stride += VertexAttributeTypeToByteSize(desc.attributes[i].type);
        newLayout->semanticToAttribIndex_[desc.attributes[i].semantic] = static_cast<int>(i);
    }
    newLayout->perInstance_ = desc.perInstance;
    newLayout->stride_ = stride;
    return vertexBufferLayoutCache.insert({ desc, std::move(newLayout)}).first->second.get();
}

bool VertexBufferLayout::IsPerInstance() const
{
    return perInstance_;
}

int VertexBufferLayout::GetElementStride() const
{
    return stride_;
}

Span<VertexBufferLayout::VertexAttribute> VertexBufferLayout::GetAttributes() const
{
    return attribs_;
}

const VertexBufferLayout::VertexAttribute *VertexBufferLayout::GetAttributeBySemantic(RHI::VertexSemantic semantic) const
{
    const int index = GetAttributeIndexBySemantic(semantic);
    return index >= 0 ? &attribs_[index] : nullptr;
}

int VertexBufferLayout::GetAttributeIndexBySemantic(RHI::VertexSemantic semantic) const
{
    auto it = semanticToAttribIndex_.find(semantic);
    return it != semanticToAttribIndex_.end() ? it->second : -1;
}

const MeshLayout *MeshLayout::Create(const MeshLayoutDSL::LayoutDesc &_desc)
{
    using namespace MeshLayoutDetail;

    static std::map<MeshLayoutDSL::LayoutDesc, Box<MeshLayout>> meshLayoutCache;
    static tbb::spin_rw_mutex meshLayoutCacheMutex;

    auto desc = _desc;
    Regularize(desc);

    {
        std::shared_lock readLock(meshLayoutCacheMutex);
        if(auto it = meshLayoutCache.find(desc); it != meshLayoutCache.end())
        {
            return it->second.get();
        }
    }

    std::unique_lock writeLock(meshLayoutCacheMutex);
    if(auto it = meshLayoutCache.find(desc); it != meshLayoutCache.end())
    {
        return it->second.get();
    }
    std::vector<const VertexBufferLayout *> buffers;
    buffers.reserve(desc.buffers.size());
    for(auto &b : desc.buffers)
    {
        buffers.push_back(VertexBufferLayout::Create(b));
    }
    auto newLayout = MakeBox<MeshLayout>();
    newLayout->vertexBuffers_ = std::move(buffers);
    for(auto &&[index, buffer] : Enumerate(newLayout->vertexBuffers_))
    {
        for(auto &attrib : buffer->GetAttributes())
        {
            assert(!newLayout->semanticToBufferIndex_.contains(attrib.semantic));
            newLayout->semanticToBufferIndex_.insert({ attrib.semantic, static_cast<int>(index) });
        }
    }
    return meshLayoutCache.insert({ desc, std::move(newLayout) }).first->second.get();
}

Span<const VertexBufferLayout *> MeshLayout::GetVertexBufferLayouts() const
{
    return vertexBuffers_;
}

const VertexBufferLayout *MeshLayout::GetVertexBufferLayoutBySemantic(RHI::VertexSemantic semantic) const
{
    const int index = GetVertexBufferIndexBySemantic(semantic);
    return index >= 0 ? vertexBuffers_[index] : nullptr;
}

int MeshLayout::GetVertexBufferIndexBySemantic(RHI::VertexSemantic semantic) const
{
    auto it = semanticToBufferIndex_.find(semantic);
    return it != semanticToBufferIndex_.end() ? it->second : -1;
}

RTRC_END
