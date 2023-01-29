#include <algorithm>
#include <array>
#include <map>

#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
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
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Blas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/BlasBuildInfo.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferView.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/MemoryBlock.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/StaticVector.h>
#include <Rtrc/Utility/Unreachable.h>

#ifdef RTRC_STATIC_RHI
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandPool.h>
#endif

RTRC_RHI_VK_BEGIN

namespace VkDeviceDetail
{

    bool CheckFormatSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkFormat expectedFormat)
    {
        uint32_t supportedFormatCount;
        RTRC_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, nullptr))
        {
            throw Exception("Failed to get vulkan physical device surface format count");
        };

        std::vector<VkSurfaceFormatKHR> supportedFormats(supportedFormatCount);
        RTRC_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, supportedFormats.data()))
        {
            throw Exception("Failed to get vulkan physical device surface formats");
        };

        return std::ranges::any_of(supportedFormats, [&](const VkSurfaceFormatKHR &f)
        {
            return f.format == expectedFormat && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        });
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

    struct ImageCreateInfoWrapper
    {
        ImageCreateInfoWrapper() = default;
        ImageCreateInfoWrapper(const ImageCreateInfoWrapper &other)
            : createInfo(other.createInfo), queueFamilyIndices(other.queueFamilyIndices)
        {
            *this = other;
        }
        ImageCreateInfoWrapper &operator=(const ImageCreateInfoWrapper &other)
        {
            createInfo = other.createInfo;
            queueFamilyIndices = other.queueFamilyIndices;
            if(createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT)
            {
                createInfo.pQueueFamilyIndices = queueFamilyIndices.GetData();
            }
            return *this;
        }

        VkImageCreateInfo createInfo = {};
        StaticVector<uint32_t, 3> queueFamilyIndices;
    };

    ImageCreateInfoWrapper TranslateImageCreateInfo(
        const TextureDesc &desc, const VulkanDevice::QueueFamilyInfo &queueFamilies)
    {
        const auto [sharingMode, sharingQueueFamilyIndices] =
            GetVulkanSharingMode(desc.concurrentAccessMode, queueFamilies);

        const uint32_t arrayLayers = desc.dim != TextureDimension::Tex3D ? desc.arraySize : 1;
        const uint32_t depth = desc.dim == TextureDimension::Tex3D ? desc.depth : 1;
        ImageCreateInfoWrapper ret;
        ret.queueFamilyIndices = sharingQueueFamilyIndices;
        ret.createInfo = VkImageCreateInfo{
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
            .queueFamilyIndexCount = static_cast<uint32_t>(ret.queueFamilyIndices.GetSize()),
            .pQueueFamilyIndices   = ret.queueFamilyIndices.GetData(),
            .initialLayout         = TranslateImageLayout(desc.initialLayout)
        };
        return ret;
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

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> TranslateVulkanShaderGroups(Span<RayTracingShaderGroup> groups)
    {
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> vkGroups;
        vkGroups.reserve(groups.size());
        for(const RayTracingShaderGroup &group : groups)
        {
            uint32_t generalIndex      = VK_SHADER_UNUSED_KHR;
            uint32_t closestHitIndex   = VK_SHADER_UNUSED_KHR;
            uint32_t anyHitIndex       = VK_SHADER_UNUSED_KHR;
            uint32_t intersectionIndex = VK_SHADER_UNUSED_KHR;
            VkRayTracingShaderGroupTypeKHR type = group.Match(
                [&](const RayTracingTriangleHitShaderGroup &g)
                {
                    if(g.closestHitShaderIndex != RAY_TRACING_UNUSED_SHADER)
                    {
                        closestHitIndex = g.closestHitShaderIndex;
                    }
                    if(g.anyHitShaderIndex != RAY_TRACING_UNUSED_SHADER)
                    {
                        anyHitIndex = g.anyHitShaderIndex;
                    }
                    return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                },
                [&](const RayTracingHitShaderGroup &g)
                {
                    if(g.closestHitShaderIndex)
                    {
                        closestHitIndex = g.closestHitShaderIndex;
                    }
                    if(g.anyHitShaderIndex)
                    {
                        anyHitIndex = g.anyHitShaderIndex;
                    }
                    assert(g.intersectionShaderIndex != RAY_TRACING_UNUSED_SHADER);
                    intersectionIndex = g.intersectionShaderIndex;
                    return VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                },
                [&](const RayTracingRayGenShaderGroup &g)
                {
                    assert(g.rayGenShaderIndex != RAY_TRACING_UNUSED_SHADER);
                    generalIndex = g.rayGenShaderIndex;
                    return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                },
                [&](const RayTracingMissShaderGroup &g)
                {
                    assert(g.missShaderIndex != RAY_TRACING_UNUSED_SHADER);
                    generalIndex = g.missShaderIndex;
                    return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                },
                [&](const RayTracingCallableShaderGroup &g)
                {
                    assert(g.callableShaderIndex != RAY_TRACING_UNUSED_SHADER);
                    generalIndex = g.callableShaderIndex;
                    return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                });
            vkGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR
            {
                .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type               = type,
                .generalShader      = generalIndex,
                .closestHitShader   = closestHitIndex,
                .anyHitShader       = anyHitIndex,
                .intersectionShader = intersectionIndex
            });
        }
        return vkGroups;
    }

} // namespace VkDeviceDetail

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
    const VmaAllocatorCreateInfo vmaCreateInfo =
    {
        .flags                = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice       = physicalDevice_.GetNativeHandle(),
        .device               = device_,
        .pAllocationCallbacks = RTRC_VK_ALLOC,
        .pVulkanFunctions     = &vmaFunctions,
        .instance             = instance_,
        .vulkanApiVersion     = RTRC_VULKAN_API_VERSION
    };
    RTRC_VK_FAIL_MSG(
        vmaCreateAllocator(&vmaCreateInfo, &allocator_),
        "Failed to create vulkan memory allocator");
}

VulkanDevice::~VulkanDevice()
{
    vmaDestroyAllocator(allocator_);
    vkDestroyDevice(device_, RTRC_VK_ALLOC);
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
    return vkQueue->_internalCreateCommandPoolImpl();
}

Ptr<Fence> VulkanDevice::CreateFence(bool signaled)
{
    const VkFenceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}
    };
    VkFence fence;
    RTRC_VK_FAIL_MSG(
        vkCreateFence(device_, &createInfo, RTRC_VK_ALLOC, &fence),
        "Failed to create vulkan fence");
    RTRC_SCOPE_FAIL{ vkDestroyFence(device_, fence, RTRC_VK_ALLOC); };
    return MakePtr<VulkanFence>(device_, fence);
}

Ptr<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    assert(presentQueue_);
    auto surface = DynamicCast<VulkanSurface>(window.CreateVulkanSurface(instance_));

    // surface capabilities

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    RTRC_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice_.GetNativeHandle(), surface->_internalGetSurface(), &surfaceCapabilities))
    {
        throw Exception("Failed to get vulkan physical device surface capabilities");
    };

    // check format

    const auto requiredFormat = TranslateTexelFormat(desc.format);
    if(!VkDeviceDetail::CheckFormatSupport(physicalDevice_.GetNativeHandle(), surface->_internalGetSurface(), requiredFormat))
    {
        throw Exception(fmt::format("Surface format {} is not supported", GetFormatName(desc.format)));
    }

    // present mode

    uint32_t supportedPresentModeCount;
    RTRC_VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->_internalGetSurface(), &supportedPresentModeCount, nullptr),
        "Failed to get vulkan surface present mode count");

    std::vector<VkPresentModeKHR> supportedPresentModes(supportedPresentModeCount);
    RTRC_VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->_internalGetSurface(),
            &supportedPresentModeCount, supportedPresentModes.data()),
        "Failed to get vulkan surface present modes");

    auto presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if(!desc.vsync)
    {
        for(auto p : supportedPresentModes)
        {
            if(p == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
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
        .surface               = surface->_internalGetSurface(),
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
    RTRC_VK_FAIL_MSG(
        vkCreateSwapchainKHR(device_, &swapchainCreateInfo, RTRC_VK_ALLOC, &swapchain),
        "Failed to create vulkan swapchain");
    RTRC_SCOPE_FAIL{ vkDestroySwapchainKHR(device_, swapchain, RTRC_VK_ALLOC); };

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
    RTRC_VK_FAIL_MSG(
        vkCreateSemaphore(device_, &createInfo, RTRC_VK_ALLOC, &semaphore),
        "Failed to create vulkan semaphore");
    RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_, semaphore, RTRC_VK_ALLOC); };

    return MakePtr<VulkanSemaphore>(device_, semaphore);
}

Ptr<RawShader> VulkanDevice::CreateShader(const void *data, size_t size, std::vector<RawShaderEntry> entries)
{
    const VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = static_cast<const uint32_t*>(data)
    };
    VkShaderModule shaderModule;
    RTRC_VK_FAIL_MSG(
        vkCreateShaderModule(device_, &createInfo, RTRC_VK_ALLOC, &shaderModule),
        "Failed to create vulkan shader module");
    return MakePtr<VulkanRawShader>(device_, shaderModule, std::move(entries));
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
        .depthAttachmentFormat = HasDepthAspect(desc.depthStencilFormat) ?
            TranslateTexelFormat(desc.depthStencilFormat) : VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = HasStencilAspect(desc.depthStencilFormat) ?
            TranslateTexelFormat(desc.depthStencilFormat) : VK_FORMAT_UNDEFINED
    };

    const VkPipelineShaderStageCreateInfo stages[] = {
        static_cast<VulkanRawShader *>(desc.vertexShader.Get())->_internalGetStageCreateInfo(),
        static_cast<VulkanRawShader *>(desc.fragmentShader.Get())->_internalGetStageCreateInfo()
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
        .compareMask = desc.stencilReadMask,
        .writeMask = desc.stencilWriteMask,
        .reference = 0
    };

    const VkStencilOpState backStencilOp = {
        .failOp = TranslateStencilOp(desc.backStencilOp.failOp),
        .passOp = TranslateStencilOp(desc.backStencilOp.passOp),
        .depthFailOp = TranslateStencilOp(desc.backStencilOp.depthFailOp),
        .compareOp = TranslateCompareOp(desc.backStencilOp.compareOp),
        .compareMask = desc.stencilReadMask,
        .writeMask = desc.stencilWriteMask,
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
    
    StaticVector<VkDynamicState, 3> dynamicStates;
    if(desc.viewports.Is<int>())
    {
        dynamicStates.PushBack(VK_DYNAMIC_STATE_VIEWPORT);
    }
    else if(desc.viewports.Is<DynamicViewportCount>())
    {
        dynamicStates.PushBack(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
    }
    if(desc.scissors.Is<int>())
    {
        dynamicStates.PushBack(VK_DYNAMIC_STATE_SCISSOR);
    }
    else if(desc.scissors.Is<DynamicScissorCount>())
    {
        dynamicStates.PushBack(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
    }
    dynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.GetSize()),
        .pDynamicStates = dynamicStates.GetData()
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
        .layout = vkBindingLayout->_internalGetNativeLayout()
    };

    VkPipeline pipeline;
    RTRC_VK_FAIL_MSG(
        vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineCreateInfo, RTRC_VK_ALLOC, &pipeline),
        "Failed to create vulkan graphics pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, RTRC_VK_ALLOC); };

    return MakePtr<VulkanGraphicsPipeline>(desc.bindingLayout, device_, pipeline);
}

Ptr<ComputePipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc &desc)
{
    const VkComputePipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = static_cast<VulkanRawShader *>(desc.computeShader.Get())->_internalGetStageCreateInfo(),
        .layout = static_cast<VulkanBindingLayout *>(desc.bindingLayout.Get())->_internalGetNativeLayout()
    };
    VkPipeline pipeline;
    RTRC_VK_FAIL_MSG(
        vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineCreateInfo, RTRC_VK_ALLOC, &pipeline),
        "Failed to create vulkan compute pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, RTRC_VK_ALLOC); };
    return MakePtr<VulkanComputePipeline>(desc.bindingLayout, device_, pipeline);
}

Ptr<RayTracingPipeline> VulkanDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc &desc)
{
    std::vector<VkPipelineShaderStageCreateInfo> vkStages;
    for(auto &rawShader : desc.rawShaders)
    {
        static_cast<const VulkanRawShader *>(rawShader.Get())->
            _internalGetStageCreateInfos(std::back_inserter(vkStages));
    }
    auto vkGroups = VkDeviceDetail::TranslateVulkanShaderGroups(desc.shaderGroups);

    std::vector<VkPipeline> vkLibraries;
    vkLibraries.reserve(desc.libraries.size());
    for(auto &l : desc.libraries)
    {
        vkLibraries.push_back(static_cast<VulkanRayTracingLibrary *>(l.Get())->_internalGetNativePipeline());
    }
    const VkPipelineLibraryCreateInfoKHR libraryCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        .libraryCount = static_cast<uint32_t>(desc.libraries.size()),
        .pLibraries = vkLibraries.data()
    };
    const VkRayTracingPipelineInterfaceCreateInfoKHR interfaceCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR,
        .maxPipelineRayPayloadSize = desc.maxRayPayloadSize,
        .maxPipelineRayHitAttributeSize = desc.maxRayHitAttributeSize
    };

    StaticVector<VkDynamicState, 1> dynamicStates;
    if(desc.useCustomStackSize)
    {
        dynamicStates.PushBack(VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR);
    }
    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.GetSize()),
        .pDynamicStates = dynamicStates.GetData()
    };

    auto pipelineLayout = static_cast<VulkanBindingLayout *>(desc.bindingLayout.Get())->_internalGetNativeLayout();

    const VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo =
    {
        .sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount                   = static_cast<uint32_t>(vkStages.size()),
        .pStages                      = vkStages.data(),
        .groupCount                   = static_cast<uint32_t>(vkGroups.size()),
        .pGroups                      = vkGroups.data(),
        .maxPipelineRayRecursionDepth = desc.maxRecursiveDepth,
        .pLibraryInfo                 = desc.libraries.empty() ? nullptr : &libraryCreateInfo,
        .pLibraryInterface            = &interfaceCreateInfo,
        .pDynamicState                = &dynamicStateCreateInfo,
        .layout                       = pipelineLayout
    };

    VkPipeline pipeline;
    RTRC_VK_FAIL_MSG(
        vkCreateRayTracingPipelinesKHR(device_, nullptr, nullptr, 1, &pipelineCreateInfo, RTRC_VK_ALLOC, &pipeline),
        "Failed to create Vulkan ray tracing pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, RTRC_VK_ALLOC); };

    return MakePtr<VulkanRayTracingPipeline>(desc.bindingLayout, this, pipeline);
}

Ptr<RayTracingLibrary> VulkanDevice::CreateRayTracingLibrary(const RayTracingLibraryDesc &desc)
{
    std::vector<VkPipelineShaderStageCreateInfo> vkStages;
    static_cast<const VulkanRawShader *>(desc.rawShader.Get())->_internalGetStageCreateInfos(std::back_inserter(vkStages));
    auto vkGroups = VkDeviceDetail::TranslateVulkanShaderGroups(desc.shaderGroups);

    const VkRayTracingPipelineInterfaceCreateInfoKHR interfaceCreateInfo =
    {
        .sType                          = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR,
        .maxPipelineRayPayloadSize      = desc.maxRayPayloadSize,
        .maxPipelineRayHitAttributeSize = desc.maxRayHitAttributeSize
    };
    const VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo =
    {
        .sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .flags             = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR,
        .stageCount        = static_cast<uint32_t>(vkStages.size()),
        .pStages           = vkStages.data(),
        .groupCount        = static_cast<uint32_t>(vkGroups.size()),
        .pGroups           = vkGroups.data(),
        .pLibraryInterface = &interfaceCreateInfo
    };
    VkPipeline library;
    RTRC_VK_FAIL_MSG(
        vkCreateRayTracingPipelinesKHR(device_, nullptr, nullptr, 1, &pipelineCreateInfo, RTRC_VK_ALLOC, &library),
        "Failed to create vulkan pipeline library");
    return MakePtr<VulkanRayTracingLibrary>(this, library, desc.maxRayPayloadSize, desc.maxRayHitAttributeSize);
}

Ptr<BindingGroupLayout> VulkanDevice::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
{
    std::vector<VkSampler> samplers;
    for(auto &binding : desc.bindings)
    {
        if(!binding.immutableSamplers.empty())
        {
            if(binding.type != BindingType::Sampler)
            {
                throw Exception("Immutable samplers are specified in non-sampler binding");
            }
            if(!binding.arraySize)
            {
                if(binding.immutableSamplers.size() != 1)
                {
                    throw Exception("Binding.immutableSamplers.size doesn't match binding.arraySize");
                }
            }
            else if(binding.immutableSamplers.size() != *binding.arraySize)
            {
                throw Exception("Binding.immutableSamplers.size doesn't match binding.arraySize");
            }
        }
        for(auto &s : binding.immutableSamplers)
        {
            samplers.push_back(static_cast<VulkanSampler*>(s.Get())->_internalGetNativeSampler());
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> descSetBindings;
    size_t samplerOffset = 0;
    for(auto &binding : desc.bindings)
    {
        auto vkType = TranslateBindingType(binding.type);
        descSetBindings.push_back(VkDescriptorSetLayoutBinding{
            .binding            = static_cast<uint32_t>(descSetBindings.size()),
            .descriptorType     = vkType,
            .descriptorCount    = binding.arraySize.value_or(1),
            .stageFlags         = TranslateShaderStageFlag(binding.shaderStages),
            .pImmutableSamplers = binding.immutableSamplers.empty() ? nullptr : &samplers[samplerOffset]
        });
        samplerOffset += binding.immutableSamplers.size();
    }

    assert(!desc.variableArraySize || desc.bindings.back().arraySize.has_value());
    assert(!desc.variableArraySize || desc.bindings.back().bindless);
    
    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(descSetBindings.size()),
        .pBindings    = descSetBindings.data()
    };

    std::vector<VkDescriptorBindingFlags> bindingFlags;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
    const bool bindless = std::ranges::any_of(desc.bindings, [](const BindingDesc &b) { return b.bindless; });
    if(bindless)
    {
        bindingFlags.resize(desc.bindings.size(), 0);
        for(auto &&[index, binding] : Enumerate(desc.bindings))
        {
            if(binding.bindless)
            {
                assert(binding.arraySize.has_value() || (index == desc.bindings.size() - 1 && desc.variableArraySize));
                bindingFlags[index] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                                     | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                                     | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
            }
        }
        if(desc.variableArraySize)
        {
            bindingFlags.back() |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        }
        bindingFlagsInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo
        {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount  = createInfo.bindingCount,
            .pBindingFlags = bindingFlags.data()
        };
        createInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        createInfo.pNext = &bindingFlagsInfo;
    }

    VkDescriptorSetLayout layout;
    RTRC_VK_FAIL_MSG(
        vkCreateDescriptorSetLayout(device_, &createInfo, RTRC_VK_ALLOC, &layout),
        "Failed to create vulkan descriptor set layout");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorSetLayout(device_, layout, RTRC_VK_ALLOC); };

    return MakePtr<VulkanBindingGroupLayout>(desc, std::move(descSetBindings), device_, layout, bindless);
}

Ptr<BindingGroup> VulkanDevice::CreateBindingGroup(
    const Ptr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize)
{
    auto vkBindingLayout = reinterpret_cast<const VulkanBindingGroupLayout *>(bindingGroupLayout.Get());
    return vkBindingLayout->_internalCreateBindingGroupImpl(variableArraySize);
}

Ptr<BindingLayout> VulkanDevice::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    std::vector<VkDescriptorSetLayout> setLayouts;
    for(auto &group : desc.groups)
    {
        setLayouts.push_back(reinterpret_cast<VulkanBindingGroupLayout *>(group.Get())->_internalGetNativeLayout());
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
    for(auto &range : desc.pushConstantRanges)
    {
        pushConstantRanges.push_back(VkPushConstantRange
        {
            .stageFlags = TranslateShaderStageFlag(range.stages),
            .offset     = range.offset,
            .size       = range.size
        });
    }

    const VkPipelineLayoutCreateInfo createInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts            = setLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges    = pushConstantRanges.data()
    };

    VkPipelineLayout layout;
    RTRC_VK_FAIL_MSG(
        vkCreatePipelineLayout(device_, &createInfo, RTRC_VK_ALLOC, &layout),
        "Failed to create vulkan pipeline layout");
    RTRC_SCOPE_FAIL{ vkDestroyPipelineLayout(device_, layout, RTRC_VK_ALLOC); };

    return MakePtr<VulkanBindingLayout>(desc, device_, layout);
}

void VulkanDevice::UpdateBindingGroups(const BindingGroupUpdateBatch &batch)
{
    // TODO: batch updates of continuous array elements

    LinearAllocator arena(1024 * 1024);
    auto records = batch.GetRecords();
    std::vector<VkWriteDescriptorSet> writes(records.GetSize());
    for(auto &&[i, record] : Enumerate(records))
    {
        auto group = static_cast<VulkanBindingGroup *>(record.group);
        record.data.Match(
            [&](const ConstantBufferUpdate &update)
            {
                auto buffer = static_cast<const VulkanBuffer *>(update.buffer);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, buffer, update.offset, update.range, writes[i]);
            },
            [&](const BufferSrv *bufferSrv)
            {
                auto srv = static_cast<const VulkanBufferSrv *>(bufferSrv);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, srv, writes[i]);
            },
            [&](const BufferUav *bufferUav)
            {
                auto uav = static_cast<const VulkanBufferUav *>(bufferUav);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, uav, writes[i]);
            },
            [&](const TextureSrv *textureSrv)
            {
                auto srv = static_cast<const VulkanTextureSrv *>(textureSrv);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, srv, writes[i]);
            },
            [&](const TextureUav *textureUav)
            {
                auto uav = static_cast<const VulkanTextureUav *>(textureUav);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, uav, writes[i]);
            },
            [&](const Sampler *sampler)
            {
                auto vkSampler = static_cast<const VulkanSampler *>(sampler);
                group->_internalTranslate(
                    arena, record.index, record.arrayElem, vkSampler, writes[i]);
            });
    }
    vkUpdateDescriptorSets(device_, records.GetSize(), writes.data(), 0, nullptr);
}

Ptr<Texture> VulkanDevice::CreateTexture(const TextureDesc &desc)
{
    const auto imageCreateInfo = VkDeviceDetail::TranslateImageCreateInfo(desc, queueFamilies_);
    const VmaAllocationCreateInfo allocCreateInfo = { .usage = VMA_MEMORY_USAGE_AUTO };

    VkImage image; VmaAllocation alloc;
    RTRC_VK_FAIL_MSG(
        vmaCreateImage(allocator_, &imageCreateInfo.createInfo, &allocCreateInfo, &image, &alloc, nullptr),
        "Failed to create vulkan image");
    RTRC_SCOPE_FAIL{ vmaDestroyImage(allocator_, image, alloc); };

    const VulkanMemoryAllocation memoryAlloc = {
        .allocator = allocator_,
        .allocation = alloc
    };

    return MakePtr<VulkanTexture>(desc, this, image, memoryAlloc, ResourceOwnership::Allocation);
}

Ptr<Buffer> VulkanDevice::CreateBuffer(const BufferDesc &desc)
{
    const auto createInfo = VkDeviceDetail::TranslateBufferCreateInfo(desc, queueFamilies_);
    const VmaAllocationCreateInfo allocCreateInfo = {
        .flags = TranslateBufferHostAccessType(desc.hostAccessType),
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBuffer buffer; VmaAllocation alloc;
    RTRC_VK_FAIL_MSG(
        vmaCreateBuffer(allocator_, &createInfo.createInfo, &allocCreateInfo, &buffer, &alloc, nullptr),
        "Failed to create vulkan buffer");
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
    RTRC_VK_FAIL_MSG(
        vkCreateSampler(device_, &createInfo, RTRC_VK_ALLOC, &sampler),
        "Failed to create vulkan sampler");
    RTRC_SCOPE_FAIL{ vkDestroySampler(device_, sampler, RTRC_VK_ALLOC); };

    return MakePtr<VulkanSampler>(desc, this, sampler);
}

Ptr<MemoryPropertyRequirements> VulkanDevice::GetMemoryRequirements(
    const TextureDesc &desc, size_t *size, size_t *alignment) const
{
    const auto imageCreateInfo = VkDeviceDetail::TranslateImageCreateInfo(desc, queueFamilies_);
    const VkDeviceImageMemoryRequirements queryInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
        .pCreateInfo = &imageCreateInfo.createInfo,
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
    const auto bufferCreateInfo = VkDeviceDetail::TranslateBufferCreateInfo(desc, queueFamilies_);
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
        .memoryTypeBits = static_cast<VulkanMemoryPropertyRequirements*>(desc.properties.Get())->_internalGetMemoryTypeBits()
    };
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation alloc;
    RTRC_VK_FAIL_MSG(
        vmaAllocateMemory(allocator_, &memoryRequirements, &allocCreateInfo, &alloc, nullptr),
        "Failed to allocate memory block");
    RTRC_SCOPE_FAIL{ vmaFreeMemory(allocator_, alloc); };

    return MakePtr<VulkanMemoryBlock>(desc, allocator_, alloc);
}

Ptr<Texture> VulkanDevice::CreatePlacedTexture(
    const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    const auto imageCreateInfo = VkDeviceDetail::TranslateImageCreateInfo(desc, queueFamilies_);
    VkImage image;
    RTRC_VK_FAIL_MSG(
        vkCreateImage(device_, &imageCreateInfo.createInfo, RTRC_VK_ALLOC, &image),
        "Failed to create placed vulkan image");
    RTRC_SCOPE_FAIL{ vkDestroyImage(device_, image, RTRC_VK_ALLOC); };

    auto vkMemoryBlock = static_cast<VulkanMemoryBlock *>(memoryBlock.Get());
    RTRC_VK_FAIL_MSG(
        vmaBindImageMemory2(allocator_, vkMemoryBlock->GetAllocation(), offsetInMemoryBlock, image, nullptr),
        "Failed to bind vma memory with placed vulkan image");

    const VulkanMemoryAllocation vmaAlloc = { allocator_, vkMemoryBlock->GetAllocation() };
    return MakePtr<VulkanTexture>(desc, this, image, vmaAlloc, ResourceOwnership::Resource);
}

Ptr<Buffer> VulkanDevice::CreatePlacedBuffer(
    const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    const auto bufferCreateInfo = VkDeviceDetail::TranslateBufferCreateInfo(desc, queueFamilies_);
    VkBuffer buffer;
    RTRC_VK_FAIL_MSG(
        vkCreateBuffer(device_, &bufferCreateInfo.createInfo, RTRC_VK_ALLOC, &buffer),
        "Failed to create placed vulkan buffer");
    RTRC_SCOPE_FAIL{ vkDestroyBuffer(device_, buffer, RTRC_VK_ALLOC); };

    auto vkMemoryBlock = static_cast<VulkanMemoryBlock *>(memoryBlock.Get());
    RTRC_VK_FAIL_MSG(
        vmaBindBufferMemory2(allocator_, vkMemoryBlock->GetAllocation(), offsetInMemoryBlock, buffer, nullptr),
        "Failed to bind vma memory with placed vulkan buffer");

    const VulkanMemoryAllocation vmaAlloc = { allocator_, vkMemoryBlock->GetAllocation() };
    return MakePtr<VulkanBuffer>(desc, this, buffer, vmaAlloc, ResourceOwnership::Resource);
}

size_t VulkanDevice::GetConstantBufferAlignment() const
{
    return physicalDevice_.GetNativeProperties().limits.minUniformBufferOffsetAlignment;
}

void VulkanDevice::WaitIdle()
{
    RTRC_VK_FAIL_MSG(
        vkDeviceWaitIdle(device_),
        "Failed to call vkDeviceWaitIdle");
}

BlasPtr VulkanDevice::CreateBlas(const BufferPtr &buffer, size_t offset, size_t size)
{
    auto vkBuffer = static_cast<VulkanBuffer *>(buffer.Get())->_internalGetNativeBuffer();
    const VkAccelerationStructureCreateInfoKHR createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = vkBuffer,
        .offset = offset,
        .size = size,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    VkAccelerationStructureKHR blas;
    RTRC_VK_FAIL_MSG(
        vkCreateAccelerationStructureKHR(device_, &createInfo, RTRC_VK_ALLOC, &blas),
        "Failed to create vulkan blas");
    RTRC_SCOPE_FAIL{ vkDestroyAccelerationStructureKHR(device_, blas, RTRC_VK_ALLOC); };

    return MakePtr<VulkanBlas>(this, blas, buffer);
}

BlasBuildInfoPtr VulkanDevice::CreateBlasBuilder(
    Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlag flags)
{
    return MakePtr<VulkanBlasBuildInfo>(this, geometries, flags);
}

void VulkanDevice::_internalSetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* name)
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

uint32_t VulkanDevice::_internalGetQueueFamilyIndex(QueueType type) const
{
    switch(type)
    {
    case QueueType::Graphics: return graphicsQueue_->_internalGetNativeFamilyIndex();
    case QueueType::Compute: return computeQueue_->_internalGetNativeFamilyIndex();
    case QueueType::Transfer: return transferQueue_->_internalGetNativeFamilyIndex();
    }
    Unreachable();
}

VkDevice VulkanDevice::_internalGetNativeDevice() const
{
    return device_;
}

RTRC_RHI_VK_END
