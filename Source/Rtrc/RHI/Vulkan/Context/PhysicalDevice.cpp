#include <ranges>

#include <Rtrc/Core/Macro/MacroForEach.h>
#include <Rtrc/RHI/Vulkan/Context/PhysicalDevice.h>

RTRC_RHI_VK_BEGIN

namespace VkPhysicalDeviceDetail
{

    struct PhysicalDeviceFeature
    {
        std::string name;
        size_t size;
        size_t alignment;
        VkStructureType type;
        std::vector<std::pair<size_t, std::string>> boolOffsetAndNames;
    };

#define PHYSICAL_DEVICE_FEATURE_BOOL_MEMBER(NAME) \
    ret.boolOffsetAndNames.push_back({ offsetof(StructType, NAME), #NAME });

#define ADD_PHYSICAL_DEVICE_FEATURE(STRUCT_NAME, VK_TYPE, ...)                 \
    ret.push_back([&]                                                          \
    {                                                                          \
        PhysicalDeviceFeature ret = {};                                        \
        ret.name = #STRUCT_NAME;                                               \
        ret.size = sizeof(STRUCT_NAME);                                        \
        ret.alignment = alignof(STRUCT_NAME);                                  \
        ret.type = VK_TYPE;                                                    \
        using StructType = STRUCT_NAME;                                        \
        RTRC_MACRO_FOREACH_1(PHYSICAL_DEVICE_FEATURE_BOOL_MEMBER, __VA_ARGS__) \
        return ret;                                                            \
    }())

    std::vector<PhysicalDeviceFeature> GetRequiredPhysicalDeviceFeatures(bool enableRayTracing)
    {
        std::vector<PhysicalDeviceFeature> ret;

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceCustomBorderColorFeaturesEXT,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT,
            customBorderColors,
            customBorderColorWithoutFormat);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceDynamicRenderingFeatures,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            dynamicRendering);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceSynchronization2Features,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            synchronization2);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDevice16BitStorageFeatures,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
            storageBuffer16BitAccess);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceVulkan12Features,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            shaderFloat16,
            shaderInputAttachmentArrayDynamicIndexing,
            shaderUniformTexelBufferArrayDynamicIndexing,
            shaderStorageTexelBufferArrayDynamicIndexing,
            shaderUniformBufferArrayNonUniformIndexing,
            shaderSampledImageArrayNonUniformIndexing,
            shaderStorageBufferArrayNonUniformIndexing,
            shaderStorageImageArrayNonUniformIndexing,
            shaderInputAttachmentArrayNonUniformIndexing,
            shaderUniformTexelBufferArrayNonUniformIndexing,
            shaderStorageTexelBufferArrayNonUniformIndexing,
            descriptorBindingUniformBufferUpdateAfterBind,
            descriptorBindingSampledImageUpdateAfterBind,
            descriptorBindingStorageImageUpdateAfterBind,
            descriptorBindingStorageBufferUpdateAfterBind,
            descriptorBindingUniformTexelBufferUpdateAfterBind,
            descriptorBindingStorageTexelBufferUpdateAfterBind,
            descriptorBindingUpdateUnusedWhilePending,
            descriptorBindingPartiallyBound,
            descriptorBindingVariableDescriptorCount,
            runtimeDescriptorArray,
            bufferDeviceAddress,
            scalarBlockLayout // Allow StructuredBuffer<struct { float3 }>
        );

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceMaintenance4Features,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
            maintenance4);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceDepthClipEnableFeaturesEXT,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT,
            depthClipEnable);

        ADD_PHYSICAL_DEVICE_FEATURE(
            VkPhysicalDeviceMeshShaderFeaturesEXT,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
            taskShader,
            meshShader);
        
        if(enableRayTracing)
        {
            ADD_PHYSICAL_DEVICE_FEATURE(
                VkPhysicalDeviceAccelerationStructureFeaturesKHR,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                accelerationStructure,
                descriptorBindingAccelerationStructureUpdateAfterBind);

            ADD_PHYSICAL_DEVICE_FEATURE(
                VkPhysicalDeviceRayTracingPipelineFeaturesKHR,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
                rayTracingPipeline,
                rayTracingPipelineTraceRaysIndirect);

            ADD_PHYSICAL_DEVICE_FEATURE(
                VkPhysicalDeviceRayQueryFeaturesKHR,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
                rayQuery);
        }

        return ret;
    }

#undef PHYSICAL_DEVICE_FEATURE_BOOL_MEMBER
#undef ADD_PHYSICAL_DEVICE_FEATURE

    struct VkPhysicalDeviceFeatureHead
    {
        VkStructureType type;
        void *pNext;
    };

    unsigned char *UpAlignPointerTo(unsigned char *ptr, size_t align)
    {
        static_assert(sizeof(unsigned char *) == sizeof(size_t));
        size_t v = reinterpret_cast<size_t>(ptr);
        v = UpAlignTo(v, align);
        return reinterpret_cast<unsigned char *>(v);
    }

    bool CheckPhysicalDeviceFeatures(
        VkPhysicalDevice device, Span<PhysicalDeviceFeature> features, std::string &unsupportedFeatureString)
    {
        if(features.IsEmpty())
        {
            return true;
        }

        size_t totalSize = 0;
        for(const PhysicalDeviceFeature &feature : features)
        {
            totalSize = UpAlignTo(totalSize, feature.alignment);
            totalSize += feature.size;
        }

        std::unique_ptr<unsigned char[]> storage(new unsigned char[totalSize]);
        std::memset(storage.get(), 0, totalSize);
        
        unsigned char *current = storage.get(), *last = nullptr;
        for(const PhysicalDeviceFeature &feature : features)
        {
            current = UpAlignPointerTo(current, feature.alignment);
            *reinterpret_cast<VkPhysicalDeviceFeatureHead *>(current) = { feature.type, last };
            last = current;
            current += feature.size;
        }
        
        VkPhysicalDeviceFeatures2 feature2 = {};
        feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        feature2.pNext = last;
        feature2.features.shaderInt16 = true;
        feature2.features.samplerAnisotropy = true;
        feature2.features.fillModeNonSolid = true;
        vkGetPhysicalDeviceFeatures2(device, &feature2);

        if(!feature2.features.shaderInt16)
        {
            unsupportedFeatureString = fmt::format("VkPhysicalDeviceFeatures.shaderInt16");
            return false;
        }

        current = storage.get();
        for(const PhysicalDeviceFeature &feature : features)
        {
            current = UpAlignPointerTo(current, feature.alignment);
            for(auto &[boolOffset, name] : feature.boolOffsetAndNames)
            {
                const bool boolValue = *reinterpret_cast<const VkBool32 *>(current + boolOffset);
                if(!boolValue)
                {
                    unsupportedFeatureString = fmt::format("{}.{}", feature.name, name);
                    return false;
                }
            }
            current += feature.size;
        }

        return true;
    }

    void *EnablePhysicalDeviceFeatures(Span<PhysicalDeviceFeature> features, std::unique_ptr<unsigned char[]> &storage)
    {
        if(features.IsEmpty())
        {
            return nullptr;
        }

        size_t totalSize = 0;
        for(const PhysicalDeviceFeature &feature : features)
        {
            totalSize = UpAlignTo(totalSize, feature.alignment);
            totalSize += feature.size;
        }

        storage.reset(new unsigned char[totalSize]);
        std::memset(storage.get(), 0, totalSize);

        unsigned char *current = storage.get(), *last = nullptr;
        for(const PhysicalDeviceFeature &feature : features)
        {
            current = UpAlignPointerTo(current, feature.alignment);
            *reinterpret_cast<VkPhysicalDeviceFeatureHead *>(current) = { feature.type, last };
            last = current;
            for(const size_t boolOffset : std::ranges::views::keys(feature.boolOffsetAndNames))
            {
                *reinterpret_cast<VkBool32 *>(current + boolOffset) = true;
            }
            current += feature.size;
        }

        return last;
    }

    bool IsSuitable(VkPhysicalDevice device, const DeviceDesc &desc, Span<PhysicalDeviceFeature> features)
    {
        // Api version

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        LogVerbose("Check physical device: {}", properties.deviceName);
        if(properties.apiVersion < RTRC_VULKAN_API_VERSION)
        {
            LogVerbose("\tRequired API version (Vulkan 1.3) is unsupported");
            return false;
        }

        // Basic extensions

        uint32_t supportedExtensionCount = 0;
        RTRC_VK_FAIL_MSG(
            vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, nullptr),
            "Failed to get device extension property count");

        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        RTRC_VK_FAIL_MSG(
            vkEnumerateDeviceExtensionProperties(
                device, nullptr, &supportedExtensionCount, supportedExtensions.data()),
            "Failed to get device extension properties");

        std::set<std::string> supportedExtensionNames;
        for(auto &e : supportedExtensions)
        {
            supportedExtensionNames.insert(e.extensionName);
        }

        auto requiredExtensions = VulkanPhysicalDevice::GetRequiredExtensions(desc);
        for(auto &required : requiredExtensions)
        {
            if(!supportedExtensionNames.contains(required))
            {
                LogVerbose("\tRequired extension {} is unsupported", required);
                return false;
            }
        }

        // Features

        std::string unsupportedFeatureName;
        if(!CheckPhysicalDeviceFeatures(device, features, unsupportedFeatureName))
        {
            LogVerbose("\t{} is unsupported", unsupportedFeatureName);
            return false;
        }

        LogVerbose("\tFound suitable physical device {}", properties.deviceName);
        return true;
    }

    uint32_t RatePhysicalDevice(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        uint32_t result = 0;
        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            result += 10000;
        }
        else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            result += 2000;
        }
        result += properties.limits.maxImageDimension2D;
        LogVerbose("\tRate physical device: {}, score = {}", properties.deviceName, result);
        return result;
    }

} // namespace VkPhysicalDeviceDetail

VulkanPhysicalDevice VulkanPhysicalDevice::Select(VkInstance instance, const DeviceDesc &desc)
{
    uint32_t deviceCount;
    RTRC_VK_FAIL_MSG(
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
        "Failed to get Vulkan physical device count");
    if(!deviceCount)
    {
        return VulkanPhysicalDevice();
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    RTRC_VK_FAIL_MSG(
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
        "Failed to get Vulkan physical devices");

    const auto physicalDeviceFeatures = VkPhysicalDeviceDetail::GetRequiredPhysicalDeviceFeatures(desc.enableRayTracing);

    uint32_t bestDeviceIndex = (std::numeric_limits<uint32_t>::max)();
    uint32_t bestScore = 0;
    for(uint32_t i = 0; i < deviceCount; ++i)
    {
        if(!IsSuitable(devices[i], desc, physicalDeviceFeatures))
            continue;

        const uint32_t newScore = VkPhysicalDeviceDetail::RatePhysicalDevice(devices[i]);
        if(newScore > bestScore)
        {
            bestDeviceIndex = i;
            bestScore = newScore;
        }
    }

    if(bestDeviceIndex >= deviceCount)
    {
        return VulkanPhysicalDevice();
    }
    return VulkanPhysicalDevice(devices[bestDeviceIndex], desc.enableRayTracing);
}

std::vector<const char*> VulkanPhysicalDevice::GetRequiredExtensions(const DeviceDesc &desc)
{
    std::vector<const char*> requiredExtensions =
    {
        VK_EXT_MESH_SHADER_EXTENSION_NAME,

        // The following extensions have been promoted to core 1.2/1.3, so they are not explicitly required
        // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        // VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        // VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
        // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
    if(desc.supportSwapchain)
    {
        requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    if(desc.enableRayTracing)
    {
        requiredExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    }
    return requiredExtensions;
}

VkPhysicalDeviceFeatures2 VulkanPhysicalDevice::GetRequiredFeatures(
    const DeviceDesc &desc, std::unique_ptr<unsigned char[]> &storage)
{
    VkPhysicalDeviceFeatures2 ret = {};
    ret.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    ret.pNext = EnablePhysicalDeviceFeatures(
        VkPhysicalDeviceDetail::GetRequiredPhysicalDeviceFeatures(desc.enableRayTracing), storage);
    ret.features.shaderInt16 = true;
    ret.features.samplerAnisotropy = true;
    ret.features.fillModeNonSolid = true;
    return ret;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetGraphicsQueueFamily() const
{
    return graphicsQueueFamily_;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetComputeQueueFamily() const
{
    return computeQueueFamily_;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetTransferQueueFamily() const
{
    return transferQueueFamily_;
}

VkPhysicalDevice VulkanPhysicalDevice::GetNativeHandle() const
{
    return physicalDevice_;
}

const VkPhysicalDeviceProperties &VulkanPhysicalDevice::GetNativeProperties() const
{
    return properties_.properties;
}

const VkPhysicalDeviceSubgroupProperties &VulkanPhysicalDevice::_internalGetSubgroupProperties() const
{
    return subgroupProperties_;
}

const std::optional<VkPhysicalDeviceAccelerationStructurePropertiesKHR> &
    VulkanPhysicalDevice::_internalGetASProperties() const
{
    return asProperties_;
}

const std::optional<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> &
    VulkanPhysicalDevice::_internalGetRtPipelineProperties() const
{
    return rtPipelineProperties_;
}

VulkanPhysicalDevice::VulkanPhysicalDevice()
    : physicalDevice_(VK_NULL_HANDLE), properties_{}, subgroupProperties_{}
{
    
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice device, bool enableRayTracing)
    : physicalDevice_(device), properties_{}, subgroupProperties_{}
{
    assert(physicalDevice_);
    subgroupProperties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    properties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties_.pNext = &subgroupProperties_;
    vkGetPhysicalDeviceProperties2(physicalDevice_, &properties_);

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for(uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        const auto flag = queueFamilies[i].queueFlags;
        const bool graphics = (flag & VK_QUEUE_GRAPHICS_BIT) != 0;
        const bool compute = (flag & VK_QUEUE_COMPUTE_BIT) != 0;
        const bool transfer = (flag & VK_QUEUE_TRANSFER_BIT) != 0;
        const bool opticalFlowNV = (flag & VK_QUEUE_OPTICAL_FLOW_BIT_NV) != 0;

        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamily_ = i;
        }
        if((!computeQueueFamily_ && compute) || (compute && !graphics))
        {
            computeQueueFamily_ = i;
        }
        if((!transferQueueFamily_ && transfer) || (transfer && !compute && !opticalFlowNV))
        {
            transferQueueFamily_ = i;
        }
    }

    if(enableRayTracing)
    {
        VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
        };
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
            .pNext = &asProperties
        };
        VkPhysicalDeviceProperties2 properties =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &rtPipelineProperties
        };
        vkGetPhysicalDeviceProperties2(physicalDevice_, &properties);
        asProperties_ = asProperties;
        rtPipelineProperties_ = rtPipelineProperties;
    }
}

RTRC_RHI_VK_END
