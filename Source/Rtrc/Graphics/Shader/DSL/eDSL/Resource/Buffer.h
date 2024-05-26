#pragma once

#include "../Value/eNumber.h"

RTRC_EDSL_BEGIN

enum class TemplateBufferType
{
    Buffer,
    StructuredBuffer,
    ByteAddressBuffer,
    RWBuffer,
    RWStructuredBuffer,
    RWByteAddressBuffer
};

template<typename T, TemplateBufferType Type>
class TemplateBuffer : public eVariable<TemplateBuffer<T, Type>>
{
public:

    static_assert(std::is_base_of_v<eVariableCommonBase, T> || std::is_same_v<T, void>);

    static const char *GetStaticTypeName();

    static TemplateBuffer CreateFromName(std::string name);

    static constexpr bool IsReadOnly =
        Type == TemplateBufferType::Buffer ||
        Type == TemplateBufferType::StructuredBuffer ||
        Type == TemplateBufferType::ByteAddressBuffer;

    TemplateBuffer() { PopConstructParentVariable(); }

    TemplateBuffer &operator=(const TemplateBuffer &other)
    {
        static_cast<eVariable<TemplateBuffer> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    TemporaryValueWrapper<T> operator[](const u32 &index) const;

    template<typename S>
    const S Load(const u32 &offsetInBytes) const;

    template<typename S>
    void Store(const u32 &offsetInBytes, const S &value) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T>
using Buffer  = TemplateBuffer<T, TemplateBufferType::Buffer>;
template<typename T>
using RWBuffer = TemplateBuffer<T, TemplateBufferType::RWBuffer>;

template<typename T>
using StructuredBuffer  = TemplateBuffer<T, TemplateBufferType::StructuredBuffer>;
template<typename T>
using RWStructuredBuffer = TemplateBuffer<T, TemplateBufferType::RWStructuredBuffer>;

using ByteAddressBuffer  = TemplateBuffer<void, TemplateBufferType::ByteAddressBuffer>;
using RWByteAddressBuffer = TemplateBuffer<void, TemplateBufferType::RWByteAddressBuffer>;

#define RTRC_EDSL_DEFINE_BUFFER(TYPE, NAME) RTRC_EDSL_DEFINE_BUFFER_IMPL(TYPE, NAME)
#define RTRC_EDSL_DEFINE_BUFFER_IMPL(TYPE, NAME) TYPE NAME = TYPE::CreateFromName(#NAME)

RTRC_EDSL_END
