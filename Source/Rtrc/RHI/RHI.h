#pragma once

#include <Rtrc/RHI/RHIDeclaration.h>

#if RTRC_STATIC_RHI && RTRC_RHI_VULKAN
#include <Rtrc/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Context/Instance.h>
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/RayTracingLibrary.h>
#include <Rtrc/RHI/Vulkan/Pipeline/RayTracingPipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/RHI/Vulkan/RayTracing/Blas.h>
#include <Rtrc/RHI/Vulkan/RayTracing/BlasPrebuildInfo.h>
#include <Rtrc/RHI/Vulkan/RayTracing/Tlas.h>
#include <Rtrc/RHI/Vulkan/RayTracing/TlasPrebuildInfo.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/BufferView.h>
#include <Rtrc/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/RHI/Vulkan/Resource/TextureView.h>
#include <Rtrc/RHI/Vulkan/Resource/TransientResourcePool/TransientResourcePool.h>
#endif

#if RTRC_STATIC_RHI && RTRC_RHI_DIRECTX12
#include <Rtrc/RHI/DirectX12/Context/BackBufferSemaphore.h>
#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Context/Instance.h>
#include <Rtrc/RHI/DirectX12/Context/Surface.h>
#include <Rtrc/RHI/DirectX12/Context/Swapchain.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingGroupLayout.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingLibrary.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/Shader.h>
#include <Rtrc/RHI/DirectX12/Queue/CommandBuffer.h>
#include <Rtrc/RHI/DirectX12/Queue/CommandPool.h>
#include <Rtrc/RHI/DirectX12/Queue/Fence.h>
#include <Rtrc/RHI/DirectX12/Queue/Queue.h>
#include <Rtrc/RHI/DirectX12/Queue/Semaphore.h>
#include <Rtrc/RHI/DirectX12/RayTracing/Blas.h>
#include <Rtrc/RHI/DirectX12/RayTracing/BlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/RayTracing/Tlas.h>
#include <Rtrc/RHI/DirectX12/RayTracing/TlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/RHI/DirectX12/Resource/BufferView.h>
#include <Rtrc/RHI/DirectX12/Resource/Sampler.h>
#include <Rtrc/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/RHI/DirectX12/Resource/TextureView.h>
#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/TransientResourcePool.h>
#endif

// =============================== inlined implementation ===============================

RTRC_RHI_BEGIN

inline Viewport Viewport::Create(const TextureOPtr &tex, float minDepth, float maxDepth)
{
    return Create(tex->GetDesc(), minDepth, maxDepth);
}

inline Scissor Scissor::Create(const TextureOPtr &tex)
{
    return Create(tex->GetDesc());
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, const ConstantBufferUpdate &cbuffer)
{
    records_.push_back({ &group, index, 0, cbuffer });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, RPtr<BufferSrv> bufferSrv)
{
    records_.push_back({ &group, index, 0, std::move(bufferSrv) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, RPtr<BufferUav> bufferUav)
{
    records_.push_back({ &group, index, 0, std::move(bufferUav) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, OPtr<Sampler> sampler)
{
    records_.push_back({ &group, index, 0, std::move(sampler) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, RPtr<TextureSrv> textureSrv)
{
    records_.push_back({ &group, index, 0, std::move(textureSrv) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, RPtr<TextureUav> textureUav)
{
    records_.push_back({ &group, index, 0, std::move(textureUav) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, OPtr<Tlas> tlas)
{
    records_.push_back({ &group, index, 0, std::move(tlas) });
}

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, int arrayElem, const ConstantBufferUpdate &cbuffer)
{
    records_.push_back({ &group, index, arrayElem, cbuffer });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, RPtr<BufferSrv> bufferSrv)
{
    records_.push_back({ &group, index, arrayElem, std::move(bufferSrv) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, RPtr<BufferUav> bufferUav)
{
    records_.push_back({ &group, index, arrayElem, std::move(bufferUav) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, OPtr<Sampler> sampler)
{
    records_.push_back({ &group, index, arrayElem, std::move(sampler) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, RPtr<TextureSrv> textureSrv)
{
    records_.push_back({ &group, index, arrayElem, std::move(textureSrv) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, RPtr<TextureUav> textureUav)
{
    records_.push_back({ &group, index, arrayElem, std::move(textureUav) });
}

inline void BindingGroupUpdateBatch::Append(BindingGroup &group, int index, int arrayElem, OPtr<Tlas> tlas)
{
    records_.push_back({ &group, index, arrayElem, std::move(tlas) });
}

RTRC_RHI_END
