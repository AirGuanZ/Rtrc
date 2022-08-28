#pragma once

#include <map>

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

namespace MeshLayoutDSL
{

    constexpr RHI::VertexAttributeType Int  = RHI::VertexAttributeType::Int;
    constexpr RHI::VertexAttributeType Int2 = RHI::VertexAttributeType::Int2;
    constexpr RHI::VertexAttributeType Int3 = RHI::VertexAttributeType::Int3;
    constexpr RHI::VertexAttributeType Int4 = RHI::VertexAttributeType::Int4;
    
    constexpr RHI::VertexAttributeType UInt  = RHI::VertexAttributeType::UInt;
    constexpr RHI::VertexAttributeType UInt2 = RHI::VertexAttributeType::UInt2;
    constexpr RHI::VertexAttributeType UInt3 = RHI::VertexAttributeType::UInt3;
    constexpr RHI::VertexAttributeType UInt4 = RHI::VertexAttributeType::UInt4;
    
    constexpr RHI::VertexAttributeType Float  = RHI::VertexAttributeType::Float;
    constexpr RHI::VertexAttributeType Float2 = RHI::VertexAttributeType::Float2;
    constexpr RHI::VertexAttributeType Float3 = RHI::VertexAttributeType::Float3;
    constexpr RHI::VertexAttributeType Float4 = RHI::VertexAttributeType::Float4;

    struct Attribute
    {
        Attribute(std::string semantic, RHI::VertexAttributeType type)
            : semantic(std::move(semantic)), type(type)
        {
            
        }
        auto operator<=>(const Attribute &) const = default;

        std::string semantic;
        RHI::VertexAttributeType type;
    };

    struct Buffer
    {
        template<typename...Ts>
        explicit Buffer(Ts...attribs)
        {
            attributes.reserve(sizeof...(attribs));
            (attributes.push_back(std::move(attribs)), ...);
        }
        Buffer(const Buffer &) = default;
        Buffer(Buffer &&) noexcept = default;
        Buffer &operator=(const Buffer &) = default;
        Buffer &operator=(Buffer &&) noexcept = default;
        auto operator<=>(const Buffer &) const = default;

        std::vector<Attribute> attributes;
    };

    struct LayoutDesc
    {
        template<typename...Ts>
        explicit LayoutDesc(Ts...bufs)
        {
            buffers.reserve(sizeof...(bufs));
            (buffers.push_back(std::move(bufs)), ...);
        }

        LayoutDesc(const LayoutDesc &) = default;
        LayoutDesc(LayoutDesc &&) noexcept = default;
        LayoutDesc &operator=(const LayoutDesc &) = default;
        LayoutDesc &operator=(LayoutDesc &&) noexcept = default;
        auto operator<=>(const LayoutDesc &) const = default;

        std::vector<Buffer> buffers;
    };

} // namespace MeshLayoutDSL

/*
    Mesh           := MeshLayout, vector<Buffer>
    Material       := map<Tag, Shader>
    Shader         := AttributeSet
    InputLayoutKey := Shader, MeshLayout
*/

class VertexBufferLayout : public Uncopyable
{
public:

    struct VertexAttribute
    {
        std::string semantic;
        RHI::VertexAttributeType type;
        int byteOffsetInBuffer;
    };

    static RC<VertexBufferLayout> Create(const MeshLayoutDSL::Buffer &desc);

    int GetElementStride() const;
    Span<VertexAttribute> GetAttributes() const;

    // return nullptr when semantic is not found
    const VertexAttribute *GetAttributeBySemantic(std::string_view semantic) const;

    // return -1 when semantic is not found
    int GetAttributeIndexBySemantic(std::string_view semantic) const;

private:

    int stride_ = 0;
    std::vector<VertexAttribute> attribs_;
    std::map<std::string, int, std::less<>> semanticToAttribIndex_;
};

class MeshLayout : public Uncopyable
{
public:

    static RC<MeshLayout> Create(const MeshLayoutDSL::LayoutDesc &desc);

    Span<RC<VertexBufferLayout>> GetVertexBufferLayouts() const;

    // return nullptr when semantic is not found
    RC<VertexBufferLayout> GetVertexBufferLayoutBySemantic(std::string_view semantic) const;

    // return -1 when semantic is not found
    int GetVertexBufferIndexBySemantic(std::string_view semantic) const;

private:

    std::vector<RC<VertexBufferLayout>> vertexBuffers_;
    std::map<std::string, int, std::less<>> semanticToBufferIndex_;
};

#define RTRC_MESH_LAYOUT(...)                  \
    (::Rtrc::MeshLayout::Create([&]            \
    {                                          \
        using namespace ::Rtrc::MeshLayoutDSL; \
        return LayoutDesc(__VA_ARGS__);        \
    }()))

RTRC_END
