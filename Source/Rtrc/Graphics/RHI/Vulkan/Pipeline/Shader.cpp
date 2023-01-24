#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>

RTRC_RHI_VK_BEGIN

VulkanRawShader::VulkanRawShader(VkDevice device, VkShaderModule shaderModule, std::vector<RawShaderEntry> entries)
    : device_(device), shaderModule_(shaderModule), entries_(std::move(entries))
{
    
}

VulkanRawShader::~VulkanRawShader()
{
    assert(shaderModule_);
    vkDestroyShaderModule(device_, shaderModule_, RTRC_VK_ALLOC);
}

VkPipelineShaderStageCreateInfo VulkanRawShader::_internalGetStageCreateInfo() const
{
    assert(entries_.size() == 1);
    return VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = TranslateShaderStage(entries_[0].stage),
        .module = shaderModule_,
        .pName  = entries_[0].name.c_str()
    };
}

RTRC_RHI_VK_END
