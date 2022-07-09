#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DRTV.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_RHI_VK_BEGIN

namespace
{

    VkImage GetVulkanImage(const Ptr<Texture> &tex)
    {
        return static_cast<VulkanTexture *>(tex.Get())->GetNativeImage();
    }

} // namespace anonymous

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer)
    : device_(device), pool_(pool), commandBuffer_(commandBuffer)
{
    
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    assert(commandBuffer_);
    vkFreeCommandBuffers(device_, pool_, 1, &commandBuffer_);
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

void VulkanCommandBuffer::ExecuteBarriers(
    Span<TextureTransitionBarrier> textureTransitions,
    Span<BufferTransitionBarrier>  bufferTransitions,
    Span<TextureReleaseBarrier>    textureReleaseBarriers,
    Span<TextureAcquireBarrier>    textureAcquireBarriers,
    Span<BufferReleaseBarrier>     bufferReleaseBarriers,
    Span<BufferAcquireBarrier>     bufferAcquireBarriers)
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

        auto srcStage = ExtractPipelineStageFlag(transition.beforeState);
        auto srcAccess = ExtractAccessFlag(transition.beforeState);
        auto srcLayout = ExtractImageLayout(transition.beforeState);

        auto dstStage = ExtractPipelineStageFlag(transition.afterState);
        auto dstAccess = ExtractAccessFlag(transition.afterState);
        auto dstLayout = ExtractImageLayout(transition.afterState);

        if(transition.beforeState == ResourceState::Present)
        {
            srcStage = dstStage;
            srcAccess = VK_PIPELINE_STAGE_2_NONE;
            srcLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        if(transition.afterState == ResourceState::Present)
        {
            dstStage = VK_PIPELINE_STAGE_2_NONE;
            dstAccess = VK_ACCESS_2_NONE;
            dstLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
            .subresourceRange    = VkImageSubresourceRange{
                .aspectMask     = TranslateAspectTypeFlag(transition.aspectTypeFlag),
                .baseMipLevel   = transition.mipLevel,
                .levelCount     = 1,
                .baseArrayLayer = transition.arrayLayer,
                .layerCount     = 1
            }
        });
    }

    for(auto &release : textureReleaseBarriers)
    {
        if(!release.texture)
        {
            continue;
        }

        assert(release.texture->Get2DDesc().concurrentAccessMode == QueueConcurrentAccessMode::Exclusive);

        auto beforeQueue = static_cast<VulkanQueue *>(release.beforeQueue.Get());
        auto afterQueue = static_cast<VulkanQueue *>(release.afterQueue.Get());
        assert(beforeQueue != afterQueue);

        // perform a normal barrier when queues are of the same family
        if(beforeQueue->GetNativeFamilyIndex() == afterQueue->GetNativeFamilyIndex())
        {
            // graphics < compute < copy
            // use fronter queue to submit the barrier
            if(beforeQueue->GetType() <= afterQueue->GetType())
            {
                auto srcStage = ExtractPipelineStageFlag(release.beforeState);
                auto srcAccess = ExtractAccessFlag(release.beforeState);
                auto srcLayout = ExtractImageLayout(release.beforeState);

                auto dstStage = ExtractPipelineStageFlag(release.afterState);
                auto dstAccess = ExtractAccessFlag(release.afterState);
                auto dstLayout = ExtractImageLayout(release.afterState);

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
                    .image               = GetVulkanImage(release.texture),
                    .subresourceRange    = VkImageSubresourceRange{
                        .aspectMask     = TranslateAspectTypeFlag(release.aspectTypeFlag),
                        .baseMipLevel   = release.mipLevel,
                        .levelCount     = 1,
                        .baseArrayLayer = release.arrayLayer,
                        .layerCount     = 1
                    }
                });
            }
        }
        else
        {
            const auto srcStage = ExtractPipelineStageFlag(release.beforeState);
            const auto srcAccess = ExtractAccessFlag(release.beforeState);
            const auto srcLayout = ExtractImageLayout(release.beforeState);
            const auto dstLayout = ExtractImageLayout(release.afterState);

            imageBarriers.push_back(VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = srcStage,
                .srcAccessMask       = srcAccess,
                .dstStageMask        = srcStage,
                .dstAccessMask       = VK_ACCESS_2_NONE,
                .oldLayout           = srcLayout,
                .newLayout           = dstLayout,
                .srcQueueFamilyIndex = beforeQueue->GetNativeFamilyIndex(),
                .dstQueueFamilyIndex = afterQueue->GetNativeFamilyIndex(),
                .image               = GetVulkanImage(release.texture),
                .subresourceRange    = VkImageSubresourceRange{
                    .aspectMask     = TranslateAspectTypeFlag(release.aspectTypeFlag),
                    .baseMipLevel   = release.mipLevel,
                    .levelCount     = 1,
                    .baseArrayLayer = release.arrayLayer,
                    .layerCount     = 1
                }
            });
        }
    }

    for(auto &acquire : textureAcquireBarriers)
    {
        if(!acquire.texture)
        {
            continue;
        }

        auto beforeQueue = static_cast<VulkanQueue *>(acquire.beforeQueue.Get());
        auto afterQueue = static_cast<VulkanQueue *>(acquire.afterQueue.Get());
        assert(beforeQueue != afterQueue);

        if(beforeQueue->GetNativeFamilyIndex() == afterQueue->GetNativeFamilyIndex())
        {
            if(beforeQueue->GetType() > afterQueue->GetType())
            {
                auto srcStage = ExtractPipelineStageFlag(acquire.beforeState);
                auto srcAccess = ExtractAccessFlag(acquire.beforeState);
                auto srcLayout = ExtractImageLayout(acquire.beforeState);

                auto dstStage = ExtractPipelineStageFlag(acquire.afterState);
                auto dstAccess = ExtractAccessFlag(acquire.afterState);
                auto dstLayout = ExtractImageLayout(acquire.afterState);

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
                    .image               = GetVulkanImage(acquire.texture),
                    .subresourceRange    = VkImageSubresourceRange{
                        .aspectMask     = TranslateAspectTypeFlag(acquire.aspectTypeFlag),
                        .baseMipLevel   = acquire.mipLevel,
                        .levelCount     = 1,
                        .baseArrayLayer = acquire.arrayLayer,
                        .layerCount     = 1
                    }
                });
            }
        }
        else
        {
            auto srcLayout = ExtractImageLayout(acquire.beforeState);
            auto dstStage = ExtractPipelineStageFlag(acquire.afterState);
            auto dstAccess = ExtractAccessFlag(acquire.afterState);
            auto dstLayout = ExtractImageLayout(acquire.afterState);
            
            imageBarriers.push_back(VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = dstStage,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = dstStage,
                .dstAccessMask       = dstAccess,
                .oldLayout           = srcLayout,
                .newLayout           = dstLayout,
                .srcQueueFamilyIndex = beforeQueue->GetNativeFamilyIndex(),
                .dstQueueFamilyIndex = afterQueue->GetNativeFamilyIndex(),
                .image               = GetVulkanImage(acquire.texture),
                .subresourceRange    = VkImageSubresourceRange{
                    .aspectMask     = TranslateAspectTypeFlag(acquire.aspectTypeFlag),
                    .baseMipLevel   = acquire.mipLevel,
                    .levelCount     = 1,
                    .baseArrayLayer = acquire.arrayLayer,
                    .layerCount     = 1
                }
            });
        }
    }

    // buffer barriers

    std::vector<VkBufferMemoryBarrier2> bufferBarriers;
    bufferBarriers.reserve(
        bufferTransitions.GetSize() + bufferReleaseBarriers.GetSize() + bufferAcquireBarriers.GetSize());

    for(auto &transition: bufferTransitions)
    {
        if(!transition.buffer)
        {
            continue;
        }

        auto srcStage = ExtractPipelineStageFlag(transition.beforeState);
        auto srcAccess = ExtractAccessFlag(transition.beforeState);

        auto dstStage = ExtractPipelineStageFlag(transition.afterState);
        auto dstAccess = ExtractAccessFlag(transition.afterState);

        bufferBarriers.push_back(VkBufferMemoryBarrier2{
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask        = srcStage,
            .srcAccessMask       = srcAccess,
            .dstStageMask        = dstStage,
            .dstAccessMask       = dstAccess,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = static_cast<VulkanBuffer*>(transition.buffer.Get())->GetNativeBuffer(),
            .offset              = 0,
            .size                = VK_WHOLE_SIZE
        });
    }

    for(auto &release : bufferReleaseBarriers)
    {
        if(!release.buffer)
        {
            continue;
        }

        assert(release.buffer->GetDesc().concurrentAccessMode == QueueConcurrentAccessMode::Exclusive);

        auto beforeQueue = static_cast<VulkanQueue *>(release.beforeQueue.Get());
        auto afterQueue = static_cast<VulkanQueue *>(release.afterQueue.Get());
        assert(beforeQueue != afterQueue);

        // perform a normal barrier when queues are of the same family
        if(beforeQueue->GetNativeFamilyIndex() == afterQueue->GetNativeFamilyIndex())
        {
            // graphics < compute < copy
            // use fronter queue to submit the barrier
            if(beforeQueue->GetType() <= afterQueue->GetType())
            {
                auto srcStage = ExtractPipelineStageFlag(release.beforeState);
                auto srcAccess = ExtractAccessFlag(release.beforeState);

                auto dstStage = ExtractPipelineStageFlag(release.afterState);
                auto dstAccess = ExtractAccessFlag(release.afterState);

                bufferBarriers.push_back(VkBufferMemoryBarrier2{
                    .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask        = srcStage,
                    .srcAccessMask       = srcAccess,
                    .dstStageMask        = dstStage,
                    .dstAccessMask       = dstAccess,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer              = static_cast<VulkanBuffer*>(release.buffer.Get())->GetNativeBuffer(),
                    .offset              = 0,
                    .size                = VK_WHOLE_SIZE
                });
            }
        }
        else
        {
            const auto srcStage = ExtractPipelineStageFlag(release.beforeState);
            const auto srcAccess = ExtractAccessFlag(release.beforeState);

            bufferBarriers.push_back(VkBufferMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask        = srcStage,
                .srcAccessMask       = srcAccess,
                .dstStageMask        = srcStage,
                .dstAccessMask       = VK_ACCESS_2_NONE,
                .srcQueueFamilyIndex = beforeQueue->GetNativeFamilyIndex(),
                .dstQueueFamilyIndex = afterQueue->GetNativeFamilyIndex(),
                .buffer              = static_cast<VulkanBuffer*>(release.buffer.Get())->GetNativeBuffer(),
                .offset              = 0,
                .size                = VK_WHOLE_SIZE
            });
        }
    }

    for(auto &acquire : bufferAcquireBarriers)
    {
        if(!acquire.buffer)
        {
            continue;
        }

        auto beforeQueue = static_cast<VulkanQueue *>(acquire.beforeQueue.Get());
        auto afterQueue = static_cast<VulkanQueue *>(acquire.afterQueue.Get());
        assert(beforeQueue != afterQueue);

        if(beforeQueue->GetNativeFamilyIndex() == afterQueue->GetNativeFamilyIndex())
        {
            if(beforeQueue->GetType() > afterQueue->GetType())
            {
                auto srcStage = ExtractPipelineStageFlag(acquire.beforeState);
                auto srcAccess = ExtractAccessFlag(acquire.beforeState);

                auto dstStage = ExtractPipelineStageFlag(acquire.afterState);
                auto dstAccess = ExtractAccessFlag(acquire.afterState);

                bufferBarriers.push_back(VkBufferMemoryBarrier2{
                    .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask        = srcStage,
                    .srcAccessMask       = srcAccess,
                    .dstStageMask        = dstStage,
                    .dstAccessMask       = dstAccess,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer              = static_cast<VulkanBuffer*>(acquire.buffer.Get())->GetNativeBuffer(),
                    .offset              = 0,
                    .size                = VK_WHOLE_SIZE
                });
            }
        }
        else
        {
            auto dstStage = ExtractPipelineStageFlag(acquire.afterState);
            auto dstAccess = ExtractAccessFlag(acquire.afterState);
            
            bufferBarriers.push_back(VkBufferMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask        = dstStage,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = dstStage,
                .dstAccessMask       = dstAccess,
                .srcQueueFamilyIndex = beforeQueue->GetNativeFamilyIndex(),
                .dstQueueFamilyIndex = afterQueue->GetNativeFamilyIndex(),
                .buffer              = static_cast<VulkanBuffer*>(acquire.buffer.Get())->GetNativeBuffer(),
                .offset              = 0,
                .size                = VK_WHOLE_SIZE
            });
        }
    }

    const VkDependencyInfo dependencyInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
        .pBufferMemoryBarriers    = bufferBarriers.data(),
        .imageMemoryBarrierCount  = static_cast<uint32_t>(imageBarriers.size()),
        .pImageMemoryBarriers     = imageBarriers.data()
    };
    vkCmdPipelineBarrier2(commandBuffer_, &dependencyInfo);
}

void VulkanCommandBuffer::BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments)
{
    assert(!colorAttachments.IsEmpty());
    std::vector<VkRenderingAttachmentInfo> vkColorAttachments(colorAttachments.GetSize());
    for(auto &&[i, a] : Enumerate(colorAttachments))
    {
        vkColorAttachments[i] = VkRenderingAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = static_cast<VulkanTexture2DRTV *>(a.rtv.Get())->GetNativeImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp      = TranslateLoadOp(a.loadOp),
            .storeOp     = TranslateStoreOp(a.storeOp),
            .clearValue  = TranslateClearValue(a.clearValue)
        };
    }

    const auto &attachment0Desc = static_cast<VulkanTexture2DRTV *>(
        colorAttachments[0].rtv.Get())->GetTexture()->Get2DDesc();
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
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, nullptr);
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
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, nullptr);
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
    const Ptr<Buffer> &dst, size_t dstOffset,
    const Ptr<Buffer> &src, size_t srcOffset, size_t range)
{
    const VkBufferCopy copy = {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size      = range
    };
    auto vkSrc = static_cast<VulkanBuffer *>(src.Get())->GetNativeBuffer();
    auto vkDst = static_cast<VulkanBuffer *>(dst.Get())->GetNativeBuffer();
    vkCmdCopyBuffer(commandBuffer_, vkSrc, vkDst, 1, &copy);
}

void VulkanCommandBuffer::CopyBufferToTexture(
    const Ptr<Texture> &dst, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer,
    const Ptr<Buffer> &src, size_t srcOffset)
{
    auto &texDesc = dst->Get2DDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = srcOffset,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = TranslateAspectTypeFlag(aspect),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { texDesc.width, texDesc.height, 1 }
    };
    auto vkSrc = static_cast<VulkanBuffer *>(src.Get())->GetNativeBuffer();
    auto vkDst = GetVulkanImage(dst);
    vkCmdCopyBufferToImage(commandBuffer_, vkSrc, vkDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanCommandBuffer::CopyTextureToBuffer(
    const Ptr<Buffer> &dst, size_t dstOffset,
    const Ptr<Texture> &src, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer)
{
    auto &texDesc = src->Get2DDesc();
    const VkBufferImageCopy copy = {
        .bufferOffset      = dstOffset,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
            .aspectMask     = TranslateAspectTypeFlag(aspect),
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { texDesc.width, texDesc.height, 1 }
    };
    auto vkSrc = GetVulkanImage(src);
    auto vkDst = static_cast<VulkanBuffer *>(dst.Get())->GetNativeBuffer();
    vkCmdCopyImageToBuffer(commandBuffer_, vkSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDst, 1, &copy);
}

const Ptr<GraphicsPipeline> &VulkanCommandBuffer::GetCurrentPipeline() const
{
    return currentGraphicsPipeline_;
}

VkCommandBuffer VulkanCommandBuffer::GetNativeCommandBuffer() const
{
    return commandBuffer_;
}

RTRC_RHI_VK_END
