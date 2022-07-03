#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanComputePipeline : public ComputePipeline
{
public:

    VulkanComputePipeline(RC<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanComputePipeline() override;

    const RC<BindingLayout> &GetBindingLayout() const override;

    VkPipeline GetNativePipeline() const;

private:

    RC<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

class VulkanComputePipelineBuilder : public ComputePipelineBuilder
{
public:

    explicit VulkanComputePipelineBuilder(VkDevice device);

    ComputePipelineBuilder &SetComputeShader(RC<RawShader> shader) override;

    ComputePipelineBuilder &SetBindingLayout(RC<BindingLayout> layout) override;

    RC<ComputePipeline> CreatePipeline() const override;

private:

    RC<RawShader> computeShader_;
    RC<BindingLayout> bindingLayout_;

    VkDevice device_;
};

RTRC_RHI_VK_END
