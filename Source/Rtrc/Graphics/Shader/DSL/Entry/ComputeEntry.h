#pragma once

#include <Rtrc/Graphics/Shader/DSL/eDSL.h>
#include <Rtrc/ShaderCommon/Preprocess/ShaderPreprocessing.h>

RTRC_BEGIN

class Device;

RTRC_END

RTRC_EDSL_BEGIN

struct UniformBufferLayout
{
    struct UniformVariable
    {
        ShaderUniformType type;
        size_t offset;
        size_t size;
    };

    std::vector<UniformVariable> variables;
};

template<typename...Args>
class ComputeEntry;

namespace ComputeEntryDetail
{

    template<typename Func>
    struct FunctionTrait
    {
        using ActualFunctionTrait = FunctionTrait<decltype(std::function{ std::declval<Func>() })>;
        using FunctionWrapper = typename ActualFunctionTrait::FunctionWrapper;
        using Entry = typename ActualFunctionTrait::Entry;
    };

    template<typename...Args>
    struct FunctionTrait<std::function<void(Args...)>>
    {
        using FunctionWrapper = std::function<void(Args...)>;
        using Entry = ComputeEntry<std::remove_cvref_t<Args>...>;
    };

    template<typename Func>
    using Entry = typename FunctionTrait<Func>::Entry;

} // namespace ComputeEntryDetail

template<typename Func>
RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
    BuildComputeEntry(Ref<Device> device, const Func &func);

// Valid types of Args:
//    * eBindingGroup
//    * eResource
//    * eValue
template<typename...Args>
class ComputeEntry
{
    template<typename Func>
    friend RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
        BuildComputeEntry(Ref<Device> device, const Func &func);

    RC<Shader> shader_;
    int defaultBindingGroupIndex_ = -1;
};

void SetThreadGroupSize(const Vector3u &size);

namespace SV
{

    u32x3 GetGroupID();
    u32x3 GetGroupThreadID();
    u32x3 GetDispatchThreadID();

} // namespace SV

#define RTRC_EDSL_THREAD_GROUP_SIZE(...) ::Rtrc::eDSL::SetThreadGroupSize(::Rtrc::Vector3u(__VA_ARGS__))
#define RTRC_EDSL_GROUP_ID (::Rtrc::eDSL::SV::GetGroupID())
#define RTRC_EDSL_GROUP_THREAD_ID (::Rtrc::eDSL::SV::GetGroupThreadID())
#define RTRC_EDSL_DISPATCH_THREAD_ID (::Rtrc::eDSL::SV::GetDispatchThreadID())

#define $numthreads(...) RTRC_EDSL_THREAD_GROUP_SIZE(__VA_ARGS__)
#define $SV_GroupID          RTRC_EDSL_GROUP_ID
#define $SV_GroupThreadID    RTRC_EDSL_GROUP_THREAD_ID
#define $SV_DispatchThreadID RTRC_EDSL_DISPATCH_THREAD_ID

RTRC_EDSL_END

#include "ComputeEntry.inl"
