#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/RayTracingPipeline.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/RHI/Vulkan/RayTracing/BlasPrebuildInfo.h>
#include <Rtrc/RHI/Vulkan/RayTracing/TlasPrebuildInfo.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/RHI/Vulkan/Resource/TextureView.h>

RTRC_RHI_VK_BEGIN

namespace CommandBufferDetail
{

    VkImage GetVulkanImage(Texture *tex)
    {
        return static_cast<VulkanTexture*>(tex)->_internalGetNativeImage();
    }

} // namespace CommandBufferDetail

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice *device, VkCommandPool pool, VkCommandBuffer commandBuffer)
    : device_(device), pool_(pool), commandBuffer_(commandBuffer)
{
    
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    assert(commandBuffer_);
    vkFreeCommandBuffers(device_->_internalGetNativeDevice(), pool_, 1, &commandBuffer_);
}

void VulkanCommandBuffer::Begin(CommandBufferFlags flags)
{
    (void)flags; // Currently not used

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    RTRC_VK_FAIL_MSG(
        vkBeginCommandBuffer(commandBuffer_, &beginInfo),
        "failed to begin vulkan command buffer");
}

void VulkanCommandBuffer::End()
{
    RTRC_VK_FAIL_MSG(
        vkEndCommandBuffer(commandBuffer_),
        "failed to end vulkan command buffer");
}

void VulkanCommandBuffer::BeginRenderPass(
    Span<ColorAttachment> colorAttachments, const DepthStencilAttachment &depthStencilAttachment)
{
    assert(!colorAttachments.IsEmpty());
    StaticVector<VkRenderingAttachmentInfo, 8> vkColorAttachments(colorAttachments.GetSize());
    for(auto &&[i, a] : Enumerate(colorAttachments))
    {
        vkColorAttachments[i] = VkRenderingAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = static_cast<VulkanTextureRtv *>(a.renderTargetView.Get())->_internalGetNativeImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp      = TranslateLoadOp(a.loadOp),
            .storeOp     = TranslateStoreOp(a.storeOp),
            .clearValue  = TranslateClearValue(a.clearValue)
        };
    }

    VkRenderingAttachmentInfo dsAttachments[2], *depthAttachment = nullptr, *stencilAttachment = nullptr;
    if(depthStencilAttachment.depthStencilView)
    {
        auto dsv = static_cast<VulkanTextureDsv *>(depthStencilAttachment.depthStencilView.Get());
        if(HasDepthAspect(dsv->GetDesc().format))
        {
            dsAttachments[0] = VkRenderingAttachmentInfo
            {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = dsv->_internalGetNativeImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .loadOp      = TranslateLoadOp(depthStencilAttachment.loadOp),
                .storeOp     = TranslateStoreOp(depthStencilAttachment.storeOp),
                .clearValue  = TranslateClearValue(depthStencilAttachment.clearValue)
            };
            depthAttachment = &dsAttachments[0];
        }
        if(HasStencilAspect(dsv->GetDesc().format))
        {
            dsAttachments[1] = VkRenderingAttachmentInfo
            {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView   = dsv->_internalGetNativeImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .loadOp      = TranslateLoadOp(depthStencilAttachment.loadOp),
                .storeOp     = TranslateStoreOp(depthStencilAttachment.storeOp),
                .clearValue  = TranslateClearValue(depthStencilAttachment.clearValue)
            };
            stencilAttachment = &dsAttachments[1];
        }
        assert(depthAttachment || stencilAttachment);
    }

    const auto &attachment0Desc = static_cast<VulkanTextureRtv *>(
        colorAttachments[0].renderTargetView.Get())->_internalGetTexture()->GetDesc();
    const VkRect2D renderArea =
    {
        .offset = { 0, 0 },
        .extent = { attachment0Desc.width, attachment0Desc.height }
    };
    const VkRenderingInfo renderingInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = renderArea,
        .layerCount           = 1,
        .colorAttachmentCount = colorAttachments.GetSize(),
        .pColorAttachments    = vkColorAttachments.GetData(),
        .pDepthAttachment     = depthAttachment,
        .pStencilAttachment   = stencilAttachment
    };

    vkCmdBeginRendering(commandBuffer_, &renderingInfo);
}

void VulkanCommandBuffer::EndRenderPass()
{
    vkCmdEndRendering(commandBuffer_);
}

void VulkanCommandBuffer::BindPipeline(const OPtr<GraphicsPipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, VK_NULL_HANDLE);
        currentGraphicsPipeline_ = nullptr;
        return;
    }
    auto vkPipeline = static_cast<VulkanGraphicsPipeline *>(pipeline.Get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->_internalGetNativePipeline());
    currentGraphicsPipeline_ = pipeline;
}

void VulkanCommandBuffer::BindPipeline(const OPtr<ComputePipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, VK_NULL_HANDLE);
        currentGraphicsPipeline_ = nullptr;
        return;
    }
    auto vkPipeline = static_cast<VulkanComputePipeline *>(pipeline.Get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->_internalGetNativePipeline());
    currentComputePipeline_ = pipeline;
}

void VulkanCommandBuffer::BindPipeline(const OPtr<RayTracingPipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, VK_NULL_HANDLE);
        currentRayTracingPipeline_ = nullptr;
        return;
    }
    auto vkPipeline = static_cast<VulkanRayTracingPipeline *>(pipeline.Get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkPipeline->_internalGetNativePipeline());
    currentRayTracingPipeline_ = pipeline;
}

void VulkanCommandBuffer::BindPipeline(
    const OPtr<WorkGraphPipeline> &pipeline,
    BufferDeviceAddress            backingMemory,
    size_t                         backingMemorySize,
    bool                           initializeBakingMemory)
{
    throw Exception("Not implemented");
}

void VulkanCommandBuffer::BindGroupsToGraphicsPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentGraphicsPipeline_->GetBindingLayout().Get());
    std::vector<VkDescriptorSet> sets(groups.GetSize());
    for(auto &&[i, g] : Enumerate(groups))
    {
        sets[i] = static_cast<VulkanBindingGroup *>(g.Get())->_internalGetNativeSet();
    }
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout->_internalGetNativeLayout(), startIndex,
        static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::BindGroupsToComputePipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    std::vector<VkDescriptorSet> sets(groups.GetSize());
    for(auto &&[i, g] : Enumerate(groups))
    {
        sets[i] = static_cast<VulkanBindingGroup *>(g.Get())->_internalGetNativeSet();
    }
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout->_internalGetNativeLayout(), startIndex,
        static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::BindGroupsToRayTracingPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentRayTracingPipeline_->GetBindingLayout().Get());
    std::vector<VkDescriptorSet> sets(groups.GetSize());
    for(auto &&[i, g] : Enumerate(groups))
    {
        sets[i] = static_cast<VulkanBindingGroup *>(g.Get())->_internalGetNativeSet();
    }
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout->_internalGetNativeLayout(), startIndex,
        static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::BindGroupsToWorkGraphPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    throw Exception("Not implemented");
}

void VulkanCommandBuffer::BindGroupToGraphicsPipeline(int index, const OPtr<BindingGroup> &group)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentGraphicsPipeline_->GetBindingLayout().Get());
    VkDescriptorSet set = static_cast<VulkanBindingGroup *>(group.Get())->_internalGetNativeSet();
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout->_internalGetNativeLayout(), index, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::BindGroupToComputePipeline(int index, const OPtr<BindingGroup> &group)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    VkDescriptorSet set = static_cast<VulkanBindingGroup *>(group.Get())->_internalGetNativeSet();
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout->_internalGetNativeLayout(), index, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::BindGroupToRayTracingPipeline(int index, const OPtr<BindingGroup> &group)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentRayTracingPipeline_->GetBindingLayout().Get());
    VkDescriptorSet set = static_cast<VulkanBindingGroup *>(group.Get())->_internalGetNativeSet();
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout->_internalGetNativeLayout(), index, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::BindGroupToWorkGraphPipeline(int index, const OPtr<BindingGroup> &group)
{
    throw Exception("Not implemented");
}

void VulkanCommandBuffer::SetViewports(Span<Viewport> viewports)
{
    std::vector<VkViewport> vkViewports(viewports.GetSize());
    for(auto &&[i, v] : Enumerate(viewports))
    {
        vkViewports[i] = TranslateViewport(v);
    }
    vkCmdSetViewport(commandBuffer_, 0, viewports.GetSize(), vkViewports.data());
}

void VulkanCommandBuffer::SetScissors(Span<Scissor> scissors)
{
    std::vector<VkRect2D> vkScissors(scissors.GetSize());
    for(auto &&[i, s] : Enumerate(scissors))
    {
        vkScissors[i] = TranslateScissor(s);
    }
    vkCmdSetScissor(commandBuffer_, 0, scissors.GetSize(), vkScissors.data());
}

void VulkanCommandBuffer::SetViewportsWithCount(Span<Viewport> viewports)
{
    std::vector<VkViewport> vkViewports(viewports.GetSize());
    for(auto &&[i, v] : Enumerate(viewports))
    {
        vkViewports[i] = TranslateViewport(v);
    }
    vkCmdSetViewportWithCount(commandBuffer_, viewports.GetSize(), vkViewports.data());
}

void VulkanCommandBuffer::SetScissorsWithCount(Span<Scissor> scissors)
{
    std::vector<VkRect2D> vkScissors(scissors.GetSize());
    for(auto &&[i, s] : Enumerate(scissors))
    {
        vkScissors[i] = TranslateScissor(s);
    }
    vkCmdSetScissorWithCount(commandBuffer_, scissors.GetSize(), vkScissors.data());
}

void VulkanCommandBuffer::SetVertexBuffer(int slot, Span<BufferRPtr> buffers, Span<size_t> byteOffsets, Span<size_t> byteStrides)
{
    std::vector<VkBuffer> vkBuffers(buffers.size());
    for(size_t i = 0; i < buffers.size(); ++i)
    {
        vkBuffers[i] = static_cast<const VulkanBuffer *>(buffers[i].Get())->_internalGetNativeBuffer();
    }
    vkCmdBindVertexBuffers(
        commandBuffer_, static_cast<uint32_t>(slot), buffers.GetSize(), vkBuffers.data(), byteOffsets.GetData());
}

void VulkanCommandBuffer::SetIndexBuffer(const BufferRPtr &buffer, size_t byteOffset, IndexFormat format)
{
    VkBuffer vkBuffer = static_cast<const VulkanBuffer *>(buffer.Get())->_internalGetNativeBuffer();
    const VkIndexType indexType = TranslateIndexFormat(format);
    vkCmdBindIndexBuffer(commandBuffer_, vkBuffer, byteOffset, indexType);
}

void VulkanCommandBuffer::SetStencilReferenceValue(uint8_t value)
{
    vkCmdSetStencilReference(commandBuffer_, VK_STENCIL_FACE_FRONT_AND_BACK, value);
}

void VulkanCommandBuffer::SetGraphicsPushConstants(
    uint32_t rangeIndex, uint32_t offsetInRange, uint32_t size, const void *data)
{
    auto vkBindingLayout = static_cast<const VulkanBindingLayout *>(currentGraphicsPipeline_->GetBindingLayout().Get());
    auto &bindingLayoutDesc = vkBindingLayout->_internalGetDesc();
    auto &range = bindingLayoutDesc.pushConstantRanges[rangeIndex];
    const size_t offset = range.offset + offsetInRange;
    vkCmdPushConstants(
        commandBuffer_, vkBindingLayout->_internalGetNativeLayout(),
        TranslateShaderStageFlag(range.stages), static_cast<uint32_t>(offset), size, data);
}

void VulkanCommandBuffer::SetComputePushConstants(
    uint32_t rangeIndex, uint32_t offsetInRange, uint32_t size, const void *data)
{
    auto vkBindingLayout = static_cast<const VulkanBindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    auto &bindingLayoutDesc = vkBindingLayout->_internalGetDesc();
    auto &range = bindingLayoutDesc.pushConstantRanges[rangeIndex];
    const size_t offset = range.offset + offsetInRange;
    vkCmdPushConstants(
        commandBuffer_, vkBindingLayout->_internalGetNativeLayout(), 
        TranslateShaderStageFlag(range.stages), static_cast<uint32_t>(offset), size, data);
}

void VulkanCommandBuffer::Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
{
    vkCmdDraw(
        commandBuffer_,
        static_cast<uint32_t>(vertexCount),
        static_cast<uint32_t>(instanceCount),
        static_cast<uint32_t>(firstVertex),
        static_cast<uint32_t>(firstInstance));
}

void VulkanCommandBuffer::DrawIndexed(
    int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance)
{
    vkCmdDrawIndexed(
        commandBuffer_,
        static_cast<uint32_t>(indexCount),
        static_cast<uint32_t>(instanceCount),
        static_cast<uint32_t>(firstIndex),
        firstVertex,
        static_cast<uint32_t>(firstInstance));
}

void VulkanCommandBuffer::DispatchMesh(int groupCountX, int groupCountY, int groupCountZ)
{
    vkCmdDrawMeshTasksEXT(commandBuffer_, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
    vkCmdDispatch(
        commandBuffer_,
        static_cast<uint32_t>(groupCountX),
        static_cast<uint32_t>(groupCountY),
        static_cast<uint32_t>(groupCountZ));
}

void VulkanCommandBuffer::DispatchNode(
    uint32_t entryIndex, uint32_t recordCount, uint32_t recordStride, const void *records)
{
    throw Exception("Not implemented");
}

void VulkanCommandBuffer::TraceRays(
    int                             rayCountX,
    int                             rayCountY,
    int                             rayCountZ,
    const ShaderBindingTableRegion &rayGenSbt,
    const ShaderBindingTableRegion &missSbt,
    const ShaderBindingTableRegion &hitSbt,
    const ShaderBindingTableRegion &callableSbt)
{
    auto ConvertSbt = [](const ShaderBindingTableRegion &sbt)
    {
        return VkStridedDeviceAddressRegionKHR
        {
            .deviceAddress = sbt.deviceAddress.address,
            .stride        = sbt.stride,
            .size          = sbt.size
        };
    };
    const VkStridedDeviceAddressRegionKHR vkRayGenSbt = ConvertSbt(rayGenSbt);
    const VkStridedDeviceAddressRegionKHR vkMissSbt = ConvertSbt(missSbt);
    const VkStridedDeviceAddressRegionKHR vkHitSbt = ConvertSbt(hitSbt);
    const VkStridedDeviceAddressRegionKHR vkCallableSbt = ConvertSbt(callableSbt);
    vkCmdTraceRaysKHR(
        commandBuffer_, &vkRayGenSbt, &vkMissSbt, &vkHitSbt, &vkCallableSbt, rayCountX, rayCountY, rayCountZ);
}

void VulkanCommandBuffer::DispatchIndirect(const BufferRPtr &buffer, size_t byteOffset)
{
    auto vkBuffer = static_cast<VulkanBuffer *>(buffer.Get())->_internalGetNativeBuffer();
    vkCmdDispatchIndirect(commandBuffer_, vkBuffer, byteOffset);
}

void VulkanCommandBuffer::DrawIndexedIndirect(
    const BufferRPtr &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride)
{
    auto vkBuffer = static_cast<VulkanBuffer *>(buffer.Get())->_internalGetNativeBuffer();
    vkCmdDrawIndexedIndirect(commandBuffer_, vkBuffer, byteOffset, drawCount, static_cast<uint32_t>(byteStride));
}

void VulkanCommandBuffer::CopyBuffer(
    Buffer *dst, size_t dstOffset,
    Buffer *src, size_t srcOffset, size_t range)
{
    const VkBufferCopy copy = {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size      = range
    };
    auto vkSrc = static_cast<VulkanBuffer *>(src)->_internalGetNativeBuffer();
    auto vkDst = static_cast<VulkanBuffer *>(dst)->_internalGetNativeBuffer();
    vkCmdCopyBuffer(commandBuffer_, vkSrc, vkDst, 1, &copy);
}

void VulkanCommandBuffer::CopyColorTexture(
    Texture *dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
    Texture *src, uint32_t srcMipLevel, uint32_t srcArrayLayer)
{
    const VkImageCopy copy =
    {
        .srcSubresource = VkImageSubresourceLayers
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = srcMipLevel,
            .baseArrayLayer = srcArrayLayer,
            .layerCount     = 1
        },
        .srcOffset = { 0, 0, 0 },
        .dstSubresource = VkImageSubresourceLayers
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = dstMipLevel,
            .baseArrayLayer = dstArrayLayer,
            .layerCount     = 1
        },
        .dstOffset = { 0, 0, 0 },
        .extent =
        {
            dst->GetDesc().width,
            dst->GetDesc().height,
            dst->GetDesc().depth
        }
    };
    auto vkSrc = CommandBufferDetail::GetVulkanImage(src);
    auto vkDst = CommandBufferDetail::GetVulkanImage(dst);
    vkCmdCopyImage(
        commandBuffer_,
        vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy);
}

void VulkanCommandBuffer::CopyColorTexture(
    Texture* dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u& dstOffset,
    Texture* src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u& srcOffset,
    const Vector3u& size)
{
    const VkImageCopy copy =
    {
        .srcSubresource = VkImageSubresourceLayers
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = srcMipLevel,
            .baseArrayLayer = srcArrayLayer,
            .layerCount     = 1
        },
        .srcOffset =
        {
            .x = static_cast<int>(srcOffset.x),
            .y = static_cast<int>(srcOffset.y),
            .z = static_cast<int>(srcOffset.z)
        },
        .dstSubresource = VkImageSubresourceLayers
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = dstMipLevel,
            .baseArrayLayer = dstArrayLayer,
            .layerCount     = 1
        },
        .dstOffset =
        {
            .x = static_cast<int>(dstOffset.x),
            .y = static_cast<int>(dstOffset.y),
            .z = static_cast<int>(dstOffset.z)
        },
        .extent =
        {
            .width  = size.x,
            .height = size.y,
            .depth  = size.z,
        }
    };
    auto vkSrc = CommandBufferDetail::GetVulkanImage(src);
    auto vkDst = CommandBufferDetail::GetVulkanImage(dst);
    vkCmdCopyImage(
        commandBuffer_,
        vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy);
}

void VulkanCommandBuffer::CopyBufferToTexture(
    Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset, size_t srcRowBytes)
{
    assert(srcRowBytes % GetTexelSize(dst->GetFormat()) == 0);
    assert(HasColorAspect(dst->GetFormat()));

    auto &texDesc = dst->GetDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = srcOffset,
        .bufferRowLength   = static_cast<uint32_t>(srcRowBytes / GetTexelSize(dst->GetFormat())),
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = GetAllAspects(texDesc.format),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = {
            (std::max)(1u, texDesc.width >> mipLevel),
            (std::max)(1u, texDesc.height >> mipLevel),
            (std::max)(1u, texDesc.depth >> mipLevel)
        }
    };
    auto vkSrc = static_cast<VulkanBuffer *>(src)->_internalGetNativeBuffer();
    auto vkDst = CommandBufferDetail::GetVulkanImage(dst);
    vkCmdCopyBufferToImage(commandBuffer_, vkSrc, vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanCommandBuffer::CopyTextureToBuffer(
    Buffer *dst, size_t dstOffset, size_t dstRowBytes, Texture *src, uint32_t mipLevel, uint32_t arrayLayer)
{
    assert(dstRowBytes % GetTexelSize(src->GetFormat()) == 0);
    assert(HasColorAspect(src->GetFormat()));
    auto &texDesc = src->GetDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = dstOffset,
        .bufferRowLength   = static_cast<uint32_t>(dstRowBytes / GetTexelSize(src->GetFormat())),
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = GetAllAspects(texDesc.format),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = {
            (std::max)(1u, texDesc.width >> mipLevel),
            (std::max)(1u, texDesc.height >> mipLevel),
            (std::max)(1u, texDesc.depth >> mipLevel)
        }
    };
    auto vkSrc = CommandBufferDetail::GetVulkanImage(src);
    auto vkDst = static_cast<VulkanBuffer *>(dst)->_internalGetNativeBuffer();
    vkCmdCopyImageToBuffer(commandBuffer_, vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDst, 1, &copy);
}

void VulkanCommandBuffer::ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue)
{
    assert(dst->GetDesc().usage.Contains(TextureUsage::ClearDst));
    auto vkTexture = static_cast<VulkanTexture *>(dst);
    const VkClearColorValue vkClearValue = TranslateClearColorValue(clearValue);
    const VkImageSubresourceRange range =
    {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };
    vkCmdClearColorImage(
        commandBuffer_,
        vkTexture->_internalGetNativeImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vkClearValue, 1, &range);
}

void VulkanCommandBuffer::ClearDepthStencilTexture(
    Texture* dst, const DepthStencilClearValue& clearValue, bool depth, bool stencil)
{
    assert(depth || stencil);
    assert(dst->GetDesc().usage.Contains(TextureUsage::ClearDst));
    auto vkTexture = static_cast<VulkanTexture *>(dst);
    const VkClearDepthStencilValue vkClearValue = { clearValue.depth, clearValue.stencil };
    VkImageAspectFlags aspects = VK_IMAGE_ASPECT_NONE;
    if(depth)
    {
        aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if(stencil)
    {
        aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    const VkImageSubresourceRange range =
    {
        .aspectMask     = aspects,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };
    vkCmdClearDepthStencilImage(
        commandBuffer_,
        vkTexture->_internalGetNativeImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vkClearValue, 1, &range);
}

void VulkanCommandBuffer::BeginDebugEvent(const DebugLabel &label)
{
    VkDebugUtilsLabelEXT vkLabel =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = label.name.c_str()
    };
    if(label.color)
    {
        vkLabel.color[0] = label.color.value()[0];
        vkLabel.color[1] = label.color.value()[1];
        vkLabel.color[2] = label.color.value()[2];
        vkLabel.color[3] = label.color.value()[3];
    }
    else
    {
        vkLabel.color[0] = 0.0f;
        vkLabel.color[1] = 0.0f;
        vkLabel.color[2] = 0.0f;
        vkLabel.color[3] = 0.0f;
    }
    vkCmdBeginDebugUtilsLabelEXT(commandBuffer_, &vkLabel);
}

void VulkanCommandBuffer::EndDebugEvent()
{
    vkCmdEndDebugUtilsLabelEXT(commandBuffer_);
}

void VulkanCommandBuffer::BuildBlas(
    const BlasPrebuildInfoOPtr  &buildInfo,
    Span<RayTracingGeometryDesc> geometries,
    const BlasOPtr              &blas,
    BufferDeviceAddress          scratchBufferAddress)
{
    static_cast<VulkanBlasPrebuildInfo *>(buildInfo.Get())
        ->_internalBuildBlas(this, geometries, blas, scratchBufferAddress);
}

void VulkanCommandBuffer::BuildTlas(
    const TlasPrebuildInfoOPtr        &buildInfo,
    const RayTracingInstanceArrayDesc &instances,
    const TlasOPtr                    &tlas,
    BufferDeviceAddress                scratchBufferAddress)
{
    static_cast<VulkanTlasPrebuildInfo *>(buildInfo.Get())
        ->_internalBuildTlas(this, instances, tlas, scratchBufferAddress);
}

VkCommandBuffer VulkanCommandBuffer::_internalGetNativeCommandBuffer() const
{
    return commandBuffer_;
}

void VulkanCommandBuffer::ExecuteBarriersInternal(
    Span<GlobalMemoryBarrier>      globalMemoryBarriers,
    Span<TextureTransitionBarrier> textureTransitions,
    Span<BufferTransitionBarrier>  bufferTransitions,
    Span<TextureReleaseBarrier>    textureReleaseBarriers,
    Span<TextureAcquireBarrier>    textureAcquireBarriers)
{
    // global barriers

    std::vector<VkMemoryBarrier2> globalBarriers;
    globalBarriers.reserve(globalMemoryBarriers.size());
    for(auto &b : globalMemoryBarriers)
    {
        globalBarriers.push_back(VkMemoryBarrier2
        {
            .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .srcStageMask  = TranslatePipelineStageFlag(b.beforeStages),
            .srcAccessMask = TranslateAccessFlag(b.beforeAccesses),
            .dstStageMask  = TranslatePipelineStageFlag(b.afterStages),
            .dstAccessMask = TranslateAccessFlag(b.afterAccesses)
        });
    }

    // texture barriers

    std::vector<VkImageMemoryBarrier2> imageBarriers;
    imageBarriers.reserve(
        textureTransitions.GetSize() + textureReleaseBarriers.GetSize() + textureAcquireBarriers.GetSize());

    for(auto &transition: textureTransitions)
    {
        if(!transition.texture)
        {
            continue;
        }

        auto srcStage = TranslatePipelineStageFlag(transition.beforeStages);
        auto srcAccess = TranslateAccessFlag(transition.beforeAccesses);
        auto srcLayout = TranslateImageLayout(transition.beforeLayout);

        auto dstStage = TranslatePipelineStageFlag(transition.afterStages);
        auto dstAccess = TranslateAccessFlag(transition.afterAccesses);
        auto dstLayout = TranslateImageLayout(transition.afterLayout);

        if(transition.beforeLayout == TextureLayout::Present)
        {
            assert(transition.afterLayout != TextureLayout::Present);
            srcStage = dstStage;
            srcAccess = VK_ACCESS_2_NONE;
            srcLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        if(transition.afterLayout == TextureLayout::Present)
        {
            assert(transition.beforeLayout != TextureLayout::Present);
            dstStage = VK_PIPELINE_STAGE_2_NONE;
            dstAccess = VK_ACCESS_2_NONE;
        }

        imageBarriers.push_back(VkImageMemoryBarrier2{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = srcStage,
            .srcAccessMask       = srcAccess,
            .dstStageMask        = dstStage,
            .dstAccessMask       = dstAccess,
            .oldLayout           = srcLayout,
            .newLayout           = dstLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = CommandBufferDetail::GetVulkanImage(transition.texture),
            .subresourceRange    = TranslateImageSubresources(transition.texture->GetFormat(), transition.subresources)
        });
    }

    for(auto &release : textureReleaseBarriers)
    {
        if(!release.texture)
        {
            continue;
        }

        const uint32_t beforeQueueFamilyIndex = device_->_internalGetQueueFamilyIndex(release.beforeQueue);
        const uint32_t afterQueueFamilyIndex = device_->_internalGetQueueFamilyIndex(release.afterQueue);
        bool shouldEmitBarrier = true;

        // perform a normal barrier when queues are of the same family
        if(beforeQueueFamilyIndex == afterQueueFamilyIndex)
        {
            // graphics < compute < copy
            // use front queue to submit the barrier
            shouldEmitBarrier = release.beforeQueue <= release.afterQueue;
        }

        if(shouldEmitBarrier)
        {
            auto srcStage = TranslatePipelineStageFlag(release.beforeStages);
            auto srcAccess = TranslateAccessFlag(release.beforeAccesses);
            auto srcLayout = TranslateImageLayout(release.beforeLayout);
            auto dstLayout = TranslateImageLayout(release.afterLayout);

            imageBarriers.push_back(VkImageMemoryBarrier2{
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = srcStage,
                .srcAccessMask       = srcAccess,
                .dstStageMask        = srcStage,
                .dstAccessMask       = VK_PIPELINE_STAGE_2_NONE,
                .oldLayout           = srcLayout,
                .newLayout           = dstLayout,
                .srcQueueFamilyIndex = beforeQueueFamilyIndex,
                .dstQueueFamilyIndex = afterQueueFamilyIndex,
                .image               = CommandBufferDetail::GetVulkanImage(release.texture),
                .subresourceRange    = TranslateImageSubresources(release.texture->GetFormat(), release.subresources)
            });
        }
    }

    for(auto &acquire : textureAcquireBarriers)
    {
        if(!acquire.texture)
        {
            continue;
        }

        const uint32_t beforeQueueFamilyIndex = device_->_internalGetQueueFamilyIndex(acquire.beforeQueue);
        const uint32_t afterQueueFamilyIndex = device_->_internalGetQueueFamilyIndex(acquire.afterQueue);

        bool shouldEmitBarrier = true;
        if(beforeQueueFamilyIndex == afterQueueFamilyIndex)
        {
            shouldEmitBarrier = acquire.beforeQueue > acquire.afterQueue;
        }

        if(shouldEmitBarrier)
        {
            auto srcLayout = TranslateImageLayout(acquire.beforeLayout);
            auto dstStage = TranslatePipelineStageFlag(acquire.afterStages);
            auto dstAccess = TranslateAccessFlag(acquire.afterAccesses);
            auto dstLayout = TranslateImageLayout(acquire.afterLayout);
            
            imageBarriers.push_back(VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = dstStage,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = dstStage,
                .dstAccessMask       = dstAccess,
                .oldLayout           = srcLayout,
                .newLayout           = dstLayout,
                .srcQueueFamilyIndex = beforeQueueFamilyIndex,
                .dstQueueFamilyIndex = afterQueueFamilyIndex,
                .image               = CommandBufferDetail::GetVulkanImage(acquire.texture),
                .subresourceRange    = TranslateImageSubresources(acquire.texture->GetFormat(), acquire.subresources)
            });
        }
    }

    // buffer barriers

    std::vector<VkBufferMemoryBarrier2> bufferBarriers;
    bufferBarriers.reserve(bufferTransitions.GetSize());

    for(auto &transition: bufferTransitions)
    {
        if(!transition.buffer)
        {
            continue;
        }

        auto srcStage = TranslatePipelineStageFlag(transition.beforeStages);
        auto srcAccess = TranslateAccessFlag(transition.beforeAccesses);

        auto dstStage = TranslatePipelineStageFlag(transition.afterStages);
        auto dstAccess = TranslateAccessFlag(transition.afterAccesses);

        bufferBarriers.push_back(VkBufferMemoryBarrier2{
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask        = srcStage,
            .srcAccessMask       = srcAccess,
            .dstStageMask        = dstStage,
            .dstAccessMask       = dstAccess,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = static_cast<VulkanBuffer*>(transition.buffer)->_internalGetNativeBuffer(),
            .offset              = 0,
            .size                = VK_WHOLE_SIZE
        });
    }

    if(!bufferBarriers.empty() || !imageBarriers.empty())
    {
        const VkDependencyInfo dependencyInfo = {
            .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount       = static_cast<uint32_t>(globalBarriers.size()),
            .pMemoryBarriers          = globalBarriers.data(),
            .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = static_cast<uint32_t>(imageBarriers.size()),
            .pImageMemoryBarriers     = imageBarriers.data()
        };
        vkCmdPipelineBarrier2(commandBuffer_, &dependencyInfo);
    }
}

RTRC_RHI_VK_END
