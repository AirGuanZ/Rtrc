#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanComputePipeline : public ComputePipeline
{
public:

    VulkanComputePipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanComputePipeline() override;

    const Ptr<BindingLayout> &GetBindingLayout() const override;

    VkPipeline GetNativePipeline() const;

private:

    Ptr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

class VulkanComputePipelineBuilder : public ComputePipelineBuilder
{
public:

    explicit VulkanComputePipelineBuilder(VkDevice device);

    ComputePipelineBuilder &SetComputeShader(Ptr<RawShader> shader) override;

    ComputePipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout) override;

    Ptr<ComputePipeline> CreatePipeline() const override;

private:

    Ptr<RawShader> computeShader_;
    Ptr<BindingLayout> bindingLayout_;

    VkDevice device_;
};

RTRC_RHI_VK_END
