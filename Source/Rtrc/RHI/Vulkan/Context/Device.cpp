#include <algorithm>
#include <array>
#include <map>

#include <fmt/format.h>

#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Pipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
#include <Rtrc/Utils/Enumerate.h>
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

} // namespace anonymous

VulkanDevice::VulkanDevice(
    VkInstance             instance,
    VulkanPhysicalDevice   physicalDevice,
    VkDevice               device,
    const QueueFamilyInfo &queueFamilyInfo)
    : instance_(instance), physicalDevice_(physicalDevice), device_(device), queueFamilies_(queueFamilyInfo)
{
    std::map<uint32_t, uint32_t> familyIndexToNextQueueIndex;
    auto getNextQueue = [&](const std::optional<uint32_t> &familyIndex) -> VkQueue
    {
        if(!familyIndex)
        {
            return nullptr;
        }
        VkQueue queue;
        auto &queueIndex = familyIndexToNextQueueIndex[familyIndex.value()];
        vkGetDeviceQueue(device_, familyIndex.value(), queueIndex, &queue);
        ++queueIndex;
        return queue;
    };

    graphicsQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex);
    presentQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex);
    computeQueue_ = getNextQueue(queueFamilies_.computeFamilyIndex);
    transferQueue_ = getNextQueue(queueFamilies_.transferFamilyIndex);

    const VmaVulkanFunctions vmaFunctions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        /* // need when VMA_DYNAMIC_VULKAN_FUNCTIONS is 0
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
        .vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements*/
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

RC<Queue> VulkanDevice::GetQueue(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return MakeRC<VulkanQueue>(device_, graphicsQueue_, *queueFamilies_.graphicsFamilyIndex);
    case QueueType::Compute: return MakeRC<VulkanQueue>(device_, computeQueue_, *queueFamilies_.computeFamilyIndex);
    case QueueType::Transfer: return MakeRC<VulkanQueue>(device_, transferQueue_, *queueFamilies_.transferFamilyIndex);
    }
    Unreachable();
}

RC<Fence> VulkanDevice::CreateFence(bool signaled)
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
    return MakeRC<VulkanFence>(device_, fence);
}

RC<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    assert(presentQueue_);
    auto surface = ToShared(DynamicCast<VulkanSurface>(window.CreateVulkanSurface(instance_)));

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

    auto queue = MakeRC<VulkanQueue>(device_, presentQueue_, queueFamilies_.graphicsFamilyIndex.value());
    const Texture2DDesc imageDescription = {
        .format      = desc.format,
        .width       = extent.width,
        .height      = extent.height,
        .mipLevels   = 1,
        .arraySize   = 1,
        .sampleCount = 1
    };
    return MakeRC<VulkanSwapchain>(std::move(surface), std::move(queue), imageDescription, device_, swapchain);
}

RC<RawShader> VulkanDevice::CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type)
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
    return MakeRC<VulkanShader>(device_, shaderModule, std::move(entryPoint), type);
}

RC<PipelineBuilder> VulkanDevice::CreatePipelineBuilder()
{
    return MakeRC<VulkanPipelineBuilder>(device_);
}

RC<BindingGroupLayout> VulkanDevice::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
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
                .descriptorCount = binding.isArray ? binding.arraySize : 1,
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
            const uint32_t arraySize = binding.isArray ? binding.arraySize : 1;
            if(arraySize != descSetBindings.back().descriptorCount)
            {
                throw Exception("aliasing between descriptor arrays with different sizes is not allowed");
            }
            descSetBindings.back().stageFlags |= TranslateShaderStageFlag(binding.shaderStages);
        }
    }

    /*std::vector<VkDescriptorBindingFlags> flags(descSetBindings.size(), 0);
    for(auto &&[i, alias] : Enumerate(desc.bindings))
    {
        auto &binding = alias.front();
        if(binding.isBindless)
        {
            assert(i == desc.bindings.size() - 1);
            flags[i] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                     | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                     | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
        }
    }

    const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = static_cast<uint32_t>(flags.size()),
        .pBindingFlags = flags.data()
    };*/

    const VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        //.pNext        = &bindingFlagsCreateInfo ,
        .bindingCount = static_cast<uint32_t>(descSetBindings.size()),
        .pBindings    = descSetBindings.data()
    };

    VkDescriptorSetLayout layout;
    VK_FAIL_MSG(
        vkCreateDescriptorSetLayout(device_, &createInfo, VK_ALLOC, &layout),
        "failed to create vulkan descriptor set layout");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorSetLayout(device_, layout, VK_ALLOC); };

    return MakeRC<VulkanBindingGroupLayout>(desc, std::move(descSetBindings), device_, layout);
}

RC<BindingLayout> VulkanDevice::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    std::vector<VkDescriptorSetLayout> setLayouts;
    for(auto &group : desc.groups)
    {
        setLayouts.push_back(reinterpret_cast<VulkanBindingGroupLayout *>(group.get())->GetLayout());
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

    return MakeRC<VulkanBindingLayout>(desc, device_, layout);
}

RC<Texture> VulkanDevice::CreateTexture2D(const Texture2DDesc &desc)
{
    const VkImageCreateInfo imageCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = TranslateTexelFormat(desc.format),
        .extent        = VkExtent3D{ desc.width, desc.height, 1 },
        .mipLevels     = desc.mipLevels,
        .arrayLayers   = desc.arraySize,
        .samples       = TranslateSampleCount(desc.sampleCount),
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = TranslateTextureUsageFlag(desc.usage),
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = TranslateTextureLayout(desc.initialLayout)
    };
    const VmaAllocationCreateInfo allocCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkImage image; VmaAllocation alloc;
    VK_FAIL_MSG(
        vmaCreateImage(allocator_, &imageCreateInfo, &allocCreateInfo, &image, &alloc, nullptr),
        "failed to create vulkan image");
    RTRC_SCOPE_FAIL{ vmaDestroyImage(allocator_, image, alloc); };

    const VulkanMemoryAllocation memoryAlloc = {
        .allocator = allocator_,
        .allocation = alloc
    };

    return MakeRC<VulkanTexture2D>(desc, device_, image, memoryAlloc, ResourceOwnership::Allocation);
}

RC<Buffer> VulkanDevice::CreateBuffer(const BufferDesc &desc)
{
    const VkBufferCreateInfo createInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = desc.size,
        .usage       = TranslateBufferUsageFlag(desc.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
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

    return MakeRC<VulkanBuffer>(desc, device_, buffer, memoryAlloc, ResourceOwnership::Allocation);
}

void VulkanDevice::WaitIdle()
{
    VK_FAIL_MSG(
        vkDeviceWaitIdle(device_),
        "failed to call vkDeviceWaitIdle");
}

RTRC_RHI_VK_END
