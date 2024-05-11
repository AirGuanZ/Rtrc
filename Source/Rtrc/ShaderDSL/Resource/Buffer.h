#pragma once

#include <Rtrc/ShaderDSL/DSL/eNumber.h>

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

    static const char *GetStaticTypeName();

    static TemplateBuffer CreateFromName(std::string name);

    TemplateBuffer() { PopParentVariable(); }

    T operator[](const u32 &index) const;

    template<typename S>
    S Load(const u32 &offsetInBytes) const;

    template<typename S>
    void Store(const u32 &offsetInBytes, const S &value) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T>
class Buffer : public TemplateBuffer<T, TemplateBufferType::Buffer>
{

};

template<typename T>
class StructuredBuffer : public TemplateBuffer<T, TemplateBufferType::StructuredBuffer>
{

};

class ByteAddressBuffer : public TemplateBuffer<void, TemplateBufferType::ByteAddressBuffer>
{

};

template<typename T>
class RWBuffer : public TemplateBuffer<T, TemplateBufferType::RWBuffer>
{

};

template<typename T>
class RWStructuredBuffer : public TemplateBuffer<T, TemplateBufferType::RWStructuredBuffer>
{

};

class RWByteAddressBuffer : public TemplateBuffer<void, TemplateBufferType::RWByteAddressBuffer>
{

};

RTRC_EDSL_END
