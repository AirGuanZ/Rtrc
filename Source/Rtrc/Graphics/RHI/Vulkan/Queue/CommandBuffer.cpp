#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureRTV.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_RHI_VK_BEGIN

namespace
{

    VkImage GetVulkanImage(Texture *tex)
    {
        return static_cast<VulkanTexture*>(tex)->GetNativeImage();
    }

} // namespace anonymous

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice *device, VkCommandPool pool, VkCommandBuffer commandBuffer)
    : device_(device), pool_(pool), commandBuffer_(commandBuffer)
{
    
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    assert(commandBuffer_);
    vkFreeCommandBuffers(device_->GetNativeDevice(), pool_, 1, &commandBuffer_);
}

void VulkanCommandBuffer::Begin()
{
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VK_FAIL_MSG(
        vkBeginCommandBuffer(commandBuffer_, &beginInfo),
        "failed to begin vulkan command buffer");
}

void VulkanCommandBuffer::End()
{
    VK_FAIL_MSG(
        vkEndCommandBuffer(commandBuffer_),
        "failed to end vulkan command buffer");
}

void VulkanCommandBuffer::BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments)
{
    assert(!colorAttachments.IsEmpty());
    std::vector<VkRenderingAttachmentInfo> vkColorAttachments(colorAttachments.GetSize());
    for(auto &&[i, a] : Enumerate(colorAttachments))
    {
        vkColorAttachments[i] = VkRenderingAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = static_cast<VulkanTextureRTV *>(a.renderTargetView)->GetNativeImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp      = TranslateLoadOp(a.loadOp),
            .storeOp     = TranslateStoreOp(a.storeOp),
            .clearValue  = TranslateClearValue(a.clearValue)
        };
    }

    const auto &attachment0Desc = static_cast<VulkanTextureRTV *>(
        colorAttachments[0].renderTargetView)->GetTexture()->GetDesc();
    const VkRect2D renderArea = {
            .offset = { 0, 0 },
            .extent = { attachment0Desc.width, attachment0Desc.height }
    };
    const VkRenderingInfo renderingInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = renderArea,
        .layerCount           = 1,
        .colorAttachmentCount = colorAttachments.GetSize(),
        .pColorAttachments    = vkColorAttachments.data()
    };

    vkCmdBeginRendering(commandBuffer_, &renderingInfo);
}

void VulkanCommandBuffer::EndRenderPass()
{
    vkCmdEndRendering(commandBuffer_);
}

void VulkanCommandBuffer::BindPipeline(const Ptr<GraphicsPipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, VK_NULL_HANDLE);
        currentGraphicsPipeline_ = nullptr;
        return;
    }
    auto vkPipeline = static_cast<VulkanGraphicsPipeline *>(pipeline.Get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetNativePipeline());
    currentGraphicsPipeline_ = DynamicCast<VulkanGraphicsPipeline>(pipeline);
}

void VulkanCommandBuffer::BindPipeline(const Ptr<ComputePipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, VK_NULL_HANDLE);
        currentGraphicsPipeline_ = nullptr;
        return;
    }
    auto vkPipeline = static_cast<VulkanComputePipeline *>(pipeline.Get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetNativePipeline());
    currentComputePipeline_ = DynamicCast<VulkanComputePipeline>(pipeline);
}

void VulkanCommandBuffer::BindGroupsToGraphicsPipeline(int startIndex, Span<RC<BindingGroup>> groups)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentGraphicsPipeline_->GetBindingLayout().Get());
    std::vector<VkDescriptorSet> sets(groups.GetSize());
    for(auto &&[i, g] : Enumerate(groups))
    {
        sets[i] = static_cast<VulkanBindingGroupInstance *>(g.get())->GetNativeSet();
    }
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout->GetNativeLayout(), startIndex,
        static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::BindGroupsToComputePipeline(int startIndex, Span<RC<BindingGroup>> groups)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    std::vector<VkDescriptorSet> sets(groups.GetSize());
    for(auto &&[i, g] : Enumerate(groups))
    {
        sets[i] = static_cast<VulkanBindingGroupInstance *>(g.get())->GetNativeSet();
    }
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout->GetNativeLayout(), startIndex,
        static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void VulkanCommandBuffer::BindGroupToGraphicsPipeline(int index, const Ptr<BindingGroup> &group)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentGraphicsPipeline_->GetBindingLayout().Get());
    VkDescriptorSet set = static_cast<VulkanBindingGroupInstance *>(group.Get())->GetNativeSet();
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout->GetNativeLayout(), index, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::BindGroupToComputePipeline(int index, const Ptr<BindingGroup> &group)
{
    auto layout = static_cast<const VulkanBindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    VkDescriptorSet set = static_cast<VulkanBindingGroupInstance *>(group.Get())->GetNativeSet();
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout->GetNativeLayout(), index, 1, &set, 0, nullptr);
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

void VulkanCommandBuffer::Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
{
    vkCmdDraw(
        commandBuffer_,
        static_cast<uint32_t>(vertexCount),
        static_cast<uint32_t>(instanceCount),
        static_cast<uint32_t>(firstVertex),
        static_cast<uint32_t>(firstInstance));
}

void VulkanCommandBuffer::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
    vkCmdDispatch(
        commandBuffer_,
        static_cast<uint32_t>(groupCountX),
        static_cast<uint32_t>(groupCountY),
        static_cast<uint32_t>(groupCountZ));
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
    auto vkSrc = static_cast<VulkanBuffer *>(src)->GetNativeBuffer();
    auto vkDst = static_cast<VulkanBuffer *>(dst)->GetNativeBuffer();
    vkCmdCopyBuffer(commandBuffer_, vkSrc, vkDst, 1, &copy);
}

void VulkanCommandBuffer::CopyBufferToColorTexture2D(
    Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset)
{
    auto &texDesc = dst->GetDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = srcOffset,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = GetAllAspects(texDesc.format),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { texDesc.width >> mipLevel, texDesc.height >> mipLevel, 1 }
    };
    auto vkSrc = static_cast<VulkanBuffer *>(src)->GetNativeBuffer();
    auto vkDst = GetVulkanImage(dst);
    vkCmdCopyBufferToImage(commandBuffer_, vkSrc, vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanCommandBuffer::CopyColorTexture2DToBuffer(
    Buffer *dst, size_t dstOffset, Texture *src, uint32_t mipLevel, uint32_t arrayLayer)
{
    auto &texDesc = src->GetDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = dstOffset,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = GetAllAspects(texDesc.format),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { texDesc.width, texDesc.height, 1 }
    };
    auto vkSrc = GetVulkanImage(src);
    auto vkDst = static_cast<VulkanBuffer *>(dst)->GetNativeBuffer();
    vkCmdCopyImageToBuffer(commandBuffer_, vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDst, 1, &copy);
}

void VulkanCommandBuffer::ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue)
{
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
        vkTexture->GetNativeImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

VkCommandBuffer VulkanCommandBuffer::GetNativeCommandBuffer() const
{
    return commandBuffer_;
}

void VulkanCommandBuffer::ExecuteBarriersInternal(
    Span<TextureTransitionBarrier> textureTransitions,
    Span<BufferTransitionBarrier>  bufferTransitions,
    Span<TextureReleaseBarrier>    textureReleaseBarriers,
    Span<TextureAcquireBarrier>    textureAcquireBarriers)
{
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
            .image               = GetVulkanImage(transition.texture),
            .subresourceRange    = TranslateImageSubresources(transition.texture->GetFormat(), transition.subresources)
        });
    }

    for(auto &release : textureReleaseBarriers)
    {
        if(!release.texture)
        {
            continue;
        }

        // assert(release.texture->GetDesc().concurrentAccessMode == QueueConcurrentAccessMode::Exclusive);

        const uint32_t beforeQueueFamilyIndex = device_->GetQueueFamilyIndex(release.beforeQueue);
        const uint32_t afterQueueFamilyIndex = device_->GetQueueFamilyIndex(release.afterQueue);
        bool shouldEmitBarrier = true;

        // perform a normal barrier when queues are of the same family
        if(beforeQueueFamilyIndex == afterQueueFamilyIndex)
        {
            // graphics < compute < copy
            // use fronter queue to submit the barrier
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
                .image               = GetVulkanImage(release.texture),
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

        const uint32_t beforeQueueFamilyIndex = device_->GetQueueFamilyIndex(acquire.beforeQueue);
        const uint32_t afterQueueFamilyIndex = device_->GetQueueFamilyIndex(acquire.afterQueue);

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
                .image               = GetVulkanImage(acquire.texture),
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
            .buffer              = static_cast<VulkanBuffer*>(transition.buffer)->GetNativeBuffer(),
            .offset              = 0,
            .size                = VK_WHOLE_SIZE
        });
    }

    if(!bufferBarriers.empty() || !imageBarriers.empty())
    {
        const VkDependencyInfo dependencyInfo = {
            .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = static_cast<uint32_t>(imageBarriers.size()),
            .pImageMemoryBarriers     = imageBarriers.data()
        };
        vkCmdPipelineBarrier2(commandBuffer_, &dependencyInfo);
    }
}

RTRC_RHI_VK_END
