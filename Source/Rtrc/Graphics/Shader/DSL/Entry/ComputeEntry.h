#pragma once

#include <Rtrc/Graphics/Shader/DSL/eDSL.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ArgumentTrait.h>

RTRC_BEGIN

class Device;

RTRC_END

RTRC_EDSL_BEGIN

template<typename...Args>
class ComputeEntry;

namespace ComputeEntryDetail
{

    // Callable -> ComputeEntry arguments

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

    // ComputeEntry argument -> invoke argument

    template<RtrcDSLBindingGroup T>
    struct BindingGroupArgumentWrapper
    {
        Variant<T, RC<BindingGroup>> bindingGroup;

        BindingGroupArgumentWrapper() = default;
        BindingGroupArgumentWrapper(const BindingGroupArgumentWrapper &) = delete;
        BindingGroupArgumentWrapper &operator=(const BindingGroupArgumentWrapper &) = delete;

        BindingGroupArgumentWrapper(T data): bindingGroup(std::move(data)) { }
        BindingGroupArgumentWrapper(RC<BindingGroup> group) : bindingGroup(std::move(group)) { }

        RC<BindingGroup> GetBindingGroup(Ref<Device> device) const;

        void DeclareRenderGraphResourceUsage(RGPass pass, RHI::PipelineStageFlag stages) const
        {
            this->bindingGroup.Match(
                [&](const RC<BindingGroup> &) {},
                [&]<RtrcGroupStruct S>(const S &data)
            {
                BindingGroupDetail::DeclareRenderGraphResourceUses(pass, data, stages);
            });
        }
    };

    template<typename T>
    struct EntryArgumentIdentityInvokeType
    {
        T value;
        EntryArgumentIdentityInvokeType(const T &value): value(value) {}

        void DeclareRenderGraphResourceUsage(RGPass pass, RHI::PipelineStageFlag stages) const {}
    };

    template<typename T>
    struct EntryArgumentAux;

    template<typename T> requires ArgumentTrait::eValueTrait<T>::IsValue
    struct EntryArgumentAux<T>
    {
        using NativeType = typename ArgumentTrait::eValueTrait<T>::NativeType;
        using InvokeType = EntryArgumentIdentityInvokeType<NativeType>;
    };

    template<typename T> requires ArgumentTrait::eResourceTrait<T>::IsResource
    struct EntryArgumentAux<T>
    {
        using InvokeType = typename ArgumentTrait::eResourceTrait<T>::ArgumentWrapperType;
    };

    template<typename T> requires RtrcDSLBindingGroup<T>
    struct EntryArgumentAux<T>
    {
        using InvokeType = BindingGroupArgumentWrapper<T>;
    };

    template<typename T>
    using InvokeType = typename EntryArgumentAux<T>::InvokeType;

    struct ValueItem
    {
        size_t offset;
        size_t size;
    };

} // namespace ComputeEntryDetail

template<typename Func>
RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
    BuildComputeEntry(Ref<Device> device, const Func &func);

template<typename...KernelArgs>
void DeclareRenderGraphResourceUses(
    RGPass                                              pass,
    const ComputeEntry<KernelArgs...>                  &entry,
    const ComputeEntryDetail::InvokeType<KernelArgs>&...args);

template<typename...KernelArgs>
void SetupComputeEntry(
    CommandBuffer                                      &commandBuffer,
    const ComputeEntry<KernelArgs...>                  &entry,
    const ComputeEntryDetail::InvokeType<KernelArgs>&...args);

class UntypedComputeEntry
{
public:

    const RC<Shader> &GetShader() const { return shader_; }
    int GetDefaultBindingGroupIndex() const { return defaultBindingGroupIndex_; }
    Span<ComputeEntryDetail::ValueItem> GetValueItems() const { return defaultBindingGroupValueItems_; }

private:

    template<typename Func>
    friend RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
        BuildComputeEntry(Ref<Device> device, const Func &func);

    RC<Shader> shader_;
    int defaultBindingGroupIndex_ = -1;
    std::vector<ComputeEntryDetail::ValueItem> defaultBindingGroupValueItems_;
};

// Valid types of Args:
//    * eBindingGroup
//    * eResource
//    * eValue
template<typename...Args>
class ComputeEntry
{
public:

    const RC<Shader> &GetShader() const { return untypedComputeEntry_->GetShader(); }

    int GetDefaultBindingGroupIndex() const { return untypedComputeEntry_->GetDefaultBindingGroupIndex(); }
    Span<ComputeEntryDetail::ValueItem> GetValueItems() const { return untypedComputeEntry_->GetValueItems(); }

private:

    template<typename Func>
    friend RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
        BuildComputeEntry(Ref<Device> device, const Func &func);

    RC<UntypedComputeEntry> untypedComputeEntry_;
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
