#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRawShader, RawShader)
{
public:

    VulkanRawShader(VkDevice device, VkShaderModule shaderModule, std::vector<RawShaderEntry> entries);

    ~VulkanRawShader() override;

    VkPipelineShaderStageCreateInfo _internalGetStageCreateInfo() const;

    template<typename OutIt>
    void _internalGetStageCreateInfos(OutIt outIt) const;

private:

    VkDevice device_;
    VkShaderModule shaderModule_;
    std::vector<RawShaderEntry> entries_;
};

template<typename OutIt>
void VulkanRawShader::_internalGetStageCreateInfos(OutIt outIt) const
{
    for(const RawShaderEntry &entry : entries_)
    {
        *outIt++ = VkPipelineShaderStageCreateInfo
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = TranslateShaderStage(entry.stage),
            .module = shaderModule_,
            .pName  = entry.name.c_str()
        };
    }
}

RTRC_RHI_VK_END
