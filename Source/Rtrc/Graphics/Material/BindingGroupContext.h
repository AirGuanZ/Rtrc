#pragma once

#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Shader/ShaderBindingGroup.h>

RTRC_BEGIN

class Shader;

class BindingGroupContext
{
public:

    void Clear();
    void Set(std::string name, RC<BindingGroup> bindingGroup);
    const RC<BindingGroup> &Get(std::string_view name) const;

    void ClearBindingRecords();
    void BindForGraphicsPipeline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader);
    void BindForComputePipeline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader);

private:

    struct Record
    {
        RC<BindingGroup> group;
    };

    RHI::CommandBufferPtr lastCommandBuffer_;
    RC<Shader> lastGraphicsShader_;
    RC<Shader> lastComputeShader_;
    std::map<std::string, Record, std::less<>> nameToRecord_;
};

RTRC_END
