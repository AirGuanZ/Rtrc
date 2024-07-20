#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d12_agility/d3d12.h>
#include <dxgi1_4.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>

#include <Rtrc/RHI/RHIDeclaration.h>

#define RTRC_RHI_D3D12_BEGIN RTRC_RHI_BEGIN namespace D3D12 {
#define RTRC_RHI_D3D12_END } RTRC_RHI_END

#define RTRC_RHI_D3D12_USE_GENERIC_PROGRAM_FOR_COMPUTE_PIPELINE 0

RTRC_RHI_D3D12_BEGIN

class DirectX12Device;

using Microsoft::WRL::ComPtr;

struct D3D12ResultChecker
{
    HRESULT result;
    template<typename F>
    void operator+(F &&f)
    {
        if(FAILED(result))
        {
            std::invoke(std::forward<F>(f), result);
        }
    }
};

std::string HResultToString(HRESULT hr);

#define RTRC_D3D12_CHECK(RESULT) ::Rtrc::RHI::D3D12::D3D12ResultChecker{(RESULT)}+[&](HRESULT errorCode)->void
#define RTRC_D3D12_FAIL_MSG(RESULT, MSG)                                                                            \
    do                                                                                                              \
    {                                                                                                               \
        const auto r = (RESULT);                                                                                    \
        RTRC_D3D12_CHECK(r)                                                                                         \
        {                                                                                                           \
            throw ::Rtrc::Exception(fmt::format(                                                                    \
                "{} (Error = {:#X}, {})", (MSG), uint32_t(r), ::Rtrc::RHI::D3D12::HResultToString(r)));             \
        };                                                                                                          \
    } while(false)

struct DirectX12MemoryAllocation
{
    ComPtr<D3D12MA::Allocation> allocation;
};

extern D3D12MA::ALLOCATION_CALLBACKS RtrcGlobalDirectX12AllocationCallbacks;
#define RTRC_D3D12_ALLOC (&RtrcGlobalDirectX12AllocationCallbacks)

std::wstring Utf8ToWin32W(std::string_view s);

#define RTRC_D3D12_IMPL_SET_NAME(OBJPTR)               \
    void SetName(std::string name) override            \
    {                                                  \
        (OBJPTR)->SetName(Utf8ToWin32W(name).c_str()); \
        RHIObject::SetName(std::move(name));           \
    }

DXGI_FORMAT                TranslateFormat(Format format);
D3D12_COMMAND_LIST_TYPE    TranslateCommandListType(QueueType type);
D3D12_TEXTURE_ADDRESS_MODE TranslateAddressMode(AddressMode mode);
D3D12_COMPARISON_FUNC      TranslateCompareOp(CompareOp op);
D3D12_SAMPLER_DESC         TranslateSamplerDesc(const SamplerDesc &desc);
D3D12_SHADER_VISIBILITY    TranslateShaderVisibility(ShaderStageFlags vis);
D3D12_VIEWPORT             TranslateViewport(const Viewport &viewport);
D3D12_RECT                 TranslateScissor(const Scissor &scissor);

D3D12_BLEND    TranslateBlendFactor(BlendFactor factor);
D3D12_BLEND_OP TranslateBlendOp(BlendOp op);

D3D12_FILL_MODE               TranslateFillMode             (FillMode mode);
D3D12_CULL_MODE               TranslateCullMode             (CullMode mode);
D3D12_STENCIL_OP              TranslateStencilOp            (StencilOp op);
D3D12_PRIMITIVE_TOPOLOGY_TYPE TranslatePrimitiveTopologyType(PrimitiveTopology topology);
D3D12_PRIMITIVE_TOPOLOGY      TranslatePrimitiveTopology    (PrimitiveTopology topology);

DXGI_FORMAT TranslateVertexAttributeType(VertexAttributeType type);

// Register is set to (0, 0)
D3D12_STATIC_SAMPLER_DESC  TranslateStaticSamplerDesc(const SamplerDesc &desc, D3D12_SHADER_VISIBILITY vis);

D3D12_BARRIER_SYNC   TranslateBarrierSync(PipelineStageFlag stages, Format format);
D3D12_BARRIER_ACCESS TranslateBarrierAccess(ResourceAccessFlag accesses);
D3D12_BARRIER_LAYOUT TranslateTextureLayout(TextureLayout layout, Format format);

DXGI_FORMAT TranslateRayTracingIndexType(RayTracingIndexFormat type);
DXGI_FORMAT TranslateRayTracingVertexFormat(RayTracingVertexFormat format);

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS
    TranslateRayTracingAccelerationStructureBuildFlags(RayTracingAccelerationStructureBuildFlags flags);

D3D12_RESOURCE_DESC TranslateBufferDesc(const BufferDesc &desc);
D3D12_RESOURCE_DESC TranslateTextureDesc(const TextureDesc &desc);

RTRC_RHI_D3D12_END
