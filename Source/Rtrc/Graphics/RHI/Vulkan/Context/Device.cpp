#include <algorithm>
#include <array>
#include <map>

#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/MemoryBlock.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/StaticVector.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_VK_BEGIN

namespace
{

    bool CheckFormatSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkFormat expectedFormat)
    {
        uint32_t supportedFormatCount;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, nullptr))
        {
            throw Exception("failed to get vulkan physical device surface format count");
        };

        std::vector<VkSurfaceFormatKHR> supportedFormats(supportedFormatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, supportedFormats.data()))
        {
            throw Exception("failed to get vulkan physical device surface formats");
        };
        
        for(auto &f : supportedFormats)
        {
            if(f.format == expectedFormat && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                return true;
            }
        }
        return false;
    }

    std::pair<VkSharingMode, StaticVector<uint32_t, 3>> GetVulkanSharingMode(
        QueueConcurrentAccessMode accessMode, const VulkanDevice::QueueFamilyInfo &families)
    {
        StaticVector<uint32_t, 3> sharingQueueFamilyIndices;

        if(accessMode == QueueConcurrentAccessMode::Concurrent)
        {
            auto addIndex = [&](const std::optional<uint32_t> &index)
            {
                if(index.has_value() && !sharingQueueFamilyIndices.Contains(index.value()))
                {
                    sharingQueueFamilyIndices.PushBack(index.value());
                }
            };
            addIndex(families.graphicsFamilyIndex);
            addIndex(families.computeFamilyIndex);
            addIndex(families.transferFamilyIndex);
        }

        const VkSharingMode sharingMode =
            sharingQueueFamilyIndices.GetSize() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

        return { sharingMode, sharingQueueFamilyIndices };
    }

    VkImageCreateInfo TranslateImageCreateInfo(
        const TextureDesc &desc, const VulkanDevice::QueueFamilyInfo &queueFamilies)
    {
        const auto [sharingMode, sharingQueueFamilyIndices] =
            GetVulkanSharingMode(desc.concurrentAccessMode, queueFamilies);

        const uint32_t arrayLayers = desc.dim != TextureDimension::Tex3D ? desc.arraySize : 1;
        const uint32_t depth = desc.dim == TextureDimension::Tex3D ? desc.depth : 1;

        const VkImageCreateInfo imageCreateInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType             = TranslateTextureDimension(desc.dim),
            .format                = TranslateTexelFormat(desc.format),
            .extent                = VkExtent3D{ desc.width, desc.height, depth },
            .mipLevels             = desc.mipLevels,
            .arrayLayers           = arrayLayers,
            .samples               = TranslateSampleCount(static_cast<int>(desc.sampleCount)),
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = TranslateTextureUsageFlag(desc.usage),
            .sharingMode           = sharingMode,
            .queueFamilyIndexCount = static_cast<uint32_t>(sharingQueueFamilyIndices.GetSize()),
            .pQueueFamilyIndices   = sharingQueueFamilyIndices.GetData(),
            .initialLayout         = TranslateImageLayout(desc.initialLayout)
        };
        return imageCreateInfo;
    }

    struct BufferCreateInfoWrapper
    {
        BufferCreateInfoWrapper() = default;
        BufferCreateInfoWrapper(const BufferCreateInfoWrapper &other)
            : createInfo(other.createInfo), queueFamilyIndices(other.queueFamilyIndices)
        {
            *this = other;
        }
        BufferCreateInfoWrapper &operator=(const BufferCreateInfoWrapper &other)
        {
            createInfo = other.createInfo;
            queueFamilyIndices = other.queueFamilyIndices;
            if(createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT)
            {
                createInfo.pQueueFamilyIndices = queueFamilyIndices.GetData();
            }
            return *this;
        }

        VkBufferCreateInfo createInfo = {};
        StaticVector<uint32_t, 3> queueFamilyIndices;
    };

    BufferCreateInfoWrapper TranslateBufferCreateInfo(
        const BufferDesc &desc, const VulkanDevice::QueueFamilyInfo &queueFamilies)
    {
        const auto [sharingMode, sharingQueueFamilyIndices] =
            GetVulkanSharingMode(QueueConcurrentAccessMode::Concurrent, queueFamilies);
        BufferCreateInfoWrapper ret;
        ret.queueFamilyIndices = sharingQueueFamilyIndices;
        ret.createInfo = VkBufferCreateInfo{
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size                  = desc.size,
            .usage                 = TranslateBufferUsageFlag(desc.usage),
            .sharingMode           = sharingMode,
            .queueFamilyIndexCount = static_cast<uint32_t>(ret.queueFamilyIndices.GetSize()),
            .pQueueFamilyIndices   = ret.queueFamilyIndices.GetData()
        };
        return ret;
    }

} // namespace anonymous

VulkanDevice::VulkanDevice(
    VkInstance             instance,
    VulkanPhysicalDevice   physicalDevice,
    VkDevice               device,
    const QueueFamilyInfo &queueFamilyInfo,
    bool                   enableDebug)
    : enableDebug_(enableDebug),
      instance_(instance),
      physicalDevice_(physicalDevice),
      device_(device),
      queueFamilies_(queueFamilyInfo),
      allocator_()
{
    std::map<uint32_t, uint32_t> familyIndexToNextQueueIndex;
    auto getNextQueue = [&](const std::optional<uint32_t> &familyIndex, QueueType type) -> Ptr<VulkanQueue>
    {
        if(!familyIndex)
        {
            return nullptr;
        }
        VkQueue queue;
        auto &queueIndex = familyIndexToNextQueueIndex[familyIndex.value()];
        vkGetDeviceQueue(device_, familyIndex.value(), queueIndex, &queue);
        ++queueIndex;
        return MakePtr<VulkanQueue>(this, queue, type, familyIndex.value());
    };

    graphicsQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex, QueueType::Graphics);
    presentQueue_  = getNextQueue(queueFamilies_.graphicsFamilyIndex, QueueType::Graphics);
    computeQueue_  = getNextQueue(queueFamilies_.computeFamilyIndex, QueueType::Compute);
    transferQueue_ = getNextQueue(queueFamilies_.transferFamilyIndex, QueueType::Transfer);

    const VmaVulkanFunctions vmaFunctions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr
    };
    const VmaAllocatorCreateInfo vmaCreateInfo = {
        .physicalDevice       = physicalDevice_.GetNativeHandle(),
        .device               = device_,
        .pAllocationCallbacks = VK_ALLOC,
        .pVulkanFunctions     = &vmaFunctions,
        .instance             = instance_,
        .vulkanApiVersion     = RTRC_VULKAN_API_VERSION
    };
    VK_FAIL_MSG(
        vmaCreateAllocator(&vmaCreateInfo, &allocator_),
        "failed to create vulkan memory allocator");
}

VulkanDevice::~VulkanDevice()
{
    vmaDestroyAllocator(allocator_);
    vkDestroyDevice(device_, VK_ALLOC);
}

Ptr<Queue> VulkanDevice::GetQueue(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return graphicsQueue_;
    case QueueType::Compute:  return computeQueue_;
    case QueueType::Transfer: return transferQueue_;
    }
    Unreachable();
}

Ptr<CommandPool> VulkanDevice::CreateCommandPool(const Ptr<Queue> &queue)
{
    auto vkQueue = reinterpret_cast<const VulkanQueue *>(queue.Get());
    return vkQueue->CreateCommandPoolImpl();
}

Ptr<Fence> VulkanDevice::CreateFence(bool signaled)
{
    const VkFenceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}
    };
    VkFence fence;
    VK_FAIL_MSG(
        vkCreateFence(device_, &createInfo, VK_ALLOC, &fence),
        "failed to create vulkan fence");
    RTRC_SCOPE_FAIL{ vkDestroyFence(device_, fence, VK_ALLOC); };
    return MakePtr<VulkanFence>(device_, fence);
}

Ptr<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    assert(presentQueue_);
    auto surface = DynamicCast<VulkanSurface>(window.CreateVulkanSurface(instance_));

    // surface capabilities

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice_.GetNativeHandle(), surface->GetSurface(), &surfaceCapabilities))
    {
        throw Exception("failed to get vulkan physical device surface capabilities");
    };

    // check format

    const auto requiredFormat = TranslateTexelFormat(desc.format);
    if(!CheckFormatSupport(physicalDevice_.GetNativeHandle(), surface->GetSurface(), requiredFormat))
    {
        throw Exception(fmt::format("surface format {} is not supported", GetFormatName(desc.format)));
    }

    // present mode

    uint32_t supportedPresentModeCount;
    VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->GetSurface(), &supportedPresentModeCount, nullptr),
        "failed to get vulkan surface present mode count");

    std::vector<VkPresentModeKHR> supportedPresentModes(supportedPresentModeCount);
    VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->GetSurface(),
            &supportedPresentModeCount, supportedPresentModes.data()),
        "failed to get vulkan surface present modes");

    auto presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(auto p : supportedPresentModes)
    {
        if(p == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // extent

    VkExtent2D extent = surfaceCapabilities.currentExtent;
    if(extent.width == UINT32_MAX)
    {
        const auto [fbWidth, fbHeight] = window.GetFramebufferSize();
        extent.width = std::clamp(
            static_cast<uint32_t>(fbWidth),
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(
            static_cast<uint32_t>(fbHeight),
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);
    }

    // image count

    const uint32_t imageCount = std::clamp(
        desc.imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : UINT32_MAX);
    if(imageCount != desc.imageCount)
    {
        throw Exception("unsupported image count");
    }

    // swapchain

    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = surface->GetSurface(),
        .minImageCount         = imageCount,
        .imageFormat           = requiredFormat,
        .imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent           = extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = presentMode,
        .clipped               = true
    };

    VkSwapchainKHR swapchain;
    VK_FAIL_MSG(
        vkCreateSwapchainKHR(device_, &swapchainCreateInfo, VK_ALLOC, &swapchain),
        "failed to create vulkan swapchain");
    RTRC_SCOPE_FAIL{ vkDestroySwapchainKHR(device_, swapchain, VK_ALLOC); };

    // construct result

    const TextureDesc imageDescription = {
        .dim                  = TextureDimension::Tex2D,
        .format               = desc.format,
        .width                = extent.width,
        .height               = extent.height,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .initialLayout        = TextureLayout::Undefined,
        .concurrentAccessMode = QueueConcurrentAccessMode::Exclusive
    };
    return MakePtr<VulkanSwapchain>(std::move(surface), presentQueue_, imageDescription, this, swapchain);
}

Ptr<Semaphore> VulkanDevice::CreateSemaphore(uint64_t initialValue)
{
    const VkSemaphoreTypeCreateInfo typeCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = initialValue
    };

    const VkSemaphoreCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &typeCreateInfo
    };

    VkSemaphore semaphore;
    VK_FAIL_MSG(
        vkCreateSemaphore(device_, &createInfo, VK_ALLOC, &semaphore),
        "failed to create vulkan semaphore");
    RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_, semaphore, VK_ALLOC); };

    return MakePtr<VulkanSemaphore>(device_, semaphore);
}

Ptr<RawShader> VulkanDevice::CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type)
{
    const VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = static_cast<const uint32_t*>(data)
    };
    VkShaderModule shaderModule;
    VK_FAIL_MSG(
        vkCreateShaderModule(device_, &createInfo, VK_ALLOC, &shaderModule),
        "failed to create vulkan shader module");
    RTRC_SCOPE_FAIL{ vkDestroyShaderModule(device_, shaderModule, VK_ALLOC); };
    return MakePtr<VulkanShader>(device_, shaderModule, std::move(entryPoint), type);
}

Ptr<GraphicsPipeline> VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
{
    assert(!desc.viewports.Is<std::monostate>());
    assert(!desc.scissors.Is<std::monostate>());

    std::vector<VkFormat> colorAttachments;
    for(auto f : desc.colorAttachments)
    {
        colorAttachments.push_back(TranslateTexelFormat(f));
    }

    const VkPipelineRenderingCreateInfo renderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachmentFormats = colorAttachments.data(),
        .depthAttachmentFormat = TranslateTexelFormat(desc.depthStencilFormat),
        .stencilAttachmentFormat = TranslateTexelFormat(desc.depthStencilFormat)
    };

    const VkPipelineShaderStageCreateInfo stages[] = {
        static_cast<VulkanShader *>(desc.vertexShader.Get())->GetStageCreateInfo(),
        static_cast<VulkanShader *>(desc.fragmentShader.Get())->GetStageCreateInfo()
    };

    std::vector<VkVertexInputBindingDescription> inputBindingDescs;
    for(auto &&[index, buffer] : Enumerate(desc.vertexBuffers))
    {
        auto &binding = inputBindingDescs.emplace_back();
        binding.binding = static_cast<uint32_t>(index);
        binding.stride = buffer.elementSize;
        binding.inputRate = buffer.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
    }

    std::vector<VkVertexInputAttributeDescription> inputAttributeDescs;
    for(auto &&[index, attrib] : Enumerate(desc.vertexAttributs))
    {
        auto &output = inputAttributeDescs.emplace_back();
        output.location = attrib.location;
        output.binding = attrib.inputBufferIndex;
        output.format = TranslateInputAttributeType(attrib.type);
        output.offset = attrib.byteOffsetInBuffer;
    }

    const VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindingDescs.size()),
        .pVertexBindingDescriptions = inputBindingDescs.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributeDescs.size()),
        .pVertexAttributeDescriptions = inputAttributeDescs.data()
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = TranslatePrimitiveTopology(desc.primitiveTopology)
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    std::vector<VkViewport> vkViewports;
    if(auto vs = desc.viewports.AsIf<std::vector<Viewport>>())
    {
        vkViewports.resize(vs->size());
        for(auto &&[i, v] : Enumerate(*vs))
        {
            vkViewports[i] = TranslateViewport(v);
        }
        viewportState.viewportCount = static_cast<uint32_t>(vkViewports.size());
        viewportState.pViewports = vkViewports.data();
    }
    else if(auto c = desc.viewports.AsIf<int>())
    {
        viewportState.viewportCount = static_cast<uint32_t>(*c);
    }

    std::vector<VkRect2D> vkScissors;
    if(auto ss = desc.scissors.AsIf<std::vector<Scissor>>())
    {
        vkScissors.resize(ss->size());
        for(auto &&[i, s] : Enumerate(*ss))
        {
            vkScissors[i] = TranslateScissor(s);
        }
        viewportState.scissorCount = static_cast<uint32_t>(vkScissors.size());
        viewportState.pScissors = vkScissors.data();
    }
    else if(auto c = desc.scissors.AsIf<int>())
    {
        viewportState.scissorCount = static_cast<uint32_t>(*c);
    }

    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = TranslateFillMode(desc.fillMode),
        .cullMode = TranslateCullMode(desc.cullMode),
        .frontFace = TranslateFrontFaceMode(desc.frontFaceMode),
        .depthBiasEnable = desc.enableDepthBias,
        .depthBiasConstantFactor = desc.depthBiasConstFactor,
        .depthBiasClamp = desc.depthBiasClampValue,
        .depthBiasSlopeFactor = desc.depthBiasSlopeFactor,
        .lineWidth = 1
    };

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = TranslateSampleCount(desc.multisampleCount)
    };

    const VkStencilOpState frontStencilOp = {
        .failOp = TranslateStencilOp(desc.frontStencilOp.failOp),
        .passOp = TranslateStencilOp(desc.frontStencilOp.passOp),
        .depthFailOp = TranslateStencilOp(desc.frontStencilOp.depthFailOp),
        .compareOp = TranslateCompareOp(desc.frontStencilOp.compareOp),
        .compareMask = desc.frontStencilOp.compareMask,
        .writeMask = desc.frontStencilOp.writeMask,
        .reference = 0
    };

    const VkStencilOpState backStencilOp = {
        .failOp = TranslateStencilOp(desc.backStencilOp.failOp),
        .passOp = TranslateStencilOp(desc.backStencilOp.passOp),
        .depthFailOp = TranslateStencilOp(desc.backStencilOp.depthFailOp),
        .compareOp = TranslateCompareOp(desc.backStencilOp.compareOp),
        .compareMask = desc.backStencilOp.compareMask,
        .writeMask = desc.backStencilOp.writeMask,
        .reference = 0
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.enableDepthTest,
        .depthWriteEnable = desc.enableDepthWrite,
        .depthCompareOp = TranslateCompareOp(desc.depthCompareOp),
        .stencilTestEnable = desc.enableStencilTest,
        .front = frontStencilOp,
        .back = backStencilOp
    };

    const auto blendAttachment = VkPipelineColorBlendAttachmentState{
        .blendEnable = desc.enableBlending,
        .srcColorBlendFactor = TranslateBlendFactor(desc.blendingSrcColorFactor),
        .dstColorBlendFactor = TranslateBlendFactor(desc.blendingDstColorFactor),
        .colorBlendOp = TranslateBlendOp(desc.blendingColorOp),
        .srcAlphaBlendFactor = TranslateBlendFactor(desc.blendingSrcAlphaFactor),
        .dstAlphaBlendFactor = TranslateBlendFactor(desc.blendingDstAlphaFactor),
        .alphaBlendOp = TranslateBlendOp(desc.blendingAlphaOp),
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    std::vector blendAttachments(colorAttachments.size(), blendAttachment);
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pAttachments = blendAttachments.data()
    };

    uint32_t dynamicStateCount = 0;
    VkDynamicState dynamicStates[2];

    if(desc.viewports.Is<int>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    }
    else if(desc.viewports.Is<DynamicViewportCount>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT;
    }

    if(desc.scissors.Is<int>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    }
    else if(desc.scissors.Is<DynamicScissorCount>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT;
    }

    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateCount,
        .pDynamicStates = dynamicStates
    };

    auto vkBindingLayout = static_cast<VulkanBindingLayout *>(desc.bindingLayout.Get());
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCreateInfo,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = vkBindingLayout->GetNativeLayout()
    };

    VkPipeline pipeline;
    VK_FAIL_MSG(
        vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_ALLOC, &pipeline),
        "failed to create vulkan graphics pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, VK_ALLOC); };

    return MakePtr<VulkanGraphicsPipeline>(desc.bindingLayout, device_, pipeline);
}

Ptr<ComputePipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc &desc)
{
    const VkComputePipelineCreateInfo pipelineCreateInfo = {
   .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = static_cast<VulkanShader *>(desc.computeShader.Get())->GetStageCreateInfo(),
   .layout = static_cast<VulkanBindingLayout *>(desc.bindingLayout.Get())->GetNativeLayout()
    };
    VkPipeline pipeline;
    VK_FAIL_MSG(
        vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_ALLOC, &pipeline),
        "failed to create vulkan compute pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, VK_ALLOC); };
    return MakePtr<VulkanComputePipeline>(desc.bindingLayout, device_, pipeline);
}

Ptr<BindingGroupLayout> VulkanDevice::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
{
    std::vector<VkDescriptorSetLayoutBinding> descSetBindings;
    for(auto &aliasedBinding : desc.bindings)
    {
        {
            auto &binding = aliasedBinding.front();
            auto vkType = TranslateBindingType(binding.type);
            descSetBindings.push_back(VkDescriptorSetLayoutBinding{
                .binding         = static_cast<uint32_t>(descSetBindings.size()),
                .descriptorType  = vkType,
                .descriptorCount = binding.arraySize.value_or(1),
                .stageFlags      = TranslateShaderStageFlag(binding.shaderStages)
            });
        }
        for(size_t i = 1; i < aliasedBinding.size(); ++i)
        {
            auto &binding = aliasedBinding[i];
            if(TranslateBindingType(binding.type) != descSetBindings.back().descriptorType)
            {
                throw Exception("aliasing between different descriptor types is not allowed");
            }
            const uint32_t arraySize = binding.arraySize.value_or(1);
            if(arraySize != descSetBindings.back().descriptorCount)
            {
                throw Exception("aliasing between descriptor arrays with different sizes is not allowed");
            }
            descSetBindings.back().stageFlags |= TranslateShaderStageFlag(binding.shaderStages);
        }
    }

    const VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(descSetBindings.size()),
        .pBindings    = descSetBindings.data()
    };

    VkDescriptorSetLayout layout;
    VK_FAIL_MSG(
        vkCreateDescriptorSetLayout(device_, &createInfo, VK_ALLOC, &layout),
        "failed to create vulkan descriptor set layout");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorSetLayout(device_, layout, VK_ALLOC); };

    return MakePtr<VulkanBindingGroupLayout>(desc, std::move(descSetBindings), device_, layout);
}

Ptr<BindingGroup> VulkanDevice::CreateBindingGroup(const Ptr<BindingGroupLayout> &bindingGroupLayout)
{
    return reinterpret_cast<const VulkanBindingGroupLayout *>(bindingGroupLayout.Get())->CreateBindingGroupImpl();
}

Ptr<BindingLayout> VulkanDevice::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    std::vector<VkDescriptorSetLayout> setLayouts;
    for(auto &group : desc.groups)
    {
        setLayouts.push_back(reinterpret_cast<VulkanBindingGroupLayout *>(group.Get())->GetLayout());
    }
    const VkPipelineLayoutCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts    = setLayouts.data()
    };

    VkPipelineLayout layout;
    VK_FAIL_MSG(
        vkCreatePipelineLayout(device_, &createInfo, VK_ALLOC, &layout),
        "failed to create vulkan pipeline layout");
    RTRC_SCOPE_FAIL{ vkDestroyPipelineLayout(device_, layout, VK_ALLOC); };

    return MakePtr<VulkanBindingLayout>(desc, device_, layout);
}

Ptr<Texture> VulkanDevice::CreateTexture(const TextureDesc &desc)
{
    const VkImageCreateInfo imageCreateInfo = TranslateImageCreateInfo(desc, queueFamilies_);
    const VmaAllocationCreateInfo allocCreateInfo = { .usage = VMA_MEMORY_USAGE_AUTO };

    VkImage image; VmaAllocation alloc;
    VK_FAIL_MSG(
        vmaCreateImage(allocator_, &imageCreateInfo, &allocCreateInfo, &image, &alloc, nullptr),
        "failed to create vulkan image");
    RTRC_SCOPE_FAIL{ vmaDestroyImage(allocator_, image, alloc); };

    const VulkanMemoryAllocation memoryAlloc = {
        .allocator = allocator_,
        .allocation = alloc
    };

    return MakePtr<VulkanTexture>(desc, this, image, memoryAlloc, ResourceOwnership::Allocation);
}

Ptr<Buffer> VulkanDevice::CreateBuffer(const BufferDesc &desc)
{
    const BufferCreateInfoWrapper createInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    const VmaAllocationCreateInfo allocCreateInfo = {
        .flags = TranslateBufferHostAccessType(desc.hostAccessType),
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBuffer buffer; VmaAllocation alloc;
    VK_FAIL_MSG(
        vmaCreateBuffer(allocator_, &createInfo.createInfo, &allocCreateInfo, &buffer, &alloc, nullptr),
        "failed to create vulkan buffer");
    RTRC_SCOPE_FAIL{ vmaDestroyBuffer(allocator_, buffer, alloc); };

    const VulkanMemoryAllocation memoryAlloc = {
        .allocator = allocator_,
        .allocation = alloc
    };

    return MakePtr<VulkanBuffer>(desc, this, buffer, memoryAlloc, ResourceOwnership::Allocation);
}

Ptr<Sampler> VulkanDevice::CreateSampler(const SamplerDesc &desc)
{
    VkSamplerCustomBorderColorCreateInfoEXT borderColor;
    VkSamplerCreateInfo createInfo = {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = TranslateSamplerFilterMode(desc.magFilter),
        .minFilter        = TranslateSamplerFilterMode(desc.minFilter),
        .mipmapMode       = TranslateSamplerMipmapMode(desc.mipFilter),
        .addressModeU     = TranslateSamplerAddressMode(desc.addressModeU),
        .addressModeV     = TranslateSamplerAddressMode(desc.addressModeV),
        .addressModeW     = TranslateSamplerAddressMode(desc.addressModeW),
        .mipLodBias       = desc.mipLODBias,
        .anisotropyEnable = desc.enableAnisotropy,
        .maxAnisotropy    = static_cast<float>(desc.maxAnisotropy),
        .compareEnable    = desc.enableComparision,
        .compareOp        = TranslateCompareOp(desc.compareOp),
        .minLod           = desc.minLOD,
        .maxLod           = desc.maxLOD
    };

    using BorderColor = std::array<float, 4>;
    if(desc.borderColor == BorderColor{ 0, 0, 0, 1 })
    {
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    }
    else if(desc.borderColor == BorderColor{ 1, 1, 1, 1 })
    {
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }
    else if(desc.borderColor == BorderColor{ 0, 0, 0, 0 })
    {
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
    else
    {
        borderColor.sType                        = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
        borderColor.pNext                        = nullptr;
        borderColor.customBorderColor.float32[0] = desc.borderColor[0];
        borderColor.customBorderColor.float32[1] = desc.borderColor[1];
        borderColor.customBorderColor.float32[2] = desc.borderColor[2];
        borderColor.customBorderColor.float32[3] = desc.borderColor[3];
        borderColor.format                       = VK_FORMAT_UNDEFINED;

        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
        createInfo.pNext       = &borderColor;
    }

    VkSampler sampler;
    VK_FAIL_MSG(
        vkCreateSampler(device_, &createInfo, VK_ALLOC, &sampler),
        "failed to create vulkan sampler");
    RTRC_SCOPE_FAIL{ vkDestroySampler(device_, sampler, VK_ALLOC); };

    return MakePtr<VulkanSampler>(desc, device_, sampler);
}

Ptr<MemoryPropertyRequirements> VulkanDevice::GetMemoryRequirements(
    const TextureDesc &desc, size_t *size, size_t *alignment) const
{
    const VkImageCreateInfo imageCreateInfo = TranslateImageCreateInfo(desc, queueFamilies_);
    const VkDeviceImageMemoryRequirements queryInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
        .pCreateInfo = &imageCreateInfo,
    };

    VkMemoryRequirements2 requirements = { .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetDeviceImageMemoryRequirements(device_, &queryInfo, &requirements);

    if(size)
    {
        *size = requirements.memoryRequirements.size;
    }
    if(alignment)
    {
        *alignment = requirements.memoryRequirements.alignment;
    }
    return MakePtr<VulkanMemoryPropertyRequirements>(requirements.memoryRequirements.memoryTypeBits);
}

Ptr<MemoryPropertyRequirements> VulkanDevice::GetMemoryRequirements(
    const BufferDesc &desc, size_t *size, size_t *alignment) const
{
    const BufferCreateInfoWrapper bufferCreateInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    const VkDeviceBufferMemoryRequirements queryInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pCreateInfo = &bufferCreateInfo.createInfo
    };

    VkMemoryRequirements2 requirements = { .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetDeviceBufferMemoryRequirements(device_, &queryInfo, &requirements);

    if(size)
    {
        *size = requirements.memoryRequirements.size;
    }
    if(alignment)
    {
        *alignment = requirements.memoryRequirements.alignment;
    }
    return MakePtr<VulkanMemoryPropertyRequirements>(requirements.memoryRequirements.memoryTypeBits);
}

Ptr<MemoryBlock> VulkanDevice::CreateMemoryBlock(const MemoryBlockDesc &desc)
{
    assert(desc.properties->IsValid());
    const VkMemoryRequirements memoryRequirements = {
        .size = desc.size,
        .alignment = desc.alignment,
        .memoryTypeBits = static_cast<VulkanMemoryPropertyRequirements*>(desc.properties.Get())->GetMemoryTypeBits()
    };
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation alloc;
    VK_FAIL_MSG(
        vmaAllocateMemory(allocator_, &memoryRequirements, &allocCreateInfo, &alloc, nullptr),
        "failed to allocate memory block");
    RTRC_SCOPE_FAIL{ vmaFreeMemory(allocator_, alloc); };

    return MakePtr<VulkanMemoryBlock>(desc, allocator_, alloc);
}

Ptr<Texture> VulkanDevice::CreatePlacedTexture(
    const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    const VkImageCreateInfo imageCreateInfo = TranslateImageCreateInfo(desc, queueFamilies_);
    VkImage image;
    VK_FAIL_MSG(
        vkCreateImage(device_, &imageCreateInfo, VK_ALLOC, &image),
        "failed to create placed vulkan image");
    RTRC_SCOPE_FAIL{ vkDestroyImage(device_, image, VK_ALLOC); };

    auto vkMemoryBlock = static_cast<VulkanMemoryBlock *>(memoryBlock.Get());
    VK_FAIL_MSG(
        vmaBindImageMemory2(allocator_, vkMemoryBlock->GetAllocation(), offsetInMemoryBlock, image, nullptr),
        "failed to bind vma memory with placed vulkan image");

    const VulkanMemoryAllocation vmaAlloc = { allocator_, vkMemoryBlock->GetAllocation() };
    return MakePtr<VulkanTexture>(desc, this, image, vmaAlloc, ResourceOwnership::Resource);
}

Ptr<Buffer> VulkanDevice::CreatePlacedBuffer(
    const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    const BufferCreateInfoWrapper bufferCreateInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    VkBuffer buffer;
    VK_FAIL_MSG(
        vkCreateBuffer(device_, &bufferCreateInfo.createInfo, VK_ALLOC, &buffer),
        "failed to create placed vulkan buffer");
    RTRC_SCOPE_FAIL{ vkDestroyBuffer(device_, buffer, VK_ALLOC); };

    auto vkMemoryBlock = static_cast<VulkanMemoryBlock *>(memoryBlock.Get());
    VK_FAIL_MSG(
        vmaBindBufferMemory2(allocator_, vkMemoryBlock->GetAllocation(), offsetInMemoryBlock, buffer, nullptr),
        "failed to bind vma memory with placed vulkan buffer");

    const VulkanMemoryAllocation vmaAlloc = { allocator_, vkMemoryBlock->GetAllocation() };
    return MakePtr<VulkanBuffer>(desc, this, buffer, vmaAlloc, ResourceOwnership::Resource);
}

size_t VulkanDevice::GetConstantBufferAlignment() const
{
    return physicalDevice_.GetNativeProperties().limits.minUniformBufferOffsetAlignment;
}

void VulkanDevice::WaitIdle()
{
    VK_FAIL_MSG(
        vkDeviceWaitIdle(device_),
        "failed to call vkDeviceWaitIdle");
}

VkDevice VulkanDevice::GetNativeDevice()
{
    return device_;
}

void VulkanDevice::SetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* name)
{
    if(enableDebug_ && vkSetDebugUtilsObjectNameEXT)
    {
        const VkDebugUtilsObjectNameInfoEXT imageNameInfo =
        {
            .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType   = objectType,
            .objectHandle = objectHandle,
            .pObjectName  = name
        };
        vkSetDebugUtilsObjectNameEXT(device_, &imageNameInfo);
    }
}

uint32_t VulkanDevice::GetQueueFamilyIndex(QueueType type) const
{
    switch(type)
    {
    case QueueType::Graphics: return graphicsQueue_->GetNativeFamilyIndex();
    case QueueType::Compute: return computeQueue_->GetNativeFamilyIndex();
    case QueueType::Transfer: return transferQueue_->GetNativeFamilyIndex();
    }
    Unreachable();
}

VkDevice VulkanDevice::GetNativeDevice() const
{
    return device_;
}

RTRC_RHI_VK_END
