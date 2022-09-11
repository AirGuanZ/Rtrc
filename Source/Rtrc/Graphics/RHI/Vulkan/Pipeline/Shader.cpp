#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>

RTRC_RHI_VK_BEGIN

VulkanShader::VulkanShader(VkDevice device, VkShaderModule shaderModule, std::string entry, ShaderStage type)
    : device_(device), shaderModule_(shaderModule), entry_(std::move(entry)), type_(type)
{
    
}

VulkanShader::~VulkanShader()
{
    assert(shaderModule_);
    vkDestroyShaderModule(device_, shaderModule_, VK_ALLOC);
}

ShaderStage VulkanShader::GetType() const
{
    return type_;
}

VkPipelineShaderStageCreateInfo VulkanShader::GetStageCreateInfo() const
{
    return VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = TranslateShaderType(type_),
        .module = shaderModule_,
        .pName  = entry_.c_str()
    };
}

RTRC_RHI_VK_END
