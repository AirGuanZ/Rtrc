#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRawShader, RawShader)
{
public:

    VulkanRawShader(VkDevice device, VkShaderModule shaderModule, std::string entry, ShaderStage type);

    ~VulkanRawShader() override;

    ShaderStage GetType() const RTRC_RHI_OVERRIDE;

    VkPipelineShaderStageCreateInfo _internalGetStageCreateInfo() const;

private:

    VkDevice device_;
    VkShaderModule shaderModule_;
    std::string entry_;
    ShaderStage type_;
};

RTRC_RHI_VK_END
