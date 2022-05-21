#pragma once

#include <volk.h>

#include <Rtrc/RHI/RHI.h>

#define RTRC_RHI_VK_BEGIN RTRC_RHI_BEGIN namespace Vk {
#define RTRC_RHI_VK_END   } RTRC_RHI_END

RTRC_RHI_VK_BEGIN

struct VkResultChecker
{
    VkResult result;
    template<typename F>
    void operator+(F &&f)
    {
        if(result != VK_SUCCESS)
        {
            std::invoke(std::forward<F>(f), result);
        }
    }
};

#define VK_CHECK(RESULT) ::Rtrc::RHI::Vk::VkResultChecker{(RESULT)}+[&](VkResult errorCode)->void
#define VK_FAIL_MSG(RESULT, MSG) do { VK_CHECK(RESULT) { throw ::Rtrc::Exception(MSG); }; } while(false)

extern VkAllocationCallbacks RtrcGlobalVulkanAllocationCallbacks;
#define VK_ALLOC (&Rtrc::RHI::Vk::RtrcGlobalVulkanAllocationCallbacks)

VkFormat TranslateTexelFormat(TexelFormat format);

#define RTRC_VULKAN_API_VERSION VK_API_VERSION_1_3

RTRC_RHI_VK_END
