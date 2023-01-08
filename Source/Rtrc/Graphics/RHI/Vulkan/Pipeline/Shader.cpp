#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>

RTRC_RHI_VK_BEGIN

VulkanRawShader::VulkanRawShader(VkDevice device, VkShaderModule shaderModule, std::string entry, ShaderStage type)
    : device_(device), shaderModule_(shaderModule), entry_(std::move(entry)), type_(type)
{
    
}

VulkanRawShader::~VulkanRawShader()
{
    assert(shaderModule_);
    vkDestroyShaderModule(device_, shaderModule_, VK_ALLOC);
}

ShaderStage VulkanRawShader::GetType() const
{
    return type_;
}

VkPipelineShaderStageCreateInfo VulkanRawShader::_internalGetStageCreateInfo() const
{
    return VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = TranslateShaderType(type_),
        .module = shaderModule_,
        .pName  = entry_.c_str()
    };
}

RTRC_RHI_VK_END
