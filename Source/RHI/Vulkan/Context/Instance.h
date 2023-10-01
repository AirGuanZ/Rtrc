#pragma once

#include <RHI/Vulkan/Common.h>

#include <VkBootstrap.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanInstance, Instance)
{
public:

    VulkanInstance(VulkanInstanceDesc desc, vkb::Instance instance);

    ~VulkanInstance() override;

    Ptr<Device> CreateDevice(const DeviceDesc &desc) RTRC_RHI_OVERRIDE;

private:

    VulkanInstanceDesc desc_;
    vkb::Instance instance_;
};

RTRC_RHI_VK_END
