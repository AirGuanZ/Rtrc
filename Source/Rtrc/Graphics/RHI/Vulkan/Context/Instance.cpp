#include <mutex>

#define VOLK_IMPLEMENTATION
#include <volk.h>
#undef VOLK_IMPLEMENTATION

#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Instance.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/PhysicalDevice.h>
#include <Rtrc/Utility/ScopeGuard.h>

RTRC_RHI_BEGIN

void InitializeVulkanBackend()
{
    static std::once_flag initVolkFlag;
    std::call_once(initVolkFlag, [] { volkInitialize(); });
}

Ptr<Instance> CreateVulkanInstance(const VulkanInstanceDesc &desc)
{
    InitializeVulkanBackend();

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder
        .set_app_name("RtrcApplication")
        .set_engine_name("RtrcEngine")
        .require_api_version(1, 3)
        .set_allocation_callbacks(VK_ALLOC);

    for(auto &e : desc.extensions)
    {
        instanceBuilder.enable_extension(e.c_str());
    }

    if(desc.debugMode)
    {
        instanceBuilder
            .use_default_debug_messenger()
            .request_validation_layers();
    }

    auto buildInstanceResult = instanceBuilder.build();
    if(!buildInstanceResult)
    {
        throw Exception("failed to create vulkan instance: " + buildInstanceResult.error().message());
    }

    auto instance = buildInstanceResult.value();
    RTRC_SCOPE_FAIL{ destroy_instance(instance); };
    volkLoadInstance(instance.instance);

    return MakePtr<Vk::VulkanInstance>(desc, instance);
}

RTRC_RHI_END

RTRC_RHI_VK_BEGIN

VulkanInstance::VulkanInstance(VulkanInstanceDesc desc, vkb::Instance instance)
    : desc_(std::move(desc)), instance_(instance)
{

}

VulkanInstance::~VulkanInstance()
{
    destroy_instance(instance_);
}

Ptr<Device> VulkanInstance::CreateDevice(const DeviceDesc &desc)
{
    // physical device

    auto physicalDevice = VulkanPhysicalDevice::Select(instance_.instance, desc);

    // queues

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const float priorities[4] = { 1, 1, 1, 1 };

    if(desc.graphicsQueue)
    {
        if(!physicalDevice.GetGraphicsQueueFamily())
        {
            throw Exception("selected physical device doesn't support graphics queue");
        }
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *physicalDevice.GetGraphicsQueueFamily(),
            .queueCount       = 2,
            .pQueuePriorities = priorities
        });
    }

    if(desc.computeQueue)
    {
        if(!physicalDevice.GetComputeQueueFamily())
        {
            throw Exception("selected physical device doesn't support compute queue");
        }

        bool needNewQueueFamily = true;
        for(auto &info : queueCreateInfos)
        {
            if(info.queueFamilyIndex == *physicalDevice.GetComputeQueueFamily())
            {
                ++info.queueCount;
                needNewQueueFamily = false;
            }
        }
        if(needNewQueueFamily)
        {
            queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = *physicalDevice.GetComputeQueueFamily(),
                .queueCount       = 1,
                .pQueuePriorities = priorities
            });
        }
    }

    if(desc.transferQueue)
    {
        if(!physicalDevice.GetTransferQueueFamily())
        {
            throw Exception("selected physical device doesn't support transfer queue");
        }

        bool needNewQueueFamily = true;
        for(auto &info : queueCreateInfos)
        {
            if(info.queueFamilyIndex == *physicalDevice.GetTransferQueueFamily())
            {
                ++info.queueCount;
                needNewQueueFamily = false;
            }
        }
        if(needNewQueueFamily)
        {
            queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = *physicalDevice.GetTransferQueueFamily(),
                .queueCount       = 1,
                .pQueuePriorities = priorities
            });
        }
    }

    // features & extensions

    ObjectReleaser arena;
    auto features2 = VulkanPhysicalDevice::GetRequiredFeatures(arena);
    const std::vector<const char *> extensions = VulkanPhysicalDevice::GetRequiredExtensions(desc);

    // logical device

    const VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = features2,
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };
    VkDevice device;
    VK_FAIL_MSG(
        vkCreateDevice(physicalDevice.GetNativeHandle(), &deviceCreateInfo, VK_ALLOC, &device),
        "failed to create vulkan device");
    RTRC_SCOPE_FAIL{ vkDestroyDevice(device, VK_ALLOC); };

    VulkanDevice::QueueFamilyInfo queueFamilies;
    if(desc.graphicsQueue)
    {
        queueFamilies.graphicsFamilyIndex = physicalDevice.GetGraphicsQueueFamily();
    }
    if(desc.computeQueue)
    {
        queueFamilies.computeFamilyIndex = physicalDevice.GetComputeQueueFamily();
    }
    if(desc.transferQueue)
    {
        queueFamilies.transferFamilyIndex = physicalDevice.GetTransferQueueFamily();
    }

    return MakePtr<VulkanDevice>(instance_.instance, physicalDevice, device, queueFamilies, desc_.debugMode);
}

RTRC_RHI_VK_END
