#pragma once

#include <Core/Serialization/TextSerializer.h>

RTRC_BEGIN

#define RTRC_SHOULD_ENABLE_MATERIAL_REGISTRATION __has_include("Generated/PreprocessedShaderFiles.inl")

namespace MaterialRegistrationDetail
{

#if RTRC_SHOULD_ENABLE_MATERIAL_REGISTRATION

    template<int>
    static void _rtrcRegisterMaterial(MaterialManager &) { }

    static constexpr int materialCounterBegin = __COUNTER__;
#define ENABLE_MATERIAL_REGISTRATION 1
#include "Generated/PreprocessedShaderFiles.inl"
#undef ENABLE_MATERIAL_REGISTRATION
    static constexpr int materialCounterEnd = __COUNTER__;

    template<int I>
    static void RegisterSingleMaterial(MaterialManager &manager)
    {
        _rtrcRegisterMaterial<I>(manager);
    }

    template<int Begin, int...Is>
    static void RegisterAllPreprocessedMaterialsImpl(MaterialManager &manager, std::integer_sequence<int, Is...>)
    {
        (RegisterSingleMaterial<Begin + Is>(manager), ...);
    }

    static void RegisterAllPreprocessedMaterials(MaterialManager &manager)
    {
        RegisterAllPreprocessedMaterialsImpl<materialCounterBegin>(
            manager, std::make_integer_sequence<int, materialCounterEnd - materialCounterBegin>());
    }

#else

    static void RegisterAllPreprocessedMaterials(MaterialManager &manager)
    {

    }

#endif

} // namespace MaterialRegistrationDetail

using MaterialRegistrationDetail::RegisterAllPreprocessedMaterials;

RTRC_END