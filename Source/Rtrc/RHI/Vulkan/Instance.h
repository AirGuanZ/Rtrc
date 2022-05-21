#pragma once

#include <VkBootstrap.h>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanInstance : public Instance
{
public:

    explicit VulkanInstance(vkb::Instance instance);

    ~VulkanInstance() override;

    Unique<Device> CreateDevice(const DeviceDescription &desc) override;

private:

    vkb::Instance instance_;
};

RTRC_RHI_VK_END
