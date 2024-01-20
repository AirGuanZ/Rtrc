#pragma once

#include <Rtrc/Core/Serialization/TextSerializer.h>

RTRC_BEGIN

namespace ShaderRegistrationDetail
{

    template<int>
    static void _rtrcRegisterShaderInfo(ShaderDatabase &) { }

    static constexpr int shaderCounterBegin = __COUNTER__;
#define ENABLE_SHADER_REGISTRATION 1
#include "Generated/PreprocessedShaderFiles.inl"
#undef ENABLE_SHADER_REGISTRATION
    static constexpr int shaderCounterEnd = __COUNTER__;

    template<int I>
    static void RegisterSingleRegisteredShader(ShaderDatabase &database)
    {
        _rtrcRegisterShaderInfo<I>(database);
    }

    template<int Begin, int...Is>
    static void RegisterAllPreprocessedShadersImpl(ShaderDatabase &database, std::integer_sequence<int, Is...>)
    {
        (RegisterSingleRegisteredShader<Begin + Is>(database), ...);
    }

    static void RegisterAllPreprocessedShaders(ShaderDatabase &database)
    {
        RegisterAllPreprocessedShadersImpl<shaderCounterBegin>(
            database, std::make_integer_sequence<int, shaderCounterEnd - shaderCounterBegin>());
    }

} // namespace ShaderRegistrationDetail

using ShaderRegistrationDetail::RegisterAllPreprocessedShaders;

RTRC_END
