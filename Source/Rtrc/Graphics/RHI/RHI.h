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
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferSRV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferUAV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/MemoryBlock.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>
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

inline void BindingGroupUpdateBatch::Append(
    BindingGroup &group, int index, const ConstantBufferUpdate &cbuffer)
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

RTRC_RHI_END
