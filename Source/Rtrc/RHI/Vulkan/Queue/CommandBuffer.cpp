#include <Rtrc/RHI/Vulkan/Pipeline/Pipeline.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DRTV.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_RHI_VK_BEGIN

namespace
{

    VkImage GetVulkanImage(const RC<Texture> &tex)
    {
        return static_cast<VulkanTexture *>(tex.get())->GetNativeImage();
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
    Span<BufferTransitionBarrier> bufferTransitions)
{
    std::vector<VkImageMemoryBarrier2> imageBarriers(textureTransitions.GetSize());
    for(auto &&[i, transition] : Enumerate(textureTransitions))
    {
        imageBarriers[i] = VkImageMemoryBarrier2{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = TranslatePipelineStageFlag(transition.beforeStages),
            .srcAccessMask    = TranslateAccessTypeFlag(transition.beforeAccess),
            .dstStageMask     = TranslatePipelineStageFlag(transition.afterStages),
            .dstAccessMask    = TranslateAccessTypeFlag(transition.afterAccess),
            .oldLayout        = TranslateTextureLayout(transition.beforeLayout),
            .newLayout        = TranslateTextureLayout(transition.afterLayout),
            .image            = GetVulkanImage(transition.texture),
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask     = TranslateAspectTypeFlag(transition.aspectTypeFlag),
                .baseMipLevel   = transition.mipLevel,
                .levelCount     = 1,
                .baseArrayLayer = transition.arrayLayer,
                .layerCount     = 1
            }
        };
    }

    std::vector<VkBufferMemoryBarrier2> bufferBarriers(bufferTransitions.GetSize());
    for(auto &&[i, transition] : Enumerate(bufferTransitions))
    {
        bufferBarriers[i] = VkBufferMemoryBarrier2{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask  = TranslatePipelineStageFlag(transition.beforeStages),
            .srcAccessMask = TranslateAccessTypeFlag(transition.beforeAccess),
            .dstStageMask  = TranslatePipelineStageFlag(transition.afterStages),
            .dstAccessMask = TranslateAccessTypeFlag(transition.afterAccess),
            .buffer        = static_cast<VulkanBuffer*>(transition.buffer.get())->GetNativeBuffer(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE
        };
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
            .imageView   = static_cast<VulkanTexture2DRTV *>(a.rtv.get())->GetNativeImageView(),
            .imageLayout = TranslateTextureLayout(a.layout),
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp      = TranslateLoadOp(a.loadOp),
            .storeOp     = TranslateStoreOp(a.storeOp),
            .clearValue  = TranslateClearValue(a.clearValue)
        };
    }

    const auto &attachment0Desc = static_cast<VulkanTexture2DRTV *>(
        colorAttachments[0].rtv.get())->GetTexture()->Get2DDesc();
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

void VulkanCommandBuffer::BindPipeline(const RC<Pipeline> &pipeline)
{
    if(!pipeline)
    {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, nullptr);
        return;
    }
    auto vkPipeline = static_cast<VulkanPipeline *>(pipeline.get());
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetNativePipeline());
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

VkCommandBuffer VulkanCommandBuffer::GetNativeCommandBuffer() const
{
    return commandBuffer_;
}

RTRC_RHI_VK_END
