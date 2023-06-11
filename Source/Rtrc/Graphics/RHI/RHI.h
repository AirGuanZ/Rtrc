#pragma once

#include <Rtrc/Graphics/RHI/RHIDeclaration.h>

#if defined(RTRC_STATIC_RHI) && defined(RTRC_RHI_VULKAN)
#include <Rtrc/Graphics/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Instance.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/RayTracingLibrary.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/RayTracingPipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Blas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/BlasBuildInfo.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Tlas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/TlasBuildInfo.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferView.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>
#endif

#if defined(RTRC_STATIC_RHI) && defined(RTRC_RHI_DIRECTX12)
#include <Rtrc/Graphics/RHI/DirectX12/Context/BackBufferSemaphore.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Instance.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Surface.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Swapchain.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/Shader.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/CommandPool.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/BufferView.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/TextureView.h>
#endif

// =============================== inlined implementation ===============================

RTRC_RHI_BEGIN

inline Viewport Viewport::Create(const TexturePtr &tex, float minDepth, float maxDepth)
{
    return Create(tex->GetDesc(), minDepth, maxDepth);
}

inline Scissor Scissor::Create(const TexturePtr &tex)
{
    return Create(tex->GetDesc());
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const ConstantBufferUpdate &cbuffer)
{
    records_.push_back({ &group, index, 0, cbuffer });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const BufferSrv *bufferSrv)
{
    records_.push_back({ &group, index, 0, bufferSrv });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const BufferUav *bufferUav)
{
    records_.push_back({ &group, index, 0, bufferUav });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const Sampler *sampler)
{
    records_.push_back({ &group, index, 0, sampler });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const TextureSrv *textureSrv)
{
    records_.push_back({ &group, index, 0, textureSrv });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const TextureUav *textureUav)
{
    records_.push_back({ &group, index, 0, textureUav });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const Tlas *tlas)
{
    records_.push_back({ &group, index, 0, tlas });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const ConstantBufferUpdate &cbuffer)
{
    records_.push_back({ &group, index, arrayElem, cbuffer });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const BufferSrv *bufferSrv)
{
    records_.push_back({ &group, index, arrayElem, bufferSrv });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const BufferUav *bufferUav)
{
    records_.push_back({ &group, index, arrayElem, bufferUav });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const Sampler *sampler)
{
    records_.push_back({ &group, index, arrayElem, sampler });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const TextureSrv *textureSrv)
{
    records_.push_back({ &group, index, arrayElem, textureSrv });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const TextureUav *textureUav)
{
    records_.push_back({ &group, index, arrayElem, textureUav });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const Tlas *tlas)
{
    records_.push_back({ &group, index, arrayElem, tlas });
}

RTRC_RHI_END
