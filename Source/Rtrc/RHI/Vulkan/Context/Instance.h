#pragma once

#include <VkBootstrap.h>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanInstance : public Instance
{
public:

    VulkanInstance(VulkanInstanceDesc desc, vkb::Instance instance);

    ~VulkanInstance() override;

    Ptr<Device> CreateDevice(const DeviceDesc &desc) override;

private:

    VulkanInstanceDesc desc_;
    vkb::Instance instance_;
};

RTRC_RHI_VK_END
