#include <algorithm>
#include <mutex>

#include <Rtrc/Mesh/MeshLayout.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

using namespace MeshLayoutDSL;

namespace
{

    std::map<Buffer, std::weak_ptr<VertexBufferLayout>> vertexBufferLayoutPool;
    std::mutex vertexBufferLayoutPoolMutex;

    std::map<LayoutDesc, std::weak_ptr<MeshLayout>> meshLayoutPool;
    std::mutex meshLayoutPoolMutex;

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
        }
        Unreachable();
    }

    void Regularize(LayoutDesc &desc)
    {
        std::ranges::sort(desc.buffers, [](const Buffer &a, const Buffer &b)
        {
            return a.attributes.front().semantic < b.attributes.front().semantic;
        });
    }

} // namespace anonymous

RC<VertexBufferLayout> VertexBufferLayout::Create(const Buffer &desc)
{
    std::lock_guard lock(vertexBufferLayoutPoolMutex);

    if(auto it = vertexBufferLayoutPool.find(desc); it != vertexBufferLayoutPool.end())
    {
        if(auto ret = it->second.lock())
        {
            return ret;
        }
    }

    auto newLayout = MakeRC<VertexBufferLayout>();
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
    newLayout->stride_ = stride;
    vertexBufferLayoutPool[desc] = newLayout;
    return newLayout;
}

int VertexBufferLayout::GetElementStride() const
{
    return stride_;
}

Span<VertexBufferLayout::VertexAttribute> VertexBufferLayout::GetAttributes() const
{
    return attribs_;
}

const VertexBufferLayout::VertexAttribute *VertexBufferLayout::GetAttributeBySemantic(std::string_view semantic) const
{
    const int index = GetAttributeIndexBySemantic(semantic);
    return index >= 0 ? &attribs_[index] : nullptr;
}

int VertexBufferLayout::GetAttributeIndexBySemantic(std::string_view semantic) const
{
    auto it = semanticToAttribIndex_.find(semantic);
    return it != semanticToAttribIndex_.end() ? it->second : -1;
}

RC<MeshLayout> MeshLayout::Create(const LayoutDesc &_desc)
{
    auto desc = _desc;
    Regularize(desc);

    std::lock_guard lock(meshLayoutPoolMutex);

    if(auto it = meshLayoutPool.find(desc); it != meshLayoutPool.end())
    {
        if(auto ret = it->second.lock())
        {
            return ret;
        }
    }

    std::vector<RC<VertexBufferLayout>> buffers;
    buffers.reserve(desc.buffers.size());
    for(auto &b : desc.buffers)
    {
        buffers.push_back(VertexBufferLayout::Create(b));
    }

    auto newLayout = MakeRC<MeshLayout>();
    newLayout->vertexBuffers_ = std::move(buffers);
    for(auto &&[index, buffer] : Enumerate(newLayout->vertexBuffers_))
    {
        for(auto &attrib : buffer->GetAttributes())
        {
            assert(!newLayout->semanticToBufferIndex_.contains(attrib.semantic));
            newLayout->semanticToBufferIndex_.insert({ attrib.semantic, static_cast<int>(index) });
        }
    }

    meshLayoutPool[desc] = newLayout;
    return newLayout;
}

Span<RC<VertexBufferLayout>> MeshLayout::GetVertexBufferLayouts() const
{
    return vertexBuffers_;
}

RC<VertexBufferLayout> MeshLayout::GetVertexBufferLayoutBySemantic(std::string_view semantic) const
{
    const int index = GetVertexBufferIndexBySemantic(semantic);
    return index >= 0 ? vertexBuffers_[index] : nullptr;
}

int MeshLayout::GetVertexBufferIndexBySemantic(std::string_view semantic) const
{
    auto it = semanticToBufferIndex_.find(semantic);
    return it != semanticToBufferIndex_.end() ? it->second : -1;
}

RTRC_END
