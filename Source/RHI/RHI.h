#pragma once

#include <RHI/RHIDeclaration.h>

#if defined(RTRC_STATIC_RHI) && defined(RTRC_RHI_VULKAN)
#include <RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <RHI/Vulkan/Context/Device.h>
#include <RHI/Vulkan/Context/Instance.h>
#include <RHI/Vulkan/Context/Surface.h>
#include <RHI/Vulkan/Context/Swapchain.h>
#include <RHI/Vulkan/Pipeline/BindingGroup.h>
#include <RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <RHI/Vulkan/Pipeline/BindingLayout.h>
#include <RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <RHI/Vulkan/Pipeline/RayTracingLibrary.h>
#include <RHI/Vulkan/Pipeline/RayTracingPipeline.h>
#include <RHI/Vulkan/Pipeline/Shader.h>
#include <RHI/Vulkan/Queue/CommandBuffer.h>
#include <RHI/Vulkan/Queue/CommandPool.h>
#include <RHI/Vulkan/Queue/Fence.h>
#include <RHI/Vulkan/Queue/Queue.h>
#include <RHI/Vulkan/Queue/Semaphore.h>
#include <RHI/Vulkan/RayTracing/Blas.h>
#include <RHI/Vulkan/RayTracing/BlasPrebuildInfo.h>
#include <RHI/Vulkan/RayTracing/Tlas.h>
#include <RHI/Vulkan/RayTracing/TlasPrebuildInfo.h>
#include <RHI/Vulkan/Resource/Buffer.h>
#include <RHI/Vulkan/Resource/BufferView.h>
#include <RHI/Vulkan/Resource/Sampler.h>
#include <RHI/Vulkan/Resource/Texture.h>
#include <RHI/Vulkan/Resource/TextureView.h>
#endif

#if defined(RTRC_STATIC_RHI) && defined(RTRC_RHI_DIRECTX12)
#include <RHI/DirectX12/Context/BackBufferSemaphore.h>
#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Context/Instance.h>
#include <RHI/DirectX12/Context/Surface.h>
#include <RHI/DirectX12/Context/Swapchain.h>
#include <RHI/DirectX12/Pipeline/BindingGroup.h>
#include <RHI/DirectX12/Pipeline/BindingGroupLayout.h>
#include <RHI/DirectX12/Pipeline/BindingLayout.h>
#include <RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <RHI/DirectX12/Pipeline/RayTracingLibrary.h>
#include <RHI/DirectX12/Pipeline/RayTracingPipeline.h>
#include <RHI/DirectX12/Pipeline/Shader.h>
#include <RHI/DirectX12/Queue/CommandBuffer.h>
#include <RHI/DirectX12/Queue/CommandPool.h>
#include <RHI/DirectX12/Queue/Fence.h>
#include <RHI/DirectX12/Queue/Queue.h>
#include <RHI/DirectX12/Queue/Semaphore.h>
#include <RHI/DirectX12/RayTracing/Blas.h>
#include <RHI/DirectX12/RayTracing/BlasPrebuildInfo.h>
#include <RHI/DirectX12/RayTracing/Tlas.h>
#include <RHI/DirectX12/RayTracing/TlasPrebuildInfo.h>
#include <RHI/DirectX12/Resource/Buffer.h>
#include <RHI/DirectX12/Resource/BufferView.h>
#include <RHI/DirectX12/Resource/Sampler.h>
#include <RHI/DirectX12/Resource/Texture.h>
#include <RHI/DirectX12/Resource/TextureView.h>
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
