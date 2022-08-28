#include <algorithm>
#include <array>
#include <map>

#include <fmt/format.h>

#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/MemoryBlock.h>
#include <Rtrc/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
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
        const Texture2DDesc &desc, const VulkanDevice::QueueFamilyInfo &queueFamilies)
    {
        const auto [sharingMode, sharingQueueFamilyIndices] =
            GetVulkanSharingMode(desc.concurrentAccessMode, queueFamilies);
        const VkImageCreateInfo imageCreateInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = TranslateTexelFormat(desc.format),
            .extent                = VkExtent3D{ desc.width, desc.height, 1 },
            .mipLevels             = desc.mipLevels,
            .arrayLayers           = desc.arraySize,
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

    VkBufferCreateInfo TranslateBufferCreateInfo(
        const BufferDesc &desc, const VulkanDevice::QueueFamilyInfo &queueFamilies)
    {
        const auto [sharingMode, sharingQueueFamilyIndices] =
            GetVulkanSharingMode(desc.concurrentAccessMode, queueFamilies);
        const VkBufferCreateInfo createInfo = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size                  = desc.size,
            .usage                 = TranslateBufferUsageFlag(desc.usage),
            .sharingMode           = sharingMode,
            .queueFamilyIndexCount = static_cast<uint32_t>(sharingQueueFamilyIndices.GetSize()),
            .pQueueFamilyIndices   = sharingQueueFamilyIndices.GetData()
        };
        return createInfo;
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
        return MakePtr<VulkanQueue>(device_, queue, type, familyIndex.value());
    };

    graphicsQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex, QueueType::Graphics);
    presentQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex, QueueType::Graphics);
    computeQueue_ = getNextQueue(queueFamilies_.computeFamilyIndex, QueueType::Compute);
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

    const Texture2DDesc imageDescription = {
        .format               = desc.format,
        .width                = extent.width,
        .height               = extent.height,
        .mipLevels            = 1,
        .arraySize            = 1,
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

Ptr<GraphicsPipelineBuilder> VulkanDevice::CreateGraphicsPipelineBuilder()
{
    return MakePtr<VulkanGraphicsPipelineBuilder>(device_);
}

Ptr<ComputePipelineBuilder> VulkanDevice::CreateComputePipelineBuilder()
{
    return MakePtr<VulkanComputePipelineBuilder>(device_);
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

Ptr<Texture2D> VulkanDevice::CreateTexture2D(const Texture2DDesc &desc)
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

    return MakePtr<VulkanTexture2D>(desc, this, image, memoryAlloc, ResourceOwnership::Allocation);
}

Ptr<Buffer> VulkanDevice::CreateBuffer(const BufferDesc &desc)
{
    const VkBufferCreateInfo createInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    const VmaAllocationCreateInfo allocCreateInfo = {
        .flags = TranslateBufferHostAccessType(desc.hostAccessType),
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBuffer buffer; VmaAllocation alloc;
    VK_FAIL_MSG(
        vmaCreateBuffer(allocator_, &createInfo, &allocCreateInfo, &buffer, &alloc, nullptr),
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
    const Texture2DDesc &desc, size_t *size, size_t *alignment) const
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
    const VkBufferCreateInfo bufferCreateInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    const VkDeviceBufferMemoryRequirements queryInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pCreateInfo = &bufferCreateInfo
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

Ptr<Texture2D> VulkanDevice::CreatePlacedTexture2D(
    const Texture2DDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
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
    return MakePtr<VulkanTexture2D>(desc, this, image, vmaAlloc, ResourceOwnership::Resource);
}

Ptr<Buffer> VulkanDevice::CreatePlacedBuffer(
    const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    const VkBufferCreateInfo bufferCreateInfo = TranslateBufferCreateInfo(desc, queueFamilies_);
    VkBuffer buffer;
    VK_FAIL_MSG(
        vkCreateBuffer(device_, &bufferCreateInfo, VK_ALLOC, &buffer),
        "failed to create placed vulkan buffer");
    RTRC_SCOPE_FAIL{ vkDestroyBuffer(device_, buffer, VK_ALLOC); };

    auto vkMemoryBlock = static_cast<VulkanMemoryBlock *>(memoryBlock.Get());
    VK_FAIL_MSG(
        vmaBindBufferMemory2(allocator_, vkMemoryBlock->GetAllocation(), offsetInMemoryBlock, buffer, nullptr),
        "failed to bind vma memory with placed vulkan buffer");

    const VulkanMemoryAllocation vmaAlloc = { allocator_, vkMemoryBlock->GetAllocation() };
    return MakePtr<VulkanBuffer>(desc, this, buffer, vmaAlloc, ResourceOwnership::Resource);
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

RTRC_RHI_VK_END
