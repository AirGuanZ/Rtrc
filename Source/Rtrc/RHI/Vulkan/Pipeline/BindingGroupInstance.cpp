#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupInstance.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupInstance::VulkanBindingGroupInstance(VulkanBindingGroupLayout *layout, VkDescriptorSet set)
    : layout_(layout), set_(set)
{
    
}

VulkanBindingGroupInstance::~VulkanBindingGroupInstance()
{
    layout_->ReleaseSet(set_);
}

RTRC_RHI_VK_END
