#pragma once

#include <ShaderCompiler/Database/ShaderDatabase.h>

RTRC_SHADER_COMPILER_BEGIN

#define RTRC_SHOULD_ENABLE_SHADER_REGISTRATION __has_include("RtrcPreprocessedShaderFiles.inl")

namespace ShaderRegistrationDetail
{

#if RTRC_SHOULD_ENABLE_SHADER_REGISTRATION

    template<int>
    static void _rtrcRegisterShaderInfo(ShaderDatabase &) { }

    static constexpr int begin = __COUNTER__;
#define ENABLE_SHADER_REGISTRATION 1
#include "RtrcPreprocessedShaderFiles.inl"
#undef ENABLE_SHADER_REGISTRATION
    static constexpr int end = __COUNTER__;

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
        RegisterAllPreprocessedShadersImpl<begin>(database, std::make_integer_sequence<int, end - begin>());
    }

#else

    static void RegisterAllPreprocessedShaders(ShaderDatabase &database)
    {

    }

#endif

} // namespace ShaderRegistrationDetail

using ShaderRegistrationDetail::RegisterAllPreprocessedShaders;

RTRC_SHADER_COMPILER_END
