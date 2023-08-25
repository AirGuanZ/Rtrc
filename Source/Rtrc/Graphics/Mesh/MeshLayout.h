#pragma once

#include <map>

#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Core/StringPool.h>
#include <Rtrc/Core/TemplateStringParameter.h>

RTRC_BEGIN

class VertexSemantic
{

    static constexpr uint32_t INVALID_KEY = (std::numeric_limits<uint32_t>::max)();

    uint32_t key_;

public:

    struct VertexSemanticTag { };
    using PooledSemanticName = PooledString<VertexSemanticTag, uint32_t>;

    static VertexSemantic FromPooledName(PooledSemanticName pooledSemanticName, uint32_t semanticIndex)
    {
        VertexSemantic ret;
        ret.key_ = pooledSemanticName.GetIndex() | (semanticIndex << 24);
        return ret;
    }

    VertexSemantic() : key_(INVALID_KEY) { }
    VertexSemantic(std::string_view semanticName, uint32_t semanticIndex)
    {
        const auto pooledString = PooledSemanticName(semanticName);
        assert(pooledString.GetIndex() < 0xffffff);
        assert(semanticIndex < 0xff);
        key_ = (semanticIndex << 24) | pooledString.GetIndex();
    }

    bool IsValid() const { return key_ != INVALID_KEY; }
    const std::string &GetName() const { return PooledSemanticName::FromIndex(key_ & 0xffffff).GetString(); }
    uint32_t GetIndex() const { return key_ >> 24; }
    std::string ToString() const { return fmt::format("{}{}", GetName(), GetIndex()); }

    auto operator<=>(const VertexSemantic &) const = default;
};

#define RTRC_VERTEX_SEMANTIC(X, INDEX) \
    (::Rtrc::VertexSemantic::FromPooledName(RTRC_POOLED_STRING(::Rtrc::VertexSemantic::PooledSemanticName, X), INDEX))

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

    constexpr RHI::VertexAttributeType UChar4UNorm = RHI::VertexAttributeType::UChar4Norm;

    struct Attribute
    {
        Attribute(std::string_view semanticName, int semanticIndex, RHI::VertexAttributeType type)
            : Attribute(VertexSemantic(semanticName, semanticIndex), type)
        {
            
        }

        Attribute(VertexSemantic semantic, RHI::VertexAttributeType type)
            : semantic(semantic), type(type)
        {
            
        }
        auto operator<=>(const Attribute &) const = default;

        VertexSemantic           semantic;
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
        bool perInstance = false;
    };

    template<typename...Ts>
    Buffer PerInstanceBuffer(Ts...attribs)
    {
        auto buffer = Buffer(std::move(attribs)...);
        buffer.perInstance = true;
        return buffer;
    }

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
        VertexSemantic           semantic;
        RHI::VertexAttributeType type;
        int                      byteOffsetInBuffer;
    };

    static const VertexBufferLayout *Create(const MeshLayoutDSL::Buffer &desc);

    bool IsPerInstance() const;
    int GetElementStride() const;
    Span<VertexAttribute> GetAttributes() const;

    // return nullptr when semantic is not found
    const VertexAttribute *GetAttributeBySemantic(VertexSemantic semantic) const;

    // return -1 when semantic is not found
    int GetAttributeIndexBySemantic(VertexSemantic semantic) const;

private:

    bool                                       perInstance_ = false;
    int                                        stride_ = 0;
    std::vector<VertexAttribute>               attribs_;
    std::map<VertexSemantic, int, std::less<>> semanticToAttribIndex_;
};

class MeshLayout : public Uncopyable
{
public:

    static const MeshLayout *Create(const MeshLayoutDSL::LayoutDesc &desc);

    Span<const VertexBufferLayout*> GetVertexBufferLayouts() const;

    // return nullptr when semantic is not found
    const VertexBufferLayout *GetVertexBufferLayoutBySemantic(VertexSemantic semantic) const;

    // return -1 when semantic is not found
    int GetVertexBufferIndexBySemantic(VertexSemantic semantic) const;

private:

    std::vector<const VertexBufferLayout *> vertexBuffers_;
    std::map<VertexSemantic, int, std::less<>> semanticToBufferIndex_;
};

#define RTRC_VERTEX_BUFFER_LAYOUT(...)          \
    (::Rtrc::VertexBufferLayout::Create([&]     \
    {                                           \
        using namespace ::Rtrc::MeshLayoutDSL;  \
        using ::Rtrc::MeshLayoutDSL::Buffer;    \
        using ::Rtrc::MeshLayoutDSL::Attribute; \
        return Buffer(__VA_ARGS__);             \
    }()))

#define RTRC_MESH_LAYOUT(...)                   \
    (::Rtrc::MeshLayout::Create([&]             \
    {                                           \
        using namespace ::Rtrc::MeshLayoutDSL;  \
        using ::Rtrc::MeshLayoutDSL::Buffer;    \
        using ::Rtrc::MeshLayoutDSL::Attribute; \
        return LayoutDesc(__VA_ARGS__);         \
    }()))

RTRC_END
