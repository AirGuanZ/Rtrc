#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanShader : public RawShader
{
public:

    VulkanShader(VkDevice device, VkShaderModule shaderModule, std::string entry, ShaderStage type);

    ~VulkanShader() override;

    ShaderStage GetType() const override;

    VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

private:

    VkDevice device_;
    VkShaderModule shaderModule_;
    std::string entry_;
    ShaderStage type_;
};

RTRC_RHI_VK_END
